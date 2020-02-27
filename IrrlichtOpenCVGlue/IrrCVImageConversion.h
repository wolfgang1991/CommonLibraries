#ifndef IMAGE_CONVERSION_H_INCLUDED
#define IMAGE_CONVERSION_H_INCLUDED

#include <opencv2/core/mat.hpp>
#include <IImage.h>
#include <ITexture.h>
#include <IVideoDriver.h>

cv::Mat convertImageToGrayscaleMat(irr::video::IImage* img);

cv::Mat convertTextureToMat(irr::video::IVideoDriver* driver, irr::video::ITexture* tex, cv::Mat(*conversionFunction)(irr::video::IImage*));

#endif
