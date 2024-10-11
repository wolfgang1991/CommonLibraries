#ifndef CommonIniEditor_H_INCLUDED
#define CommonIniEditor_H_INCLUDED

#include "ColorSelector.h"
#include "ICommonAppContext.h"
#include "ForwardDeclarations.h"

#include <IniFile.h>

#include <IGUIElement.h>

#include <string>
#include <list>
#include <functional>

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

//! useful e.g. for unit conversions before loading/saving
class IIniEditorPostProcessor{
	
	public:
	
	virtual ~IIniEditorPostProcessor(){}
	
	//! called when loading the ini editor definition,for each field, key/fieldName/defaultValue may be modified
	virtual void OnLoadField(std::string& key, std::wstring& fieldName, std::string& defaultValue) const = 0;
	
	//! called when an actual value is loaded from the ini, value may be modified
	virtual void OnLoadValue(const std::string& key, std::string& value) const = 0;
	
	//! called when an actual value is saved to the ini, value may be modified
	virtual void OnSaveValue(const std::string& key, std::string& value) const = 0;
	
};

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

	int state;//0: normal, 1: android-like scrollen
	int py;//altes mousey
	irr::s32 oldScrollPos;

	bool showHelpButton;
	std::string helpFile;

	IIniEditorPostProcessor* pp;
	
	IniFile* ini;

	void createGUI();

	void removeGUI();
	
	//common init method for constructors
	void init(const char* HelpFile);

	public:

	CommonIniEditor(ICommonAppContext* context, std::string Section, int FieldCount, std::string* Keys, std::string* FieldNames, std::string* DefaultValues, ValueType* ValTypes, IIniEditorCustomization* customization, const char* HelpFile = NULL, IIniEditorPostProcessor* pp = NULL);
	
	//! Syntax (see UnicodeCfgParser): <key>,<fieldname>,<defaultvalue>,<valuetype: see ValueType enum>;
	CommonIniEditor(ICommonAppContext* context, std::string Section, const std::wstring& guiCode, IIniEditorCustomization* customization, const char* HelpFile = NULL, IIniEditorPostProcessor* pp = NULL);
	
	virtual ~CommonIniEditor();
	
	//! pp gets deleted on destruction
	virtual void setIniPostProcessor(IIniEditorPostProcessor* pp);

	//! sets a section for editing
	virtual void setSection(std::string Section){section = Section;}

	//! check if the content contains a valid literal for the given type
	virtual bool isCorrectType(std::wstring content, ValueType type);

	//! make the editor visible
	virtual void edit(IniFile* Ini);

	//! should be called on each event
	virtual void processEvent(const irr::SEvent& event);

	virtual bool isVisible();

	//! true if pressed ok
	virtual bool lastSuccess(){return lastSucc;}

	//! render "non gui" stuff
	virtual void render();
	
	//! same effect as pressing the cancel button
	virtual void cancelEdit();
	
	virtual irr::gui::IGUIEnvironment* getGUIEnvironment() const{return env;}
	
	virtual irr::video::IVideoDriver* getVideoDriver() const{return driver;}
	
	virtual ICommonAppContext* getContext() const{return c;}
	
	virtual irr::gui::IGUIWindow* getWindow() const{return win;}
	
	//! may be overidden by child class
	virtual void OnCancel(){};
	
	//! may be overidden by child class
	virtual void OnSuccess(){};

};

//! A GUI Element wrapper to embed the IniEditor into the Irrlicht GUI
class IniEditorGUIElement : public irr::gui::IGUIElement{

	CommonIniEditor* iniEditor;
	std::function<void()> onSuccess;
	std::function<void()> onCancel;
	
	irr::core::dimension2d<irr::u32> ss;
	bool removeOnScreenResize;
	bool mustRemove;
	
	public:
	
	//! the iniEditor will be deleted in the destructor
	IniEditorGUIElement(CommonIniEditor* iniEditor, irr::gui::IGUIElement* parent = NULL, bool removeOnScreenResize = true);
	
	~IniEditorGUIElement();
	
	bool OnEvent(const irr::SEvent& event) override;
	
	void draw() override;
	
	void edit(IniFile* ini, const std::function<void()>& onSuccess, const std::function<void()>& onCancel = [](){});
	
	void setVisible(bool visible) override;
	
};

#endif
