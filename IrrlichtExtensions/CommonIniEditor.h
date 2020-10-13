#ifndef CommonIniEditor_H_INCLUDED
#define CommonIniEditor_H_INCLUDED

#include <ColorSelector.h>
#include <ICommonAppContext.h>
#include <ForwardDeclarations.h>

#include <IniFile.h>

#include <string>
#include <list>

class AggregateGUIElement;

class IIniEditorCustomization{

	public:
	
	virtual int32_t getCancelButtonId() = 0;

	virtual int32_t getOkButtonId() = 0;
	
	virtual void OnHelpButtonPressed(const char* helpFile) = 0;
	
	virtual irr::video::ITexture* getAlphaBackground() = 0;
	
	virtual irr::video::ITexture* getWhiteBackground() = 0;
	
	virtual int32_t getAggregationID() = 0;
	
	virtual int32_t getInvisibleAggregationID() = 0;
	
	virtual int32_t getAggregatableID() = 0;
	
	virtual int32_t getScrollBarID() = 0;
	
};

//TODO: refactor to inherit from IGUIElement and replace processEvent by OnEvent
//! Dialog for editing Ini Files Note: rgba default value: r,g,b,a oder bei rgb: r,g,b; ONE_OF default: alternative1;2;3;...
class CommonIniEditor{

	public:
	enum ValueType {INT, DOUBLE, STRING, BOOLEAN, NOT_EDITABLE, ONE_OF, COLOR_RGB, COLOR_RGBA, TYPE_COUNT};//ONE_OF, 

	private:
	ICommonAppContext* c;
	IIniEditorCustomization* customization;
	irr::gui::IGUIEnvironment* env;
	irr::video::IVideoDriver* driver;
	std::string section;
	int fieldCount;
	std::string* key;
	std::wstring* fieldName;
	std::string* defaultValue;
	ValueType* valType;
	irr::gui::IGUIElement** input;
	int* height;
	int totalHeight;
	std::wstring* lastContent;
	int pw, ph;

	bool lastSucc;
	irr::core::rect<irr::s32> wrect;
	irr::gui::IGUIWindow* win;
	irr::core::rect<irr::s32> prect;
	AggregateGUIElement* agg;
	irr::gui::IGUIButton* cancel;
	irr::gui::IGUIButton* help;
	irr::gui::IGUIButton* ok;

	ColorSelector sel;
	int colorState;//-1 if no color edited, valindex if edited

	bool showHelpButton;
	std::string helpFile;

	IniFile* ini;

	void createGUI();

	void removeGUI();
	
	//common init method for constructors
	void init(const char* HelpFile);

	public:

	CommonIniEditor(ICommonAppContext* context, std::string Section, int FieldCount, std::string* Keys, std::string* FieldNames, std::string* DefaultValues, ValueType* ValTypes, IIniEditorCustomization* customization, const char* HelpFile = NULL);
	
	//! Syntax (see UnicodeCfgParser): <key>,<fieldname>,<defaultvalue>,<valuetype: see ValueType enum>;
	CommonIniEditor(ICommonAppContext* context, std::string Section, const std::wstring& guiCode, IIniEditorCustomization* customization, const char* HelpFile = NULL);
	
	virtual ~CommonIniEditor();

	//! sets a section for editing
	virtual void setSection(std::string Section){section = Section;}

	//! check if the content contains a valid literal for the given type
	virtual bool isCorrectType(std::wstring content, ValueType type);

	//! make the editor visible
	virtual void edit(IniFile* Ini);

	//! should be called on each event
	virtual void processEvent(irr::SEvent event);

	virtual bool isVisible();

	//! true if pressed ok
	virtual bool lastSuccess(){return lastSucc;}

	//! render "non gui" stuff
	virtual void render();
	
	//! same effect as pressing the cancel button
	virtual void cancelEdit();

};

#endif
