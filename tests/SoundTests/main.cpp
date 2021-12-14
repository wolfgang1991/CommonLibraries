#include <SoundManager.h>
#include <SineWaveSoundSource.h>
#include <StaticWaveFileSource.h>
#include <timing.h>

#include <cmath>
#include <csignal>
#include <iostream>

volatile bool running = true;

void sigfunc(int sig){
	running = false;
}

int main(int argc, char *argv[]){

	signal(SIGINT, sigfunc);
	
	SoundManager soundmgr(0.1);
	
	SineWaveSoundSource* sine = soundmgr.create<SineWaveSoundSource>(200, 2000);
	
	StaticWaveFileSource* theme = soundmgr.create<StaticWaveFileSource>("IrrlichtThemeCutted.wav");
	//theme->setLoop(true);
	
	StaticWaveFileSource* impact = soundmgr.create<StaticWaveFileSource>("impact.wav");
	
	StaticWaveFileSource* beep = soundmgr.create<StaticWaveFileSource>("3-Beep.wav");
	
	soundmgr.play(theme);
	soundmgr.play(sine);
	soundmgr.play(impact);
	soundmgr.play(beep);
	
	impact->setLoop(true);
	beep->setLoop(true);
	
	//TODO Test pause
	//TODO test play after finish (no loop)
	
	uint32_t delayTime = 2000;
	std::cout << "playing" << std::endl;
	delay(delayTime);
	std::cout << "pause" << std::endl;
	soundmgr.pause(theme);
	delay(delayTime);
	std::cout << "continue" << std::endl;
	soundmgr.play(theme);
	delay(delayTime);
	std::cout << "seek" << std::endl;
	theme->seek(0.5f);
	
	while(running){
		delay(10);
		double f = 600+400*sin(2*3.14*0.025*getSecs());
		sine->setFrequency(f);
		soundmgr.update();
		if(!theme->isPlayingOrReady()){
			soundmgr.update();
			delay(1000);
			soundmgr.play(theme);//restart after 1s
		}
	}
	
	return 0;
}
