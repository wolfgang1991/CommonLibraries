#ifndef FlexibleFont_H_INCLUDED
#define FlexibleFont_H_INCLUDED

#include <IGUIFont.h>
#include <irrMap.h>
#include <irrArray.h>
#include <irrList.h>
#include <path.h>
#include <IFileSystem.h>
#include <CMeshBuffer.h>
#include <IShaderConstantSetCallBack.h>

#include "ForwardDeclarations.h"
#include "Transformation2DHelpers.h"

class FlexibleFont;
class FlexibleFontManager;

//TODO: irr::core::map has a small memleak (but it doesn't matter since the amount of fonts used is usually low)

//! Interface for Font Loaders
class IFontLoader : public irr::IReferenceCounted{

	protected:
	
	irr::io::IFileSystem* fsys;

	public:
	
	IFontLoader(irr::io::IFileSystem* fsys):irr::IReferenceCounted(),fsys(fsys){}
	
	//! characters inside textures, all in pixels
	struct FontCharacter{
		irr::u32 x, y, width, height; //! Location in texture
		irr::core::vector2d<irr::f32> minTexBB, maxTexBB; //! Texture Coordinates Bounding Box based on x,y,width,height
		irr::s32 xoffset, yoffset; //! Offset of the character when copying to the screen
		irr::u32 xadvance; //! How much the x position should be altered after drawing a character
	};
	
	//! The whole font, all in pixels
	struct Font{
		irr::u32 lineHeight; //! Total Height of a line
		irr::u32 base; //! Height from the base of the characters to the top of the line
		irr::u32 size; //! Size of the original font used to create the bitmap font
		irr::core::map<irr::u32, FontCharacter> characters; //! all characters in the font indexed by their Unicode IDs
		irr::core::map<irr::core::stringw, irr::s32> kerning; //! String of two characters -> kerning offset in x
		irr::video::ITexture* page;//texture with glyphs
		irr::core::stringw name;
	};

	virtual bool isALoadableFileExtension(const irr::io::path& filename) = 0;
	
	virtual FlexibleFont* loadFont(irr::io::IReadFile* file) = 0;
	
	FlexibleFont* loadFont(const irr::io::path& filename){
		irr::io::IReadFile* file = fsys->createAndOpenFile(filename);
		FlexibleFont* result = loadFont(file);
		file->drop();
		return result;
	}

};

//! converts x,y,width,height -> minTexBB, maxTexBB
void convertFontCoords2TexCoords(IFontLoader::Font& fontDef, bool offsetForFiltering = false);

//! Loader for AngelCode BMFonts (format documentation see: http://www.angelcode.com/products/bmfont/doc/file_format.html)
//! Only supports fonts with one page (all glyphs on one texture)
class BMFontLoader : public IFontLoader{
	
	private:
	
	FlexibleFontManager* fmgr;
	irr::IrrlichtDevice* device;
	
	public:
	
	BMFontLoader(FlexibleFontManager* fmgr);
	
	bool isALoadableFileExtension(const irr::io::path& filename);
	
	FlexibleFont* loadFont(irr::io::IReadFile* file);
	
};


//! Calculates the dimensions using the geometry instead of the characters. The bounding box must be correct.
//! this may differ from getDimension/getDimensionWithTabs if transformations are used while creating the meshbuffer or if the per-character offsets are != 0 in the used fonts
irr::core::dimension2d<irr::u32> get2DDimensionFromMeshBuffer(const irr::scene::IMeshBuffer& mb);

class FlexibleFontManager : public irr::video::IShaderConstantSetCallBack{

	public:
	
	struct Shadow{
		irr::core::vector2d<irr::f32> offset;//! Direction of the shadow (should depend on the glyph's size on the texture)
		irr::f32 smoothing;//! Should be much more smooth than the font itself (usually sth like 0.1)
		irr::video::SColor color;
	};

	private:
	
	irr::IrrlichtDevice* device;
	irr::core::map<irr::io::path, FlexibleFont*> loadedFonts;
	irr::core::list<IFontLoader*> loaders;

	irr::video::E_MATERIAL_TYPE sdffType;
	irr::video::E_MATERIAL_TYPE sdffWithShadowsType;
	
	irr::core::map<irr::video::E_MATERIAL_TYPE, irr::video::IShaderConstantSetCallBack*> callbacks;
	irr::video::IShaderConstantSetCallBack* usedCbk;
	
	irr::f32 smoothing, fatness, borderSize;
	irr::video::SColor borderColor;
	
	Shadow shadow;
	
	public:
	
	FlexibleFontManager(irr::IrrlichtDevice* device);
	
	~FlexibleFontManager();
	
	irr::IrrlichtDevice* getDevice() const;
	
	void addFontLoader(IFontLoader* loader);
	
	//! loads the font if it hasn't been loaded before
	FlexibleFont* getFont(const irr::io::path& filename);
	
	void removeFont(FlexibleFont* font);
	
	void OnSetConstants(irr::video::IMaterialRendererServices *services, irr::s32 userData);
	
	void OnSetMaterial(const irr::video::SMaterial& material);
	
	irr::video::E_MATERIAL_TYPE getSDFFMaterialType() const;
	
	irr::video::E_MATERIAL_TYPE getSDFFWithShadowsMaterialType() const;
	
	//--- The following parameters are automatically set by the FlexibleFont ---
	
	//! fatness in [0-1], normal fatness: 0.5, lower value => more fat; only supported for SDFF (or similar in future)
	void setFatness(irr::f32 fatness = 0.5f);
	
	irr::f32 getFatness() const;
	
	//! within a distance of +-smooting from the border (defined by fatness) the alpha value is interpolated, this results in a smooth border (larger values => more smooth)
	//! The recommended smoothing is (1/16)/scale .
	//! It is used by FlexibleFont to automatically make good smoothing.
	void setSmoothing(irr::f32 smoothing = 1.f/16.f);
	
	irr::f32 getSmoothing() const;
	
	void setBorderColor(irr::video::SColor borderColor = irr::video::SColor(255,0,0,0));
	
	irr::video::SColor getBorderColor() const;
	
	void setBorderSize(irr::f32 borderSize = 0.f);
	
	irr::f32 getBorderSize() const;
	
	void setShadow(const Shadow& shadow);
	
	const Shadow& getShadow() const;

};

//! Assumes that each wchar_t contains a unicode codepoint. If this is not the case you need to convert the strings.
//! On windows you therefore only have the first 65536 unicode codepoints.
class FlexibleFont : public irr::gui::IGUIFont{

	private:
	
	FlexibleFontManager* fmgr;
	irr::IrrlichtDevice* device;
	IFontLoader::Font fontDefinition;
	Camera2DParameterCalculator cCalc;
	bool autoResetEnabled;
	
	irr::s32 defaultTabSize;
	irr::f32 defaultItalicGradient;
	irr::f32 defaultFatness;
	irr::f32 defaultSmoothingFactor;
	irr::f32 defaultBorderSize;
	irr::video::SColor defaultBorderColor;
	irr::core::vector2d<irr::f32> defaultScale;
	FlexibleFontManager::Shadow defaultShadow;
	irr::video::E_MATERIAL_TYPE defaultMaterialType;
	irr::scene::SMeshBuffer defaultMeshBuffer;
	

	public:
	
	FlexibleFont(FlexibleFontManager* fmgr);
	
	FlexibleFontManager* getFontManager(){return fmgr;}
	
	//--- Methods for creating and drawing meshbuffers for text ---
	
	//! set to true if transformations of active camera shall be restored automatically after drawing
	void setAutoResetTransformEnabled(bool enabled);
	
	void reset2DTransforms();
	
	IFontLoader::Font& getFontDefintion();
	
	//! Fills a Meshbuffer (16 bit indices) with vertices and indices for rendering text; newLinesCentered: if true shorter lines get centered horizontally
	//! The MeshBuffer may already contain other geometry or characters. It must therefore be cleared manually before if previous geometry shall be discarded.
	//! There can not be more than 2^16/4 characters (4 vertices per character).
	//! transformation: if != NULL it represents a transformation for the vertices inside the MeshBuffer.
	void fillMeshBuffer(irr::scene::SMeshBuffer& mb, const wchar_t* text, irr::s32 tabSize = 4, bool newLinesCentered = false, irr::video::SColor color = irr::video::SColor(255,255,255,255), irr::f32 italicGradient = 0.f, irr::core::matrix4* transformation = NULL, irr::video::E_MATERIAL_TYPE materialType = irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL, bool filteringEnabled = true, bool recalcBB = true) const;
	
	irr::core::dimension2d<irr::u32> getDimensionWithTabs(const wchar_t* text, irr::s32 tabSize = 4, const irr::core::vector2d<irr::f32>& scale = irr::core::vector2d<irr::f32>(1.f,1.f)) const;
	
	//! position defines a clipping rect and the origin (upper left corner) for the font on the screen
	//! angle in degrees
	//! smoothingFactor, fatness and borderSize only has an affect on SDFF
	//! shadow has only an affect on SDFF with Shadows (NULL means no shadow)
	void drawFontMeshBuffer(const irr::scene::IMeshBuffer& mb, const irr::core::rect<irr::s32>& position, irr::f32 smoothingFactor = 1.5f, irr::f32 fatness = 0.5f, irr::f32 borderSize = 0.f, irr::video::SColor borderColor = irr::video::SColor(255,0,0,0), FlexibleFontManager::Shadow* shadow = NULL, const irr::core::vector2d<irr::f32>& translation = irr::core::vector2d<irr::f32>(0.f,0.f), const irr::core::vector2d<irr::f32>& scale = irr::core::vector2d<irr::f32>(1.f,1.f), irr::f32 angle = 0.f, const irr::core::vector2d<irr::f32>& origin = irr::core::vector2d<irr::f32>(0.f,0.f));
	
	//! like the draw function for regular font rendering but instead accepting a meshbuffer and a text size, text size without scale correction (see: getDimensionWithTabs)
	void drawFontMeshBuffer(const irr::scene::IMeshBuffer& mb, irr::core::dimension2d<irr::u32> textSize, const irr::core::rect<irr::s32>& position, bool hcenter=false, bool vcenter=false, const irr::core::rect<irr::s32>* clip=0);
	
	//! Calculates the optimal scale that the text fits in the given space.
	//! If tabSize==-1 the default tabSize is used.
	irr::f32 calculateOptimalScale(const wchar_t* text, irr::core::dimension2d<irr::u32> space, irr::s32 tabSize = -1);
		
	//--- Default Parameters for MeshBuffer creation when using the IGUIFont methods ---
	
	void setDefaultTabSize(irr::s32 tabSize = 4);
	
	irr::s32 getDefaultTabSize() const;
	
	void setDefaultItalicGradient(irr::f32 italicGradient = 0.f);
	
	irr::f32 getDefaultItalicGradient() const;
	
	void setDefaultScale(const irr::core::vector2d<irr::f32>& scale = irr::core::vector2d<irr::f32>(1.f,1.f));
	
	const irr::core::vector2d<irr::f32>& getDefaultScale() const;
	
	//! greater values => smoother edges
	void setDefaultSmoothingFactor(irr::f32 smoothingFactor = 1.5f);
	
	irr::f32 getDefaultSmoothingFactor() const;
	
	void setDefaultBorderColor(irr::video::SColor borderColor = irr::video::SColor(255,0,0,0));
	
	irr::video::SColor getDefaultBorderColor() const;
	
	//! hint: 0.05f is already a large border
	void setDefaultBorderSize(irr::f32 borderSize = 0.f);
	
	irr::f32 getDefaultBorderSize() const;
	
	//! lower values => more fat
	void setDefaultFatness(irr::f32 fatness = 0.5f);
	
	irr::f32 getDefaultFatness() const;
	
	void setDefaultShadow(const FlexibleFontManager::Shadow& shadow = FlexibleFontManager::Shadow{irr::core::vector2d<irr::f32>(0,0), 0.2f, irr::video::SColor(255,0,0,0)});
	
	const FlexibleFontManager::Shadow& getDefaultShadow() const;
	
	void setDefaultMaterialType(irr::video::E_MATERIAL_TYPE matType = irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL);
	
	irr::video::E_MATERIAL_TYPE getDefaultMaterialType();
	
	//! draws the text with the best fitting scale without permanently changing ht default scale
	void drawWithOptimalScale(const irr::core::stringw& text, irr::video::SColor color, const irr::core::rect<irr::s32>& labelRect, const irr::core::rect<irr::s32>* clip = NULL, bool useCurrentScaleIfLarger = true);
	
	//--- Methods inherited from IGUIFont ---
	
	void draw(const irr::core::stringw& text, const irr::core::rect<irr::s32>& position, irr::video::SColor color, bool hcenter=false, bool vcenter=false, const irr::core::rect<irr::s32>* clip=0);

	irr::core::dimension2d<irr::u32> getDimension(const wchar_t* text) const;

	irr::s32 getCharacterFromPos(const wchar_t* text, irr::s32 pixel_x) const;

	irr::gui::EGUI_FONT_TYPE getType() const;

	void setKerningWidth(irr::s32 kerning);

	void setKerningHeight(irr::s32 kerning);

	irr::s32 getKerningWidth(const wchar_t* thisLetter=0, const wchar_t* previousLetter=0) const;

	irr::s32 getKerningHeight() const;

	void setInvisibleCharacters(const wchar_t* s);

};

#endif
