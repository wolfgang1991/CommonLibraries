#include <SoundManager.h>
#include <SineWaveSoundSource.h>
#include <StaticWaveFileSource.h>
#include <timing.h>

#include <cmath>
#include <iostream>

int main(int argc, char *argv[]){
	
	SineWaveSoundSource sine(200, 1000);
	
	StaticWaveFileSource theme("IrrlichtThemeCutted.wav");
	theme.setLoop(true);
	
	StaticWaveFileSource impact("impact.wav");
	
	SoundManager soundmgr;//(10.0);
	
	//soundmgr.play(&theme);
	soundmgr.play(&sine);
	//soundmgr.play(&impact);
	
	impact.setLoop(true);
	
	//TODO Test pause, get rid of distortions
	
	while(true){
		delay(10);
		double f = 600+400*sin(2*3.14*0.25*getSecs());
		sine.setFrequency(f);
	}
	
	return 0;
}
