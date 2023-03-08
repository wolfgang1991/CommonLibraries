#include "NoSoundDriver.h"

class NoSoundPlaybackContext: public IPCMPlaybackContext{
public:

    NoSoundPlaybackContext(ISoundSource* source):IPCMPlaybackContext(source){}

    virtual ~NoSoundPlaybackContext(){}

    virtual uint32_t update(bool pause, uint32_t maxBytes){
        return 0;
    }

    virtual uint32_t getDelayFrameCount(){
        return 0;
    }

};

IPCMPlaybackContext* NoSoundDriver::createPlaybackContext(ISoundSource* source){
    return new NoSoundPlaybackContext(source);
}
