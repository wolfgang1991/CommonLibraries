#include "NamedPipes.h"

#include <iostream>
#include <cstdint>

#if !SIMPLESOCKETS_WIN
#error Named pipes are windows only.
#endif

int main(){
	
	const uint32_t bufSize = 512;
	char buf[bufSize];
	
	std::string reply("This is the answer.");
	
	NamedPipeServer s("\\\\.\\pipe\\TestPipe", false);
	
	std::cout << "Waiting for connection ..." << std::endl;
	while(true){
		if(s.connect()){
			std::cout << "Connected" << std::endl;
			while(s.isConnected()){
				uint32_t received;
				if((received = s.recv(buf, bufSize))>0){
					std::cout << std::string(buf, received) << std::flush;
					s.send(reply.c_str(), reply.size());
				}
				Sleep(10);
			}
			std::cout << "isConnected: " << (int)s.isConnected() << std::endl;
		}
		Sleep(10);
	}
	
	return 0;
}
