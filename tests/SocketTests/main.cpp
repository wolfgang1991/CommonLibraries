#include <SimpleSockets.h>
#include <timing.h>

#include <functional>
#include <iostream>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <list>
#include <pthread.h>
#include <csignal>

void* SocketMain(void* arg){
	IPv4TCPSocket* server = (IPv4TCPSocket*)arg;
	std::list<IPv4TCPSocket*> clients;
	std::string test("test_from_server");
	uint32_t bufSize = 512;
	char buf[bufSize];
	while(true){
		IPv4TCPSocket* newClient = server->accept();
		if(newClient!=NULL){
			std::cout << "New Client\n" << std::flush;
			clients.push_back(newClient);
		}
		for(auto it = clients.begin(); it != clients.end(); ++it){
			IPv4TCPSocket* client = *it;
			client->send(test.c_str(), test.size());
			uint32_t received = client->recv(buf, bufSize);
			if(received>0){
				std::cout << "Server received: " << std::string(buf, received) << std::endl << std::flush;//TODO: remote ip
			}
		}
		delay(10);
	}
	return NULL;
}

template <typename TException, bool shouldPrint = false>
bool throwsException(const std::function<void()>& function){
	bool catched = false;
	try{
		function();
	}catch(TException exception){
		catched = true;
		if(shouldPrint){std::cout << exception << std::endl;}
	}
	return catched;
}

static void test(bool in, const char* label = "Test"){
	if(!in){
		std::cout << label << " failed.\n" << std::flush;
		std::raise(SIGINT);
	}else{
		std::cout << label << " successful.\n" << std::flush;
	}
}

int main(int argc, char *argv[]){

	std::list<IPv4Address> brdAddrList = queryIPv4BroadcastAdresses(666);
	for(auto it=brdAddrList.begin(); it!=brdAddrList.end(); ++it){
		std::cout << it->getAddressAsString() << ":" << it->getPort() << std::endl;
	}
	std::cout << std::endl;

	//Exceptions replaced by console output because of iOS problems:
	//test(throwsException<const char*,true>([](){IPv4Address("invalid",0);}), "Invalid address test");
	//test(throwsException<const char*,true>([](){IPv6Address("invalid",0);}), "Invalid address test2");
	
	test(IPv4Address("127.0.0.1",0).getAddressAsString().compare("127.0.0.1")==0);
	test(IPv6Address("::1",0).getAddressAsString().compare("::1")==0);
	
	std::cout << "UDP:" << std::endl << std::flush;
//	IPv4UDPSocket s0;
//	std::string target = "127.0.0.1";//"255.255.255.255";//
	IPv6UDPSocket s0;
	std::string target = "::1";//"ff02::1";//
	s0.setUDPTarget(IPv6Address(target, 8888));
	
	std::cout << "target: " << s0.getUDPTarget().getAddressAsString() << ":" << s0.getUDPTarget().getPort() << std::endl;
	assert(target.compare(s0.getUDPTarget().getAddressAsString())==0);
	
	uint32_t bufSize = 512;
	char buf[bufSize];
	
	//IPv4UDPSocket s1;
	IPv6UDPSocket s1;
	bool success = s1.bind(8888);
	if(!success){
		std::cout << "Error: " << strerror(errno) << std::endl;
	}
	
	const std::string test("testtesttest");
	
	for(int i=0; i<10; i++){
		s0.send(test.c_str(), test.size());
		uint32_t received = s1.recv(buf, bufSize);
		if(received>0){
			//IPv4Address fromAddress = s1.getLastDatagramAddress();
			IPv6Address fromAddress = s1.getLastDatagramAddress();
			std::cout << "received: " << std::string(buf, received) << " from " << fromAddress.getAddressAsString() << ":" << fromAddress.getPort() << std::endl << std::flush;//TODO: remote ip
		}
		delay(10);
	}
	std::cout << std::endl;
	
	std::cout << "TCP:" << std::endl << std::flush;
	IPv4TCPSocket client;
	IPv4Address a("127.0.0.1", 9999);
	
//	bool result = client.connect(a, 5);
//	assert(!result);//should fails since no listening socket
	
	IPv4TCPSocket server;
	server.bind(9999);
	server.listen(10);
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_t sockThread;
	pthread_create(&sockThread, &attr, SocketMain, (void*)(&server));
	pthread_attr_destroy(&attr);

	bool result = client.connect(a, 5);
	assert(result);
	
	for(int i=0; i<20; i++){
		client.send(test.c_str(), test.size());
		uint32_t received = client.recv(buf, bufSize);
		if(received>0){
			std::cout << "Client received: " << std::string(buf, received) << std::endl << std::flush;
		}
		delay(10);
	}
	
	std::list<IIPAddress*> addressList = queryIPAddressesForHostName("google.de", 666);
	for(auto it = addressList.begin(); it != addressList.end(); ++it){
		std::cout << "address: " << (*it)->getAddressAsString() << std::endl << std::flush;
	}

	return 0;
}
