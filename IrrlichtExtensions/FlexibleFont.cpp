#include "FlexibleFont.h"

#include "MaterialWorkaround.h"

#include <IrrlichtDevice.h>
#include <fast_atof.h>
#include <S3DVertex.h>
#include <SMaterial.h>
#include <EMaterialTypes.h>
#include <IMaterialRendererServices.h>
#include <IGPUProgrammingServices.h>
#include <IMaterialRenderer.h>

#include <cassert>
#include <iostream>
#include <csignal>

using namespace irr;
using namespace core;
using namespace video;
using namespace scene;

const char* OGLFontVS = 
"#ifdef GL_ES\n"
"attribute vec3 inVertexPosition;\n"
"attribute vec3 inVertexNormal;\n"
"attribute vec4 inVertexColor;\n"
"attribute vec2 inTexCoord0;\n"
"varying vec4 vVertexColor;\n"
"#endif\n"
"uniform mat4 uWVPMatrix;\n"
"varying vec2 vTextureCoord0;\n"
"\n"
"void main()\n"
"{\n"
"	#ifdef GL_ES\n"
"  vTextureCoord0 = inTexCoord0;\n"
"	vVertexColor = inVertexColor.bgra;\n"//Color order different in gles
"	gl_Position = uWVPMatrix * vec4(inVertexPosition, 1.0);\n"
"	#else\n"
"  vTextureCoord0 = gl_MultiTexCoord0.xy;\n"
"	gl_FrontColor = gl_Color;\n"
"	gl_Position = uWVPMatrix * gl_Vertex;\n"
"	#endif\n"
"}\n";

const char* OGLFontFS = 
"#ifdef GL_ES\n"
"precision mediump float;\n"
"varying vec4 vVertexColor;\n"
"#endif\n"
"uniform sampler2D uTextureUnit0;\n"
"varying vec2 vTextureCoord0;\n"
"uniform float uSmoothing;\n"
"uniform float uFatness;\n"
"uniform vec3 uBorderColor;\n"
"uniform float uBorderSize;\n"
"#ifdef SHADOWS\n"
"uniform vec2 uShadowOffset;\n"
"uniform float uShadowSmoothing;\n"
"uniform vec4 uShadowColor;\n"
"#endif\n"
"void main()\n"
"{\n"
"	float distance = texture2D(uTextureUnit0, vTextureCoord0).a;\n"
"	float alpha = smoothstep(uFatness - uSmoothing, uFatness + uSmoothing, distance);\n"
"	float borderPos = uFatness + uBorderSize;\n"
"	float borderFactor = smoothstep(borderPos - uSmoothing, borderPos + uSmoothing, distance);\n"
"	#ifdef GL_ES\n"
"	vec4 text = vec4(mix(uBorderColor, vVertexColor.rgb, borderFactor), alpha*vVertexColor.a);\n"
"	#else\n"
"	vec4 text = vec4(mix(uBorderColor, gl_Color.rgb, borderFactor), alpha*gl_Color.a);\n"
"	#endif\n"
"	#ifdef SHADOWS\n"
"	float shadowDistance = texture2D(uTextureUnit0, vTextureCoord0 - uShadowOffset).a;\n"
"	float shadowAlpha = smoothstep(uFatness - uShadowSmoothing, uFatness + uShadowSmoothing, shadowDistance);\n"
"	vec4 shadow = vec4(uShadowColor.rgb, uShadowColor.a * shadowAlpha);\n"
"	gl_FragColor = mix(shadow, text, text.a);\n"
"	#else\n"
"	gl_FragColor = text;\n"
"	#endif\n"
"}\n";

static void convertColor2RGBAFloats(irr::f32* outArray, irr::video::SColor color){
	outArray[0] = ((f32)color.getRed())/255.f;
	outArray[1] = ((f32)color.getGreen())/255.f;
	outArray[2] = ((f32)color.getBlue())/255.f;
	outArray[3] = ((f32)color.getAlpha())/255.f;
}

class SDFFCallback : public irr::video::IShaderConstantSetCallBack{

	protected:
	
	IrrlichtDevice* device;
	IVideoDriver* driver;

	bool firstSetConstants;
	s32 wvpMatrixID, textureUnit0ID;
	s32 fatnessID, smoothingID;
	s32 borderColorID, borderSizeID;
	
	FlexibleFontManager* fmgr;
	
	public:
	
	SDFFCallback(IrrlichtDevice* device, FlexibleFontManager* fmgr):
		device(device),
		driver(device->getVideoDriver()),
		firstSetConstants(true),
		fmgr(fmgr){}
		
	virtual ~SDFFCallback(){}
	
	virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData){
		if(firstSetConstants){
			wvpMatrixID = services->getVertexShaderConstantID("uWVPMatrix");
			textureUnit0ID = services->getPixelShaderConstantID("uTextureUnit0");
			smoothingID = services->getPixelShaderConstantID("uSmoothing");
			fatnessID = services->getPixelShaderConstantID("uFatness");
			borderColorID = services->getPixelShaderConstantID("uBorderColor");
			borderSizeID = services->getPixelShaderConstantID("uBorderSize");
			firstSetConstants = false;
		}
		
		const matrix4 W = driver->getTransform(ETS_WORLD);
		const matrix4 V = driver->getTransform(ETS_VIEW);
		const matrix4 P = driver->getTransform(ETS_PROJECTION);
		matrix4 Matrix = P * V * W;
		services->setVertexShaderConstant(wvpMatrixID, Matrix.pointer(), 16);
		s32 textureUnit0 = 0;
		services->setPixelShaderConstant(textureUnit0ID, &textureUnit0, 1);
		f32 fatness = fmgr->getFatness();
		services->setPixelShaderConstant(fatnessID, &fatness, 1);
		f32 smoothing = fmgr->getSmoothing();
		services->setPixelShaderConstant(smoothingID, &smoothing, 1);
		f32 borderColorF[4]; convertColor2RGBAFloats(borderColorF, fmgr->getBorderColor());
		services->setPixelShaderConstant(borderColorID, borderColorF, 3);//only rgb
		f32 borderSize = fmgr->getBorderSize();
		if(borderSize==0.f){borderSize = -1.f;}//to avoid border colored pixels if border completely disabled
		services->setPixelShaderConstant(borderSizeID, &borderSize, 1);
	}
	
};

class SDFFWithShadowsCallback : public SDFFCallback{

	protected:
	
	s32 shadowOffsetID, shadowSmoothingID, shadowColorID;
	
	public:
	
	SDFFWithShadowsCallback(IrrlichtDevice* device, FlexibleFontManager* fmgr):SDFFCallback(device, fmgr){}
	
	virtual void OnSetConstants(IMaterialRendererServices *services, s32 userData){
		if(firstSetConstants){
			shadowOffsetID = services->getPixelShaderConstantID("uShadowOffset");
			shadowSmoothingID = services->getPixelShaderConstantID("uShadowSmoothing");
			shadowColorID = services->getPixelShaderConstantID("uShadowColor");
		}
		
		SDFFCallback::OnSetConstants(services, userData);
		
		const FlexibleFontManager::Shadow& shadow = fmgr->getShadow();
		services->setPixelShaderConstant(shadowOffsetID, &(shadow.offset.X), 2);
		services->setPixelShaderConstant(shadowSmoothingID, &(shadow.smoothing), 1);
		f32 shadowColorF[4]; convertColor2RGBAFloats(shadowColorF, shadow.color);
		services->setPixelShaderConstant(shadowColorID, shadowColorF, 4);
	}
	
};

FlexibleFontManager::FlexibleFontManager(irr::IrrlichtDevice* device):
	irr::IReferenceCounted(),
	device(device){
	usedCbk = NULL;
	fatness = 0.5f;
	smoothing = 1.f/16.f;
	borderSize = 0.f;
	borderColor = SColor(255,0,0,0);
	shadow = Shadow{vector2df(0.0005,0.0005), 0.1, SColor(255,0,0,0)};
	IVideoDriver* driver = device->getVideoDriver();
	stringc fsFontWithShadows = stringc("#define SHADOWS\n").append(OGLFontFS);
	if(driver->getDriverType()==EDT_OPENGL || driver->getDriverType()==EDT_OGLES2){
		sdffType = (E_MATERIAL_TYPE)driver->getGPUProgrammingServices()->addHighLevelShaderMaterial(OGLFontVS, "main", EVST_VS_2_0, OGLFontFS, "main", EPST_PS_2_0, this, EMT_TRANSPARENT_ALPHA_CHANNEL, 0);
		sdffWithShadowsType = (E_MATERIAL_TYPE)driver->getGPUProgrammingServices()->addHighLevelShaderMaterial(OGLFontVS, "main", EVST_VS_2_0, fsFontWithShadows.c_str(), "main", EPST_PS_2_0, this, EMT_TRANSPARENT_ALPHA_CHANNEL, 0);
	}else{//currently only opengl and opengl es supported
		sdffWithShadowsType = sdffType = EMT_TRANSPARENT_ALPHA_CHANNEL;
	}
	callbacks[sdffType] = new SDFFCallback(device, this);
	callbacks[sdffWithShadowsType] = new SDFFWithShadowsCallback(device, this);
}

irr::IrrlichtDevice* FlexibleFontManager::getDevice() const{
	return device;
}

void FlexibleFontManager::OnSetConstants(IMaterialRendererServices *services, s32 userData){
	usedCbk->OnSetConstants(services, userData);
}

void FlexibleFontManager::OnSetMaterial(const irr::video::SMaterial& material){
	usedCbk = callbacks[material.MaterialType];
	usedCbk->OnSetMaterial(material);
}
	
E_MATERIAL_TYPE FlexibleFontManager::getSDFFMaterialType() const{
	return sdffType;
}

irr::video::E_MATERIAL_TYPE FlexibleFontManager::getSDFFWithShadowsMaterialType() const{
	return sdffWithShadowsType;
}
	
FlexibleFontManager::~FlexibleFontManager(){
	for(auto it = loadedFonts.getIterator(); !it.atEnd(); it++){
		it->getValue()->drop();
	}
	for(auto it = loaders.begin(); it != loaders.end(); it++){
		(*it)->drop();
	}
	for(auto it = callbacks.getIterator(); !it.atEnd(); it++){
		it->getValue()->drop();
	}
}
	
void FlexibleFontManager::addFontLoader(IFontLoader* loader){
	loader->grab();
	loaders.push_back(loader);
}

FlexibleFont* FlexibleFontManager::getFont(const irr::io::path& filename){
	irr::core::map<irr::io::path, FlexibleFont*>::Node* node = loadedFonts.find(filename);
	if(node){
		return node->getValue();
	}
	for(auto it = loaders.begin(); it != loaders.end(); it++){
		if((*it)->isALoadableFileExtension(filename)){
			FlexibleFont* font = (*it)->loadFont(filename);
			if(font){loadedFonts[filename] = font;}
			return font;
		}
	}
	return NULL;
}

void FlexibleFontManager::removeFont(FlexibleFont* font){
	for(auto it = loadedFonts.getIterator(); !it.atEnd(); it++){//O(n), but n usually small and removeFont should not be a method which is called with high frequency
		FlexibleFont* foundFont = it->getValue();
		if(foundFont == font){
			loadedFonts.remove(it.getNode());
			return;
		}
	}
}

void FlexibleFontManager::setFatness(irr::f32 fatness){
	this->fatness = fatness;
}

irr::f32 FlexibleFontManager::getFatness() const{
	return fatness;
}

void FlexibleFontManager::setSmoothing(irr::f32 smoothing){
	this->smoothing = smoothing;
}

irr::f32 FlexibleFontManager::getSmoothing() const{
	return smoothing;
}

void FlexibleFontManager::setBorderColor(irr::video::SColor borderColor){
	this->borderColor = borderColor;
}
	
irr::video::SColor FlexibleFontManager::getBorderColor() const{
	return borderColor;
}

void FlexibleFontManager::setBorderSize(irr::f32 borderSize){
	this->borderSize = borderSize;
}

irr::f32 FlexibleFontManager::getBorderSize() const{
	return borderSize;
}

void FlexibleFontManager::setShadow(const Shadow& shadow){
	this->shadow = shadow;
}
	
const FlexibleFontManager::Shadow& FlexibleFontManager::getShadow() const{
	return shadow;
}

BMFontLoader::BMFontLoader(FlexibleFontManager* fmgr):IFontLoader(fmgr->getDevice()->getFileSystem()),fmgr(fmgr),device(fmgr->getDevice()){}

bool BMFontLoader::isALoadableFileExtension(const irr::io::path& filename){
	return hasFileExtension(filename, "fnt");
}

struct BMFontLine{
	irr::core::stringc type;
	irr::core::map<irr::core::stringc, irr::core::stringc> values;//key -> value
};

static inline bool isWhitespace(char c){
	return c=='\r' || c==' ' || c=='\t';
}

FlexibleFont* BMFontLoader::loadFont(irr::io::IReadFile* file){
	size_t fileSize = file->getSize();
	char* buffer = new char[fileSize+1];
	file->read(buffer, fileSize);
	buffer[fileSize] = '\n';//makes parsing easier
	//Parse the BMFontLines
	irr::core::list<BMFontLine*> lines;
	lines.push_back(new BMFontLine());
	BMFontLine* currentLine = *(lines.getLast());
	int state = 0;//0: beginning, 1: type, 2: whitespace before key, 3: key, 4: value, 5: value in ""
	size_t tokenStart = 0;//where the current token starts
	irr::core::stringc key;
	for(size_t i=0; i<fileSize+1; i++){
		char c = buffer[i];
		if(state==0){//beginning
			if(!isWhitespace(c) && c!='\n'){//tolerate empty lines and whitespace 
				state = 1;
				tokenStart = i;
			}
		}else if(state==1){//type
			if(isWhitespace(c)){
				currentLine->type = irr::core::stringc(&(buffer[tokenStart]), i-tokenStart);
				state = 2;
			}
		}else if(state==2){//whitespace before key
			if(c=='\n'){
				state = 0;
				lines.push_back(new BMFontLine());
				currentLine = *(lines.getLast());
			}else if(!isWhitespace(c)){
				state = 3;
				tokenStart = i;
			}
		}else if(state==3){//key
			if(c=='='){
				key = irr::core::stringc(&(buffer[tokenStart]), i-tokenStart);
				state = 4;
				tokenStart = i+1;
			}
		}else if(state==4){//value
			if(c=='\"'){
				tokenStart = i+1;
				state = 5;
			}else if(c=='\n' || isWhitespace(c)){
				currentLine->values[key] = irr::core::stringc(&(buffer[tokenStart]), i-tokenStart);
				if(c=='\n'){
					state = 0;
					lines.push_back(new BMFontLine());
					currentLine = *(lines.getLast());
				}else{
					state = 2;
				}
			}
		}else if(state==5){//value in ""
			if(c=='\"'){
				currentLine->values[key] = irr::core::stringc(&(buffer[tokenStart]), i-tokenStart);
				state = 2;
			}
		}
	}
	delete[] buffer;
	//Convert BMFontLines to a IFontLoader::Font definition
	FlexibleFont* font = new FlexibleFont(fmgr);
	IFontLoader::Font& fontDef = font->getFontDefintion();
	fontDef.page = NULL;
	for(auto it = lines.begin(); it != lines.end(); it++){
		BMFontLine* line = *it;
		if(line->type=="kerning"){
			const stringc& first = line->values["first"];
			const stringc& second = line->values["second"];
			const stringc& amount = line->values["amount"];
			wchar_t key[3]; key[0] = (wchar_t)(strtoul10(first.c_str())); key[1] = (wchar_t)(strtoul10(second.c_str())); key[2] = L'\0';
			fontDef.kerning[key] = strtol10(amount.c_str());
		}else if(line->type=="char"){
			const stringc& id = line->values["id"];
			const stringc& x = line->values["x"];
			const stringc& y = line->values["y"];
			const stringc& width = line->values["width"];
			const stringc& height = line->values["height"];
			const stringc& xoffset = line->values["xoffset"];
			const stringc& yoffset = line->values["yoffset"];
			const stringc& xadvance = line->values["xadvance"];
			fontDef.characters[strtoul10(id.c_str())] = FontCharacter{strtoul10(x.c_str()), strtoul10(y.c_str()), strtoul10(width.c_str()), strtoul10(height.c_str()), vector2df(0,0), vector2df(0,0), strtol10(xoffset.c_str()), strtol10(yoffset.c_str()), strtoul10(xadvance.c_str())};
		}else if(line->type=="common"){
			const stringc& lineHeight = line->values["lineHeight"];
			const stringc& base = line->values["base"];
			fontDef.lineHeight = strtoul10(lineHeight.c_str());
			fontDef.base = strtoul10(base.c_str());
		}else if(line->type=="info"){
			const stringc& size = line->values["size"];
			fontDef.size = strtoul10(size.c_str());
		}else if(line->type=="page"){
			const stringc& id = line->values["id"];
			assert(strtoul10(id.c_str())==0);
			const stringc& fileAttr = line->values["file"];
			const stringc& fntPath = file->getFileName();
			s32 endPos = fntPath.findLast('/');
			#ifdef _WIN32
			s32 endPos2 = fntPath.findLast('\\');
			endPos = endPos>endPos2?endPos:endPos2;
			#endif
			stringc texPath(fntPath.subString(0, endPos<0?fntPath.size():(endPos+1)).append(fileAttr));
			device->getLogger()->log(stringc("Loading tex for font: ").append(texPath).c_str());
			fontDef.page = device->getVideoDriver()->getTexture(texPath);
		}
		delete line;
	}
	convertFontCoords2TexCoords(fontDef);
	return font;
}

void convertFontCoords2TexCoords(IFontLoader::Font& fontDef, bool offsetForFiltering){
	assert(fontDef.page!=NULL);
	dimension2d<u32> tDim = fontDef.page->getOriginalSize();
	irr::f32 mulX = 1.f/tDim.Width, mulY = 1.f/tDim.Height;
	int offset = offsetForFiltering?1:0;
	for(auto it = fontDef.characters.getIterator(); !it.atEnd(); it++){
		IFontLoader::FontCharacter& fc = it->getValue();
		fc.minTexBB.X = (fc.x+offset)*mulX;//+offset to avoid problems with filtering, not required when using SDFF, but useful in case of normal bitmap fonts
		fc.minTexBB.Y = (fc.y+offset)*mulY;
		fc.maxTexBB.X = (fc.x+fc.width)*mulX;
		fc.maxTexBB.Y = (fc.y+fc.height)*mulY;
	}
}

FlexibleFont::FlexibleFont(FlexibleFontManager* fmgr):irr::gui::IGUIFont(),
	fmgr(fmgr),
	device(fmgr->getDevice()),
	cCalc(16.f/9.f),
	autoResetEnabled(true),
	defaultTabSize(4),
	defaultItalicGradient(.0f),
	defaultFatness(0.5f),
	defaultSmoothingFactor(1.5f),
	defaultBorderSize(0.f),
	defaultBorderColor(255,0,0,0),
	defaultScale(1.f,1.f),
	defaultShadow{vector2df(0,0), 0.2f, SColor(255,0,0,0)},
	defaultMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL){
	defaultMeshBuffer.Material = SMaterial();
	defaultMeshBuffer.setHardwareMappingHint(EHM_NEVER);
}

void FlexibleFont::setDefaultTabSize(irr::s32 tabSize){
	defaultTabSize = tabSize;
}

irr::s32 FlexibleFont::getDefaultTabSize() const{
	return defaultTabSize;
}

void FlexibleFont::setDefaultItalicGradient(irr::f32 italicGradient){
	defaultItalicGradient = italicGradient;
}

irr::f32 FlexibleFont::getDefaultItalicGradient() const{
	return defaultItalicGradient;
}

void FlexibleFont::setDefaultScale(const irr::core::vector2d<irr::f32>& scale){
	defaultScale = scale;
}
	
const irr::core::vector2d<irr::f32>& FlexibleFont::getDefaultScale() const{
	return defaultScale;
}

void FlexibleFont::setAutoResetTransformEnabled(bool enabled){
	autoResetEnabled = enabled;
}

void FlexibleFont::setDefaultMaterialType(irr::video::E_MATERIAL_TYPE matType){
	defaultMaterialType = matType;
}
	
irr::video::E_MATERIAL_TYPE FlexibleFont::getDefaultMaterialType(){
	return defaultMaterialType;
}

void FlexibleFont::setDefaultSmoothingFactor(irr::f32 smoothingFactor){
	defaultSmoothingFactor = smoothingFactor;
}

irr::f32 FlexibleFont::getDefaultSmoothingFactor() const{
	return defaultSmoothingFactor;
}

void FlexibleFont::setDefaultFatness(irr::f32 fatness){
	defaultFatness = fatness;
}

irr::f32 FlexibleFont::getDefaultFatness() const{
	return defaultFatness;
}

void FlexibleFont::setDefaultBorderColor(irr::video::SColor borderColor){
	defaultBorderColor = borderColor;
}

irr::video::SColor FlexibleFont::getDefaultBorderColor() const{
	return defaultBorderColor;
}

void FlexibleFont::setDefaultBorderSize(irr::f32 borderSize){
	defaultBorderSize = borderSize;
}

irr::f32 FlexibleFont::getDefaultBorderSize() const{
	return defaultBorderSize;
}

void FlexibleFont::setDefaultShadow(const FlexibleFontManager::Shadow& shadow){
	defaultShadow = shadow;
}

const FlexibleFontManager::Shadow& FlexibleFont::getDefaultShadow() const{
	return defaultShadow;
}

void FlexibleFont::reset2DTransforms(){
	cCalc.reset2DTransforms(device);
}

IFontLoader::Font& FlexibleFont::getFontDefintion(){
	return fontDefinition;
}

class ICharacterCallback{
	
	public:
	
	virtual void OnCharacter(IFontLoader::FontCharacter& fc, s32 idxInString, s32 finalX, s32 finalY) = 0;
	
	virtual void OnLineFinished(s32 curX){}
	
	virtual ~ICharacterCallback(){}

};

static void iterateOverString(const wchar_t* text, size_t textLen, irr::s32 tabSize, const IFontLoader::Font& fontDefinition, ICharacterCallback& cbk){
	irr::core::stringw kerningKey(L"\1\1");
	irr::core::map<irr::u32, IFontLoader::FontCharacter>::Node* fallbackNode = fontDefinition.characters.find((u32)L' ');
	assert(fallbackNode!=NULL);
	u32 spaceAdvance = fallbackNode->getValue().xadvance;
	s32 curX = 0, curY = 0;
	for(size_t i=0; i<textLen; i++){
		wchar_t c = text[i];
		kerningKey[0] = kerningKey[1];
		kerningKey[1] = c;
		if(c==L'\n'){
			curY += fontDefinition.base;
			cbk.OnLineFinished(curX);
			curX = 0;
		}else if(c==L'\r'){
			curX = 0;
		}else if(c==L'\t'){
			u32 tabsInLine = (curX/spaceAdvance)/tabSize;
			curX = (tabsInLine+1)*tabSize*spaceAdvance;
		}else{
			irr::core::map<irr::u32, IFontLoader::FontCharacter>::Node* node = fontDefinition.characters.find((u32)c);
			if(node==NULL){node = fallbackNode;}
			IFontLoader::FontCharacter& fc = node->getValue();
			irr::core::map<irr::core::stringw, irr::s32>::Node* kerningNode = fontDefinition.kerning.find(kerningKey);
			if(kerningNode){
				curX += kerningNode->getValue();
			}
			s32 finalX = curX + fc.xoffset;
			s32 finalY = curY + fc.yoffset;
			cbk.OnCharacter(fc, i, finalX, finalY);
			curX += fc.xadvance;
		}
	}
	cbk.OnLineFinished(curX);
}

class FillMeshBufferCharacterCallback : public ICharacterCallback{
	
	public:
	
	irr::scene::SMeshBuffer& mb;
	irr::core::matrix4* transformation;
	irr::video::SColor color;
	irr::f32 italicGradient;
	
	irr::s32 wholeWidth;//not centered if 0
	irr::u32 lineStartVertex;
	
	FillMeshBufferCharacterCallback(irr::scene::SMeshBuffer& mb, irr::core::matrix4* transformation, irr::video::SColor color, irr::f32 italicGradient, irr::s32 wholeWidth):mb(mb),transformation(transformation),color(color),italicGradient(italicGradient),wholeWidth(wholeWidth),lineStartVertex(mb.Vertices.size()){}
	
	void OnCharacter(IFontLoader::FontCharacter& fc, s32 idxInString, s32 finalX, s32 finalY){
		u32 upperLeft = mb.Vertices.size();
		u32 lowerLeft = upperLeft+1, upperRight = upperLeft+2, lowerRight = upperLeft+3;
		f32 italicOffset = italicGradient*fc.height;
		mb.Vertices.push_back(S3DVertex(finalX+italicOffset, finalY, 1.f, 0.f, 0.f, -1.f, color, fc.minTexBB.X, fc.minTexBB.Y));//upper left
		mb.Vertices.push_back(S3DVertex(finalX, finalY+fc.height, 1.f, 0.f, 0.f, -1.f, color, fc.minTexBB.X, fc.maxTexBB.Y));//lower left
		mb.Vertices.push_back(S3DVertex(finalX+italicOffset+fc.width, finalY, 1.f, 0.f, 0.f, -1.f, color, fc.maxTexBB.X, fc.minTexBB.Y));//upper right
		mb.Vertices.push_back(S3DVertex(finalX+fc.width, finalY+fc.height, 1.f, 0.f, 0.f, -1.f, color, fc.maxTexBB.X, fc.maxTexBB.Y));//lower right
		if(transformation){
			transformation->transformVect(mb.Vertices[upperLeft].Pos);
			transformation->transformVect(mb.Vertices[lowerLeft].Pos);
			transformation->transformVect(mb.Vertices[upperRight].Pos);
			transformation->transformVect(mb.Vertices[lowerRight].Pos);
		}
		mb.Indices.push_back(upperLeft);//first triangle
		mb.Indices.push_back(lowerLeft);
		mb.Indices.push_back(upperRight);
		mb.Indices.push_back(upperRight);//second triangle
		mb.Indices.push_back(lowerLeft);
		mb.Indices.push_back(lowerRight);
	}
	
	void OnLineFinished(s32 curX){
		s32 delta = (wholeWidth-curX)/2;
		u32 size = mb.Vertices.size();
		if(delta>0){//center line if line size smaller than whole width
			for(u32 i=lineStartVertex; i<size; i++){
				mb.Vertices[i].Pos.X += delta;
			}
		}
		lineStartVertex = size;
	}

};

void FlexibleFont::fillMeshBuffer(irr::scene::SMeshBuffer& mb, const wchar_t* text, irr::s32 tabSize, bool newLinesCentered, irr::video::SColor color, irr::f32 italicGradient, irr::core::matrix4* transformation, irr::video::E_MATERIAL_TYPE materialType, bool filteringEnabled, bool recalcBB) const{
	size_t textLen = wcslen(text);
	if(mb.Vertices.size()+textLen*4>65536){
		text = L"Text too long for meshbuffer.";
		textLen = wcslen(text);
		if(mb.Vertices.size()+textLen*4>65536){
			std::cerr << "Meshbuffer full: Cannot even fill error message." << std::endl;
			return;
		}
	}
	mb.Vertices.reallocate(mb.Vertices.size()+textLen*4);
	mb.Indices.reallocate(mb.Indices.size()+textLen*6);
	FillMeshBufferCharacterCallback cbk(mb, transformation, color, italicGradient, newLinesCentered?getDimensionWithTabs(text, tabSize).Width:0);
	iterateOverString(text, textLen, tabSize, fontDefinition, cbk);
	SMaterial& mat = mb.getMaterial();
	mat.setFlag(EMF_LIGHTING, false);
	mat.setFlag(EMF_ZBUFFER, false);
	mat.setFlag(EMF_ZWRITE_ENABLE, false);
	mat.setFlag(EMF_BILINEAR_FILTER, filteringEnabled);
	mat.setFlag(EMF_TRILINEAR_FILTER, filteringEnabled);
	mat.setFlag(EMF_ANISOTROPIC_FILTER, filteringEnabled);
	mat.setFlag(EMF_ANTI_ALIASING, true);
	mat.setFlag(EMF_ANISOTROPIC_FILTER, true);
	mat.setFlag(EMF_BACK_FACE_CULLING, false);
	mat.MaterialType = materialType;
	mat.setTexture(0, fontDefinition.page);
	for(u32 j=0; j<MATERIAL_MAX_TEXTURES; j++){
		mat.TextureLayer[j].TextureWrapU = ETC_CLAMP;
		mat.TextureLayer[j].TextureWrapV = ETC_CLAMP;
	}
	if(recalcBB){mb.recalculateBoundingBox();}
	mb.setDirty();
}

void FlexibleFont::drawFontMeshBuffer(const irr::scene::IMeshBuffer& mb, const irr::core::rect<irr::s32>& position, irr::f32 smoothingFactor, irr::f32 fatness, irr::f32 borderSize, irr::video::SColor borderColor, FlexibleFontManager::Shadow* shadow, const irr::core::vector2d<irr::f32>& translation, const irr::core::vector2d<irr::f32>& scale, irr::f32 angle, const irr::core::vector2d<irr::f32>& origin){
	dimension2d<u32> vdim(position.getWidth(), position.getHeight());
	if(vdim.Width>0 && vdim.Height>0){
		IVideoDriver* driver = device->getVideoDriver();
		irr::core::rect<irr::s32> oldViewPort = driver->getViewPort();
		driver->setViewPort(position);
		cCalc.setAspectRatio((f32)vdim.Width/(f32)vdim.Height);
		irr::core::matrix4 finalTransformation = createOptimized2DTransform(translation, scale, angle*(3.141592654/180.0), origin, cCalc.getCameraScreenParameters(), vdim, cCalc.getRecommendedInverseProjectionDistance());
		cCalc.set2DTransforms(device, &finalTransformation, true);
		fmgr->setSmoothing(smoothingFactor/(16.f*sqrtf(0.5f*(scale.X*scale.X+scale.Y*scale.Y))));
		fmgr->setFatness(fatness);
		fmgr->setBorderSize(borderSize);
		fmgr->setBorderColor(borderColor);
		if(shadow){
			fmgr->setShadow(*shadow);
		}else{
			fmgr->setShadow(FlexibleFontManager::Shadow{vector2df(0,0), 0.f, SColor(0,0,0,0)});
		}
		const SMaterial& mat = mb.getMaterial();
		driver->setMaterial(mat);//mb.getMaterial());//SMaterial());//
		SET_MATERIAL_WORKAROUND(driver, mat)
		driver->drawMeshBuffer(&mb);
		driver->setViewPort(oldViewPort);
		if(autoResetEnabled){
			reset2DTransforms();
		}
		UNSET_MATERIAL_WORKAROUND
	}
}

irr::f32 FlexibleFont::calculateOptimalScale(const wchar_t* text, irr::core::dimension2d<u32> space, s32 tabSize){
	if(tabSize<0){tabSize = getDefaultTabSize();}
	dimension2d<u32> actualSpace = getDimensionWithTabs(text, tabSize, vector2d<f32>(1.f,1.f));
	actualSpace.Width += 1;//Compensate for potential rounding errors.
	actualSpace.Height += 1;
	f32 scaleWidth = ((f32)space.Width)/((f32)actualSpace.Width);
	f32 scaleHeight = ((f32)space.Height)/((f32)actualSpace.Height);
	return scaleWidth<scaleHeight?scaleWidth:scaleHeight;
}

irr::core::dimension2d<irr::u32> get2DDimensionFromMeshBuffer(const IMeshBuffer& mb){
	const aabbox3df& bb = mb.getBoundingBox();
	return dimension2d<irr::u32>(bb.MaxEdge.X-bb.MinEdge.X, bb.MaxEdge.Y-bb.MinEdge.Y); 
}

class DimensionFinderCallback : public ICharacterCallback{
	
	public:
	
	s32 maxX, maxY;
	const IFontLoader::Font& fontDefinition;
	
	DimensionFinderCallback(const IFontLoader::Font& fontDefinition):maxX(0),maxY(0),fontDefinition(fontDefinition){}
	
	void OnCharacter(IFontLoader::FontCharacter& fc, s32 idxInString, s32 finalX, s32 finalY){
		s32 newMaxX = finalX-fc.xoffset+fc.xadvance;
		s32 newMaxY = finalY-fc.yoffset;
		if(newMaxX>maxX){maxX = newMaxX;}
		if(newMaxY>maxY){maxY = newMaxY;}
	}
	
	void OnLineFinished(s32 curX){
		maxY += fontDefinition.base;
	}

};

irr::core::dimension2d<irr::u32> FlexibleFont::getDimensionWithTabs(const wchar_t* text, irr::s32 tabSize, const irr::core::vector2d<irr::f32>& scale) const{
	size_t textLen = wcslen(text);
	DimensionFinderCallback cbk(fontDefinition);
	iterateOverString(text, textLen, tabSize, fontDefinition, cbk);
	cbk.maxY += fontDefinition.lineHeight-fontDefinition.base;//some distance at the end (similar to the top distance), otherwise centering doesn't work well
	return dimension2d<irr::u32>((u32)(scale.X*cbk.maxX+.5f), (u32)(scale.Y*cbk.maxY+.5f));
}

irr::core::dimension2d<irr::u32> FlexibleFont::getDimension(const wchar_t* text) const{
	return getDimensionWithTabs(text, defaultTabSize, defaultScale);
}

static inline void clipPos(irr::core::rect<irr::s32>& position, const irr::core::rect<irr::s32>& clip){
	position.clipAgainst(clip);
	if(position.UpperLeftCorner.X>=clip.UpperLeftCorner.X && position.UpperLeftCorner.X<=clip.LowerRightCorner.X){position.UpperLeftCorner.X = clip.UpperLeftCorner.X;}
	if(position.UpperLeftCorner.Y>=clip.UpperLeftCorner.Y && position.UpperLeftCorner.Y<=clip.LowerRightCorner.Y){position.UpperLeftCorner.Y = clip.UpperLeftCorner.Y;}
	if(position.LowerRightCorner.X>=clip.UpperLeftCorner.X && position.LowerRightCorner.X<=clip.LowerRightCorner.X){position.LowerRightCorner.X = clip.LowerRightCorner.X;}
	if(position.LowerRightCorner.Y>=clip.UpperLeftCorner.Y && position.LowerRightCorner.Y<=clip.LowerRightCorner.Y){position.LowerRightCorner.Y = clip.LowerRightCorner.Y;}
}

void FlexibleFont::draw(const irr::core::stringw& text, const irr::core::rect<irr::s32>& position, irr::video::SColor color, bool hcenter, bool vcenter, const irr::core::rect<irr::s32>* clip){
	//Fill MeshBuffer
	defaultMeshBuffer.Vertices.set_used(0);
	defaultMeshBuffer.Indices.set_used(0);
	fillMeshBuffer(defaultMeshBuffer, text.c_str(), defaultTabSize, hcenter, color, defaultItalicGradient, NULL, defaultMaterialType);
	defaultMeshBuffer.setDirty();
	//TODO if text too long for meshbuffer: split at \n
	drawFontMeshBuffer(defaultMeshBuffer, getDimensionWithTabs(text.c_str(), defaultTabSize), position, hcenter, vcenter, clip);
}

void FlexibleFont::drawFontMeshBuffer(const irr::scene::IMeshBuffer& mb, irr::core::dimension2d<irr::u32> textSize, const irr::core::rect<irr::s32>& position, bool hcenter, bool vcenter, const irr::core::rect<irr::s32>* clip){
	rect<s32> clippedPos(position);
	rect<s32> clipRect(device->getVideoDriver()->getViewPort());
	if(clip){
		rect<s32> clipCp(*clip);
		clipCp.clipAgainst(clipRect);
		clipRect = clipCp;//clipPos(clippedPos, *clip);
	}
	clipPos(clippedPos, clipRect);//clipping against viewport required for render targets / fbos
	//Set Transformation and Render
	vector2d<f32> translation(position.UpperLeftCorner.X-clippedPos.UpperLeftCorner.X, position.UpperLeftCorner.Y-clippedPos.UpperLeftCorner.Y);
	vector2d<f32> origin(0.f, 0.f);
	if(hcenter || vcenter){
		origin = vector2d<f32>(hcenter?(textSize.Width/2.f):0.f, vcenter?(textSize.Height/2.f):0.f);
		translation += vector2d<f32>(hcenter?(position.getWidth()/2.f):0.f, vcenter?(position.getHeight()/2.f):0.f);
	}
	drawFontMeshBuffer(mb, clippedPos, defaultSmoothingFactor, defaultFatness, defaultBorderSize, defaultBorderColor, &defaultShadow, translation, defaultScale, 0.f, origin);
}

class CharPosFinderCallback : public ICharacterCallback{
	
	public:
	
	s32 pos;
	s32 resultIdx;
	
	CharPosFinderCallback(s32 pos):pos(pos),resultIdx(-1){}
	
	void OnCharacter(IFontLoader::FontCharacter& fc, s32 idxInString, s32 finalX, s32 finalY){
		if(finalX>pos && resultIdx==-1){
			resultIdx = idxInString-1;
		}
	}

};

irr::s32 FlexibleFont::getCharacterFromPos(const wchar_t* text, irr::s32 pixel_x) const{
	size_t textLen = wcslen(text);
	s32 inverseScaledX = round_(pixel_x/defaultScale.X);
	CharPosFinderCallback cbk(inverseScaledX);
	iterateOverString(text, textLen, defaultTabSize, fontDefinition, cbk);
	return cbk.resultIdx;
}

irr::gui::EGUI_FONT_TYPE FlexibleFont::getType() const{
	return irr::gui::EGFT_CUSTOM;
}

void FlexibleFont::setKerningWidth(irr::s32 kerning){
	//not applicable: using kerning from the font
}

void FlexibleFont::setKerningHeight(irr::s32 kerning){
	//not applicable: using kerning from the font
}

irr::s32 FlexibleFont::getKerningWidth(const wchar_t* thisLetter, const wchar_t* previousLetter) const{
	s32 kerningWidth = 0;
	if(thisLetter!=NULL && previousLetter!=NULL){
		stringw kerningKey(L"  ");
		kerningKey[0] = *previousLetter;
		kerningKey[1] = *thisLetter;
		irr::core::map<irr::core::stringw, irr::s32>::Node* kerningNode = fontDefinition.kerning.find(kerningKey);
		if(kerningNode){kerningWidth += kerningNode->getValue();}
	}
	return round_(defaultScale.X*kerningWidth);
}

irr::s32 FlexibleFont::getKerningHeight() const{
	return 0;
}

void FlexibleFont::setInvisibleCharacters(const wchar_t* s){
	//not applicable: invisible characters are defined by the font
}

void FlexibleFont::drawWithOptimalScale(const irr::core::stringw& text, irr::video::SColor color, const irr::core::rect<irr::s32>& labelRect, const irr::core::rect<irr::s32>* clip, bool useCurrentScaleIfLarger){
    vector2d<f32> curScale = getDefaultScale();
	f32 optimalScale = calculateOptimalScale(text.c_str(), dimension2d<u32>(labelRect.getWidth(), labelRect.getHeight()));
	if(useCurrentScaleIfLarger && curScale.X>optimalScale){optimalScale = curScale.X;}
    setDefaultScale(vector2d<f32>(optimalScale, optimalScale));
    draw(text, labelRect, color, true, true, clip);
    setDefaultScale(curScale);
}
