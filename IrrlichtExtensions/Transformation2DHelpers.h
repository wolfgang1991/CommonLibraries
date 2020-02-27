#ifndef Transformation2DHelpers_H_INCLUDED
#define Transformation2DHelpers_H_INCLUDED

#include <matrix4.h>
#include <SViewFrustum.h>

namespace irr{
	class IrrlichtDevice;
}

//! Creates a CMatrix4 where the elements are given row-wise
template<typename T>
inline irr::core::CMatrix4<T> createReadableCMatrix4(	T e00, T e10, T e20, T e30,
																		T e01, T e11, T e21, T e31,
																		T e02, T e12, T e22, T e32,
																		T e03, T e13, T e23, T e33){
	return irr::core::CMatrix4<T>(e00, e01, e02, e03, e10, e11, e12, e13, e20, e21, e22, e23, e30, e31, e32, e33);
}

//! Calculates the sequence of: Translation Origin->0; Scale; Rotate; Translation 0->Origin for homogeneous coordinates (2D embedded in 3D with Z coordinate 1)
irr::core::matrix4 create2DWorldTransformation(const irr::core::vector2d<irr::f32>& translation = irr::core::vector2d<irr::f32>(0.f,0.f), const irr::core::vector2d<irr::f32>& scale = irr::core::vector2d<irr::f32>(1.f,1.f), irr::f32 angle = 0.f, const irr::core::vector2d<irr::f32>& origin = irr::core::vector2d<irr::f32>(0.f,0.f));

struct CameraScreenParameters{
	irr::core::vector3df farLeftUp;
	irr::core::vector3df leftToRight;
	irr::core::vector3df upToDown;
};

//! Creates the inverse camera projection transformation for a 2D via 3D.
irr::core::matrix4 create2DInverseProjection(const CameraScreenParameters& params, const irr::core::dimension2d<irr::u32>& viewPortSize, irr::f32 distance);

//! Returns an optimized version of create2DInverseProjection(...) * create2DWorldTransformation(...)
irr::core::matrix4 createOptimized2DTransform(const irr::core::vector2d<irr::f32>& translation, const irr::core::vector2d<irr::f32>& scale, irr::f32 angle, const irr::core::vector2d<irr::f32>& origin, const CameraScreenParameters& params, const irr::core::dimension2d<irr::u32>& viewPortSize, irr::f32 distance);

//! Calculates the camera parameters for 2d via 3d without having a Camera Scene Node
class Camera2DParameterCalculator{

	private:
	
	CameraScreenParameters params;
	irr::scene::SViewFrustum vf;
	irr::f32 aspectRatio, zNear, zFar, recalculateEps;
	
	void recalculate();
	
	public:
	
	Camera2DParameterCalculator(irr::f32 aspectRatio, irr::f32 zNear = 1.f, irr::f32 zFar = 1000.f, irr::f32 recalculateEps = 0.0001f);
	
	//! recalculates the camera parameters in case of significant change (see recalculateEps), returns true if aspect ratio has changed the "camera" transformations
	bool setAspectRatio(irr::f32 aspectRatio);
	
	const CameraScreenParameters& getCameraScreenParameters() const;
	
	const irr::core::matrix4& getProjectionMatrix() const;
	
	const irr::core::matrix4& getViewMatrix() const;
	
	irr::f32 getRecommendedInverseProjectionDistance() const;
	
	//! set 2D Transformations based on the current viewport
	//! This function should be called before any 2D rendering and after any viewport change
	//! If no world transformation is given it is calculated via create2DInverseProjection.
	//! forceSet forces the setting even if 2d transformations have been set before (useful after manual tampering with the transformations).
	void set2DTransforms(irr::IrrlichtDevice* device, const irr::core::matrix4* overrideWorldTransform = NULL, bool forceSet = false);
	
	//! restores the active cameras transformations and the world transform
	//! This function should be called after 2d rendering
	void reset2DTransforms(irr::IrrlichtDevice* device);
	
	//! MUST be called (on Android and on any OS which do not reinit (global) static variables on restart) before using a Drawer2D for the first time after app start
	static void initStaticVars();
	
};

#endif
