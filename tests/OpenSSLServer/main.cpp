#include <JSONRPC2Client.h>
#include <timing.h>
#include <SSLSocket.h>

#include <cassert>
#include <iostream>

struct Client{
	IPv4TCPSocket* tcp;
	SSLSocket* ssl;
};

const std::string hello = "HELLO FROM SSL SERVER\n";

const uint32_t buflen = 512;
char buf[buflen];

int main(int argc, char *argv[]){

	IPv4TCPSocket server;
	server.bind(9999);
	server.listen(10);
	
	SSLContext c(SSLContext::SERVER);
	bool success = c.usePrivateKeyFromFile("./selfsigned-private.key");
	assert(success);
	success = c.useCertificateFromFile("./selfsigned-public.crt");
	assert(success);
	
	std::list<Client> clients;

	while(true){
		IPv4TCPSocket* newClient = server.accept();
		if(newClient!=NULL){
			std::cout << "New Client" << std::endl;
			clients.push_back(Client{newClient, new SSLSocket(&c, newClient)});//, &store
			SSLSocket* s = clients.back().ssl;
			bool res = s->accept();
			assert(res);
			s->send(hello.c_str(), hello.size());
		}
		for(Client& c : clients){
			SSLSocket* s = c.ssl;
			int32_t count = s->recv(buf, buflen);
			if(count>0){
				s->send(buf, count);
				std::cout << std::string(buf, count) << std::flush;
			}
		}
		delay(1);
	}
	
	
	return 0;
}
