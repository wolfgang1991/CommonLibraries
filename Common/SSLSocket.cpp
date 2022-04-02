#ifndef NO_OPENSSL

#include "SSLSocket.h"

#include <timing.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509_vfy.h>

#include <fstream>
#include <cassert>
#include <cstring>
#include <iostream>

class SSLContextPrivate{
	
	public:
	
	SSL_CTX* ctx;
	
	std::list<RSA*> privateKeys;
	std::list<X509*> certificates;
	
	SSLContextPrivate(SSLContext::Mode mode){
		ctx = SSL_CTX_new(mode==SSLContext::SERVER?TLS_server_method():TLS_client_method());
	}
	
	~SSLContextPrivate(){
		SSL_CTX_free(ctx);
		for(RSA* rsa : privateKeys){
			RSA_free(rsa);
		}
		for(X509* cert : certificates){
			X509_free(cert);
		}
	}
	
};

SSLContext::SSLContext(Mode mode){
	p = new SSLContextPrivate(mode);
}
	
SSLContext::~SSLContext(){
	delete p;
}

bool SSLContext::usePrivateKey(const std::vector<char>& data){
	BIO* kbio = BIO_new_mem_buf((void*)&(data[0]), data.size());
	RSA* rsa = PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL);
	bool success = rsa!=NULL;
	if(success){
		SSL_CTX_use_RSAPrivateKey(p->ctx, rsa);
		p->privateKeys.push_back(rsa);
	}
	BIO_free(kbio);
	return success;
}
	
bool SSLContext::useCertificate(const std::vector<char>& data){
	BIO* cbio = BIO_new_mem_buf((void*)&(data[0]), data.size());
	X509* cert = PEM_read_bio_X509(cbio, NULL, 0, NULL);
	bool success = cert!=NULL;
	if(success){
		SSL_CTX_use_certificate(p->ctx, cert);
		p->certificates.push_back(cert);
	}
	BIO_free(cbio);
	return success;
}

static bool loadFile(std::vector<char>& out, const std::string& path){
	std::ifstream f(path.c_str(), std::ifstream::binary);
	if(f.good()){
		f.seekg(0, std::ifstream::end);
		out.resize(f.tellg());
		f.seekg(0, std::ifstream::beg);
		f.read(&out[0], out.size());
		return true;
	}
	return false;
}

bool SSLContext::usePrivateKeyFromFile(const std::string& path){
	std::vector<char> v;
	if(loadFile(v, path)){
		return usePrivateKey(v);
	}
	return false;
}
	
bool SSLContext::useCertificateFromFile(const std::string& path){
	std::vector<char> v;
	if(loadFile(v, path)){
		return useCertificate(v);
	}
	return false;
}

static BIO_METHOD* bio_method = NULL;
static int init_bio_method();

class SSLSocketPrivate{
	
	public:
	
	SSLContext* c;
	
	ICommunicationEndpoint* slaveSocket;
	bool mustDeleteSlaveSocket;
	
	bool pseudoBlocking;//during accept and connect
	
	SSL* ssl;
	
	SSLSocketPrivate(SSLContext* c, ICommunicationEndpoint* slaveSocket, bool mustDeleteSlaveSocket):c(c),slaveSocket(slaveSocket),mustDeleteSlaveSocket(mustDeleteSlaveSocket),pseudoBlocking(false){
		ssl = SSL_new(c->p->ctx);
		if(ssl){
			if(bio_method==NULL){init_bio_method();}
			BIO* bio = BIO_new(bio_method);
			BIO_set_data(bio, this);
			SSL_set_bio(ssl, bio, bio);//bio is reference counted
		}
	}
	
	~SSLSocketPrivate(){
		if(ssl){
			int res = SSL_shutdown(ssl);
			if(res<=0){ERR_print_errors_fp(stderr);}
			SSL_free(ssl);
		}
		if(mustDeleteSlaveSocket){delete slaveSocket;}
	}
	
};

static int bio_create(BIO *b){
	BIO_set_init(b, 1);
	BIO_set_data(b, NULL);
	return 1;
}

static int bio_destroy(BIO *b){
	if(!b){return 0;}
	BIO_set_data(b, NULL);
	return 1;
}

static int bio_read(BIO *b, char* buf, int len){
	if(b==NULL){return 0;}
	SSLSocketPrivate* p = (SSLSocketPrivate*)BIO_get_data(b);
	int res = p->slaveSocket->recv(buf, len);
	while(p->pseudoBlocking && res==0){delay(1); res = p->slaveSocket->recv(buf, len);}//pseudo blocking to assure accept and connect
	return res;
}

static int bio_write(BIO* b, const char* buf, int len){
	if(b==NULL){return 0;}
	SSLSocketPrivate* p = (SSLSocketPrivate*)BIO_get_data(b);
	bool success = p->slaveSocket->send(buf, len);
	return success?len:-1;
}

static long bio_ctrl(BIO *b, int cmd, long num, void *ptr){
	if(cmd == BIO_CTRL_FLUSH){return 1;}
	return 0;
}

static int bio_gets(BIO *b, char *buf, int len){
	return -2;
}

static int bio_puts(BIO *b, const char *str){
	return bio_write(b, str, strlen(str));
}

static int init_bio_method(){
	bio_method = BIO_meth_new(BIO_TYPE_SOURCE_SINK | BIO_get_new_index(), "ssl_socket_cpp");
	BIO_meth_set_write(bio_method, bio_write);
	BIO_meth_set_read(bio_method, bio_read);
	BIO_meth_set_puts(bio_method, bio_puts);
	BIO_meth_set_gets(bio_method, bio_gets);
	BIO_meth_set_ctrl(bio_method, bio_ctrl);
	BIO_meth_set_create(bio_method, bio_create);
	BIO_meth_set_destroy(bio_method, bio_destroy);
	return 0;
}

SSLSocket::SSLSocket(SSLContext* c, ICommunicationEndpoint* slaveSocket, bool mustDeleteSlaveSocket){
	p = new SSLSocketPrivate(c, slaveSocket, mustDeleteSlaveSocket);
}
	
SSLSocket::~SSLSocket(){
	delete p;
}

int32_t SSLSocket::recv(char* buf, uint32_t bufSize){
	if(!p->ssl){return -1;}
	int res = SSL_read(p->ssl, buf, bufSize);
	if(res>=0){
		return res;
	}else{
		ERR_print_errors_fp(stderr);
		return -1;
	}
}

bool SSLSocket::send(const char* buf, uint32_t bufSize){
	if(!p->ssl){return false;}
	if(bufSize>0){
		int res = SSL_write(p->ssl, buf, bufSize);
		if(res<=0){ERR_print_errors_fp(stderr);}
		return res>0;
		//TODO handle SSL_ERROR_WANT_READ and SSL_ERROR_WANT_WRITE
	}
	return true;
}

static bool printRecentError(){
	long errorCode = ERR_get_error();
	bool noError = errorCode==0;
	if(!noError){
		std::cerr << "ErrorCode: " << errorCode << "\n" << ERR_error_string(errorCode, NULL) << std::endl;
	}
	return noError;
}

static void printLastSSLError(SSL* ssl, int retvalue, const char* prefix = NULL){
	printRecentError();
	if(prefix){std::cerr << prefix << ": ";}
	int error = SSL_get_error(ssl, retvalue);
	if(error==SSL_ERROR_NONE){
		std::cerr << "SSL_ERROR_NONE" << std::endl;
	}else if(error==SSL_ERROR_ZERO_RETURN){
		std::cerr << "SSL_ERROR_ZERO_RETURN" << std::endl;
	}else if(error==SSL_ERROR_WANT_READ){
		std::cerr << "SSL_ERROR_WANT_READ" << std::endl;
	}else if(error==SSL_ERROR_WANT_WRITE){
		std::cerr << "SSL_ERROR_WANT_WRITE" << std::endl;
	}else if(error==SSL_ERROR_WANT_CONNECT){
		std::cerr << "SSL_ERROR_WANT_CONNECT" << std::endl;
	}else if(error==SSL_ERROR_WANT_ACCEPT){
		std::cerr << "SSL_ERROR_WANT_ACCEPT" << std::endl;
	}else if(error==SSL_ERROR_WANT_X509_LOOKUP){
		std::cerr << "SSL_ERROR_WANT_X509_LOOKUP" << std::endl;
	}else if(error==SSL_ERROR_WANT_ASYNC){
		std::cerr << "SSL_ERROR_WANT_ASYNC" << std::endl;
	}else if(error==SSL_ERROR_WANT_ASYNC_JOB){
		std::cerr << "SSL_ERROR_WANT_ASYNC_JOB" << std::endl;
	}else if(error==SSL_ERROR_WANT_CLIENT_HELLO_CB){
		std::cerr << "SSL_ERROR_WANT_CLIENT_HELLO_CB" << std::endl;
	}else if(error==SSL_ERROR_SYSCALL){
		std::cerr << "SSL_ERROR_SYSCALL" << std::endl;
	}else if(error==SSL_ERROR_SSL){
		std::cerr << "SSL_ERROR_SSL" << std::endl;
	}else{
		std::cerr << "Unknown error: " << error << std::endl;
	}
}

bool SSLSocket::accept(){
	p->pseudoBlocking = true;
	int res = SSL_accept(p->ssl);
	p->pseudoBlocking = false;
	if(res<=0){printLastSSLError(p->ssl, res, "SSLSocket::accept");}
	return res>0;
}

bool SSLSocket::connect(){
	p->pseudoBlocking = true;
	int res = SSL_connect(p->ssl);
	p->pseudoBlocking = false;
	if(res<=0){printLastSSLError(p->ssl, res, "SSLSocket::connect");}
	return res>0;
}

bool SSLSocket::verifyPeerCertificate(){
	if(hasPeerCertificate()){
		return SSL_get_verify_result(p->ssl)==X509_V_OK;
	}
	return false;
}

bool SSLSocket::hasPeerCertificate(){
	X509* cert = SSL_get_peer_certificate(p->ssl);
	if(cert){
		X509_free(cert);
	}
	return cert!=NULL;
}

void X509Cert::init(const std::vector<char>& data){
	BIO* cbio = BIO_new_mem_buf((void*)&(data[0]), data.size());
	this->data = PEM_read_bio_X509(cbio, NULL, 0, NULL);
}

X509Cert::X509Cert(const std::vector<char>& data){
	init(data);
}

X509Cert::X509Cert(const std::string& path){
	std::vector<char> v;
	if(loadFile(v, path)){
		init(v);
	}else{
		data = NULL;
	}
}

X509Cert::~X509Cert(){
	if(data){
		X509_free((X509*)data);
	}
}

bool SSLSocket::isPeerCertificateEqual(const X509Cert& cert){
	if(cert.data==NULL){return false;}
	X509* peerCert = SSL_get_peer_certificate(p->ssl);
	bool success = peerCert!=NULL;
	if(success){
		success = X509_cmp(peerCert, (X509*)cert.data)==0;
		X509_free(peerCert);
	}
	return success;
}

#endif
