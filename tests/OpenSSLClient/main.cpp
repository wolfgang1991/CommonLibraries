#include <JSONRPC2Client.h>
#include <timing.h>
#include <SSLSocket.h>

#include <cassert>
#include <iostream>

const uint32_t buflen = 512;
char buf[buflen];

const std::string hello = "SOME_DATA\n";

int main(int argc, char *argv[]){
	
	SSLContext c(SSLContext::CLIENT);
//	bool success = c.useCertificateFromFile("../OpenSSLServer/selfsigned-public.crt");
//	assert(success);
	
	IPv4TCPSocket client;
	bool success = client.connect(IPv4Address("127.0.0.1", 9999), 1000);
	assert(success);
	
	SSLSocket ssl(&c, &client, false);
	success = ssl.connect();
	assert(success);
	
//	bool certVerified = ssl.verifyPeerCertificate();
//	std::cout << "certVerified: " << (int)certVerified << " hasPeerCertificate: " << (int)ssl.hasPeerCertificate() << std::endl;
//TODO CA based verification

	X509Cert expectedServerCert("../OpenSSLServer/selfsigned-public.crt");
	if(!ssl.isPeerCertificateEqual(expectedServerCert)){
		std::cerr << "Error: Invalid/unknown server certificate." << std::endl;
		return -1;
	}
	
	X509Cert wrongServerCert("./wrong-public-key.crt");
	assert(!ssl.isPeerCertificateEqual(wrongServerCert));

	while(true){
		int32_t count = ssl.recv(buf, buflen);
		if(count>0){
			std::cout << std::string(buf, count) << std::flush;
		}
		ssl.send(hello.c_str(), hello.size());
		delay(100);
	}
	
	
	return 0;
}
