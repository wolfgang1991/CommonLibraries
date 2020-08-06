#ifndef ConcurrentBlobDetection_H_INCLUDED
#define ConcurrentBlobDetection_H_INCLUDED

#include <ForwardDeclarations.h>

#include <vector2d.h>

#include <vector>

class ConcurrentBlobDetectionPrivate;

//! Useful to avoid the low framerate caused by serial execution of rendering and blob detection.
class ConcurrentBlobDetection{
	friend class ConcurrentBlobDetectionPrivate;
	
	public:
	
	//! used for output
	struct Blob{
		irr::core::vector2d<irr::f32> position;
		irr::f32 size;//! relevant size of the blob, e.g. it's diameter
	};
	
	private:
	
	ConcurrentBlobDetectionPrivate* prv;
	
	public:
	
	ConcurrentBlobDetection();
	
	~ConcurrentBlobDetection();
	
	//! must be called to exchange the current blobs if new blobs are available
	//! returns true if the blob thread is ready for new stuff
	bool update();
	
	//! gets the current blobs (MUST NOT be used after a follwing call of update())
	const std::vector<Blob>& getCurrentBlobs() const;
	
	//! if the thread is not idle any other pending request will be deleted
	//! The image is provided to the thread via pointer and dropped after done, therefore the reference counter should be 1 here. The image must not be used simultaneously by anything else for thread safety.
	//TODO: other blob detection parameters
	void requestBlobDetection(irr::video::IImage* img);
	
	//! returns [0-1]: 0 dark blobs, 1: light blobs
	irr::f32 getIntensity() const;
	
	void setIntensity(irr::f32 intensity);
	
	void setFilterIntensityEnabled(bool enabled);
	
	bool isFilterIntensityEnabled() const;
	
	irr::f32 getMinCircularity() const;
	
	irr::f32 getMaxCircularity() const;
	
	bool isFilterCircularityEnabled() const;
	
	void setCircularity(irr::f32 minValue, irr::f32 maxValue);
	
	void setFilterCircularityEnabled(bool enabled);
	
	irr::f32 getMinInertia() const;
	
	irr::f32 getMaxInertia() const;
	
	bool isFilterInertiaEnabled() const;
	
	void setInertia(irr::f32 minValue, irr::f32 maxValue);
	
	void setFilterInertiaEnabled(bool enabled);
	
	irr::f32 getMinConvexity() const;
	
	irr::f32 getMaxConvexity() const;
	
	bool isFilterConvexityEnabled() const;
	
	void setConvexity(irr::f32 minValue, irr::f32 maxValue);
	
	void setFilterConvexityEnabled(bool enabled);
	
	//! if false: black on white
	void setWhiteOnBlack(bool whiteOnBlack);
	
	bool isWhiteOnBlack() const;
	
};

#endif
