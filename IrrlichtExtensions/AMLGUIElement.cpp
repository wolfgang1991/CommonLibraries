#include <AMLGUIElement.h>
#include <AggregatableGUIElementAdapter.h>
#include <XMLParser.h>
#include <StringHelpers.h>
#include <PathFunctions.h>
#include <BeautifulGUIImage.h>
#include <BeautifulGUIText.h>
#include <BeautifulGUIButton.h>
#include <Drawer2D.h>
#include <utilities.h>
#include <FlexibleFont.h>
#include <font.h>
#include <ICommonAppContext.h>

#include <IGUIButton.h>
#include <ITexture.h>
#include <IReadFile.h>
#include <IFileSystem.h>
#include <IrrlichtDevice.h>

#include <fstream>
#include <iterator>
#include <iostream>

using namespace irr;
using namespace gui;
using namespace core;
using namespace video;
using namespace io;

const AMLGUIElement::NavButtons AMLGUIElement::defaultNavButtons{L"<", -1, L">", -1, L"1", -1, L"-", -1, L"+", -1, 0.05f, -1};

AMLGUIElement::AMLGUIElement(ICommonAppContext* context, irr::f32 recommendedSpace, irr::f32 aspectRatio, bool maintainAspectRatio, irr::s32 id, irr::s32 invisibleAggregationId, irr::s32 scrollbarId, std::string searchPath, std::string language, const NavButtons* navButtons, IGUIElement* parent, const irr::core::rect<irr::s32>& rectangle, irr::s32 buttonId, irr::video::SColor* overrideTextColor):
	AggregateGUIElement(context->getIrrlichtDevice()->getGUIEnvironment(), recommendedSpace, aspectRatio, recommendedSpace, aspectRatio, maintainAspectRatio, false, false, {}, {}, false, id, NULL, parent, rectangle),
	language(language),
	searchPath(appendMissingPathDelimeter(searchPath)),
	c(context),
	drawer(context->getDrawer()){
	this->buttonId = buttonId;
	back = fwd = reset = zoomOut = zoomIn = NULL;
	printParsingErrors = true;
	utf8Code = "<empty />";
	navGUI = NULL;
	buttonSpace = 0.f;
	this->invisibleAggregationId = invisibleAggregationId;
	this->scrollbarId = scrollbarId;
	if(navButtons!=NULL){
		buttonSpace = navButtons->buttonSpace;
		navGUI = new AggregateGUIElement(Environment, buttonSpace, 1.f, buttonSpace, 1.f, false, true, false, {}, {}, false, navButtons->buttonBarId==-1?id:navButtons->buttonBarId);
		bool addPadding = false;
		for(int i=0; i<5; i++){
			std::wstring label = i==0?navButtons->backButtonLabel:(i==1?navButtons->fwdButtonLabel:(i==2?navButtons->resetButtonLabel:(i==3?navButtons->zoomOutButtonLabel:navButtons->zoomInButtonLabel)));
			s32 id = i==0?navButtons->backButtonId:(i==1?navButtons->fwdButtonId:(i==2?navButtons->resetButtonId:(i==3?navButtons->zoomOutButtonId:navButtons->zoomInButtonId)));
			irr::gui::IGUIButton** button = &(i==0?back:(i==1?fwd:(i==2?reset:(i==3?zoomOut:zoomIn))));
			if(!label.empty()){
				if(addPadding){navGUI->addSubElement(new EmptyGUIElement(Environment, .25f, 1.f, false, false, -1));}
				*button = Environment->addButton(rect<s32>(0,0,0,0), NULL, id, label.c_str(), NULL);
				navGUI->addSubElement(new AggregatableGUIElementAdapter(Environment, 1.f, 1.f, false, *button, false, 0));
				addPadding = true;
			}
		}
		addSubElement(navGUI);
		navGUI->setVisible(false);
	}
	contentIndex = getSubElementCount();
	content = new EmptyGUIElement(Environment, 1.f-buttonSpace, 1.f, false, false, -1);
	addSubElement(content);
	zoom = 1.f;
	cbk = NULL;
	contentSet = false;
	useOverrideTextColor = overrideTextColor!=NULL;
	if(useOverrideTextColor){this->overrideTextColor = *overrideTextColor;}
}

AMLGUIElement::~AMLGUIElement(){
	//nothing to do (IGUIElement desctructor already deletes the children)
}

irr::video::SColor AMLGUIElement::getDefaultTextColor() const{
	return useOverrideTextColor?overrideTextColor:(Environment->getSkin()->getColor(EGDC_BUTTON_TEXT));
}

bool AMLGUIElement::hasContent() const{
	return contentSet;
}

bool AMLGUIElement::setCode(const std::string& utf8Code, bool printParsingErrors){
	if(navGUI){navGUI->setVisible(true);}
	Environment->setFocus(NULL);
	this->utf8Code = utf8Code;
	this->printParsingErrors = printParsingErrors;
	contentSet = true;
	return parseAndCreateGUI();
}

bool AMLGUIElement::setCodeFromFile(const std::string& path, bool printParsingErrors, bool adaptPath){
	Environment->setFocus(NULL);
	for(auto it=textures.begin(); it!=textures.end(); ++it){
		Environment->getVideoDriver()->removeTexture(*it);
	}
	textures.clear();
	bool res = false;
	IReadFile* rf = Environment->getFileSystem()->createAndOpenFile(path.c_str());
	if(rf){
		if(adaptPath){
			searchPath = getFolderPath(path);
		}
		uint32_t size = rf->getSize();
		char* buf = new char[size+1];
		buf[size] = '\0';
		rf->read(buf, size);
		rf->drop();
		res = setCode(buf, printParsingErrors);
		delete[] buf;
	}else{
		clear();
		std::wstringstream ss; ss << L"File could not be opened: " << convertUtf8ToWString(path);
		content = new BeautifulGUIText(ss.str().c_str(), getDefaultTextColor(), 0.f, NULL, true, true, Environment, 1.f-buttonSpace);
		addSubElement(content);
	}
	return res;
}

void AMLGUIElement::setLanguage(const std::string& language){
	this->language = language;
	parseAndCreateGUI();
}

bool AMLGUIElement::OnEvent(const irr::SEvent& event){
	if(event.EventType==EET_GUI_EVENT){
		const SEvent::SGUIEvent& g = event.GUIEvent;
		if(g.EventType==EGET_BUTTON_CLICKED){
			auto it = button2Params.find(g.Caller);
			if(it!=button2Params.end()){
				std::string& href = it->second.href;
				std::string& action = it->second.action;
				if(!action.empty() && cbk!=NULL){//action must be processed first since button2Params may be cleared in case of a href
					cbk->OnAMLAction(action);
				}
				if(!href.empty()){
					if(isPrefixEqual(href, "http:") || isPrefixEqual(href, "https:")){
						c->gotoURL(href);
					}else{
						gotoFile(isAbsolutePath(href)?href:(std::string(searchPath).append(href)));
						return true;
					}
				}
				return AggregateGUIElement::OnEvent(event);
			}
			if(back!=NULL && g.Caller==back){
				goBack();
			}else if(fwd!=NULL && g.Caller==fwd){
				goForward();
			}else if(reset!=NULL && g.Caller==reset){
				changeZoom(1.f);
			}else if(zoomOut!=NULL && g.Caller==zoomOut){
				changeZoom(zoom/1.1f);
			}else if(zoomIn!=NULL && g.Caller==zoomIn){
				changeZoom(zoom*1.1f);
			}
		}
	}
	return AggregateGUIElement::OnEvent(event);
}

void AMLGUIElement::setActionCallback(IAMLActionCallback* cbk){
	this->cbk = cbk;
}

void AMLGUIElement::changeZoom(irr::f32 newZoom){
	zoom = newZoom;
	std::list<std::pair<std::string, irr::f32>> scrollPositions;
	for(auto it = id2ScrollLink.begin(); it != id2ScrollLink.end(); ++it){
		scrollPositions.emplace_back(it->first, it->second.scrollbar->getPos());
	}
	parseAndCreateGUI();
	for(auto it = scrollPositions.begin(); it != scrollPositions.end(); ++it){
		id2ScrollLink[it->first].scrollbar->setPos(it->second, true);
	}
}

AMLGUIElement::ScrollLink::ScrollLink(){
	scrollable = NULL;
	scrollbar = NULL;
}

static inline bool eq(const std::string& a, const std::string& b){return a.compare(b)==0;}

class AMLParseCallback : public IParsingCallback{

	public:
	
	bool success;
	std::wstring errorMessage;
	
	bool isLanguageMatching;
	
	bool matchingLangFoundInTranslation;
	
	struct AggregateStackFrame{
		AggregateGUIElement* ele;
		bool isHorizontal;
		irr::f32 spaceX;//space in pixels of the whole aggregation
		irr::f32 spaceY;
		//irr::f32 maxChildSpace;//filled by children
		irr::f32 thisSpace;//sum of other space in current aggregation (this dimension e.g. vertical/horizontal space requirement)
		irr::f32 otherSpace;//maximum of space in current aggregation (other dimension space requirement)
		bool useChildSpace;//if true aggregate gets resized according to child space
		
		void updateMaxChildSpace(irr::f32 thisChildSpace, irr::f32 otherChildSpace){
			if(otherChildSpace>otherSpace){otherSpace = otherChildSpace;}
			thisSpace += thisChildSpace;
		}
		
		irr::f32 getSpace(){
			return isHorizontal?spaceX:spaceY;
		}
		
	};
	
	std::list<AggregateStackFrame> stack;
	
	AMLGUIElement* ele;
	
	std::string path;
	
	uint32_t disableCnt;//counts the children tags if a parent is disabled to properly determine the end of the disabled part
	
	void addElementIdIfAvail(IGUIElement* elementToAdd, XMLTag* t){
		auto it = t->attributes.find("id");
		if(it!=t->attributes.end()){
			std::string& id = it->second;
			auto it2 = ele->id2element.find(id);
			if(it2==ele->id2element.end()){
				ele->id2element[id] = elementToAdd;
			}else{
				std::cerr << "Error: Duplicate id: " << id << std::endl;
			}
		}
	}
	
	AMLParseCallback(AMLGUIElement* ele, const AggregateStackFrame& firstFrame):ele(ele){
		stack.push_back(firstFrame);
		success = true;
		isLanguageMatching = true;
		matchingLangFoundInTranslation = false;
		disableCnt = 0;
	}
	
	static irr::f32 getF32Value(const std::string& v, irr::f32 defaultValue){
		return (v.empty()||eq(v,"auto"))?defaultValue:convertStringTo<f32>(v);
	}
	
	static bool getBooleanValue(const std::string& v, bool defaultValue){
		return eq(v,"yes")?true:(eq(v,"no")?false:defaultValue);
	}
	
	static void setColorValue(irr::video::SColor& c, int index, int value){
		if(index==0){
			c.setRed(value);
		}else if(index==1){
			c.setGreen(value);
		}else if(index==2){
			c.setBlue(value);
		}else{
			c.setAlpha(value);
		}
	}
	
	static irr::video::SColor getColorValue(const std::string& v, irr::video::SColor defaultValue){
		if(!v.empty()){
			int state = 0;//0=r, 1=g, 2=b, 3=a
			std::stringstream ss;
			for(uint32_t i=0; i<v.size(); i++){
				char c = v[i];
				if(c==','){
					setColorValue(defaultValue, state, convertStringTo<int>(ss.str()));
					state++;
					ss.str("");
				}else if(c!=' ' && c!='\t'){
					ss << c;
				}
			}
			setColorValue(defaultValue, state, convertStringTo<int>(ss.str()));
		}
		return defaultValue;
	}

	void OnOpenTag(XMLParser* p){
		if(disableCnt>0){disableCnt++;return;}
		if(!success){return;}
		XMLTag* t = p->getCurrentDOM();
		AggregateStackFrame& prev = stack.back();
		if(isLanguageMatching){
			//check enable-if
			auto enableIt = t->attributes.find("enable-if");
			if(enableIt!=t->attributes.end()){
				auto it = ele->functions.find(enableIt->second);
				if(it!=ele->functions.end()){
					if(!it->second(ele, t)){
						disableCnt = 1;
						return;
					}
				}else{
					std::cerr << "Error: Function " << (enableIt->second) << " not found." << std::endl;
				}
			}
			//process tag
			bool isLink = eq(t->name, "link");
			if(eq(t->name,"aggregate") || isLink){
				const std::string& space = t->attributes["space"];
				bool useChildSpace = eq(space, "child");
				irr::f32 fSpace = useChildSpace?1.f:getF32Value(space, 1.f);//1.f in case of useChildSpace to propagate the parents space to own children
				const std::string& aspectRatio = t->attributes["aspectRatio"];
				bool maintainAspectRatio = !aspectRatio.empty();
				bool isHorizontal = eq(t->attributes["align"], "horizontal");//default: vertical
				bool isScrollable = getBooleanValue(t->attributes["scrollable"], !isLink);
				if(useChildSpace && isHorizontal==prev.isHorizontal){
					success = false;
					errorMessage = L"Previous aggregation has same alignment as current aggregation.\nThis is not possible in combination with automatically using the children's size for the current aggregation size.";
				}else{
					AggregateGUIElement* agg;
					if(isLink){
						BeautifulGUIButton* butt = new BeautifulGUIButton(ele->Environment, fSpace, getF32Value(aspectRatio, 1.f), maintainAspectRatio, isHorizontal, isScrollable, {}, {}, ele->buttonId);
						ele->button2Params[butt] = AMLGUIElement::ButtonParams{t->attributes["href"], t->attributes["action"]};
						agg = butt;
					}else{
						agg = new AggregateGUIElement(ele->Environment, fSpace, getF32Value(aspectRatio, 1.f), 1.f, 1.f, maintainAspectRatio, isHorizontal, isScrollable, {}, {}, false, ele->invisibleAggregationId);
					}
					addElementIdIfAvail(agg, t);
					prev.ele->addSubElement(agg);
					stack.push_back(AggregateStackFrame{agg, isHorizontal, prev.isHorizontal?(fSpace*prev.spaceX):prev.spaceX, prev.isHorizontal?prev.spaceY:(fSpace*prev.spaceY), 0.f, 0.f, useChildSpace});
					const std::string& scrollId = t->attributes["scrollId"];
					if(!scrollId.empty()){ele->id2ScrollLink[scrollId].scrollable = agg;}
				}
			}else if(eq(t->name,"img")){
				const std::string& src = t->attributes["src"];
				ITexture* tex = ele->Environment->getVideoDriver()->getTexture(std::string(ele->searchPath).append(src).c_str());
				if(tex){
					ele->textures.insert(tex);
					BeautifulGUIImage* img = new BeautifulGUIImage(ele->drawer, tex, ele->Environment, getF32Value(t->attributes["space"], 1.f), true, -1, getColorValue(t->attributes["color"], SColor(255,255,255,255)));
					prev.updateMaxChildSpace(img->getThisSpace(prev.getSpace()), img->getOtherSpaceForAspectRatio(prev.isHorizontal, prev.getSpace(), 0.f));
					prev.ele->addSubElement(img);
					addElementIdIfAvail(img, t);
				}else{
					success = false;
					errorMessage = L"Image could not be loaded, please check the src attribute.";
				}
			}else if(eq(t->name,"empty")){
				const std::string& aspectRatio = t->attributes["aspectRatio"];
				bool maintainAspectRatio = !aspectRatio.empty();
				EmptyGUIElement* e = new EmptyGUIElement(ele->Environment, getF32Value(t->attributes["space"], 1.f), getF32Value(aspectRatio, 1.f), maintainAspectRatio, false, -1);
				prev.updateMaxChildSpace(e->getThisSpace(prev.getSpace()), e->getOtherSpaceForAspectRatio(prev.isHorizontal, prev.getSpace(), 0.f));
				prev.ele->addSubElement(e);
				addElementIdIfAvail(e, t);
			}else if(eq(t->name,"scrollbar")){
				ScrollBar* sc = new ScrollBar(ele->Environment, getF32Value(t->attributes["space"], .1f), eq(t->attributes["align"], "horizontal"), ele->scrollbarId);
				prev.updateMaxChildSpace(sc->getThisSpace(prev.getSpace()), sc->getOtherSpaceForAspectRatio(prev.isHorizontal, prev.getSpace(), 0.f));
				prev.ele->addSubElement(sc);
				const std::string& scrollId = t->attributes["scrollId"];
				if(!scrollId.empty()){ele->id2ScrollLink[scrollId].scrollbar = sc;}
				addElementIdIfAvail(sc, t);
			}else if(eq(t->name,"lang")){
				isLanguageMatching = eq(t->attributes["label"], ele->language);
				matchingLangFoundInTranslation = matchingLangFoundInTranslation || isLanguageMatching;
			}else if(eq(t->name,"translation")){
				matchingLangFoundInTranslation = false;
			}else if(eq(t->name,"defaultLang")){
				isLanguageMatching = !matchingLangFoundInTranslation;
			}else if(t->name=="set"){
				for(auto& it : t->attributes){
					ele->environment[it.first] = it.second;
				}
			}else if(t->name=="execute"){
				auto it = ele->functions.find(t->attributes["function"]);
				if(it!=ele->functions.end()){
					it->second(ele, t);
				}else{
					std::cerr << "Error: Function " << (t->attributes["function"]) << " not found." << std::endl;
				}
			}else if(!eq(t->name,"text") && t->name!="span"){
				success = false;
				errorMessage = std::wstring(L"Unknown opening tag: ").append(convertUtf8ToWString(t->name));
			}
		}
	}

	void OnCloseTag(XMLParser* p){
		if(disableCnt>0){disableCnt--;return;}
		if(!success){return;}
		XMLTag* t = p->getCurrentDOM();
		if(isLanguageMatching){
			if((eq(t->name, "aggregate") || eq(t->name, "link")) && !stack.empty()){
				AggregateStackFrame cur = stack.back();
				stack.pop_back();
				AggregateStackFrame& prev = stack.back();
				if(cur.useChildSpace){
					f32 space = cur.otherSpace/prev.getSpace();
					cur.ele->setRecommendedSpace(space);
					cur.ele->setRecommendedActiveSpace(space);
				}
				prev.updateMaxChildSpace(cur.otherSpace, cur.thisSpace);
			}else if(eq(t->name, "text")){
				FlexibleFont* guiFont = (FlexibleFont*)(ele->Environment->getSkin()->getFont());
				auto oldScale = guiFont->getDefaultScale();
				f32 scale = ele->zoom*getF32Value(t->attributes["scale"], 1.f);
				guiFont->setDefaultScale(scale*oldScale);
				AggregateStackFrame& prev = stack.back();
				std::wstring text = convertUtf8ToWString(t->intermediate.str());
				const std::string& space = t->attributes["space"];
				bool spaceAvail = !(space.empty() || eq(space, "auto"));
				f32 fSpace = convertStringTo<f32>(space);
				f32 wordWrapLimit = -1.f;
				if(prev.isHorizontal && spaceAvail){
					wordWrapLimit = fSpace*prev.spaceX;
					text = makeWordWrappedText(text, wordWrapLimit, guiFont);
				}else if(!prev.isHorizontal){
					wordWrapLimit = prev.spaceX;
					text = makeWordWrappedText(text, prev.spaceX, guiFont);
				}
				f32 recommendedSpace = 1.f;
				dimension2d<u32> dim = guiFont->getDimension(text.c_str());
				if(!spaceAvail){
					if(prev.isHorizontal){
						recommendedSpace = ((f32)dim.Width)/(prev.spaceX);
					}else{
						recommendedSpace = ((f32)dim.Height)/prev.spaceY;
					}
				}else{
					recommendedSpace = fSpace;
				}
				BeautifulGUIText* bt = new BeautifulGUIText(text.c_str(), getColorValue(t->attributes["color"], ele->getDefaultTextColor()), getF32Value(t->attributes["italic"], 0.f), NULL, getBooleanValue(t->attributes["hcenter"], false), getBooleanValue(t->attributes["vcenter"], false), ele->Environment, recommendedSpace, -1, scale);
				prev.updateMaxChildSpace(recommendedSpace*prev.getSpace(), prev.isHorizontal?dim.Height:(wordWrapLimit>0.f?wordWrapLimit:dim.Width));//TODO fix problem with long one line texts: horizontal space is a little bit too small, why???
				prev.ele->addSubElement(bt);
				guiFont->setDefaultScale(oldScale);
				addElementIdIfAvail(bt, t);
			}else if(eq(t->name,"translation")){
				matchingLangFoundInTranslation = false;
			}else if(!eq(t->name, "img") && !eq(t->name, "empty") && !eq(t->name, "scrollbar") && !eq(t->name, "lang") && !eq(t->name, "defaultLang") && t->name!="set" && t->name!="execute" && t->name!="span"){
				success = false;
				errorMessage = std::wstring(L"Unknown/invalid closing tag: ").append(convertUtf8ToWString(t->name));
			}
		}else{
			isLanguageMatching = eq(t->name, "lang") || eq(t->name,"defaultLang");//a closing language tag enables all following tags independent of the language
		}
	}
	
	void OnFinishFile(XMLParser* p){
		for(auto it=ele->id2ScrollLink.begin(); it!=ele->id2ScrollLink.end(); ++it){
			AMLGUIElement::ScrollLink& link = it->second;
			if(link.scrollbar!=NULL && link.scrollable!=NULL){
				link.scrollbar->linkToScrollable(link.scrollable);
			}
		}
	}

};

bool AMLGUIElement::parseAndCreateGUI(){
	clear();
	AggregateGUIElement* agg = new AggregateGUIElement(Environment, 1.f-buttonSpace, 1.f, 1.f, 1.f, false, false, false, {}, {}, false, invisibleAggregationId);
	content = agg;
	addSubElement(content);
	updateAbsolutePosition();
	auto r = content->getAbsolutePosition();
	AMLParseCallback::AggregateStackFrame firstFrame{agg, false, (f32)r.getWidth(), (f32)r.getHeight(), 0.f, 0.f, false};
	AMLParseCallback cbk(this, firstFrame);
	XMLParser parser(&cbk);
	uint32_t lineCount = 1;
	for(uint32_t i=0; i<utf8Code.size(); i++){
		if(utf8Code[i]=='\n'){lineCount++;}
		parser.parse(utf8Code[i]);
		if(!cbk.success){
			clear();
			std::wstringstream ss; ss << cbk.errorMessage << L"\nIn line: " << lineCount;
			std::cerr << "Error: " << convertWStringToUtf8String(ss.str()) << std::endl;
			std::wstring text = makeWordWrappedText(ss.str(), r.getWidth(), Environment->getSkin()->getFont());
			content = new BeautifulGUIText(text.c_str(), getDefaultTextColor(), 0.f, NULL, true, true, Environment, 1.f-buttonSpace);
			addSubElement(content);
			break;
		}
	}
	if(cbk.success){cbk.OnFinishFile(&parser);}
	updateAbsolutePosition();
	return cbk.success;
}

void AMLGUIElement::goBack(){
	if(!backStack.empty()){
		if(!current.empty()){fwdStack.push_back(current);}
		current = backStack.back();
		backStack.pop_back();
		setCodeFromFile(current);
	}
}
	
void AMLGUIElement::goForward(){
	if(!fwdStack.empty()){
		if(!current.empty()){backStack.push_back(current);}
		current = fwdStack.back();
		fwdStack.pop_back();
		setCodeFromFile(current);
	}
}

void AMLGUIElement::gotoFile(const std::string& path){
	if(!current.empty()){backStack.push_back(current);}
	current = path;
	fwdStack.clear();
	setCodeFromFile(current);
}

const std::string& AMLGUIElement::getVariable(const std::string& name) const{
	auto it = environment.find(name);
	if(it!=environment.end()){
		return it->second;
	}
	static const std::string empty;
	return empty;
}

void AMLGUIElement::setVariable(const std::string& name, const std::string& value){
	environment[name] = value;
}

irr::gui::IGUIElement* AMLGUIElement::getElementById(const std::string& id) const{
	auto it = id2element.find(id);
	if(it!=id2element.end()){
		return it->second;
	}
	return NULL;
}

void AMLGUIElement::addFunction(const std::string& name, const std::function<bool(AMLGUIElement* ele, XMLTag* t)>& function){
	functions[name] = function;
}

void AMLGUIElement::clear(){
	id2element.clear();
	button2Params.clear();
	id2ScrollLink.clear();
	removeSubElement(contentIndex);
}
