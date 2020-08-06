#ifndef IMAGE_CONVERSION_H_INCLUDED
#define IMAGE_CONVERSION_H_INCLUDED

#include <opencv2/core/mat.hpp>
#include <IImage.h>
#include <ITexture.h>
#include <IVideoDriver.h>

#include <functional>
#include <cstdint>

//! converts an image be sequentially applying a conversion function for each pixel, examples see: convertImageToGrayscaleMat and convertInverseImageToGrayscaleMat
//! TPrimitiveType: a primitive type e.g. uint8_t
//! TCVType: OpenCV type id corresponding to TPrimitiveType e.g. CV_8U for uint8_t
template <typename TPrimitiveType, int TCVType>
cv::Mat convertImageToMat(irr::video::IImage* img, const std::function<TPrimitiveType(irr::video::SColor)>& convertPixel){
	irr::core::dimension2d<irr::u32> size = img->getDimension();
	cv::Mat res(size.Height, size.Width, TCVType);
	for(irr::u32 x=0; x<size.Width; x++){
		for(irr::u32 y=0; y<size.Height; y++){
			res.at<TPrimitiveType>(y,x) = convertPixel(img->getPixel(x,y));
		}
	}
	return res;
}

cv::Mat convertImageToGrayscaleMat(irr::video::IImage* img);

cv::Mat convertInverseImageToGrayscaleMat(irr::video::IImage* img);

cv::Mat convertTextureToMat(irr::video::IVideoDriver* driver, irr::video::ITexture* tex, cv::Mat(*conversionFunction)(irr::video::IImage*));

#endif
