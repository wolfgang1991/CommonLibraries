# add source files to library
add_library(Sound SineWaveSoundSource.cpp SoundManager.cpp StaticWaveFileSource.cpp NoSoundDriver.h NoSoundDriver.cpp)

# include needed header file directories
include_directories(../Irrlicht/include ../Common)

# interface library for targets
target_include_directories(Sound INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})