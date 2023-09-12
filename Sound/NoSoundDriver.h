#ifndef CLIENT_NOSOUNDDRIVER_H
#define CLIENT_NOSOUNDDRIVER_H

#include "ISoundDriver.h"

class NoSoundDriver : public ISoundDriver{

public:

    NoSoundDriver(){}

    IPCMPlaybackContext* createPlaybackContext(ISoundSource* source);

    uint32_t getNextAcceptableSamplingFrequency(uint32_t desiredFrequency){
        return desiredFrequency;
    }

};

#endif //CLIENT_NOSOUNDDRIVER_H
