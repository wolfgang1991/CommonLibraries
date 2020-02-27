#include <RectangleGradientDescent.h>
#include <timing.h>
#include <Threading.h>

#include <limits>
#include <list>
#include <iostream>

using namespace irr;
using namespace core;

template<typename T>
static inline bool isRectInside(const irr::core::rect<T>& inner, const irr::core::rect<T>& outer){
	return outer.isPointInside(inner.UpperLeftCorner) && outer.isPointInside(inner.LowerRightCorner);
}

//! a1, a2 overlapping with b1, b2, result is negative if no overlapping
//! precondition: a1<a2, b1<b2
template<typename T>
static inline T calcOverlapping(T a1, T a2, T b1, T b2){
	return b1>a1?(a2-b1):(b2-a1);
}

//! A quadtree implementation for rectangles to quickly find the overlapping area
class RectangleQuadTree{

	private:
	
	typedef RectangleGradientDescent::RectanglePair RectanglePair;
	typedef RectangleGradientDescent::Rectangle Rectangle;
	
	irr::core::rect<double> quadRect;
	
	std::list<rect<double>*> rects;//Rectangles in this list are the ones which are too big for the subQuads, except if there is only one rectangle in the tree (starting from this).
	
	enum SUBQUAD{UPPER_LEFT, LOWER_LEFT, LOWER_RIGHT, UPPER_RIGHT, SUBQUAD_COUNT};
	
	RectangleQuadTree* subQuads[SUBQUAD_COUNT];
	
	//! O(logn) in case of evenly distributed rectangles
	double calcOverlappingArea(rect<double>* r, const rect<double>& newRectangle){
		double area = 0.0;
		for(auto it = rects.begin(); it != rects.end(); ++it){
			rect<double>* other = *it;
			if(other != r){
				double overlapX = calcOverlapping(newRectangle.UpperLeftCorner.X, newRectangle.LowerRightCorner.X, other->UpperLeftCorner.X, other->LowerRightCorner.X);//second.
				double overlapY = calcOverlapping(newRectangle.UpperLeftCorner.Y, newRectangle.LowerRightCorner.Y, other->UpperLeftCorner.Y, other->LowerRightCorner.Y);//second.
				if(overlapX>0.0 && overlapY>0.0){
					area += (overlapX*overlapY);
				}
			}
		}
		if(subQuads[0]!=NULL){
			for(int i=0; i<SUBQUAD_COUNT; i++){
				if(newRectangle.isRectCollided(subQuads[i]->quadRect)){
					area += subQuads[i]->calcOverlappingArea(r, newRectangle);
				}
			}
		}
		return area;
	}

	public:
	
	RectangleQuadTree(irr::core::rect<double> quadRect):quadRect(quadRect){
		for(int i=0; i<(int)SUBQUAD_COUNT; i++){subQuads[i] = NULL;}
	}
	
	~RectangleQuadTree(){
		for(int i=0; i<(int)SUBQUAD_COUNT; i++){
			if(subQuads[i]!=NULL){delete subQuads[i];}
		}
	}
	
	const irr::core::rect<double>& getQuadRect() const{
		return quadRect;
	}
	
	//! O(logn) in case of evenly distributed rectangles
	void insert(rect<double>* r){
		if(subQuads[0]==NULL){
			if(rects.empty()){
				rects.push_back(r);//allow one rect without creating subQuads to avoid crazy trees in case of small rects in a huge overall space
			}else{
				vector2d<double> center = quadRect.getCenter();
				subQuads[UPPER_LEFT] = new RectangleQuadTree(rect<double>(quadRect.UpperLeftCorner, center));
				subQuads[LOWER_LEFT] = new RectangleQuadTree(rect<double>(quadRect.UpperLeftCorner.X, center.Y, center.X, quadRect.LowerRightCorner.Y));
				subQuads[LOWER_RIGHT] = new RectangleQuadTree(rect<double>(center, quadRect.LowerRightCorner));
				subQuads[UPPER_RIGHT] = new RectangleQuadTree(rect<double>(center.X, quadRect.UpperLeftCorner.Y, quadRect.LowerRightCorner.X, center.Y));
				rect<double>* prev = rects.back();
				rects.clear();
				insert(prev);
				insert(r);
			}
		}else{
			for(int i=0; i<SUBQUAD_COUNT; i++){
				if(isRectInside(*r, subQuads[i]->quadRect)){//r->second
					subQuads[i]->insert(r);
					return;
				}
			}
			rects.push_back(r);
		}
	}
	
	//! O(logn) in case of evenly distributed rectangles
	double calcOverlappingArea(RectanglePair* r, double deltaAngle){
		Rectangle rCopy(r->first);
		rCopy.angle += deltaAngle;
		rect<double> newRectangle = rCopy.convertToRect();
		return calcOverlappingArea(&(r->second), newRectangle);//r
	}

};

irr::core::rect<double> RectangleGradientDescent::Rectangle::convertToRect() const{
	double r = 0.5*sqrt(width*width+height*height)+0.5*sqrt(rectWidth*rectWidth+rectHeight*rectHeight);//distanceFromPivot
	vector2d<double> center = pivot+r*vector2d<double>(sin(angle),-cos(angle));//clockwise rotation 0°==top (like geographic heading) x to the right, y to the bottom
	vector2d<double> offset(0.5*width, 0.5*height);
	rect<double> res(center-offset, center+offset);
	//Move rectangles if far away of the rectangle around the pivot
	rect<double> pivotRect(pivot.X-0.5*rectWidth, pivot.Y-0.5*rectHeight, pivot.X+0.5*rectWidth, pivot.Y+0.5*rectHeight);
	if(res.LowerRightCorner.Y<pivotRect.UpperLeftCorner.Y){
		double delta = pivotRect.UpperLeftCorner.Y-res.LowerRightCorner.Y;
		res.LowerRightCorner.Y += delta; res.UpperLeftCorner.Y += delta;
	}else if(res.UpperLeftCorner.Y>pivotRect.LowerRightCorner.Y){
		double delta = res.UpperLeftCorner.Y-pivotRect.LowerRightCorner.Y;
		res.LowerRightCorner.Y -= delta; res.UpperLeftCorner.Y -= delta;
	}
	if(res.LowerRightCorner.X<pivotRect.UpperLeftCorner.X){
		double delta = pivotRect.UpperLeftCorner.X-res.LowerRightCorner.X;
		res.LowerRightCorner.X += delta; res.UpperLeftCorner.X += delta;
	}else if(res.UpperLeftCorner.X>pivotRect.LowerRightCorner.X){
		double delta = res.UpperLeftCorner.X-pivotRect.LowerRightCorner.X;
		res.LowerRightCorner.X -= delta; res.UpperLeftCorner.X -= delta;
	}
	//Limit rectangles with a screen like limit
	if(limit){
		if(res.UpperLeftCorner.X<limit->UpperLeftCorner.X){
			double delta = limit->UpperLeftCorner.X - res.UpperLeftCorner.X;
			res.UpperLeftCorner.X += delta;
			res.LowerRightCorner.X += delta;
		}else if(res.LowerRightCorner.X>limit->LowerRightCorner.X){
			double delta = res.LowerRightCorner.X-limit->LowerRightCorner.X;
			res.UpperLeftCorner.X -= delta;
			res.LowerRightCorner.X -= delta;
		}
		if(res.UpperLeftCorner.Y<limit->UpperLeftCorner.Y){
			double delta = limit->UpperLeftCorner.Y - res.UpperLeftCorner.Y;
			res.UpperLeftCorner.Y += delta;
			res.LowerRightCorner.Y += delta;
		}else if(res.LowerRightCorner.Y>limit->LowerRightCorner.Y){
			double delta = res.LowerRightCorner.Y-limit->LowerRightCorner.Y;
			res.UpperLeftCorner.Y -= delta;
			res.LowerRightCorner.Y -= delta;
		}
	}
	return res;
}

RectangleGradientDescent::RectangleGradientDescent(irr::core::rect<double> availableSpace):availableSpace(availableSpace){}

RectangleGradientDescent::~RectangleGradientDescent(){
	clear();
}

RectangleGradientDescent::RectanglePair* RectangleGradientDescent::addRectangle(const Rectangle& r){
	RectanglePair* res = new std::pair<Rectangle,irr::core::rect<double>>(r, r.convertToRect());
	rectangles.push_back(res);
	return res;
}

const irr::core::rect<double>& RectangleGradientDescent::getSpaceLimit(){
	return availableSpace;
}

void RectangleGradientDescent::clear(){
	for(uint32_t i=0; i<rectangles.size(); i++){
		delete rectangles[i];
	}
	rectangles.clear();
}

bool RectangleGradientDescent::optimize(uint32_t maxStepCount, double deltaAngle, double gradientEps, bool avoidPivotRectangles){
	double halfDeltaAngle = 0.5*deltaAngle;
	for(uint32_t i=0; i<maxStepCount; i++){
		//Insert into Quadtree:
		RectangleQuadTree* tree = new RectangleQuadTree(availableSpace);
		for(uint32_t j=0; j<rectangles.size(); j++){//O(nlogn) (evenly distributed rectangles)
			tree->insert(&(rectangles[j]->second));
		}
		//Calculate fixed rectangles (pivot rectangles) and insert them into the tree
		std::list<rect<double>> pivotRectangles;//pointers must still live outside the if
		if(avoidPivotRectangles){
			for(uint32_t j=0; j<rectangles.size(); j++){
				Rectangle& r = rectangles[j]->first;
				pivotRectangles.push_back(rect<double>(r.pivot.X-0.5*r.rectWidth, r.pivot.Y-0.5*r.rectHeight, r.pivot.X+0.5*r.rectWidth, r.pivot.Y+0.5*r.rectHeight));
				tree->insert(&(pivotRectangles.back()));
			}
		}
		//Calculate gradient:
		std::vector<double> gradient(rectangles.size(), 0.0);
		bool allSmallerEps = true;
		for(uint32_t j=0; j<gradient.size(); j++){//O(nlogn) (evenly distributed rectangles)
			RectanglePair* r = rectangles[j];
			gradient[j] = (tree->calcOverlappingArea(r, halfDeltaAngle)-tree->calcOverlappingArea(r, -halfDeltaAngle))/deltaAngle;//O(logn) (evenly distributed rectangles)
			allSmallerEps = allSmallerEps && fabs(gradient[j])<gradientEps;
		}
		//Follow (negative) gradient:
		if(allSmallerEps){
			delete tree;
			return true;
		}
		for(uint32_t j=0; j<gradient.size(); j++){//O(n) (evenly distributed rectangles)
			if(fabs(gradient[j])>=gradientEps){
				RectanglePair* r = rectangles[j];
				r->first.angle -= gradient[j]>0?halfDeltaAngle:-halfDeltaAngle;
				r->second = r->first.convertToRect();
			}
		}
		delete tree;
	}
	return false;
}

const RectangleGradientDescent::RectanglePair* RectangleGradientDescent::getRectangle(uint32_t index) const{
	return rectangles[index];
}
	
uint32_t RectangleGradientDescent::getRectangleCount() const{
	return rectangles.size();
}

struct CrgdStartParams{
	ConcurrentRectangleGradientDescent* crgd;
	uint32_t maxStepCount;
	double deltaAngle;
	double gradientEps;
	bool avoidPivotRectangles;
	double timeout;
};

void* ConcurrentRectangleGradientDescent::threadMain(void* data){
	CrgdStartParams* d = (CrgdStartParams*)data;
	ConcurrentRectangleGradientDescent* crgd = d->crgd;
	const uint32_t stepsPerIteration = 1;//use a small value like 1 to reduce the abort time
	uint32_t maxIterations = (d->maxStepCount/stepsPerIteration)+(d->maxStepCount%stepsPerIteration>0?1:0);
	bool running = true;
	double startT = getSecs();
	for(uint32_t i=0; i<maxIterations && running; i++){
		if(crgd->descent.optimize(stepsPerIteration, d->deltaAngle, d->gradientEps, d->avoidPivotRectangles)){
			break;
		}
		pthread_mutex_lock(&crgd->mutexStuff);
		running = !crgd->mustExit && (getSecs()-startT)<d->timeout;
		pthread_mutex_unlock(&crgd->mutexStuff);
	}
	std::cout << "CRGD finished" << std::endl;
	pthread_mutex_lock(&crgd->mutexStuff);
	crgd->running = false;
	pthread_mutex_unlock(&crgd->mutexStuff);
	delete d;
	return NULL;
}

static ThreadPool pool(1);

ConcurrentRectangleGradientDescent::ConcurrentRectangleGradientDescent(irr::core::rect<double> availableSpace):
	descent(availableSpace),
	running(false),
	mustExit(false),
	lastKnownRunning(false){
	pthread_mutex_init(&mutexStuff, NULL);
}

ConcurrentRectangleGradientDescent::~ConcurrentRectangleGradientDescent(){
	abort();
	pthread_mutex_destroy(&mutexStuff);
}

RectangleGradientDescent* ConcurrentRectangleGradientDescent::getRectangleGradientDescent(){
	return &descent;
}

bool ConcurrentRectangleGradientDescent::start(uint32_t maxStepCount, double deltaAngle, double gradientEps, bool avoidPivotRectangles, double timeout){
	if(isRunning()){return false;}
	mustExit = false;
	running = lastKnownRunning = true;
	CrgdStartParams* data = new CrgdStartParams{this, maxStepCount, deltaAngle, gradientEps, avoidPivotRectangles, timeout};
	PooledThread* pt = pool.startThreadedFunction(ConcurrentRectangleGradientDescent::threadMain, (void*)data);
	if(pt==NULL){
		running = lastKnownRunning = false;
		delete data;
		return false;
	}
	return true;
}

void ConcurrentRectangleGradientDescent::abort(){
	if(isRunning()){
		pthread_mutex_lock(&mutexStuff);
		mustExit = true;
		pthread_mutex_unlock(&mutexStuff);
		while(isRunning()){delay(1);}
	}
}

bool ConcurrentRectangleGradientDescent::isRunning(){
	if(lastKnownRunning){
		pthread_mutex_lock(&mutexStuff);
		lastKnownRunning = running;
		pthread_mutex_unlock(&mutexStuff);
	}
	return lastKnownRunning;
}
