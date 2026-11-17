#ifndef MJPGRTPPayloadProcessor_H_
#define MJPGRTPPayloadProcessor_H_

#include <RTPReceiver.h>

#include <dimension2d.h>
#include <ForwardDeclarations.h>

#include <string>

class MJPGRTPPayloadProcessorPrivate;

class MJPGRTPPayloadProcessor{

	MJPGRTPPayloadProcessorPrivate* p;
	
	public:
	
	MJPGRTPPayloadProcessor();
	
	virtual ~MJPGRTPPayloadProcessor();
	
	//! returns true if a new jpeg is available
	bool update(FrameBasedRTPReceiver::Buffer* b);
	
	//! call only after update returns true
	void writeJPEG(const std::string& path);
	
	//! call only after update returns true
	irr::core::dimension2d<irr::u32> getSize() const;
	
	//! call only after update returns true
	irr::video::IImage* createImageForSize() const;
	
	//! img must be large enough, see getSize, call only after update returns true, must be ECF_R8G8B8 color format, returns true if successful
	bool fillImage(irr::video::IImage* img);
	
};

#endif
