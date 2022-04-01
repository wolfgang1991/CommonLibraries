//TODO https://cpp.hotexamples.com/de/site/file?hash=0x8f38744526ac8b15e9cc9efc074275226a30c98531eb75d9da55ea2d7eece355&fullName=libgit2-master/openssl_stream.c&project=YueLinHo/libgit2
#ifndef NO_OPENSSL

#include "SSLSocket.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509_vfy.h>

#include <fstream>
#include <cassert>
#include <cstring>
#include <iostream>

//CertStore::CertStore(){
//	store = X509_STORE_new();
//}
//	
//CertStore::~CertStore(){
//	X509_STORE_free((X509_STORE*)store);
//}

//bool CertStore::loadX509PEMCert(const std::function<bool(char*&, uint32_t&)>& loadFunction){
//	char* buf = NULL; uint32_t len;
//	bool success = loadFunction(buf, len);
//	if(success){
//		success = loadX509PEMCert(buf, len);
//	}
//	delete[] buf;
//	return success;
//}

//bool CertStore::loadX509PEMCert(const std::string& path){
//	return loadX509PEMCert([path](char*& bufOut, uint32_t& lenOut){
//		std::ifstream is(path.c_str(), std::ifstream::binary);
//		bool good = is.good();
//		if(good){
//			is.seekg(0, is.end);
//			lenOut = is.tellg();
//			is.seekg(0, is.beg);
//			bufOut = new char[lenOut];
//			is.read(bufOut, lenOut);
//		}
//		return good;
//	});
//}

//bool CertStore::loadX509PEMCert(char* buf, uint32_t bufSize){
//	BIO* cbio = BIO_new_mem_buf((void*)buf, (int)bufSize);
//	X509_STORE * cts = (X509_STORE*)store;
//	if(!cts || !cbio){return false;}
//	STACK_OF(X509_INFO)* inf = PEM_X509_INFO_read_bio(cbio, NULL, NULL, NULL);
//	if(!inf){
//		BIO_free(cbio);//cleanup
//		return false;
//	}
//	//iterate over all entries from the pem file, add them to the x509_store one by one
//	int count = 0;
//	std::cout << "sk_X509_INFO_num(inf): " << sk_X509_INFO_num(inf) << std::endl;
//	for(int i=0; i<sk_X509_INFO_num(inf); i++) {
//		X509_INFO* itmp = sk_X509_INFO_value(inf, i);
//		if(itmp->x509){
//		      X509_STORE_add_cert(cts, itmp->x509);
//		      count++;
//		}
//		if(itmp->crl){
//		      X509_STORE_add_crl(cts, itmp->crl);
//		      count++;
//		}
//	}
//	sk_X509_INFO_pop_free(inf, X509_INFO_free); //cleanup
//	BIO_free(cbio);//cleanup
//	std::cout << "Added " << count << " certs." << std::endl;
//	return true;
//}

static BIO_METHOD* bio_method = NULL;
static int init_bio_method();

class SSLSocketPrivate{
	
	public:
	
	ISocket* slaveSocket;
	bool mustDeleteSlaveSocket;
	bool readBlocking;
	
	SSL_CTX* ctx;
//	CertStore* store;
	
	SSL* ssl;
	
	std::list<RSA*> privateKeys;
	std::list<X509*> certificates;
	
	SSLSocketPrivate(ISocket* slaveSocket, SSLSocket::Mode mode, bool mustDeleteSlaveSocket):slaveSocket(slaveSocket),mustDeleteSlaveSocket(mustDeleteSlaveSocket),readBlocking(false){
		ctx = SSL_CTX_new(mode==SSLSocket::SERVER?TLS_server_method():TLS_client_method());
		assert(ctx!=NULL);
		//store = NULL;
		ssl = NULL;
	}
	
	void initSSLFromCTX(){
		ssl = SSL_new(ctx);
		if(bio_method==NULL){init_bio_method();}
		BIO* bio = BIO_new(bio_method);
		BIO_set_data(bio, this);
		SSL_set_bio(ssl, bio, bio);//bio is reference counted
	}
	
	~SSLSocketPrivate(){
		if(ssl){
			int res = SSL_shutdown(ssl);
			if(res<=0){ERR_print_errors_fp(stderr);}
			SSL_free(ssl);
		}
		SSL_CTX_free(ctx);
		for(RSA* rsa : privateKeys){
			RSA_free(rsa);
		}
		for(X509* cert : certificates){
			X509_free(cert);
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
	return (int)(p->slaveSocket->recv(buf, len, p->readBlocking));
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

SSLSocket::SSLSocket(ISocket* slaveSocket, Mode mode, bool mustDeleteSlaveSocket){//, CertStore* store
	p = new SSLSocketPrivate(slaveSocket, mode, mustDeleteSlaveSocket);
	//setCertStore(store);
	p->initSSLFromCTX();
}
	
SSLSocket::~SSLSocket(){
	delete p;
}

uint32_t SSLSocket::getAvailableBytes() const{
	return p->slaveSocket->getAvailableBytes();
}

uint32_t SSLSocket::recv(char* buf, uint32_t bufSize, bool readBlocking){
	p->readBlocking = readBlocking;
	int res = SSL_read(p->ssl, buf, bufSize);
	if(res>=0){
		return res;
	}else{
		ERR_print_errors_fp(stderr);
		return 0;
	}
}

bool SSLSocket::send(const char* buf, uint32_t bufSize){
	if(bufSize>0){
		int res = SSL_write(p->ssl, buf, bufSize);
		if(res<=0){ERR_print_errors_fp(stderr);}
		return res>0;
		//TODO handle SSL_ERROR_WANT_READ and SSL_ERROR_WANT_WRITE
	}
	return true;
}

//void SSLSocket::setCertStore(CertStore* store){
//	SSL_CTX_set1_cert_store(p->ctx, (X509_STORE*)(store->store));
//	p->store = store;
//}
//	
//CertStore* SSLSocket::getCertStore() const{
//	return p->store;
//}

bool SSLSocket::accept(){
	int res = SSL_accept(p->ssl);
	if(res<=0){std::cerr << "accept result: " << res << std::endl; ERR_print_errors_fp(stderr); std::cerr << std::flush;}//TODO geht nicht immer ??
	return res>0;
}

bool SSLSocket::connect(){
	int res = SSL_connect(p->ssl);
	if(res<=0){ERR_print_errors_fp(stderr);}
	return res>0;
}

bool SSLSocket::usePrivateKey(const std::vector<char>& data){
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
	
bool SSLSocket::useCertificate(const std::vector<char>& data){
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

bool SSLSocket::usePrivateKeyFromFile(const std::string& path){
	std::vector<char> v;
	if(loadFile(v, path)){
		return usePrivateKey(v);
	}
	return false;
}
	
bool SSLSocket::useCertificateFromFile(const std::string& path){
	std::vector<char> v;
	if(loadFile(v, path)){
		return useCertificate(v);
	}
	return false;
}

#endif
