#ifndef RectangleGradientDescent_H_INCLUDED
#define RectangleGradientDescent_H_INCLUDED

#include <vector2d.h>
#include <rect.h>

#include <utility>
#include <vector>
#include <cstdint>
#include <pthread.h>

//! Representation of a distribution of rectangles (e.g. used as labels) with a minimization of overlapping rectangles via gradient descent
class RectangleGradientDescent{
	
	public:
	
	struct Rectangle{
		irr::core::vector2d<double> pivot;
		double width;
		double height;
		double angle;//! angle in rad (gets optimized by the gradient descent)
		double rectWidth;//! rectangle which defines the area around the pivot where the rectangle for gradient descent cannot be
		double rectHeight;
		//double distanceFromPivot;
		const irr::core::rect<double>* limit;//NULL if rectangles unlimited, ptr will not be deleted automatically
		
		irr::core::rect<double> convertToRect() const;
		
	};
	
	typedef std::pair<Rectangle, irr::core::rect<double>> RectanglePair;
	
	private:
	
	std::vector< RectanglePair* > rectangles;
	
	irr::core::rect<double> availableSpace;
	
	public:
	
	//! availableSpace defines the area on which the rects can be placed/moved.
	RectangleGradientDescent(irr::core::rect<double> availableSpace);
	
	~RectangleGradientDescent();
	
	//! O(logn) in case of evenly distributed rectangles
	RectanglePair* addRectangle(const Rectangle& r);
	
	const irr::core::rect<double>& getSpaceLimit();
	
	void clear();
	
	
	//! there are some cases where gradient descent does not halt if there's no maximum amount of steps (e.g. all rects overlap each other exactly)
	//! deltaAngle in rad, defines the step with and the delta for gradient calculation
	//! gradientEps defines a threshold value for the gradient which is considere "good enough" (unit: overlapping area / angle)
	//! O(maxStepCount*nlogn)
	//! returns true if finished (no significant gradient found)
	bool optimize(uint32_t maxStepCount, double deltaAngle, double gradientEps, bool avoidPivotRectangles = true);
	
	const RectanglePair* getRectangle(uint32_t index) const;
	
	uint32_t getRectangleCount() const;
	
};

class ConcurrentRectangleGradientDescent{

	private:
	
	pthread_mutex_t mutexStuff;
	RectangleGradientDescent descent;
	bool running;
	bool mustExit;

	bool lastKnownRunning;
	
	static void* threadMain(void* data);
	
	public:
	
	ConcurrentRectangleGradientDescent(irr::core::rect<double> availableSpace);
	
	~ConcurrentRectangleGradientDescent();
	
	//! do not use if thread running (check with isFinished)
	RectangleGradientDescent* getRectangleGradientDescent();
	
	//! timeout in s
	//! true if thread has been started, false if not startable or already running
	bool start(uint32_t maxStepCount, double deltaAngle, double gradientEps, bool avoidPivotRectangles = true, double timeout = 10.0);
	
	//! Exit the thread. In case you use this to fill new rectangles you might want to call RectangleGradientDescent::clear afterwards.
	void abort();
	
	//! true if thread still running, false if finished or aborted
	bool isRunning();

};

#endif
