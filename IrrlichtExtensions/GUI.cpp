#include <string>
#include <sstream>
#include <irrlicht.h>
#include <map>
#include <list>

#include <GUI.h>
#include <IniFile.h>
#include <StringHelpers.h>

using namespace std;
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

IGUIWindow* addWindowForRatio(IGUIEnvironment* env, IniFile* gini2, rect<s32> parentRect, bool modal, IGUIElement* parent, s32 id){
	double detRatio = convertStringTo<double>(gini2->get("","Ratio"));
	rect<s32> r = calcRatioRect(parentRect, detRatio);
	IGUIWindow* win = env->addWindow(r, modal, L"", parent, id);
	win->getCloseButton()->setVisible(false);
	win->setDrawTitlebar(false);
	return win;
}

irr::core::rect<irr::s32> calcRatioRect(irr::core::rect<irr::s32> original, double ratio){
	double w = original.getWidth(), h = original.getHeight();
	vector2d<double> center((double)original.UpperLeftCorner.X+w/2.0, (double)original.UpperLeftCorner.Y+h/2.0);
	if(w/ratio>=h+0.5){//w verkleinern
		w = h*ratio;
	}else{
		h = w/ratio;
	}
	return rect<s32>((int)(center.X-w/2.0+0.5), (int)(center.Y-h/2.0+0.5), (int)(center.X+w/2.0+0.5), (int)(center.Y+h/2.0+0.5));
}

ExtendedGUIElement::ExtendedGUIElement(irr::gui::IGUIElement* ele, int visDomain, const std::string& defaults){
	this->ele = ele;
	this->visDomain = visDomain;
	this->defaults = defaults;
}

ExtendedGUIElement::~ExtendedGUIElement(){
	ele->remove();
}

ExtendedEditBox::ExtendedEditBox(irr::gui::IGUIElement* ele, int visDomain, const std::string& defaults):ExtendedGUIElement(ele, visDomain, defaults){}

void ExtendedEditBox::setDefaults(){
	ele->setText(convertStringToWString(defaults).c_str());
}

ExtendedCheckBox::ExtendedCheckBox(irr::gui::IGUIElement* ele, int visDomain, const std::string& defaults, int hides, int switches, const std::list<std::string>& exclusiveCounterParts):ExtendedGUIElement(ele, visDomain, defaults){
	this->hides = hides;
	this->switches = switches;
	this->exclusiveCounterParts = exclusiveCounterParts;
}

void ExtendedCheckBox::setDefaults(){
	((IGUICheckBox*)ele)->setChecked((bool)convertStringTo<int>(defaults));
}

void ExtendedCheckBox::OnEvent(const irr::SEvent::SGUIEvent& event){
	if(event.Caller==ele && event.EventType==EGET_CHECKBOX_CHANGED){
		if(!exclusiveElements.empty()){
			((IGUICheckBox*)ele)->setChecked(true);
		}
		bool checked = ((IGUICheckBox*)ele)->isChecked();
		for(std::list<ExtendedGUIElement*>::iterator it = hideElements.begin(); it != hideElements.end(); ++it){(*it)->ele->setVisible(checked);}
		for(std::list<ExtendedGUIElement*>::iterator it = switchElements.begin(); it != switchElements.end(); ++it){(*it)->ele->setEnabled(checked);}
		for(std::list<ExtendedGUIElement*>::iterator it = exclusiveElements.begin(); it != exclusiveElements.end(); ++it){(*it)->sendIntMessage(*it, 666);}
	}
}

void ExtendedCheckBox::sendIntMessage(ExtendedGUIElement* caller, int message){
	if(message==666){//hiden + disablen
		((IGUICheckBox*)ele)->setChecked(false);
		for(std::list<ExtendedGUIElement*>::iterator it = hideElements.begin(); it != hideElements.end(); ++it){(*it)->ele->setVisible(false);}
		for(std::list<ExtendedGUIElement*>::iterator it = switchElements.begin(); it != switchElements.end(); ++it){(*it)->ele->setEnabled(false);}
	}
}

void ExtendedCheckBox::OnFinish(std::map<std::string, ExtendedGUIElement*>* elements){
	bool checked = ((IGUICheckBox*)ele)->isChecked();
	//Ausschlusselemente finden
	for(std::list<std::string>::iterator it = exclusiveCounterParts.begin(); it != exclusiveCounterParts.end(); ++it){
		std::map<std::string, ExtendedGUIElement*>::iterator it2 = elements->find(*it);
		if(it2 != elements->end()){
			IGUICheckBox* cele = ((IGUICheckBox*)it2->second->ele);
			cele->setChecked(cele->isChecked() && !checked);
			exclusiveElements.push_back(it2->second);
		}
	}
	//Zu Versteckende und Auschaltende Elemente finden
	for(std::map<std::string, ExtendedGUIElement*>::iterator it = elements->begin(); it != elements->end(); ++it){
		if(it->second->visDomain==hides){
			it->second->ele->setVisible(checked);
			hideElements.push_back(it->second);
		}
		if(it->second->visDomain==switches){
			it->second->ele->setEnabled(checked);
			switchElements.push_back(it->second);
		}
	}
}

static void readGuiElement(IniFile* ini, std::string section, int& visibilityDomain, irr::core::rect<irr::f32>& r, irr::gui::EGUI_ALIGNMENT& hori, irr::gui::EGUI_ALIGNMENT& verti, std::string& text, std::string& uuid, std::string& standardValue){
	visibilityDomain = convertStringTo<int>(ini->get(section, "VisibilityDomain"));
	std::string halign = ini->get(section, "HAlign");
	hori = halign.compare("Left")==0?EGUIA_UPPERLEFT:(halign.compare("Center")==0?EGUIA_CENTER:EGUIA_LOWERRIGHT);
	std::string valign = ini->get(section, "VAlign");
	verti = valign.compare("Top")==0?EGUIA_UPPERLEFT:(valign.compare("Center")==0?EGUIA_CENTER:EGUIA_LOWERRIGHT);
	//("%s %s %i %i\n", halign.c_str(), valign.c_str(), (int)hori, (int)verti);
	text = ini->get(section, "Text");
	uuid = ini->get(section, "Id");
	standardValue = ini->get(section, "Default");
	std::string rs = ini->get(section, "Rect");
	std::stringstream ss;
	int filled = 0;
	for(unsigned int i=0; i<rs.size(); i++){
		char c = rs[i];
		if(c==',' || c==';'){
			(filled==0?r.UpperLeftCorner.X:(filled==1?r.UpperLeftCorner.Y:(filled==2?r.LowerRightCorner.X:r.LowerRightCorner.Y))) = convertStringTo<irr::f32>(ss.str());
			ss.str("");
			filled++;
		}else if(c!=' ' && c!='\t'){
			ss << c;
		}
	}
	(filled==0?r.UpperLeftCorner.X:(filled==1?r.UpperLeftCorner.Y:(filled==2?r.LowerRightCorner.X:r.LowerRightCorner.Y))) = convertStringTo<irr::f32>(ss.str());
}

GUI::GUI(irr::gui::IGUIEnvironment* env, IniFile* ini, irr::core::rect<s32> r, irr::gui::IGUIElement* parent, bool keep_aspect_ratio){
	this->env = env;
	this->r = r;
	this->parent = parent;
	this->keep_aspect_ratio = keep_aspect_ratio;
	int eleCount = convertStringTo<int>(ini->get("", "EleCount"));
	double ratio = convertStringTo<double>(ini->get("", "Ratio"));//TODO
	for(int i=0; i<eleCount; i++){
		std::string section = std::string("E").append(convertToString(i));
		std::string type = ini->get(section,"Type");
		int visibilityDomain; irr::core::rect<irr::f32> er; irr::gui::EGUI_ALIGNMENT hori; irr::gui::EGUI_ALIGNMENT verti; std::string text; std::string uuid; std::string standardValue;
		readGuiElement(ini, section, visibilityDomain, er, hori, verti, text, uuid, standardValue);
		int w = r.getWidth(), h = r.getHeight();
		rect<s32> fr(r.UpperLeftCorner.X+er.UpperLeftCorner.X*w, r.UpperLeftCorner.Y+er.UpperLeftCorner.Y*h, r.UpperLeftCorner.X+er.LowerRightCorner.X*w, r.UpperLeftCorner.Y+er.LowerRightCorner.Y*h);
		if(keep_aspect_ratio){
			double eleRatio = er.getWidth()*ratio/er.getHeight();//da ratio = w/h = w0/1.0
			double frw = (er.LowerRightCorner.X-er.UpperLeftCorner.X)*w;//alles in double rechnen wegen rundungsfehlern
			double frh = (er.LowerRightCorner.Y-er.UpperLeftCorner.Y)*h;
			//printf("eleRatio: %f, text: %s er: %f %f, frw: %f, frh: %f\n", eleRatio, text.c_str(), er.getWidth(), er.getHeight(), frw, frh);
			vector2d<double> center(r.UpperLeftCorner.X+(er.LowerRightCorner.X+er.UpperLeftCorner.X)/2.0*w, r.UpperLeftCorner.Y+(er.LowerRightCorner.Y+er.UpperLeftCorner.Y)/2.0*h);//(double)fr.UpperLeftCorner.X+frw/2.0, (double)fr.UpperLeftCorner.Y+frh/2.0);
			if(frw/eleRatio>frh){//frw muss verkleinert werden eps=0.5px
				frw = frh*eleRatio;
			}else{
				frh = frw/eleRatio;
			}
			fr = rect<s32>((int)(center.X-frw/2.0+0.5), (int)(center.Y-frh/2.0+0.5), (int)(center.X+frw/2.0+0.5), (int)(center.Y+frh/2.0+0.5));
		}
		//printf("%i %i %i %i\n", fr.UpperLeftCorner.X, fr.UpperLeftCorner.Y, fr.LowerRightCorner.X, fr.LowerRightCorner.Y);
		if(type.compare("Button")==0){
			IGUIButton* but = env->addButton(fr, parent, -1, convertStringToWString(text).c_str(), NULL);
			elements[uuid] = new ExtendedGUIElement(but, visibilityDomain, standardValue);
		}else if(type.compare("Text")==0){
			IGUIStaticText* but = env->addStaticText(convertStringToWString(text).c_str(), fr, false, true, parent, -1, false);
			but->setTextAlignment(hori,verti);
			elements[uuid] = new ExtendedGUIElement(but, visibilityDomain, standardValue);
		}else if(type.compare("ListBox")==0){
			IGUIListBox* but = env->addListBox(fr, parent, -1, true);
			elements[uuid] = new ExtendedGUIElement(but, visibilityDomain, standardValue);
		}else if(type.compare("ComboBox")==0){
			IGUIComboBox* but = env->addComboBox(fr, parent, -1);
			elements[uuid] = new ExtendedGUIElement(but, visibilityDomain, standardValue);
		}else if(type.compare("CheckBox")==0){
			IGUICheckBox* but = env->addCheckBox(false, fr, parent, -1, convertStringToWString(text).c_str());
			ExtendedGUIElement* ee = new ExtendedCheckBox(but, visibilityDomain, standardValue, convertStringTo<int>(ini->get(section,"Hides")), convertStringTo<int>(ini->get(section,"Switches")), parseSeparatedString(ini->get(section,"ExclusiveCounterParts"),','));
			ee->setDefaults();
			elements[uuid] = ee;
		}else if(type.compare("VerticalScrollBar")==0){
			IGUIScrollBar* scroll = env->addScrollBar(false, fr, parent, -1);
			elements[uuid] = new ExtendedGUIElement(scroll, visibilityDomain, standardValue);
		}else if(type.compare("HorizontalScrollBar")==0){
			IGUIScrollBar* scroll = env->addScrollBar(true, fr, parent, -1);
			elements[uuid] = new ExtendedGUIElement(scroll, visibilityDomain, standardValue);
		}else if(type.compare("EditBox")==0){
			int id = -3;//STRING_EDIT
			std::string editType = ini->get(section,"EditType");
			if(editType.compare("INT_EDIT")==0){
				id = -1;
			}else if(editType.compare("DOUBLE_EDIT")==0){
				id = -2;
			}else if(editType.compare("NO_EDIT")==0){
				id = -4;
			}else if(editType.compare("MULTILINE_STRING_EDIT")==0){
				id = -5;
			}else if(editType.compare("DESTINATION_EDIT")==0){
				id = -6;
			}else if(editType.compare("TERMINAL_EDIT")==0){
				id = -7;
			}
			IGUIEditBox* but = env->addEditBox(L"", fr, true, parent, id);
			ExtendedGUIElement* ee = new ExtendedEditBox(but, visibilityDomain, standardValue);
			elements[uuid] = ee;
			ee->setDefaults();
			but->setTextAlignment(hori,verti);
		}else if(type.compare("SpecialRectangle")==0){
			rects[uuid] = fr;
		}//TODO: weitere Typen
	}
	for(std::map<std::string, ExtendedGUIElement*>::iterator it = elements.begin(); it != elements.end(); ++it){
		it->second->OnFinish(&elements);
	}
}

GUI::~GUI(){
	for(std::map<std::string, ExtendedGUIElement*>::iterator it = elements.begin(); it != elements.end(); ++it){
		delete it->second;
	}
}


void GUI::Update(const irr::SEvent::SGUIEvent& event){
	for(std::map<std::string, ExtendedGUIElement*>::iterator it = elements.begin(); it != elements.end(); ++it){
		it->second->OnEvent(event);
	}
}


irr::gui::IGUIElement* GUI::getElement(const std::string& uid){
	std::map<std::string, ExtendedGUIElement*>::iterator it = elements.find(uid);
	if(it != elements.end()){
		return it->second->ele;
	}else{
		return NULL;
	}
}


void GUI::setDefaults(){
	for(std::map<std::string, ExtendedGUIElement*>::iterator it = elements.begin(); it != elements.end(); ++it){
		it->second->setDefaults();
	}
}


irr::core::rect<s32> GUI::getSpecialRectangle(const std::string& uid){
	std::map<std::string, irr::core::rect<s32> >::iterator it = rects.find(uid);
	if(it != rects.end()){
		return it->second;
	}else{
		return rect<s32>(0,0,0,0);
	}
}

void GUI::setVisible(bool visible){
	for(std::map<std::string, ExtendedGUIElement*>::iterator it = elements.begin(); it != elements.end(); ++it){
		it->second->ele->setVisible(visible);
	}
}

const std::map<std::string, irr::core::rect<irr::s32> >& GUI::getAllSpecialRectangles(){
	return rects;
}

