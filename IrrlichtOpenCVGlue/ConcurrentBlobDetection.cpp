#include "ConcurrentBlobDetection.h"
#include "IrrCVImageConversion.h"

#include <timing.h>

#include <opencv2/features2d.hpp>

using namespace cv;
using namespace irr;
using namespace core;
using namespace video;

//! input
struct BlobDetectionParameters{
	ConcurrentBlobDetection* detection;
	irr::video::IImage* img;
	//TODO other parameters
};

ConcurrentBlobDetection::ConcurrentBlobDetection():pool(1),isNew(false),intermediateBlobs(new std::vector<Blob>()),params(NULL),currentBlobs(new std::vector<Blob>()){}
	
bool ConcurrentBlobDetection::update(){
	bool running = pool.hasRunningThreads();
	if(!running){
		if(isNew){
			auto tmp = intermediateBlobs;
			intermediateBlobs = currentBlobs;
			currentBlobs = tmp;
			isNew = false;
		}
		if(params!=NULL){
			running = pool.startThreadedFunction(detectBlobs, params)!=NULL;
			params = NULL;
		}
	}
	return !running;
}
	
const std::vector<ConcurrentBlobDetection::Blob>& ConcurrentBlobDetection::getCurrentBlobs() const{
	return *currentBlobs;
}
	
void ConcurrentBlobDetection::requestBlobDetection(irr::video::IImage* img){
	assert(img->getReferenceCount()==1);
	if(this->params){this->params->img->drop();}
	delete this->params;//delete old request
	params = new BlobDetectionParameters{this, img};
}

void* ConcurrentBlobDetection::detectBlobs(void* params){
	BlobDetectionParameters* p = (BlobDetectionParameters*)params;
	std::vector<Blob>& blobs = *(p->detection->intermediateBlobs);
	blobs.clear();
	Mat blobMat = convertImageToGrayscaleMat(p->img);//TODO inverse if white on black setting
	std::vector<KeyPoint> keypoints;// Detect blobs.
	cv::Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(); 
	detector->detect(blobMat, keypoints);
	blobs.reserve(keypoints.size());
	//std::cout << "found blobs: " << keypoints.size() << std::endl;
	for(uint32_t i=0; i<keypoints.size(); i++){
		KeyPoint& p = keypoints[i];
		blobs.push_back(Blob{vector2d<f32>(p.pt.x, p.pt.y), p.size});
		//std::cout << "blob " << i << ": " << p.pt.x << ", " << p.pt.y << " angle: " << p.angle << " response: " << p.response << " size: " << p.size << std::endl;
	}
	p->detection->isNew = true;
	p->img->drop();
	delete p;
	return NULL;
}

ConcurrentBlobDetection::~ConcurrentBlobDetection(){
	while(pool.hasRunningThreads()){//wait until all is done
		delay(10);
	}
	delete intermediateBlobs;
	delete currentBlobs;
	delete params;
}
