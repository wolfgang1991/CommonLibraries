#ifndef AppTracker_H_INCLUDED
#define AppTracker_H_INCLUDED

#include <IRPC.h>
#include <IniFile.h>
#include <ForwardDeclarations.h>

#include <set>

class TrackGUIElement;

//! Tracking of app usage statistics (useful to find out how users are using an application / what to improve)
class AppTracker{
	friend class TrackGUIElement;

	public:
	
	class AppTrackerData{
	
		public:
		
		std::string id;//some unique id for synchronization purposes / should be unique for each installation
		std::string appVersion;
		std::string os;
		uint64_t longestUsageTime;//seconds while isWindowActive
		uint64_t totalUsageTime;//seconds while isWindowActive
		uint32_t startCount;
		uint64_t lastUsageUnixTime;
		std::string lastUsageTime;//utc
		std::string lastVisibleGUIElements;//space separated names if multiple
		
		std::vector<std::string> additionalFields;//application specific additional fields, appended at the CSV line
		
		bool dirty;//not synchronized, for local use only
		
		CREATE_BEGIN(AppTrackerData)
			FILL_FIELD(id)
			FILL_FIELD(appVersion)
			FILL_FIELD(os)
			FILL_FIELD(longestUsageTime)
			FILL_FIELD(totalUsageTime)
			FILL_FIELD(startCount)
			FILL_FIELD(lastUsageUnixTime)
			FILL_FIELD(lastUsageTime)
			FILL_FIELD(lastVisibleGUIElements)
			FILL_FIELD(additionalFields)
		CREATE_END

		CREATE_NATIVE_BEGIN(AppTrackerData)
			FILL_NATIVE_FIELD_IF_AVAILABLE(id, "")
			FILL_NATIVE_FIELD_IF_AVAILABLE(appVersion, "")
			FILL_NATIVE_FIELD_IF_AVAILABLE(os, "")
			FILL_NATIVE_FIELD_IF_AVAILABLE(longestUsageTime, 0)
			FILL_NATIVE_FIELD_IF_AVAILABLE(totalUsageTime, 0)
			FILL_NATIVE_FIELD_IF_AVAILABLE(startCount, 0)
			FILL_NATIVE_FIELD_IF_AVAILABLE(lastUsageUnixTime, 0)
			FILL_NATIVE_FIELD_IF_AVAILABLE(lastUsageTime, "")
			FILL_NATIVE_FIELD_IF_AVAILABLE(lastVisibleGUIElements, "")
			FILL_NATIVE_FIELD_IF_AVAILABLE(additionalFields, {})
		CREATE_NATIVE_END
		
		AppTrackerData(const std::string& id);
		
		AppTrackerData();
		
		void writeToIni(IniFile* ini) const;
		
		void readFromIni(const IniFile* ini);
		
		std::string calcCSVLine() const;
		
	};
	
	protected:
	
	IRPC* rpc;
	irr::IrrlichtDevice* device;
	
	AppTrackerData data;
	
	std::string iniPath;
	IniFile ini;
	
	std::set<TrackGUIElement*> trackElements;
	
	TrackGUIElement* lastLastRenderedElement;
	TrackGUIElement* lastRenderedElement;
	
	bool lastActive;
	
	uint64_t usageTime;
	uint64_t startTrackTime;//epoch secs if 0 not tracking
	
	double lastPushTime;
	
	bool enabled;
	
	//! called in constructor and destructor, assumes a rpc function "void pushAppTrackerData(data)" at the server
	virtual void pushToServerAndSave();
	
	virtual void startTimeTracking();
	
	virtual void stopTimeTracking();
	
	void setDirty();
	
	public:
	
	AppTracker(IRPC* rpc, irr::IrrlichtDevice* device, const std::string& iniPath, const std::string& id, const std::string& appVersion, bool enabled);
	
	virtual ~AppTracker();
	
	//! if not enabled tracking results won't be stored or synchronized with the server
	virtual void setEnabled(bool enabled);
	
	//! observes the true visibility of a GUI Element when the application window is no longer active, automatically removed when GUI Element is deleted
	virtual void addGUIElementTracker(const std::string& name, irr::gui::IGUIElement* ele);
	
	virtual void update();
	
	//! resizes the internal array if necessary, if makeDirty true it may cause a push to server
	virtual void setAdditionalField(uint32_t index, const std::string& value, bool makeDirty = true);
	
	//! empty string if not available
	virtual const std::string& getAdditionalField(uint32_t index) const;

};

#endif
