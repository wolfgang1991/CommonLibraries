#include "NamedPipes.h"

#include <iostream>

#if SIMPLESOCKETS_WIN

ANamedPipe::ANamedPipe(){
	hPipe = INVALID_HANDLE_VALUE;
	connected = false;
}

bool ANamedPipe::isConnected() const{
	return connected && hPipe!=INVALID_HANDLE_VALUE;
}

uint32_t ANamedPipe::getAvailableBytes() const{
	if(hPipe!=INVALID_HANDLE_VALUE){
		DWORD availableBytes = 0;
		if(!PeekNamedPipe(hPipe,NULL,0,NULL,&availableBytes,NULL)){
			DWORD lastError = GetLastError();
			if(lastError==ERROR_BROKEN_PIPE || lastError==ERROR_NO_DATA){
				connected = false;
			}else{
				std::cerr << "Error while getting available bytes, GetLastError: " << GetLastError() << std::endl;
			}
		}
		return availableBytes;
	}
	return 0;
}

uint32_t ANamedPipe::recv(char* buf, uint32_t bufSize, bool readBlocking){
	if(hPipe == INVALID_HANDLE_VALUE){return 0;}
	if(!readBlocking){
		if(getAvailableBytes()==0){return 0;}
	}
	DWORD result;
	if(!ReadFile(hPipe, buf, bufSize, &result, NULL)){
		DWORD lastError = GetLastError();
		if(lastError==ERROR_BROKEN_PIPE){// || lastError==ERROR_NO_DATA){
			connected = false;
		}else if(lastError!=ERROR_NO_DATA){
			std::cerr << "Error while reading from pipe, GetLastError: " << GetLastError() << std::endl;
		}
	}
	return result;
}

bool ANamedPipe::send(const char* buf, uint32_t bufSize){
	if(hPipe == INVALID_HANDLE_VALUE){return false;}
	DWORD written;
	bool success = WriteFile(hPipe, buf, bufSize, &written, NULL);
	if(!success){
		DWORD lastError = GetLastError();
		if(lastError==ERROR_BROKEN_PIPE || lastError==ERROR_NO_DATA){
			connected = false;
		}else{
			std::cerr << "Error while writing to pipe, GetLastError: " << GetLastError() << std::endl;
		}
	}else if(written!=bufSize){//TODO perhaps automatically retry to write the rest
		std::cerr << "Error: Written bytes (" << written << ") do not match the amount of bytes to write (" << bufSize << ")" << std::endl;
	}
	return success && written==bufSize;
}

void NamedPipeServer::recreate(){
	if(hPipe != INVALID_HANDLE_VALUE){
		DisconnectNamedPipe(hPipe);
	}
	DWORD pipeMode = (blocking?PIPE_WAIT:PIPE_NOWAIT) | (byteOriented?PIPE_TYPE_BYTE:PIPE_TYPE_MESSAGE) | (byteOriented?PIPE_READMODE_BYTE:PIPE_READMODE_MESSAGE);
	hPipe = CreateNamedPipe( 
		namedPipe.c_str(),             // pipe name 
		PIPE_ACCESS_DUPLEX,       // read/write access 
		pipeMode,
		PIPE_UNLIMITED_INSTANCES, // max. instances  
		bufSize,                  // output buffer size 
		bufSize,                  // input buffer size 
		0,                        // client time-out 
		NULL);
	if(hPipe == INVALID_HANDLE_VALUE){
		std::cerr << "CreateNamedPipe failed, GetLastError: " << GetLastError() << std::endl; 
	}
}

NamedPipeServer::NamedPipeServer(const std::string& namedPipe, bool blocking, bool byteOriented, uint32_t bufSize):namedPipe(namedPipe),blocking(blocking),byteOriented(byteOriented),bufSize(bufSize){
	recreate();
}
	
NamedPipeServer::~NamedPipeServer(){
	if(hPipe != INVALID_HANDLE_VALUE){
		DisconnectNamedPipe(hPipe);
	}
}

bool NamedPipeServer::connect(){
	if(hPipe==INVALID_HANDLE_VALUE){recreate();}
	if(hPipe!=INVALID_HANDLE_VALUE){
		if(blocking){
			if(!(connected = ConnectNamedPipe(hPipe, NULL))){
				std::cerr << "Error in ConnectNamedPipe, GetLastError: " << GetLastError() << std::endl;
				recreate();
				connected = ConnectNamedPipe(hPipe, NULL);
			}
			return connected;
		}else{
			ConnectNamedPipe(hPipe, NULL);
			DWORD lastError = GetLastError();
			if(lastError!=ERROR_PIPE_LISTENING && lastError!=ERROR_PIPE_CONNECTED){
				if(lastError!=ERROR_NO_DATA){std::cerr << "Error in ConnectNamedPipe, GetLastError: " << lastError << std::endl;}
				recreate();
				ConnectNamedPipe(hPipe, NULL);
				lastError = GetLastError();
			}
			connected = lastError==ERROR_PIPE_CONNECTED;
			return connected;
		}
	}
	return false;
}

NamedPipeClient::NamedPipeClient(){}
	
NamedPipeClient::~NamedPipeClient(){
	if(hPipe != INVALID_HANDLE_VALUE){
		CloseHandle(hPipe);
	}
}

bool NamedPipeClient::connect(const std::string& namedPipe){
	if(hPipe != INVALID_HANDLE_VALUE){
		CloseHandle(hPipe);
	}
	hPipe = CreateFile(namedPipe.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	connected = hPipe!=INVALID_HANDLE_VALUE;
	if(hPipe == INVALID_HANDLE_VALUE){
		std::cerr << "CreateFile (for named pipe) failed, GetLastError: " << GetLastError() << std::endl; 
	}
	return connected;
}

#endif
