#ifndef ConcurrentBlobDetection_H_INCLUDED
#define ConcurrentBlobDetection_H_INCLUDED

#include <Threading.h>
#include <ForwardDeclarations.h>

#include <vector2d.h>

#include <vector>

struct BlobDetectionParameters;

//! Useful to avoid the low framerate caused by serial execution of rendering and blob detection.
class ConcurrentBlobDetection{
	
	public:
	
	//! used for output
	struct Blob{
		irr::core::vector2d<irr::f32> position;
		irr::f32 size;//! relevant size of the blob, e.g. it's diameter
	};
	
	private:
	
	ThreadPool pool;
	
	//may only be changed by the blob thread or if it is idle:
	bool isNew;
	std::vector<Blob>* intermediateBlobs;
	
	BlobDetectionParameters* params;
	
	std::vector<Blob>* currentBlobs;
	
	static void* detectBlobs(void* params);
	
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
	
};

#endif
