#include "Transformation2DHelpers.h"

#include <IrrlichtDevice.h>
#include <ICameraSceneNode.h>
#include <ISceneManager.h>

using namespace irr;
using namespace core;
using namespace video;
using namespace scene;

irr::core::matrix4 create2DWorldTransformation(const irr::core::vector2d<irr::f32>& translation, const irr::core::vector2d<irr::f32>& scale, irr::f32 angle, const irr::core::vector2d<irr::f32>& origin){
	irr::f32 cosAngle = cosf(angle);
	irr::f32 sinAngle = sinf(angle);
//	irr::core::matrix4 O = createReadableCMatrix4(	1.f,	0.f,	-origin.X,	0.f,
//																	0.f,	1.f,	-origin.Y,	0.f,
//																	0.f,	0.f,	1.f,			0.f,
//																	0.f,	0.f,	0.f,			1.f);
//	irr::core::matrix4 S = createReadableCMatrix4(	scale.X,			0.f,	0.f,	0.f,
//																	0.f,	scale.Y,	0.f,	0.f,
//																	0.f,	0.f,		1.f,	0.f,
//																	0.f,	0.f,		0.f,	1.f);
//	irr::core::matrix4 D = createReadableCMatrix4(	cosAngle,	sinAngle,	0.f,	0.f,
//																	-sinAngle,	cosAngle,	0.f,	0.f,
//																	0.f,			0.f,			1.f,	0.f,
//																	0.f,			0.f,			0.f,	1.f);
//	irr::core::matrix4 I = createReadableCMatrix4(	1.f,	0.f,	translation.X,	0.f,
//																	0.f,	1.f,	translation.Y,	0.f,
//																	0.f,	0.f,	1.f,				0.f,
//																	0.f,	0.f,	0.f,				1.f);
//	return I*D*S*O;
	const irr::f32 sx = scale.X;
	const irr::f32 sy = scale.Y;
	const irr::f32 ox = origin.X;
	const irr::f32 oy = origin.Y;
	const irr::f32 tx = translation.X;
	const irr::f32 ty = translation.Y;
	return createReadableCMatrix4(	sx*cosAngle,	sy*sinAngle,	-oy*sy*sinAngle-ox*sx*cosAngle+tx,	0.f,
												-sx*sinAngle,	sy*cosAngle,	ox*sx*sinAngle-oy*sy*cosAngle+ty,	0.f,
												0.f,				0.f,				1.f,											0.f,
												0.f,				0.f,				0.f,											1.f);
}

irr::core::matrix4 create2DInverseProjection(const CameraScreenParameters& params, const irr::core::dimension2d<irr::u32>& viewPortSize, irr::f32 distance){
	irr::f32 k = distance/params.farLeftUp.Z;
	return createReadableCMatrix4(	k*params.leftToRight.X/viewPortSize.Width,	0.f,													k*params.farLeftUp.X,	0.f,
												0.f,														k*params.upToDown.Y/viewPortSize.Height,	k*params.farLeftUp.Y,	0.f,
												0.f,														0.f,													k*params.farLeftUp.Z,	0.f,
												0.f,														0.f,													0.f,							1.f);
}

irr::core::matrix4 createOptimized2DTransform(const irr::core::vector2d<irr::f32>& translation, const irr::core::vector2d<irr::f32>& scale, irr::f32 angle, const irr::core::vector2d<irr::f32>& origin, const CameraScreenParameters& params, const irr::core::dimension2d<irr::u32>& viewPortSize, irr::f32 distance){
	const irr::f32 k = distance/params.farLeftUp.Z;
	const irr::f32 rx = (k*params.leftToRight.X)/viewPortSize.Width;
	const irr::f32 ry = (k*params.upToDown.Y)/viewPortSize.Height;
	const irr::f32 fx = k*params.farLeftUp.X;
	const irr::f32 fy = k*params.farLeftUp.Y;
	const irr::f32 fz = distance;//k*params.farLeftUp.Z;
	const irr::f32 cosAngle = cosf(angle);
	const irr::f32 sinAngle = sinf(angle);
	const irr::f32 sx = scale.X;
	const irr::f32 sy = scale.Y;
	const irr::f32 ox = origin.X;
	const irr::f32 oy = origin.Y;
	const irr::f32 tx = translation.X;
	const irr::f32 ty = translation.Y;
	return createReadableCMatrix4(	rx*sx*cosAngle,	rx*sy*sinAngle,	rx*(-oy*sy*sinAngle-ox*sx*cosAngle+tx)+fx,	0.f,
												-ry*sx*sinAngle,	ry*sy*cosAngle,	ry*(ox*sx*sinAngle-oy*sy*cosAngle+ty)+fy,		0.f,
												0.f,					0.f,					fz,														0.f,
												0.f,					0.f,					0.f,														1.f);
}

Camera2DParameterCalculator::Camera2DParameterCalculator(irr::f32 aspectRatio, irr::f32 zNear, irr::f32 zFar, irr::f32 recalculateEps):
	aspectRatio(aspectRatio),
	zNear(zNear),
	zFar(zFar),
	recalculateEps(recalculateEps){
	vf.setFarNearDistance(zFar-zNear);
	recalculate();
}

void Camera2DParameterCalculator::recalculate(){
	vf.getTransform(ETS_PROJECTION).buildProjectionMatrixPerspectiveFovLH(core::PI/2.5f, aspectRatio, zNear, zFar);
	vf.setFrom(vf.getTransform(ETS_PROJECTION));//usually projection*view but since view is the unit matrix here just projection
	params.farLeftUp = vf.getFarLeftUp();
	params.leftToRight = vf.getFarRightUp() - params.farLeftUp;
	params.upToDown = vf.getFarLeftDown() - params.farLeftUp;
}

bool Camera2DParameterCalculator::setAspectRatio(irr::f32 aspectRatio){
	if(aspectRatio>0.f){
		f32 delta = fabsf(aspectRatio-this->aspectRatio);
		this->aspectRatio = aspectRatio;
		if(delta>recalculateEps){
			recalculate();
			return true;
		}
	}
	return false;
}
	
const CameraScreenParameters& Camera2DParameterCalculator::getCameraScreenParameters() const{
	return params;
}

const irr::core::matrix4& Camera2DParameterCalculator::getProjectionMatrix() const{
	return vf.getTransform(ETS_PROJECTION);
}

const irr::core::matrix4& Camera2DParameterCalculator::getViewMatrix() const{
	return vf.getTransform(ETS_VIEW);//view is the unit matrix
}

irr::f32 Camera2DParameterCalculator::getRecommendedInverseProjectionDistance() const{
	return zNear+1.f;
}

//! global, because there could be multiple Camera2DParameterCalculators
static bool are2DTransformsSet = false;

void Camera2DParameterCalculator::initStaticVars(){
	are2DTransformsSet = false;
}

void Camera2DParameterCalculator::set2DTransforms(irr::IrrlichtDevice* device, const irr::core::matrix4* overrideWorldTransform, bool forceSet){
	IVideoDriver* driver = device->getVideoDriver();
	irr::core::rect<s32> vport = driver->getViewPort();
	dimension2d<u32> vdim(vport.getWidth(), vport.getHeight());
	bool transformChanged = setAspectRatio((f32)vdim.Width/(f32)vdim.Height);
	if(transformChanged || !are2DTransformsSet || forceSet){
		are2DTransformsSet = true;
		driver->setTransform(ETS_VIEW, getViewMatrix());
		driver->setTransform(ETS_PROJECTION, getProjectionMatrix());
		if(overrideWorldTransform){
			driver->setTransform(ETS_WORLD, *overrideWorldTransform);
		}else{
			driver->setTransform(ETS_WORLD, create2DInverseProjection(getCameraScreenParameters(), vdim, getRecommendedInverseProjectionDistance()));
		}
	}
}

void Camera2DParameterCalculator::reset2DTransforms(irr::IrrlichtDevice* device){
	are2DTransformsSet = false;
	ICameraSceneNode* cam = device->getSceneManager()->getActiveCamera();
	IVideoDriver* driver = device->getVideoDriver();
	if(cam){
		driver->setTransform(ETS_VIEW, cam->getViewMatrix());
		driver->setTransform(ETS_PROJECTION, cam->getProjectionMatrix());
	}
	driver->setTransform(ETS_WORLD, matrix4());
}
