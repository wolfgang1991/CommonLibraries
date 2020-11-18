#ifndef Drawer2D_H_INCLUDED
#define Drawer2D_H_INCLUDED

#include "Transformation2DHelpers.h"

#include <SMeshBuffer.h>
#include <SColor.h>
#include <SMaterial.h>
#include <irrTypes.h>
#include <vector2d.h>
#include <vector3d.h>
#include <line3d.h>
#include <dimension2d.h>
#include <irrArray.h>

namespace irr{
	class IrrlichtDevice;
	namespace scene{
		class ISceneManager;
		class ICameraSceneNode;
	}
	namespace video{
		class ITexture;
		class IVideoDriver;
	}
}

//! Draw 2D Images, Polygons, Arcs with 3D geometry (e.g. useful for Shading, Transformation)
class Drawer2D{

	private:

	irr::IrrlichtDevice* device;
	irr::scene::ISceneManager* smgr;
	irr::video::IVideoDriver* driver;
	
	irr::scene::SMeshBuffer mb;//for polygons

	irr::scene::SMeshBuffer imgMb;//for 2D images
	irr::video::SColor stdColor[4];
	irr::video::SMaterial stdMat;
	irr::core::vector2d<irr::f32> v[4];
	irr::core::vector2d<irr::f32> tcoords[4];
	
	Camera2DParameterCalculator cCalc;
	
	bool autoResetEnabled;
	
	irr::f32 defaultPolyAAValue;
	
	irr::video::E_MATERIAL_TYPE overridePolyMatType;
	bool useOverridePolyMatType;
	
	irr::core::array< irr::core::vector2d<irr::f32> > preprocessedPoly;//common intermediate representation for preprocessed polys to prevent large reallocations

	public:

	Drawer2D(irr::IrrlichtDevice* device);

	~Drawer2D();
	
	irr::IrrlichtDevice* getDevice(){return device;}
	
	irr::video::SMaterial& getMaterial(){return stdMat;}

	irr::core::line3d<irr::f32> getRayFromScreenCoordinatesImgCam(const irr::core::position2d<irr::f32> & pos);

	//! Draw a textured line
	void drawLine(irr::video::ITexture* tex, irr::core::position2d<int> start, irr::core::position2d<int> end, int height = 5, float verticalOffset = 0.5);

	void setTextureCoordinates(irr::core::vector2d<irr::f32>* coords);
	
	//! set defaults
	void setTextureCoordinates();

	void draw(irr::video::ITexture* tex, irr::core::position2d<int> pos, irr::core::dimension2d<int> size = irr::core::dimension2d<int>(0, 0), float rotation = 0.0f, irr::core::vector2d<irr::f32> origin = irr::core::vector2d<irr::f32>(0, 0));
	
	void draw(irr::video::ITexture* tex, irr::core::position2d<irr::f32> pos, irr::core::dimension2d<irr::f32> size = irr::core::dimension2d<irr::f32>(0, 0), float rotation = 0.0f, irr::core::vector2d<irr::f32> origin = irr::core::vector2d<irr::f32>(0, 0));
	
	void draw(irr::video::ITexture* tex, const irr::core::rect<irr::s32>& screenRect, const irr::core::rect<irr::s32>* clipRect = NULL);

	//! Draw a textured arc, m center, r radius, startAngle and endAngle in degrees
	void drawArc(irr::video::ITexture* tex, irr::core::position2d<int> m, float r, float startAngle, float endAngle, float deltaAngle = 5, int height = 5, float verticalOffset = 0.5);

	//! Filter settings
	void setFiltering(bool bilinear = true,bool trilinear = true, int anisotropic = 16);

	//! Set all vertex colors for Images, Lines and Arcs (Polys have their own color)
	void setColor(irr::video::SColor color = irr::video::SColor(255,255,255,255));

	//! Set 4 different vertex colors for Images, Lines and Arcs (Polys have their own color)
	void setColors(irr::video::SColor* colors);

	//! Set up Material Type
	void setMaterialType(irr::video::E_MATERIAL_TYPE type);

	irr::video::E_MATERIAL_TYPE getMaterialType();

	//! Mirror UV Coordinates for Images, call again with same parameters to undo/mirror twice
	void setMirroring(bool horizontal, bool vertical);
	
	void drawRectWithCorner(const irr::core::rect<irr::f32>& rectangle, irr::f32 uvCornerSize, irr::f32 realCornerSize, irr::video::ITexture* tex, irr::video::SColor color = irr::video::SColor(255,255,255,255), const irr::core::rect<irr::s32>* clip = NULL);
	
	//! creates a rectangle which vertices make it possible to have non sharp corners. This is done by inner and outer vertices.
	//! uvCornerSize defines the size of the (e.g. round) corners in the texture space (parts of the whole width/height). 
	//! rectangle defines the inner rectangle (without corners)
	//! realCornerSize defines the screen space corner size
	static void fillRectWithCornerMeshBuffer(irr::scene::SMeshBuffer* mb, const irr::core::rect<irr::f32>& rectangle, irr::f32 uvCornerSize, irr::f32 realCornerSize, irr::video::ITexture* tex, irr::video::SColor color = irr::video::SColor(255,255,255,255));

	//! p: Array of pixel coordinates, color vertex color, lineThickness in pixels, texPerLinePixel: texture parts [0-1] per pixel
	//! weldedVertices: true: less vertex count but distorted texture coordinates, false: double vertex count but correct texture coordinates
	void drawPolygon(irr::core::array< irr::core::vector2d<irr::f32> >& p, irr::video::SColor color, irr::f32 lineThickness = 3,float texPerLinePixel = 0.01f, irr::video::ITexture* tex = NULL, bool closed = true, float lineHandle = 0.f, bool weldedVertices = true);
	
	//! fill arc using a polygon, angle in degrees, deltaAngle: distance between subdivisions in degrees (e.g. 5)
	//! angles start at (center.X-radius, center.Y) and increase in clockwise direction
	void fillArcMeshBuffer(irr::scene::SMeshBuffer* mb, irr::video::ITexture* tex, irr::core::vector2d<irr::f32> center, irr::f32 radius, irr::f32 startAngle, irr::f32 endAngle, irr::f32 deltaAngle, irr::video::SColor color, irr::f32 height = 5.f, float lineHandle = 0.5f, float texPerLinePixel = 0.01f, bool weldedVertices = true, irr::f32 aaFactor = 0.25f);
	
	//! Parameters see drawPolygon, aaFactor: 0: no AA, 0.5: full AA with center no alpha, >0.5 extreme values (do not really make sense)
	void fillPolygonMeshBuffer(irr::scene::SMeshBuffer* mb, irr::core::array< irr::core::vector2d<irr::f32> >& p, irr::video::SColor color, irr::f32 lineThickness = 3, float texPerLinePixel = 0.01f, irr::video::ITexture* tex = NULL, bool closed = true, float lineHandle = 0.f, bool weldedVertices = true, irr::f32 aaFactor = 0.25f);
	
	void setDefaultPolygonAAValue(irr::f32 aaValue = 0.f);
	
	irr::f32 getDefaultPolygonAAValue() const;
	
	//! NULL: removes the override
	void setOverridePolygonMaterialType(irr::video::E_MATERIAL_TYPE* overrideMatType = NULL);
	
	//! Draws a meshbuffer for 2D purposes, see code of drawPolygon for an example
	void draw2DMeshBuffer(irr::scene::SMeshBuffer* mb, const irr::core::vector2d<irr::f32>& translation, const irr::core::vector2d<irr::f32>& scale = irr::core::vector2d<irr::f32>(1.f,1.f), irr::f32 angle = 0.f, const irr::core::vector2d<irr::f32>& origin = irr::core::vector2d<irr::f32>(0.f,0.f), const irr::core::rect<irr::s32>* clip = NULL);

	//! without any additional transformations
	void draw2DMeshBuffer(irr::scene::SMeshBuffer* mb);

	void setTextureWrap(irr::video::E_TEXTURE_CLAMP uwrap = irr::video::ETC_REPEAT, irr::video::E_TEXTURE_CLAMP vwrap = irr::video::ETC_REPEAT);
	
	//! restores the active cameras transformations and the world transform
	//! this is done automatically except if it is turned off
	void reset2DTransforms();
	
	//! set to true if transformations of active camera shall be restored automatically after drawing
	void setAutoResetTransformEnabled(bool enabled = true);

	//! MUST be called (on Android and on any OS which do not reinit (global) static variables on restart) before using a Drawer2D for the first time after app start
	static void initStaticVars();

	private:
	
	Drawer2D();

};

//! prerpocesses a polygon to solve sharp angles problem and to remove duplicated vertices
void preprocessPoly(const irr::core::array< irr::core::vector2d<irr::f32> >& inP, irr::core::array< irr::core::vector2d<irr::f32> >& outP, irr::f32 lineThickness, irr::f32 lineHandle, bool closed);

//! sets 2D Material parameters and type
void set2DMaterialParams(irr::video::SMaterial& material, irr::video::E_TEXTURE_CLAMP wrapU = irr::video::ETC_REPEAT, irr::video::E_TEXTURE_CLAMP wrapV = irr::video::ETC_REPEAT, irr::video::E_MATERIAL_TYPE type = irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL);

template<typename T>
void appendMeshBuffer(irr::scene::CMeshBuffer<T>* target, irr::scene::CMeshBuffer<T>* source){//TODO use a list of sources to avoid unnecesary reallocation and copying in case of multiple meshbuffers to append
	target->append(source->getVertices(), source->getVertexCount(), source->getIndices(), source->getIndexCount());
}

#endif
