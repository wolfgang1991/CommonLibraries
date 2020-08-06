#include "IrrCVImageConversion.h"

using namespace cv;
using namespace irr;
using namespace video;
using namespace core;

Mat convertImageToGrayscaleMat(IImage* img){
//	dimension2d<u32> size = img->getDimension();
//	Mat res(size.Height, size.Width, CV_8U);
//	for(u32 x=0; x<size.Width; x++){
//		for(u32 y=0; y<size.Height; y++){
//			SColor c = img->getPixel(x,y);
//			u32 gray = (30*c.getRed() + 59*c.getGreen() + 11*c.getBlue())/100;
//			res.at<uint8_t>(y,x) = gray;
//		}
//	}
//	return res;
	return convertImageToMat<uint8_t,CV_8U>(img, [](irr::video::SColor c){return (30*c.getRed() + 59*c.getGreen() + 11*c.getBlue())/100;});
}

cv::Mat convertInverseImageToGrayscaleMat(irr::video::IImage* img){
	return convertImageToMat<uint8_t,CV_8U>(img, [](irr::video::SColor c){return 255-(30*c.getRed() + 59*c.getGreen() + 11*c.getBlue())/100;});
}

Mat convertTextureToMat(IVideoDriver* driver, ITexture* tex, Mat(*conversionFunction)(IImage*)){
	IImage* img = driver->createImageFromData(tex->getColorFormat(), tex->getSize(), tex->lock(ETLM_READ_ONLY), true, false);
	Mat res = conversionFunction(img);
	img->drop();
	return res;
}
