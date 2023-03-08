#include "Drawer2D.h"
#include "utilities.h"
#include "mathUtils.h"
#include "MaterialWorkaround.h"
#include "Triangulate.h"

#include <IrrlichtDevice.h>
#include <ISceneManager.h>
#include <ICameraSceneNode.h>
#include <ITexture.h>
#include <IVideoDriver.h>
#include <SViewFrustum.h>
#include <IShaderConstantSetCallBack.h>
#include <IMaterialRendererServices.h>
#include <IGPUProgrammingServices.h>

#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <fstream>
#include <cassert>

#define EPS 0.1

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;
using namespace std;

#define MAX_VERTICES 65536

//#define NEW_POLY_ALGORITHM

static const char* Poly2DVS = 
"#ifdef GL_ES\n"
"attribute vec3 inVertexPosition;\n"
"attribute vec4 inVertexColor;\n"
"attribute vec2 inTexCoord0;\n"
"varying vec4 vVertexColor;\n"
"#define VERTEX inVertexPosition\n"
"#define VCOLOR inVertexColor.bgra\n"//Color order different in gles
"#define OUTCOLOR vVertexColor\n"
"#define TEXCOORD0 inTexCoord0\n"
"#else\n"
"#define VERTEX gl_Vertex.xyz\n"
"#define VCOLOR gl_Color\n"
"#define OUTCOLOR gl_FrontColor\n"
"#define TEXCOORD0 gl_MultiTexCoord0.xy\n"
"#endif\n"
"uniform mat4 uWVPMatrix;\n"
"varying vec2 vTextureCoord0;\n"
"\n"
"void main()\n"
"{\n"
"	OUTCOLOR = VCOLOR;\n"
"	vTextureCoord0 = TEXCOORD0;\n"
"	gl_Position = uWVPMatrix * vec4(VERTEX,1.0);\n"
"}\n";

static const char* Poly2DFS = 
"#ifdef GL_ES\n"
"precision mediump float;\n"
"varying vec4 vVertexColor;\n"
"#define VCOLOR vVertexColor\n"
"#else\n"
"#define VCOLOR gl_Color\n"
"#endif\n"
"\n"
"uniform float uAADelta;\n"
"uniform sampler2D uTextureUnit0;\n"
"varying vec2 vTextureCoord0;\n"
"void main()\n"
"{\n"
"	#ifdef NO_TEXTURE\n"
"	vec4 color = VCOLOR;\n"
"	#else\n"
"	vec4 color = VCOLOR*texture2D(uTextureUnit0, vTextureCoord0);\n"
"	#endif\n"
"	color.a *= smoothstep(0.0,uAADelta,vTextureCoord0.y)-smoothstep(1.0-uAADelta,1.0,vTextureCoord0.y);\n"
"	gl_FragColor = color;\n"
"}\n";

//! Callback for 2D Polygon shaders
class PolyShaderCallback : public IShaderConstantSetCallBack{

	protected:
	
	IrrlichtDevice* device;
	IVideoDriver* driver;

	bool firstSetConstants;
	s32 wvpMatrixID;
	s32 textureUnit0ID;
	s32 aaDeltaID;
	
	const SMaterial* usedMaterial;
	
	public:
	
	irr::f32 aaDelta;
	
	PolyShaderCallback(){
		device = NULL;
		driver = NULL;
		firstSetConstants = true;
		aaDelta = 0.f;
	}
	
	virtual ~PolyShaderCallback(){}
	
	virtual void init(IrrlichtDevice* device){
		firstSetConstants = true;
		this->device = device;
		driver = device->getVideoDriver();
		wvpMatrixID = textureUnit0ID = aaDeltaID = -1;
	}
	
	virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData){
		if(firstSetConstants){
			wvpMatrixID = services->getVertexShaderConstantID("uWVPMatrix");
			textureUnit0ID = services->getPixelShaderConstantID("uTextureUnit0");
			aaDeltaID = services->getPixelShaderConstantID("uAADelta");
			firstSetConstants = false;
		}
		const matrix4& W = driver->getTransform(ETS_WORLD);
		const matrix4& V = driver->getTransform(ETS_VIEW);
		const matrix4& P = driver->getTransform(ETS_PROJECTION);
		matrix4 Matrix = P * V * W;
		services->setVertexShaderConstant(wvpMatrixID, Matrix.pointer(), 16);
		s32 textureUnit0 = 0;
		services->setPixelShaderConstant(textureUnit0ID, &textureUnit0, 1);
		f32 finalAADelta = Max(aaDelta, 0.0001f);
		services->setPixelShaderConstant(aaDeltaID, &finalAADelta, 1);
	}
	
	virtual void OnSetMaterial(const video::SMaterial& material){
		usedMaterial = &material;
		aaDelta = material.Shininess;//encode anitaliasing value in shininess value
	}
	
};

class PolyShaderWithTextureCallback : public PolyShaderCallback{

	protected:
	
	s32 textureUnit0ID;
	
	public:
	
	virtual ~PolyShaderWithTextureCallback(){}
	
	virtual void init(IrrlichtDevice* device){
		textureUnit0ID = -1;
		PolyShaderCallback::init(device);
	}
	
	virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData){
		if(firstSetConstants){
			textureUnit0ID = services->getPixelShaderConstantID("uTextureUnit0");
		}
		PolyShaderCallback::OnSetConstants(services, userData);
		s32 textureUnit0 = 0;
		services->setPixelShaderConstant(textureUnit0ID, &textureUnit0, 1);
	}
	
};

static E_MATERIAL_TYPE polyMatType = EMT_TRANSPARENT_ALPHA_CHANNEL;
static E_MATERIAL_TYPE polyMatNoTexType = EMT_TRANSPARENT_ALPHA_CHANNEL;
static PolyShaderCallback pcbk;
static PolyShaderWithTextureCallback pwtcbk;

void Drawer2D::initStaticVars(){
	Camera2DParameterCalculator::initStaticVars();
	polyMatNoTexType = polyMatType = EMT_TRANSPARENT_ALPHA_CHANNEL;
	pcbk = PolyShaderCallback();
	pwtcbk = PolyShaderWithTextureCallback();
}

void set2DMaterialParams(irr::video::SMaterial& material, irr::video::E_TEXTURE_CLAMP wrapU, irr::video::E_TEXTURE_CLAMP wrapV, irr::video::E_MATERIAL_TYPE type){
	material.setFlag(EMF_LIGHTING, false);
	material.setFlag(EMF_ZBUFFER, false);
	material.setFlag(EMF_ZWRITE_ENABLE, false);
	material.setFlag(EMF_BILINEAR_FILTER, true);
	material.setFlag(EMF_TRILINEAR_FILTER, true);
	material.setFlag(EMF_ANISOTROPIC_FILTER, true);
	material.setFlag(EMF_ANTI_ALIASING, true);
	material.setFlag(EMF_ANISOTROPIC_FILTER, true);
	material.setFlag(EMF_BACK_FACE_CULLING, false);
	material.MaterialType = type;
	for(u32 j=0; j<MATERIAL_MAX_TEXTURES; j++){
		material.TextureLayer[j].TextureWrapU = wrapU;
		material.TextureLayer[j].TextureWrapV = wrapV;
	}
}

Drawer2D::Drawer2D(irr::IrrlichtDevice* device):cCalc(16.f/9.f){
	this->device=device;
	smgr=device->getSceneManager();
	driver = device->getVideoDriver();
	set2DMaterialParams(stdMat, ETC_REPEAT, ETC_REPEAT, EMT_TRANSPARENT_ALPHA_CHANNEL);
	mb.Vertices.reallocate(MAX_VERTICES);
	mb.Indices.reallocate(2*3*MAX_VERTICES);
	mb.Indices.setAllocStrategy(ALLOC_STRATEGY_DOUBLE);
	imgMb.Vertices.reallocate(4);
	imgMb.Indices.reallocate(6);
	imgMb.Indices.setAllocStrategy(ALLOC_STRATEGY_DOUBLE);
	stdColor[0] = stdColor[1] = stdColor[2] = stdColor[3] = SColor(255,255,255,255);
	setTextureCoordinates();
	v[0] = vector2d<f32>(0.f,0.f); v[1] = vector2d<f32>(1.f,0.f); v[2] = vector2d<f32>(0.f,1.f); v[3] = vector2d<f32>(1.f,1.f);
	setFiltering(true,true,16);
	imgMb.Indices.push_back(0);//Indices for a rectangle consisting of two triangles (vertex coordinates in array v)
	imgMb.Indices.push_back(2);
	imgMb.Indices.push_back(1);
	imgMb.Indices.push_back(1);
	imgMb.Indices.push_back(2);
	imgMb.Indices.push_back(3);
	imgMb.MappingHint_Index = EHM_STATIC;
	autoResetEnabled = true;
	triangulated.reallocate(MAX_VERTICES);
	if(polyMatType==EMT_TRANSPARENT_ALPHA_CHANNEL && (driver->getDriverType()==EDT_OGLES2 || driver->getDriverType()==EDT_OPENGL)){//TODO other drivers
		pcbk.init(device);
		pwtcbk.init(device);
		//polyMatType = EMT_TRANSPARENT_ADD_COLOR;
		polyMatType = (E_MATERIAL_TYPE)driver->getGPUProgrammingServices()->addHighLevelShaderMaterial(Poly2DVS, "main", EVST_VS_2_0, Poly2DFS, "main", EPST_PS_2_0, &pwtcbk, EMT_TRANSPARENT_ALPHA_CHANNEL, 0);
		std::string finalPoly2DFS = std::string("#define NO_TEXTURE\n").append(Poly2DFS);
		polyMatNoTexType = (E_MATERIAL_TYPE)driver->getGPUProgrammingServices()->addHighLevelShaderMaterial(Poly2DVS, "main", EVST_VS_2_0, finalPoly2DFS.c_str(), "main", EPST_PS_2_0, &pcbk, EMT_TRANSPARENT_ALPHA_CHANNEL, 0);
		assert(polyMatType>=0 && polyMatNoTexType>=0);
	}
	defaultPolyAAValue = 0.f;
	overridePolyMatType = EMT_TRANSPARENT_ALPHA_CHANNEL;
	useOverridePolyMatType = false;
}

void Drawer2D::setOverridePolygonMaterialType(E_MATERIAL_TYPE* overrideMatType){
	useOverridePolyMatType = overrideMatType!=NULL;
	if(useOverridePolyMatType){
		this->overridePolyMatType = *overrideMatType;
	}
}

void Drawer2D::setAutoResetTransformEnabled(bool enabled){
	autoResetEnabled = enabled;
}

Drawer2D::~Drawer2D(){}

	
void Drawer2D::reset2DTransforms(){
	cCalc.reset2DTransforms(device);
}

void Drawer2D::setTextureWrap(irr::video::E_TEXTURE_CLAMP uwrap, irr::video::E_TEXTURE_CLAMP vwrap){
	for(u32 j=0; j<MATERIAL_MAX_TEXTURES; j++){
		stdMat.TextureLayer[j].TextureWrapU = uwrap;
		stdMat.TextureLayer[j].TextureWrapV = vwrap;
	}
}

void Drawer2D::drawArc(irr::video::ITexture* tex, irr::core::position2d<int> m, float r, float startAngle, float endAngle, float deltaAngle, int height, float verticalOffset){
	mb.MappingHint_Index = EHM_NEVER;
	mb.MappingHint_Vertex = EHM_NEVER;
	mb.Material = SMaterial(stdMat);
	fillArcMeshBuffer(&mb, tex, vector2d<f32>(m.X,m.Y), r, startAngle, endAngle, deltaAngle, stdColor[0], height, verticalOffset, 0.01f, true, .1f);
	draw2DMeshBuffer(&mb);
}

void Drawer2D::drawLine(irr::video::ITexture* tex, irr::core::position2d<s32> start, irr::core::position2d<s32> end, s32 height, f32 verticalOffset){
	drawLine(tex, vector2d<f32>(start.X, start.Y), vector2d<f32>(end.X, end.Y), height, verticalOffset);
}

void Drawer2D::drawLine(irr::video::ITexture* tex, irr::core::position2d<f32> start, irr::core::position2d<f32> end, f32 height, f32 verticalOffset){
	if(start.X==end.X && start.Y==end.Y){return;}
	vector2d<f32> v = vector2d<f32>(end.X-start.X, end.Y-start.Y);
	f32 len = v.getLength();
	f32 rot = DEG_RAD*acos2(v.X/len);
	if(end.Y>=start.Y){rot = 360.f-rot;}
	draw(tex, start, dimension2d<f32>(len, height), rot, vector2d<f32>(0, verticalOffset));
}

void Drawer2D::draw(ITexture* tex, position2d<irr::f32> pos, dimension2d<f32> size, float rotation, vector2d<f32> origin, irr::core::vector2d<irr::f32>* customTCoords){
	if(size.Width>FLOAT_EPS && size.Height>FLOAT_EPS){
		irr::core::vector2d<irr::f32>* tc = customTCoords==NULL?tcoords:customTCoords;
		cCalc.set2DTransforms(device);
		imgMb.Vertices.set_used(0);
		imgMb.Material = SMaterial(stdMat);
		f32 cosphi = cosf(rotation/DEG_RAD), sinphi = sinf(rotation/DEG_RAD);
		for(int i = 0; i<4; i++){//Vertices
			vector2d<f32> vi = v[i]-origin;//Move origin -> 0
			vi.X *= size.Width; vi.Y *= size.Height;//Scale to size
			vector3d<f32> vertex(pos.X+cosphi*vi.X+sinphi*vi.Y, pos.Y-sinphi*vi.X+cosphi*vi.Y, 1.f);//Rotate and move to pos
			imgMb.Vertices.push_back(S3DVertex(vertex, vector3d<f32>(0.0,0.0,-1.0), stdColor[i], tc[i]));
		}
		imgMb.MappingHint_Vertex = EHM_NEVER;
		imgMb.setDirty(EBT_VERTEX);
		imgMb.Material.TextureLayer[0].Texture = tex;//NULL also fine
		imgMb.Material.BackfaceCulling = false;//otherwise index order is important
		driver->setMaterial(imgMb.Material);
		SET_MATERIAL_WORKAROUND(driver, imgMb.Material)
		driver->drawMeshBuffer(&imgMb);
		UNSET_MATERIAL_WORKAROUND
		if(autoResetEnabled){
			cCalc.reset2DTransforms(device);
		}
	}
}

void Drawer2D::draw(ITexture* tex, position2d<int> pos, dimension2d<int> size, float rotation, vector2d<f32> origin){
	draw(tex, position2d<irr::f32>(pos.X, pos.Y), dimension2d<f32>(size.Width, size.Height), rotation, origin);
}

void Drawer2D::draw(irr::video::ITexture* tex, const irr::core::rect<irr::s32>& screenRect, const irr::core::rect<irr::s32>* clipRect){
	if(clipRect==NULL){
		draw(tex, screenRect.UpperLeftCorner, dimension2d<int>(screenRect.getWidth(), screenRect.getHeight()));
	}else{
		core::rect<s32> vp = driver->getViewPort();
		driver->setViewPort(*clipRect);
		setTextureWrap(ETC_CLAMP, ETC_CLAMP);
		draw(tex, screenRect.UpperLeftCorner-clipRect->UpperLeftCorner, dimension2d<int>(screenRect.getWidth(), screenRect.getHeight()));
		setTextureWrap();
		driver->setViewPort(vp);	
	}
}

//! Calculates solution for u0+a0*v0=u1+a1*v1; if lines are nearly parallel the fallback is used.
//! v0 and v1 need to have length 1.
static vector2d<f32> calcLineIntersection(vector2d<f32> u0, vector2d<f32> v0, vector2d<f32> u1, vector2d<f32> v1, vector2d<f32> fallback, f32 eps = EPS){
	f32 z0 = v1.Y*v0.X-v0.Y*v1.X;
	if(fabs(z0)>eps){
		return u1+((v0.X*(u0.Y-u1.Y)+v0.Y*(u1.X-u0.X))/z0)*v1;
	}else{
		return fallback;
	}
}

//! fills a rectangle in an existing meshbuffer without allocation, needs space for 4 vertices and 6 indices
static void fillRect(irr::scene::SMeshBuffer* mb, irr::s32 vOffset, irr::s32 iOffset, const irr::core::rect<irr::f32>& rectangle, const irr::core::rect<irr::f32>& uvCoords, irr::video::SColor color){
	mb->Vertices[vOffset] = S3DVertex(rectangle.UpperLeftCorner.X, rectangle.UpperLeftCorner.Y, 1.f, 0.f, 0.f, -1.f, color, uvCoords.UpperLeftCorner.X, uvCoords.UpperLeftCorner.Y);
	mb->Vertices[vOffset+1] = S3DVertex(rectangle.LowerRightCorner.X, rectangle.UpperLeftCorner.Y, 1.f, 0.f, 0.f, -1.f, color, uvCoords.LowerRightCorner.X, uvCoords.UpperLeftCorner.Y);
	mb->Vertices[vOffset+2] = S3DVertex(rectangle.UpperLeftCorner.X, rectangle.LowerRightCorner.Y, 1.f, 0.f, 0.f, -1.f, color, uvCoords.UpperLeftCorner.X, uvCoords.LowerRightCorner.Y);
	mb->Vertices[vOffset+3] = S3DVertex(rectangle.LowerRightCorner.X, rectangle.LowerRightCorner.Y, 1.f, 0.f, 0.f, -1.f, color, uvCoords.LowerRightCorner.X, uvCoords.LowerRightCorner.Y);
	mb->Indices[iOffset] = vOffset;
	mb->Indices[iOffset+1] = vOffset+3;
	mb->Indices[iOffset+2] = vOffset+1;
	mb->Indices[iOffset+3] = vOffset;
	mb->Indices[iOffset+4] = vOffset+2;
	mb->Indices[iOffset+5] = vOffset+3;
}

void Drawer2D::fillRectWithCornerMeshBuffer(irr::scene::SMeshBuffer* mb, const irr::core::rect<irr::f32>& rectangle, irr::f32 uvCornerSize, irr::f32 realCornerSize, irr::video::ITexture* tex, irr::video::SColor color){
	mb->Material.TextureLayer[0].Texture = tex;//NULL also fine
	mb->Material.BackfaceCulling = false;//otherwise index order is important
	mb->Vertices.set_used(16);
	mb->Indices.set_used(9*2*3);
	//rectangle i at vertex i*4
	fillRect(mb, 0, 0, rect<f32>(rectangle.UpperLeftCorner-vector2d<f32>(realCornerSize,realCornerSize), rectangle.UpperLeftCorner), rect<f32>(0,0,uvCornerSize,uvCornerSize), color);
	fillRect(mb, 4, 6, rect<f32>(rectangle.LowerRightCorner.X, rectangle.UpperLeftCorner.Y-realCornerSize, rectangle.LowerRightCorner.X+realCornerSize, rectangle.UpperLeftCorner.Y), rect<f32>(1.f-uvCornerSize,0,1.f,uvCornerSize), color);
	fillRect(mb, 8, 12, rect<f32>(rectangle.UpperLeftCorner.X-realCornerSize, rectangle.LowerRightCorner.Y, rectangle.UpperLeftCorner.X, rectangle.LowerRightCorner.Y+realCornerSize), rect<f32>(0,1.f-uvCornerSize,uvCornerSize,1.f), color);
	fillRect(mb, 12, 18, rect<f32>(rectangle.LowerRightCorner, rectangle.LowerRightCorner+vector2d<f32>(realCornerSize,realCornerSize)), rect<f32>(1.f-uvCornerSize,1.f-uvCornerSize,1.f,1.f), color);
	mb->Indices[24] = 1;//oben
	mb->Indices[25] = 4+2;
	mb->Indices[26] = 4+0;
	mb->Indices[27] = 1;
	mb->Indices[28] = 3;
	mb->Indices[29] = 4+2;
	mb->Indices[30] = 8+1;//unten
	mb->Indices[31] = 12+2;
	mb->Indices[32] = 12+0;
	mb->Indices[33] = 8+1;
	mb->Indices[34] = 8+3;
	mb->Indices[35] = 12+2;
	mb->Indices[36] = 2;//links
	mb->Indices[37] = 8+1;
	mb->Indices[38] = 3;
	mb->Indices[39] = 2;
	mb->Indices[40] = 8+0;
	mb->Indices[41] = 8+1;
	mb->Indices[42] = 4+2;//rechts
	mb->Indices[43] = 12+1;
	mb->Indices[44] = 4+3;
	mb->Indices[45] = 4+2;
	mb->Indices[46] = 12+0;
	mb->Indices[47] = 12+1;
	mb->Indices[48] = 3;//mitte
	mb->Indices[49] = 12+0;
	mb->Indices[50] = 4+2;
	mb->Indices[51] = 3;
	mb->Indices[52] = 8+1;
	mb->Indices[53] = 12+0;
	mb->setDirty();
	mb->recalculateBoundingBox();
}

void Drawer2D::drawRectWithCorner(const irr::core::rect<irr::f32>& rectangle, irr::f32 uvCornerSize, irr::f32 realCornerSize, irr::video::ITexture* tex, irr::video::SColor color, const irr::core::rect<irr::s32>* clip){
	mb.MappingHint_Index = EHM_NEVER;
	mb.MappingHint_Vertex = EHM_NEVER;
	mb.Material = SMaterial(stdMat);
	fillRectWithCornerMeshBuffer(&mb, rectangle, uvCornerSize, realCornerSize, tex, color);
	if(clip){
		draw2DMeshBuffer(&mb, vector2d<f32>(0,0), vector2d<f32>(1.f,1.f), 0.f, vector2d<f32>(0.f,0.f), clip);
	}else{
		draw2DMeshBuffer(&mb);
	}
}

void Drawer2D::drawFilledPolygon(irr::core::array< irr::core::vector2d<irr::f32> >& p, irr::video::SColor color, irr::core::vector2d<irr::f32> bbMin, irr::core::vector2d<irr::f32> bbMax, irr::video::ITexture* tex, float texScale){
	cCalc.set2DTransforms(device);	
	//Triangulieren
	triangulated.set_used(0);
	Triangulate::Process(p, triangulated);//bool success = 
	//Meshbuffer aufbauen
	mb.MappingHint_Index = EHM_NEVER;
	mb.MappingHint_Vertex = EHM_NEVER;
	mb.Material = SMaterial(stdMat);//gleiches Material wie für Linien
	mb.Material.TextureLayer[0].Texture = tex;//NULL auch ok
	mb.Material.BackfaceCulling = false;//da Backfaces ggf. verwendet beim Indizieren
	int vc = triangulated.size();
	if(vc>MAX_VERTICES){return;}//TODO writeLog("Too many vertices: ");writeLog(vc);writeLog("\n");
	//Vertices aufbauen
	mb.Vertices.set_used(vc);
	f32 bbW = bbMax.X-bbMin.X;
	f32 bbH = bbMax.Y-bbMin.Y;
	for(int i=0; i<vc; i++){
		S3DVertex& v = mb.Vertices[i];
		v.Color = color;
		v.Normal = vector3d<f32>(0.f,0.f,-1.f);
		v.Pos = vector3d<f32>(triangulated[i].X, triangulated[i].Y, 1.f);
		v.TCoords = vector2d<f32>((triangulated[i].X-bbMin.X)/bbW*texScale, (triangulated[i].Y-bbMin.Y)/bbH*texScale);
	}
	//Indices aufbauen
	mb.Indices.set_used(0);
	for(int i=0; i<vc; i++){
		mb.Indices.push_back(i);
	}
	mb.setDirty();
	//ViewParameter setzen + Rendern #2
	driver->setMaterial(mb.Material);
	SET_MATERIAL_WORKAROUND(driver, mb.Material)
	driver->drawMeshBuffer(&mb);
	UNSET_MATERIAL_WORKAROUND
	if(autoResetEnabled){
		cCalc.reset2DTransforms(device);
	}
}

static vector2d<f32> calcArcPoint(const irr::core::vector2d<irr::f32>& center, irr::f32 radius, irr::f32 angle){
	irr::f32 angleRad = angle/DEG_RAD;
	return vector2d<f32>(center.X-radius*cos(angleRad), center.Y-radius*sin(angleRad));
}

void Drawer2D::fillArcMeshBuffer(irr::scene::SMeshBuffer* mb, irr::video::ITexture* tex, irr::core::vector2d<irr::f32> center, irr::f32 radius, irr::f32 startAngle, irr::f32 endAngle, irr::f32 deltaAngle, irr::video::SColor color, irr::f32 height, float lineHandle, float texPerLinePixel, bool weldedVertices, irr::f32 aaFactor){
	startAngle = Angle0to360(startAngle);
	endAngle = Angle0to360(endAngle);
	if(endAngle<startAngle){endAngle += 360.f;}
	s32 subdivisions = (s32)((endAngle-startAngle)/deltaAngle);
	irr::core::array< irr::core::vector2d<irr::f32> > poly;
	poly.reallocate(2+subdivisions);
	poly.push_back(calcArcPoint(center, radius, startAngle));
	f32 realDelta = (endAngle-startAngle)/(subdivisions+1);
	for(s32 i=0; i<subdivisions; i++){
		poly.push_back(calcArcPoint(center, radius, startAngle+i*realDelta));
	}
	poly.push_back(calcArcPoint(center, radius, endAngle));
	fillPolygonMeshBuffer(mb, poly, color, height, texPerLinePixel, tex, false, lineHandle, weldedVertices, aaFactor);
}

#define MIN_SHARP_ANGLE 0.785398163397448//45° in radians (angle which defines the maximum border width in sharp corners of a polyline, smaller angles mean larger border width)
static const irr::f32 maxBorderWidthFactor = 1.f/sinf(MIN_SHARP_ANGLE/2.f);

void Drawer2D::fillPolygonMeshBuffer(irr::scene::SMeshBuffer* mb, irr::core::array< irr::core::vector2d<irr::f32> >& inPoly, irr::video::SColor color, irr::f32 lineThickness, float texPerLinePixel, irr::video::ITexture* tex, bool closed, float lineHandle, bool weldedVertices, irr::f32 aaFactor){
	f32 maxThickness = lineThickness*maxBorderWidthFactor;
	mb->Material.TextureLayer[0].Texture = tex;//NULL also fine
	mb->Material.BackfaceCulling = false;//otherwise index order is important
	mb->Material.MaterialType = useOverridePolyMatType?overridePolyMatType:(tex==NULL?polyMatNoTexType:polyMatType);
	mb->Material.Shininess = aaFactor;
	#ifdef NEW_POLY_ALGORITHM
	core::array< vector2d<irr::f32> >& p = inPoly;
	#else
//	irr::core::array< irr::core::vector2d<irr::f32> >& p = inPoly;
	preprocessPoly(inPoly, preprocessedPoly, lineThickness, lineHandle, closed);
	core::array< vector2d<irr::f32> >& p = preprocessedPoly;
	#endif
	int vc = p.size();
	if(vc<=1){return;}//vc<=1 meaningless (may also lead to crashes)
	#ifdef NEW_POLY_ALGORITHM
	//new poly creation:
	int32_t maxVCnt = closed?(4*(vc+1)):(4*vc);
	if(maxVCnt>MAX_VERTICES){return;}
	mb->Vertices.set_used(0);
	mb->Vertices.reallocate(maxVCnt);
	mb->Indices.set_used(0);
	mb->Indices.reallocate(4*3*maxVCnt);
	vector2d<f32> lastP = closed?p[vc-1]:(p[0]+vector2d<f32>(2*lineThickness,0));//TODO: what if p[vc-1] closer p p[0] than lineThickness ??? TODO: #2: what if p[vc-1] will not be used because too close to p[vc-2]???
	f32 tcoord = 0.f;
	f32 invLineHandle = 1.f-lineHandle;
	int32_t m1Indices[2] = {-1, -1}; int32_t m2Index = -1;//indices which must be corrected after finished closed polygon
	for(int32_t i=0; i<vc; i++){
		vector2d<f32>& thisP = p[i];
		vector2d<f32> dir = thisP-lastP;
		f32 len = dir.getLength();
		if(len>lineThickness || (closed && i==vc-1)){//omit vertices closer than line thickness
			dir /= len;//normalize
			//Find next polyline vertex index and next direction
			int nextIndex = i+1;
			nextIndex = closed?nextIndex%vc:Min(nextIndex,vc-1);//in case of closed nextIndex can be 0
			vector2d<f32> nextDir = p[nextIndex]-thisP;
			f32 nextLen = nextDir.getLength();
			nextDir /= nextLen;
			//Calc/Add Vertices and Triangles
			s32 offset = mb->Vertices.size();
			vector2d<f32> leftDir = leftSide(dir), leftNextDir = leftSide(nextDir);
			vector2d<f32> handleOffset;
			vector2d<f32> v2, v3;
			if(closed || (i>=1 && i<vc-1)){//create curve with lastP and nextP (dir and leftDir and nextDir and leftNextDir is only valid if this condition is true)
				if(leftDir.dotProduct(nextDir)<0.0174524064372834f){//right turn (<cos(89°))
					v2 = thisP;
					v3 = v2+leftNextDir*lineThickness;
					vector2d<f32> v0 = v2+leftDir*lineThickness;
					vector2d<f32> alternative = v2-maxThickness*(leftDir+leftNextDir).normalize();
					vector2d<f32> v1 = calcLineIntersection(v0, dir, v3, nextDir, alternative);
					vector2d<f32> v2v1 = v1-v2;
					f32 v2v1Len = v2v1.getLength();
					if(v2v1Len>maxThickness+FLOAT_EPS){
						v2v1 *= maxThickness/v2v1Len;
						v1 = v2+v2v1;
					}
					handleOffset = -v2v1*invLineHandle;
					v0 += handleOffset; v1 += handleOffset;
					mb->Vertices.push_back(S3DVertex(v0.X, v0.Y, 1.f, 0.f, 0.f, -1.f, color, tcoord, 0.f));
					mb->Vertices.push_back(S3DVertex(v1.X, v1.Y, 1.f, 0.f, 0.f, -1.f, color, tcoord+v1.getDistanceFrom(v0)*texPerLinePixel, 0.f));//mirrors texture between 0 -> 1 <- 3
					if(closed && offset<2){//in case of closed triangles remember indices which must be corrected at end
						uint32_t is = mb->Indices.size();
						m1Indices[0] = is;
						m1Indices[1] = is+3;
						m2Index = is+5;
					}
					mb->Indices.push_back(offset-1);//
					mb->Indices.push_back(offset);
					mb->Indices.push_back(offset+2);
					mb->Indices.push_back(offset-1);//
					mb->Indices.push_back(offset+2);
					mb->Indices.push_back(offset-2);
					mb->Indices.push_back(offset);//
					mb->Indices.push_back(offset+1);
					mb->Indices.push_back(offset+2);
					mb->Indices.push_back(offset+2);//
					mb->Indices.push_back(offset+1);
					mb->Indices.push_back(offset+3);
					//}
				}else{//left turn
					vector2d<f32> v1 = thisP;
					vector2d<f32> alternative = v1-maxThickness*(leftDir+leftNextDir).normalize();
					v3 = calcLineIntersection(v1+leftDir*lineThickness, dir, v1+leftNextDir*lineThickness, nextDir, alternative);
					vector2d<f32> v1v3 = v3-v1;
					f32 v1v3Len = v1v3.getLength();
					if(v1v3Len>maxThickness+FLOAT_EPS){
						v1v3 *= maxThickness/v1v3Len;
						v1v3Len = maxThickness;
						v3 = v1+v1v3;
					}
					vector2d<f32> limitLineDir = (p[nextIndex]-lastP).normalize();
					vector2d<f32> limitPoint = calcLineIntersection(lastP, limitLineDir, v1, v1v3, v3);//Intersection of line from current poly vertex with the connection between the previous and next vertex as limit for the inner S3DVertex
					if(v1v3Len>limitPoint.getDistanceFrom(v1)+FLOAT_EPS){
						v3 = limitPoint+v1v3*(lineThickness/v1v3Len);
					}
					vector2d<f32> v0 = calcLineIntersection(v3, leftDir, lastP, dir, v1);
					v2 = calcLineIntersection(v3, leftNextDir, v1, nextDir, v1);
					handleOffset = (v1-v3)*invLineHandle;
					v0 += handleOffset; v1 += handleOffset;
					mb->Vertices.push_back(S3DVertex(v0.X, v0.Y, 1.f, 0.f, 0.f, -1.f, color, tcoord, 1.f));
					mb->Vertices.push_back(S3DVertex(v1.X, v1.Y, 1.f, 0.f, 0.f, -1.f, color, tcoord+v1.getDistanceFrom(v0)*texPerLinePixel, 1.f));//mirrors texture between 0 -> 1 <- 2
					if(closed && offset<2){//in case of closed triangles remember indices which must be corrected at end
						uint32_t is = mb->Indices.size();
						m1Indices[0] = is+1;
						m1Indices[1] = is+5;
						m2Index = is+4;
					}
					mb->Indices.push_back(offset);//
					mb->Indices.push_back(offset-1);
					mb->Indices.push_back(offset+3);
					mb->Indices.push_back(offset);//
					mb->Indices.push_back(offset-2);
					mb->Indices.push_back(offset-1);
					mb->Indices.push_back(offset);//
					mb->Indices.push_back(offset+3);
					mb->Indices.push_back(offset+1);
					mb->Indices.push_back(offset+1);//
					mb->Indices.push_back(offset+3);
					mb->Indices.push_back(offset+2);
					//}
				}
			}else{
				v2 = thisP;
				if(i==0){
					handleOffset = -leftNextDir*(lineThickness*invLineHandle);
					v3 = v2+leftNextDir*lineThickness;
				}else if(i==vc-1){
					handleOffset = -leftDir*(lineThickness*invLineHandle);
					v3 = v2+leftDir*lineThickness;
					mb->Indices.push_back(offset-1);//
					mb->Indices.push_back(offset+1);
					mb->Indices.push_back(offset-2);
					mb->Indices.push_back(offset-2);//
					mb->Indices.push_back(offset+1);
					mb->Indices.push_back(offset);
				}
			}
			v2 += handleOffset; v3 += handleOffset;
			mb->Vertices.push_back(S3DVertex(v2.X, v2.Y, 1.f, 0.f, 0.f, -1.f, color, tcoord, 1.f));
			mb->Vertices.push_back(S3DVertex(v3.X, v3.Y, 1.f, 0.f, 0.f, -1.f, color, tcoord, 0.f));
			lastP = thisP;
			tcoord += nextLen*texPerLinePixel;
		}
	}
	if(closed){//correct indices for proper triangles in closed polygons
		uint32_t finalVC = mb->Vertices.size();
		mb->Indices[m2Index] = finalVC-2;
		mb->Indices[m1Indices[0]] = finalVC-1;
		mb->Indices[m1Indices[1]] = finalVC-1;
	}
	#else
	//TODO in case of closed: adapt texture coordinates of last vertices???
	if(weldedVertices){//Less vertices but bad texture coordinates TODO: refactor common stuff with unwelded vertices calculation
		//Create vertices
		int vcnt = closed?(2*(vc+1)):(2*vc);
		if(vcnt>MAX_VERTICES){return;}
		mb->Vertices.set_used(vcnt);
		f32 tcoord = 0.f;
		for(int i=0; i<vc; i++){
			S3DVertex& v = mb->Vertices[2*i];//Outer vertex
			S3DVertex& vi = mb->Vertices[2*i+1];//Inner vertex
			vi.Color = v.Color = color;
			vi.Normal = v.Normal = vector3d<f32>(0.f,0.f,-1.f);
			vector2d<f32> d0 = (p[i]-p[(vc+i-1)%vc]).normalize();//Direction of the first line
			vector2d<f32> d1 = (p[i]-p[(i+1)%vc]).normalize();//Direction of the second line
			vector2d<f32> viP, normal(1.f,0.f);
			if(!closed && (i==0 || i==vc-1)){
				if(i==0){
					normal = -leftSide(d1).normalize();
				}else{
					normal = leftSide(d0).normalize();
				}				
				viP = p[i]-lineThickness*normal;
			}else{
				normal = (leftSide(d0)-leftSide(d1)).normalize();//Add left and other right (negative other left) side
				vector2d<f32> alternative = p[i]-lineThickness*normal;
				viP = calcLineIntersection(p[i]-lineThickness*leftSide(d0), d0, p[i]+lineThickness*leftSide(d1), d1, alternative);
			}
			vector2d<f32> deltaP = viP-p[i];
			if(deltaP.getLength()>maxThickness){
				deltaP = maxThickness*vector2d<f32>(deltaP).normalize();
				viP = p[i] + deltaP;
			}
			//Centered
			vector2d<f32> conn = lineHandle*deltaP;//Part of the connecting line
			vector2d<f32> insideVertex = p[i]-conn;
			v.Pos = vector3d<f32>(insideVertex.X, insideVertex.Y, 1.f);
			vector2d<f32> outsideVertex = viP-conn;
			vi.Pos = vector3d<f32>(outsideVertex.X, outsideVertex.Y, 1.f);
			v.TCoords = vector2d<f32>(tcoord, 0.f);
			vi.TCoords = vector2d<f32>(tcoord, 1.f);
			f32 delta = (p[(i+1)%vc]-p[i]).getLength()*texPerLinePixel;
			tcoord += delta;
		}
		if(closed){//copy first vertices and set tex coords
			mb->Vertices[2*vc] = mb->Vertices[0];
			mb->Vertices[2*vc+1] = mb->Vertices[1];
			mb->Vertices[2*vc].TCoords = vector2d<f32>(tcoord, 0.f);
			mb->Vertices[2*vc+1].TCoords = vector2d<f32>(tcoord, 1.f);
		}
		//Create indices
		int toIndex = closed?vc:(vc-1);
		mb->Indices.set_used(2*3*toIndex);
		for(int i=0; i<toIndex; i++){
			mb->Indices[2*3*i] = 2*i;//1 triangle
			mb->Indices[2*3*i+1] = 2*(i+1);
			mb->Indices[2*3*i+2] = 2*(i+1)+1;
			mb->Indices[2*3*i+3] = 2*i;//2 triangle
			mb->Indices[2*3*i+4] = 2*(i+1)+1;
			mb->Indices[2*3*i+5] = 2*i+1;
		}
	}else{//double amount of vertices but good texture coordinates
		//Create vertices
		int vcnt = 4*vc-(closed?0:2);
		if(vcnt>MAX_VERTICES){return;}
		mb->Vertices.set_used(vcnt);
		f32 tcoord = 0.f;
		for(int i=0; i<vc; i++){
			S3DVertex& v = mb->Vertices[(4*i+vcnt-2)%vcnt];//Outer vertex (previous)
			S3DVertex& vi = mb->Vertices[(4*i+1+vcnt-2)%vcnt];//Inner vertex
			S3DVertex& v2 = mb->Vertices[4*i];//Outer vertex #2 (current)
			S3DVertex& vi2 = mb->Vertices[4*i+1];//Inner vertex #2
			vi.Color = v.Color = v2.Color = vi2.Color = color;
			vi.Normal = v.Normal = v2.Normal = vi2.Normal = vector3d<f32>(0.f,0.f,-1.f);
			vector2d<f32> d0 = (p[i]-p[(vc+i-1)%vc]).normalize();//Direction of the first inner line
			vector2d<f32> d1 = (p[i]-p[(i+1)%vc]).normalize();//Direction of the second inner line
			vector2d<f32> viP, normal(1.f,0.f);
			if(!closed && (i==0 || i==vc-1)){
				if(i==0){
					normal = -leftSide(d1).normalize();
				}else{
					normal = leftSide(d0).normalize();
				}
				viP = p[i]-lineThickness*normal;
			}else{
				normal = (leftSide(d0)-leftSide(d1)).normalize();//Add left and other right (negative other left) side
				vector2d<f32> alternative = p[i]-lineThickness*normal;
				viP = calcLineIntersection(p[i]-lineThickness*leftSide(d0), d0, p[i]+lineThickness*leftSide(d1), d1, alternative);
			}
			vector2d<f32> deltaP = viP-p[i];
			if(deltaP.getLength()>maxThickness){
				deltaP = maxThickness*vector2d<f32>(deltaP).normalize();
				viP = p[i] + deltaP;
			}
			//Centered
			vector2d<f32> conn = lineHandle*deltaP;//Part of the connecting line
			vector2d<f32> insideVertex = p[i]-conn;
			v2.Pos = v.Pos = vector3d<f32>(insideVertex.X, insideVertex.Y, 1.f);
			vector2d<f32> outsideVertex = viP-conn;
			vi2.Pos = vi.Pos = vector3d<f32>(outsideVertex.X, outsideVertex.Y, 1.f);
			v.TCoords = vector2d<f32>(tcoord+(insideVertex-p[i]).dotProduct(d0)*texPerLinePixel, 0.f);//Projection onto d0 (direction vector) yields length offset into d0 direction => texcoord offset
			vi.TCoords = vector2d<f32>(tcoord+(outsideVertex-p[i]).dotProduct(d0)*texPerLinePixel, 1.f);//analog
			v2.TCoords = vector2d<f32>(tcoord+(insideVertex-p[i]).dotProduct(-d1)*texPerLinePixel, 0.f);//analog
			vi2.TCoords = vector2d<f32>(tcoord+(outsideVertex-p[i]).dotProduct(-d1)*texPerLinePixel, 1.f);//analog
			f32 delta = (p[(i+1)%vc]-p[i]).getLength()*texPerLinePixel;
			tcoord += delta;
		}
		if(closed){//Correct texture coordinates, since last ones 0 (created at beginning).
			mb->Vertices[vcnt-1].TCoords.X += tcoord;//Addition (offset previously already calculated)
			mb->Vertices[vcnt-2].TCoords.X += tcoord;
		}
		//Create indices
		int toIndex = closed?vc:(vc-1);
		mb->Indices.set_used(2*3*toIndex);
		for(int i=0; i<toIndex; i++){
			mb->Indices[2*3*i] = 4*i;//1 triangle
			mb->Indices[2*3*i+1] = 4*(i+1)-2;
			mb->Indices[2*3*i+2] = 4*(i+1)+1-2;
			mb->Indices[2*3*i+3] = 4*i;//2 triangle
			mb->Indices[2*3*i+4] = 4*(i+1)+1-2;
			mb->Indices[2*3*i+5] = 4*i+1;
		}
	}
	#endif
	mb->setDirty();
	mb->recalculateBoundingBox();
}

void preprocessPoly(const irr::core::array< irr::core::vector2d<irr::f32> >& inP, irr::core::array< irr::core::vector2d<irr::f32> >& outP, irr::f32 lineThickness, irr::f32 lineHandle, bool closed){
	lineHandle = Clamp(lineHandle, 0.01f, 0.99f);//for preventing d < 0+eps
	uint32_t vc = inP.size();
	outP.set_used(0);
	if(vc<2){return;}
	outP.reallocate(vc*3, false);//outP.reallocate(vc*2, false);//upper limit
	vector2d<f32> lastP;
	uint32_t start = 0;
	if(closed){
		lastP = inP[vc-1];
	}else{
		lastP = inP[0];
		outP.push_back(inP[0]);
		start = 1;
	}
	f32 positionEps = lineThickness*0.01f;
	f32 l1 = lineHandle*lineThickness;
	f32 l2 = (1.f-lineHandle)*lineThickness;
	for(uint32_t i=start; i<vc; i++){
		const vector2d<f32>& thisP = inP[i];
		f32 lastDistance = thisP.getDistanceFrom(lastP);
		if(lastDistance>positionEps){
			if(closed || i<vc-1){
				const vector2d<f32>& nextP = inP[(i+1)%vc];
				vector2d<f32> toLast = (lastP-thisP).normalize();
				vector2d<f32> toNext = (nextP-thisP).normalize();
				bool isLeftTurn = leftSide(-toLast).dotProduct(toNext)>0.f;
				f32 phi = 0.5f*acosf(toLast.dotProduct(toNext));
				if(phi<MIN_SHARP_ANGLE){
					f32 d = Max((isLeftTurn?l1:l2)/tanf(phi), (f32)FLOAT_EPS);//1.f;//eps/cosf(phi);//
					if(lastDistance>d+FLOAT_EPS){
						outP.push_back(thisP+toLast*d);
						outP.push_back(thisP);
						f32 nextDistance = thisP.getDistanceFrom(nextP);
						if(nextDistance>d+FLOAT_EPS){
							outP.push_back(thisP+toNext*d);
						}else if(nextDistance>positionEps){
							outP.push_back(nextP);
							i++;
						}
					}else{
						outP.push_back(thisP);
					}
				}else{
					outP.push_back(thisP);
				}
				lastP = outP[outP.size()-1];
			}else{
				outP.push_back(thisP);
				lastP = thisP;
			}
		}
//		}else{
//			outP.push_back(thisP);
//			lastP = thisP;
//		}
	}
	//std::cout << "inP.size(): " << inP.size() << " outP.size(): " << outP.size() << std::endl;
}

void Drawer2D::drawPolygon(irr::core::array< irr::core::vector2d<irr::f32> >& p, irr::video::SColor color, irr::f32 lineThickness, float texPerLinePixel, irr::video::ITexture* tex, bool closed, float lineHandle, bool weldedVertices){
	//Build meshbuffer
	mb.MappingHint_Index = EHM_NEVER;
	mb.MappingHint_Vertex = EHM_NEVER;
	mb.Material = SMaterial(stdMat);
	fillPolygonMeshBuffer(&mb, p, color, lineThickness, texPerLinePixel, tex, closed, lineHandle, weldedVertices, defaultPolyAAValue);
	//Render meshbuffer
	draw2DMeshBuffer(&mb);
/*
	//Testsave
	if(!saved){
		saved = true;
		scene::IMeshWriter* writer = device->getSceneManager()->createMeshWriter(EMWT_PLY);//EMWT_STL);//EMWT_IRR_MESH);//EMWT_OBJ);
		io::IWriteFile* file = device->getSceneManager()->getFileSystem()->createAndWriteFile("test.ply");
		SMesh m;
		mb.Vertices[0].Pos.Z=0.0;//damit BB entsteht
		m.MeshBuffers.push_back(&mb);
		m.recalculateBoundingBox();
		writer->writeMesh(file,&m);
		m.MeshBuffers.clear();
		writer->drop();
		file->drop();
	}
*/
}

void Drawer2D::setDefaultPolygonAAValue(irr::f32 aaValue){
	defaultPolyAAValue = aaValue;
}
	
irr::f32 Drawer2D::getDefaultPolygonAAValue() const{
	return defaultPolyAAValue;
}

void Drawer2D::draw2DMeshBuffer(irr::scene::SMeshBuffer* mb, const irr::core::vector2d<irr::f32>& translation, const irr::core::vector2d<irr::f32>& scale, irr::f32 angle, const irr::core::vector2d<irr::f32>& origin, const irr::core::rect<irr::s32>* clip){
	irr::core::rect<s32> oldVport = driver->getViewPort();
	irr::core::rect<s32> vport(oldVport);
	vector2d<irr::f32> finalTranslation(translation);
	if(clip){
		driver->setViewPort(*clip);
		finalTranslation.X -= clip->UpperLeftCorner.X;
		finalTranslation.Y -= clip->UpperLeftCorner.Y;
		vport = *clip;
	}
	dimension2d<u32> vdim(vport.getWidth(), vport.getHeight());
	cCalc.setAspectRatio((f32)vdim.Width/(f32)vdim.Height);
	irr::core::matrix4 T = createOptimized2DTransform(finalTranslation, scale, angle/DEG_RAD, origin, cCalc.getCameraScreenParameters(), vdim, cCalc.getRecommendedInverseProjectionDistance());
	cCalc.set2DTransforms(device, &T, true);
	driver->setMaterial(mb->Material);
	SET_MATERIAL_WORKAROUND(driver, mb->Material)
	driver->drawMeshBuffer(mb);
	UNSET_MATERIAL_WORKAROUND
	if(autoResetEnabled || clip){
		cCalc.reset2DTransforms(device);
	}
	if(clip){
		driver->setViewPort(oldVport);
	}
}

void Drawer2D::draw2DMeshBuffer(irr::scene::SMeshBuffer* mb){
	cCalc.set2DTransforms(device);
	driver->setMaterial(mb->Material);
	SET_MATERIAL_WORKAROUND(driver, mb->Material)
	driver->drawMeshBuffer(mb);
	UNSET_MATERIAL_WORKAROUND
	if(autoResetEnabled){
		cCalc.reset2DTransforms(device);
	}
}

void Drawer2D::setFiltering(bool bilinear,bool trilinear, int anisotropic){
	for(u32 j=0; j<MATERIAL_MAX_TEXTURES; j++){
		stdMat.TextureLayer[j].TrilinearFilter = trilinear;
		stdMat.TextureLayer[j].BilinearFilter = bilinear;
		stdMat.TextureLayer[j].AnisotropicFilter = anisotropic;
	}
}

void Drawer2D::setColor(SColor color){
	for(int i = 0; i<4; i++){
		stdColor[i] = color;
	}
}

irr::video::SColor Drawer2D::getColor(irr::u32 index) const{
	return stdColor[index];
}

void Drawer2D::setColors(irr::video::SColor* colors){
	for(int i = 0; i<4; i++){
		stdColor[i] = colors[i];
	}
}

void Drawer2D::setTextureCoordinates(vector2d<f32>* coords){
	for(int i=0; i<4; i++){
		tcoords[i] = coords[i];
	}
}

void Drawer2D::setTextureCoordinates(){
	tcoords[0] = vector2d<f32>(0.f,0.f);
	tcoords[1] = vector2d<f32>(1.f,0.f);
	tcoords[2] = vector2d<f32>(0.f,1.f);
	tcoords[3] = vector2d<f32>(1.f,1.f);
}

void Drawer2D::setMirroring(bool horizontal, bool vertical){
	for(int i = 0; i<4; i++){
		if(horizontal){tcoords[i].X = 1.0f-tcoords[i].X;}
		if(vertical){tcoords[i].Y = 1.0f-tcoords[i].Y;}
	}
}

void Drawer2D::setMaterialType(E_MATERIAL_TYPE type){
	stdMat.MaterialType = type;
}

irr::video::E_MATERIAL_TYPE Drawer2D::getMaterialType(){
	return stdMat.MaterialType;
}

static void calcRoundLoadingBar(irr::core::array< irr::core::vector2d<irr::f32> >& out, const irr::core::rect<irr::s32>& r, irr::f32 maxX){
	out.set_used(0);
	out.reallocate(360);
	f32 radius = 0.5f*r.getHeight();
	vector2d<f32> center(r.LowerRightCorner.X-radius, r.LowerRightCorner.Y-radius);
	for(int i=0; i<180; i++){//1 degrees per vertex
		f32 angle = i/DEG_RAD;
		out.push_back(center + vector2d<f32>(radius*sinf(angle), radius*cosf(angle)));
	}
	center = vector2d<f32>(r.UpperLeftCorner.X+radius, r.UpperLeftCorner.Y+radius);
	for(int i=0; i<180; i++){//1 degrees per vertex
		f32 angle = i/DEG_RAD;
		out.push_back(center + vector2d<f32>(-radius*sinf(angle), -radius*cosf(angle)));
	}
	for(uint32_t i=0; i<out.size(); i++){
		if(out[i].X>maxX){
			out[i].X = maxX;
		}
	}
}

void Drawer2D::drawRoundLoadingBar(const irr::video::SColor& color, const irr::core::rect<irr::s32>& positions, irr::f32 progress, irr::f32 lineThicknes){
	irr::core::array< irr::core::vector2d<irr::f32> > poly;
	calcRoundLoadingBar(poly, positions, positions.LowerRightCorner.X);
	drawPolygon(poly, color, lineThicknes, 0.01f, NULL, true, 0.5f, true);
	s32 dist = 0.3*Min(positions.getWidth(), positions.getHeight());
	s32 origin = positions.UpperLeftCorner.X+dist;
	rect<s32> r(origin, positions.UpperLeftCorner.Y+dist, positions.LowerRightCorner.X-dist, positions.LowerRightCorner.Y-dist);
	s32 fillX = origin+progress*(r.LowerRightCorner.X-origin);
	calcRoundLoadingBar(poly, r, fillX);
	auto mt = getMaterialType();
	setMaterialType(EMT_SOLID);
	drawFilledPolygon(poly, color, vector2d<f32>(0,0), vector2d<f32>(1,1), NULL);
	setMaterialType(mt);
}

void drawLoadingBar(irr::IrrlichtDevice* device, const irr::video::SColor& color, const irr::core::rect<irr::s32>& positions, irr::f32 progress){
	IVideoDriver* driver = device->getVideoDriver();
	s32 dist = 0.2*Min(positions.getWidth(), positions.getHeight());
	s32 origin = positions.UpperLeftCorner.X+dist;
	rect<s32> r(origin, positions.UpperLeftCorner.Y+dist, origin+progress*(positions.LowerRightCorner.X-dist-origin), positions.LowerRightCorner.Y-dist);
	driver->draw2DRectangleOutline(positions, color);
	driver->draw2DRectangle(color, r, NULL);
}

static inline f32 calcTriangleArea(const vector2d<f32>& a, const vector2d<f32>& b, const vector2d<f32>& c){
	return fabsf(.5f*(a.X*(b.Y-c.Y)+b.X*(c.Y-a.Y)+c.X*(a.Y-b.Y)));
}

static inline bool mustRemoveB(const vector2d<f32>& a, const vector2d<f32>& b, const vector2d<f32>& c, irr::f32 cutoffArea){
	vector2d<f32> ba = a-b;
	vector2d<f32> bc = c-b;
	f32 mulLength = ba.getLength()*bc.getLength();
	if(mulLength<0.001f*cutoffArea || mulLength<0.000001f){return true;}
	double alpha = acos((double)ba.dotProduct(bc)/(double)mulLength);
	f32 area = calcTriangleArea(a,b,c);
	double m = ((double)area)*tan(0.5*alpha);
	return m<cutoffArea;
}

//static inline bool mustRemoveB(const vector2d<f32>& a, const vector2d<f32>& b, const vector2d<f32>& c, irr::f32 cutoffArea){
//	return calcTriangleArea(a,b,c)<cutoffArea;
//}

void optimizePolygon(irr::core::array< irr::core::vector2d<irr::f32> >& polygon, irr::f32 cutoffArea, bool isClosedPolygon){
	uint32_t shrink = 0;
	int32_t size = polygon.size();
	if(size<3){return;}
	int32_t start = isClosedPolygon?0:1;
	int32_t end = isClosedPolygon?size:(size-1);
	for(int32_t i=start; i<end; i++){
		int32_t nextIndex = (i+1)%size;
		int32_t ims = i-shrink;
		int32_t prevIndex = ims==0?(size-1):(ims-1);
		if(mustRemoveB(polygon[prevIndex],polygon[i],polygon[nextIndex],cutoffArea)){
			shrink++;//remove by increasing the shrink counter
			int32_t j = i-shrink;
			while(j>=start){//recheck reverse for required removals which appear because of the current removal
				if(mustRemoveB(polygon[j==0?(size-1):(j-1)],polygon[j],polygon[nextIndex],cutoffArea)){
					shrink++;//remove by increasing the shrink counter
					j = i-shrink;
				}else{
					break;
				}
			}
		}else if(shrink>0){
			polygon[ims] = polygon[i];
		}
	}
	polygon.set_used(size-shrink);
}
