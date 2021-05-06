#include "NamedPipes.h"

#include <iostream>
#include <cstdint>

#if !SIMPLESOCKETS_WIN
#error Named pipes are windows only.
#endif

static void tryConnectForever(NamedPipeClient& c){
	while(!c.connect("\\\\.\\pipe\\TestPipe")){
		Sleep(100);
	}
}

int main(){
	
	std::string test("This is a testtring.\n");
	
	const uint32_t bufSize = 512;
	char buf[bufSize];
	
	NamedPipeClient c;
	tryConnectForever(c);
	
	for(uint32_t i=0; i<10; i++){
		c.send(test.c_str(), test.size());
		std::cout << "isConnected: " << (int)c.isConnected() << std::endl;
		if(!c.isConnected()){
			tryConnectForever(c);
		}else{
			Sleep(1000);
		}
		uint32_t received;
		while((received=c.recv(buf,bufSize))>0){
			std::cout << std::string(buf,received) << std::endl;
		}
	}
	
	return 0;
}
