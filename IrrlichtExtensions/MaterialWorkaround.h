#ifndef MATERIAL_WORKAROUND_H_INCLUDED
#define MATERIAL_WORKAROUND_H_INCLUDED

#include <IMaterialRenderer.h>

#define SET_MATERIAL_WORKAROUND(driver, mat)		IMaterialRenderer* mr = driver->getMaterialRenderer(mat.MaterialType);\
																E_DRIVER_TYPE driverType = driver->getDriverType(); assert(driverType!=EDT_DIRECT3D9 && driverType<=EDT_OGLES2);\
																if(mr){mr->OnSetMaterial(mat, mat, true, NULL);}//NULL does not work in Direct3D, just for Software, OpenGL, OpenGL ES drivers (WebGL not tested), see assert
											
#define UNSET_MATERIAL_WORKAROUND if(mr){mr->OnUnsetMaterial();}

#endif
