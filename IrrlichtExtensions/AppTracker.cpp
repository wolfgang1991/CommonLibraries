#include <AppTracker.h>
#include <platforms.h>

#include <IGUIElement.h>
#include <IrrlichtDevice.h>
#include <IGUIEnvironment.h>

#include <timing.h>

#include <iostream>
#include <cassert>

#define PUSH_PERIOD 60.0//s

using namespace irr;
using namespace core;
using namespace gui;

class TrackGUIElement : public IGUIElement{
	
	public:
	
	AppTracker* at;
	std::string name;
	
	TrackGUIElement(AppTracker* at, IGUIEnvironment* env, IGUIElement* parent, const std::string& name):IGUIElement(EGUIET_ELEMENT, env, parent,-1,rect<s32>(0,0,1,1)),at(at),name(name){
		drop();
	}
	
	~TrackGUIElement(){
		at->trackElements.erase(this);
		if(at->lastRenderedElement==this){
			at->lastRenderedElement = NULL;
		}
	}
	
	void draw() override{
		at->lastRenderedElement = this;
	}
	
};

AppTracker::AppTrackerData::AppTrackerData(const std::string& id):id(id),longestUsageTime(0),totalUsageTime(0),startCount(0),lastUsageUnixTime(0),dirty(false){}

AppTracker::AppTrackerData::AppTrackerData():longestUsageTime(0),totalUsageTime(0),startCount(0),lastUsageUnixTime(0),dirty(false){}
		
void AppTracker::AppTrackerData::writeToIni(IniFile* ini) const{
	INI_SELECT_SECTION(*ini, id)
	INI_SET(appVersion)
	INI_SET(os)
	INI_SET(longestUsageTime)
	INI_SET(totalUsageTime)
	INI_SET(startCount)
	INI_SET(lastUsageUnixTime)
	INI_SET(lastUsageTime)
	INI_SET(lastVisibleGUIElements)
	INI_SET_VECTOR(additionalFields)
}

void AppTracker::AppTrackerData::readFromIni(const IniFile* ini){
	INI_SELECT_CONST_SECTION(*ini, id)
	INI_GET(appVersion, std::string())
	INI_GET(os, std::string())
	INI_GET(longestUsageTime, (uint64_t)0)
	INI_GET(totalUsageTime, (uint64_t)0)
	INI_GET(startCount, (uint32_t)0)
	INI_GET(lastUsageUnixTime, (uint64_t)0)
	INI_GET(lastUsageTime, std::string())
	INI_GET(lastVisibleGUIElements, std::string())
	INI_GET_VECTOR(additionalFields)
}

std::string AppTracker::AppTrackerData::calcCSVLine() const{
	std::stringstream ss;
	ss << id << ',' << appVersion << ',' << os << ',' << longestUsageTime << ',' << totalUsageTime << ',' << startCount << ',' << lastUsageUnixTime << ',' << lastUsageTime << ',' << lastVisibleGUIElements;
	for(uint32_t i=0; i<additionalFields.size(); i++){
		ss << ',' << additionalFields[i];
	}
	ss << "\r\n";
	return ss.str();
}

AppTracker::AppTracker(IRPC* rpc, irr::IrrlichtDevice* device, const std::string& iniPath, const std::string& id, const std::string& appVersion, bool enabled):
	rpc(rpc),device(device),data(id),iniPath(iniPath),ini(iniPath),enabled(enabled){
	lastLastRenderedElement = lastRenderedElement = NULL;
	data.readFromIni(&ini);
	data.id = id;
	data.appVersion = appVersion;
	data.os = PLATFORM_STRING;
	time_t rawtime = time(NULL);
	data.lastUsageUnixTime = rawtime;
	std::stringstream ss; ss << std::put_time(std::gmtime(&rawtime), "%c %Z");
	data.lastUsageTime = ss.str();
	data.startCount++;
	pushToServerAndSave();//push last data just in case it didn't work last time (e.g. no internet connection)
	data.lastVisibleGUIElements = "";//reset after push to push last data
	lastPushTime = 0;
	lastActive = device->isWindowActive();
	startTrackTime = lastActive?getEpochSecs():0;
	usageTime = 0;
}

void AppTracker::setEnabled(bool enabled){
	this->enabled = enabled;
}

void AppTracker::startTimeTracking(){
	if(startTrackTime==0){
		startTrackTime = getEpochSecs();
	}
}

void AppTracker::setDirty(){
	if(!data.dirty){
		data.dirty = true;
		lastPushTime = getSecs();//not true but is shall be pushed 1min after it became dirty, otherwise immediate push
	}
}
	
void AppTracker::stopTimeTracking(){
	if(startTrackTime!=0){
		uint64_t dt = getEpochSecs()-startTrackTime;
		usageTime += dt;
		data.totalUsageTime += dt;
		if(usageTime>data.longestUsageTime){
			data.longestUsageTime = usageTime;
		}
		startTrackTime = 0;
		setDirty();
	}
}

AppTracker::~AppTracker(){
	stopTimeTracking();
	if(data.dirty){
		pushToServerAndSave();
	}
	std::set<TrackGUIElement*> moved(std::move(trackElements));
	trackElements.clear();
	for(TrackGUIElement* ele : moved){
		ele->remove();
	}
}

void AppTracker::addGUIElementTracker(const std::string& name, irr::gui::IGUIElement* ele){
	if(ele){
		trackElements.insert(new TrackGUIElement(this, device->getGUIEnvironment(), ele, name));
	}
}

void AppTracker::update(){
	//Time Tracking
	bool isNowActive = device->isWindowActive();
	if(isNowActive!=lastActive){
		if(isNowActive){
			startTimeTracking();
		}else{
			stopTimeTracking();
		}
		lastActive = isNowActive;
	}
	//Top Window Tracking
	if(lastRenderedElement!=lastLastRenderedElement){
		lastLastRenderedElement = lastRenderedElement;
		if(lastRenderedElement){
			std::stringstream ss; ss << data.lastVisibleGUIElements << ' ' << lastRenderedElement->name;
			data.lastVisibleGUIElements = ss.str();
		}else{
			data.lastVisibleGUIElements += " -";
		}
		setDirty();
	}
	lastRenderedElement = NULL;//gets overwritten if elements visible
	//sync data if dirty but not to quick in case more fields change
	if(data.dirty){
		double t = getSecs();
		if(t-lastPushTime>PUSH_PERIOD){
			pushToServerAndSave();
			lastPushTime = t;
			data.dirty = false;
		}
	}
}
	
void AppTracker::setAdditionalField(uint32_t index, const std::string& value, bool makeDirty){
	if(index>=data.additionalFields.size()){
		data.additionalFields.resize(index+1);
	}
	if(data.additionalFields[index] != value){
		data.additionalFields[index] = value;
		if(makeDirty){setDirty();}
	}
}

const std::string& AppTracker::getAdditionalField(uint32_t index) const{
	if(index>=data.additionalFields.size()){
		static const std::string empty("");
		return empty;
	}
	return data.additionalFields[index];
}

void AppTracker::pushToServerAndSave(){
	if(enabled){
		rpc->callRemoteProcedure("pushAppTrackerData", std::vector<IRPCValue*>{createRPCValue(data)});
		data.writeToIni(&ini);
		ini.save(iniPath);
	}
}
