#include <JSONRPC2Client.h>
#include <timing.h>
#include <SSLSocket.h>

#include <cassert>
#include <iostream>

struct Client{
	IPv4TCPSocket* tcp;
	SSLSocket* ssl;
};

const std::string hello = "HELLO FROM SSL\n";

const uint32_t buflen = 512;
char buf[buflen];

int main(int argc, char *argv[]){

	IPv4TCPSocket server;
	server.bind(9999);
	server.listen(10);
	
	std::list<Client> clients;
	
	CertStore store;
	bool res = store.loadX509PEMCert("./privatekey.pem");
	assert(res);

	while(true){
		IPv4TCPSocket* newClient = server.accept();
		if(newClient!=NULL){
			std::cout << "New Client" << std::endl;
			clients.push_back(Client{newClient, new SSLSocket(newClient, SSLSocket::SERVER, &store)});
			SSLSocket* s = clients.back().ssl;
			bool res = s->accept();
			assert(res);
			s->send(hello.c_str(), hello.size());
		}
		for(Client& c : clients){
			SSLSocket* s = c.ssl;
			uint32_t count = s->recv(buf, buflen);
			if(count>0){
				s->send(buf, count);
				std::cout << std::string(buf, count) << std::flush;
			}
		}
		delay(1);
	}
	
	
	return 0;
}
