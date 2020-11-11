#include "ConcurrentBlobDetection.h"
#include "IrrCVImageConversion.h"

#include <timing.h>
#include <Threading.h>
#include <mathUtils.h>
#include <Matrix.h>

#include <opencv2/features2d.hpp>

using namespace cv;
using namespace irr;
using namespace core;
using namespace video;

class ConcurrentBlobDetectionPrivate{
	
	public:
	
	//! input
	struct BlobDetectionParameters{
		ConcurrentBlobDetection* detection;
		irr::video::IImage* img;
		cv::SimpleBlobDetector::Params params;
		bool whiteOnBlack;//if false black on white
		Matrix<2,3,double> homogenousCamTransform;
	};
	
	ThreadPool pool;
	
	//may only be changed by the blob thread or if it is idle:
	bool isNew;
	std::vector<ConcurrentBlobDetection::Blob>* intermediateBlobs;
	
	BlobDetectionParameters* params;
	
	cv::SimpleBlobDetector::Params paramsToCopy;
	bool whiteOnBlackToCopy;
	double camRotationDegrees;
	irr::f32 minArea, maxArea;
	
	std::vector<ConcurrentBlobDetection::Blob>* currentBlobs;
	
	static void* detectBlobs(void* params);
	
	ConcurrentBlobDetectionPrivate():pool(1),isNew(false),intermediateBlobs(new std::vector<ConcurrentBlobDetection::Blob>()),params(NULL),paramsToCopy(),whiteOnBlackToCopy(false),camRotationDegrees(0.0),minArea(3.17891e-05),maxArea(0.0625),currentBlobs(new std::vector<ConcurrentBlobDetection::Blob>()){}
	
	~ConcurrentBlobDetectionPrivate(){
		while(pool.hasRunningThreads()){//wait until all is done
			delay(10);
		}
		delete intermediateBlobs;
		delete currentBlobs;
		delete params;
	}
	
};

ConcurrentBlobDetection::ConcurrentBlobDetection(){
	prv = new ConcurrentBlobDetectionPrivate();
}
	
bool ConcurrentBlobDetection::update(){
	bool running = prv->pool.hasRunningThreads();
	if(!running){
		if(prv->isNew){
			auto tmp = prv->intermediateBlobs;
			prv->intermediateBlobs = prv->currentBlobs;
			prv->currentBlobs = tmp;
			prv->isNew = false;
		}
		if(prv->params!=NULL){
			running = prv->pool.startThreadedFunction(prv->detectBlobs, prv->params)!=NULL;
			prv->params = NULL;
		}
	}
	return !running;
}
	
const std::vector<ConcurrentBlobDetection::Blob>& ConcurrentBlobDetection::getCurrentBlobs() const{
	return *prv->currentBlobs;
}
	
void ConcurrentBlobDetection::requestBlobDetection(irr::video::IImage* img){
	assert(img->getReferenceCount()==1);
	if(prv->params){prv->params->img->drop();}
	delete prv->params;//delete old request
	u32 totalArea = img->getDimension().getArea();
	prv->paramsToCopy.minArea = prv->minArea*totalArea;
	prv->paramsToCopy.maxArea = prv->maxArea*totalArea;
	double cosPhi = cos(-prv->camRotationDegrees/DEG_RAD);
	double sinPhi = sin(-prv->camRotationDegrees/DEG_RAD);
	double t_x = -((double)img->getDimension().Width/2.0);
	double t_y = -((double)img->getDimension().Height/2.0);
	Matrix<2,3,double> homoTransform = {//(1) translation center->origin, (2) rotation, (3) translation origin->center
		cosPhi, -sinPhi,  -sinPhi*t_y+cosPhi*t_x-t_x,
		sinPhi,	cosPhi,	cosPhi*t_y-t_y+sinPhi*t_x,
	};
	prv->params = new ConcurrentBlobDetectionPrivate::BlobDetectionParameters{this, img, prv->paramsToCopy, prv->whiteOnBlackToCopy, homoTransform};
}

void ConcurrentBlobDetection::setCameraRotationDegrees(double degrees){
	prv->camRotationDegrees = degrees;
}

double ConcurrentBlobDetection::getCameraRotationDegrees() const{
	return prv->camRotationDegrees;
}

irr::f32 ConcurrentBlobDetection::getMinAreaProportion() const{
	return prv->minArea;
}

irr::f32 ConcurrentBlobDetection::getMaxAreaProportion() const{
	return prv->maxArea;
}

void ConcurrentBlobDetection::setAreaProportion(irr::f32 minValue, irr::f32 maxValue){
	prv->minArea = minValue;
	prv->maxArea = maxValue;
}

bool ConcurrentBlobDetection::isFilterAreaEnabled() const{
	return prv->paramsToCopy.filterByArea;
}

void ConcurrentBlobDetection::setFilterAreaEnabled(bool enabled){
	prv->paramsToCopy.filterByArea = enabled;
}

void* ConcurrentBlobDetectionPrivate::detectBlobs(void* params){
	BlobDetectionParameters* p = (BlobDetectionParameters*)params;
	std::vector<ConcurrentBlobDetection::Blob>& blobs = *(p->detection->prv->intermediateBlobs);
	blobs.clear();
	Mat blobMat = p->whiteOnBlack?convertInverseImageToGrayscaleMat(p->img):convertImageToGrayscaleMat(p->img);
	std::vector<KeyPoint> keypoints;// Detect blobs.
	cv::Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(p->params); 
	detector->detect(blobMat, keypoints);
	blobs.reserve(keypoints.size());
	//std::cout << "found blobs: " << keypoints.size() << std::endl;
	for(uint32_t i=0; i<keypoints.size(); i++){
		KeyPoint& kp = keypoints[i];
		Vector2D<double> transformedBlob = p->homogenousCamTransform * Vector3D<double>(kp.pt.x, kp.pt.y, 1.0);
		blobs.push_back(ConcurrentBlobDetection::Blob{vector2d<f32>(transformedBlob[0], transformedBlob[1]), kp.size});
		//std::cout << "blob " << i << ": " << kp.pt.x << ", " << kp.pt.y << " angle: " << p.angle << " response: " << p.response << " size: " << p.size << std::endl;
	}
	p->detection->prv->isNew = true;
	p->img->drop();
	delete p;
	return NULL;
}

ConcurrentBlobDetection::~ConcurrentBlobDetection(){
	delete prv;
}

irr::f32 ConcurrentBlobDetection::getIntensity() const{
	return ((f32)prv->paramsToCopy.blobColor)/255.f;
}

void ConcurrentBlobDetection::setIntensity(irr::f32 intensity){
	prv->paramsToCopy.blobColor = Clamp(rd<f32,int>(intensity*255.f),0,255);
}

void ConcurrentBlobDetection::setFilterIntensityEnabled(bool enabled){
	prv->paramsToCopy.filterByColor = enabled;
}

bool ConcurrentBlobDetection::isFilterIntensityEnabled() const{
	return prv->paramsToCopy.filterByColor;
}

irr::f32 ConcurrentBlobDetection::getMinCircularity() const{
	return prv->paramsToCopy.minCircularity;
}

irr::f32 ConcurrentBlobDetection::getMaxCircularity() const{
	return prv->paramsToCopy.maxCircularity;
}

bool ConcurrentBlobDetection::isFilterCircularityEnabled() const{
	return prv->paramsToCopy.filterByCircularity;
}

void ConcurrentBlobDetection::setCircularity(irr::f32 minValue, irr::f32 maxValue){
	prv->paramsToCopy.minCircularity = minValue;
	prv->paramsToCopy.maxCircularity = maxValue;
}

void ConcurrentBlobDetection::setFilterCircularityEnabled(bool enabled){
	prv->paramsToCopy.filterByCircularity = enabled;
}

irr::f32 ConcurrentBlobDetection::getMinInertia() const{
	return prv->paramsToCopy.minInertiaRatio;
}

irr::f32 ConcurrentBlobDetection::getMaxInertia() const{
	return prv->paramsToCopy.maxInertiaRatio;
}

bool ConcurrentBlobDetection::isFilterInertiaEnabled() const{
	return prv->paramsToCopy.filterByInertia;
}

void ConcurrentBlobDetection::setInertia(irr::f32 minValue, irr::f32 maxValue){
	prv->paramsToCopy.minInertiaRatio = minValue;
	prv->paramsToCopy.maxInertiaRatio = maxValue;
}

void ConcurrentBlobDetection::setFilterInertiaEnabled(bool enabled){
	prv->paramsToCopy.filterByInertia = enabled;
}

irr::f32 ConcurrentBlobDetection::getMinConvexity() const{
	return prv->paramsToCopy.minConvexity;
}

irr::f32 ConcurrentBlobDetection::getMaxConvexity() const{
	return prv->paramsToCopy.maxConvexity;
}

bool ConcurrentBlobDetection::isFilterConvexityEnabled() const{
	return prv->paramsToCopy.filterByConvexity;
}

void ConcurrentBlobDetection::setConvexity(irr::f32 minValue, irr::f32 maxValue){
	prv->paramsToCopy.minConvexity = minValue;
	prv->paramsToCopy.maxConvexity = maxValue;
}

void ConcurrentBlobDetection::setFilterConvexityEnabled(bool enabled){
	prv->paramsToCopy.filterByConvexity = enabled;
}

void ConcurrentBlobDetection::setWhiteOnBlack(bool whiteOnBlack){
	prv->whiteOnBlackToCopy = whiteOnBlack;
}

bool ConcurrentBlobDetection::isWhiteOnBlack() const{
	return prv->whiteOnBlackToCopy;
}
