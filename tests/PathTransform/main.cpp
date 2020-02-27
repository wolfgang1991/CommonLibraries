#include <Duernion.h>
#include <Matrix.h>
#include <PathTransformCreation.h>
#include <StringHelpers.h>

#include <irrlicht.h>

#include <vector>
#include <list>
#include <iostream>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

#define DEG_RAD (180.0/3.141592654)
#define ANGLE_EPS 0.1//degrees
#define MATRIX_EPS 0.00001
#define PI 3.141592654

#define SOURCE_COLOR SColor(255,0,200,0)
#define MEAS_COLOR SColor(255,255,0,0)

#define LINE_DISTANCE 2
#define MIN_AREA_WITH_NEIGHBORS 10

template <typename A, typename B, typename C>
A Clamp(A value, B min, C max){
	return value<(A)min?(A)min:(value>(A)max?(A)max:value);
}

template <typename TPath, typename TFloat>
class DrawableTriangulatedPathTransform : public TriangulatedPathTransfrom<TPath, TFloat>{
	
	public:
	
	typedef TriangulatedPathTransfrom<TPath, TFloat> Super;
	typedef typename Super::Scalar Scalar;
	
	template <typename TContainer>
	DrawableTriangulatedPathTransform(const TContainer& matchedPoints, TFloat distanceEPS = (TFloat)0.0001):TriangulatedPathTransfrom<TPath, TFloat>(matchedPoints, LINE_DISTANCE, MIN_AREA_WITH_NEIGHBORS, distanceEPS){}
	
	void drawTriangle(IVideoDriver* driver, const Vector2D<Scalar>& a, const Vector2D<Scalar>& b, const Vector2D<Scalar>& c, SColor color){
		driver->draw2DLine(vector2d<s32>(a[0], a[1]), vector2d<s32>(b[0], b[1]), color);
		driver->draw2DLine(vector2d<s32>(c[0], c[1]), vector2d<s32>(b[0], b[1]), color);
		driver->draw2DLine(vector2d<s32>(a[0], a[1]), vector2d<s32>(c[0], c[1]), color);
	}
	
	void drawTriangulation(IVideoDriver* driver, SColor sourceColor, SColor measColor){
		uint32_t tcnt = Super::indices.size()/3;
		for(uint32_t i=0; i<tcnt; i++){
			drawTriangle(driver, Super::points[Super::indices[i*3]].sourcePoint, Super::points[Super::indices[i*3+1]].sourcePoint, Super::points[Super::indices[i*3+2]].sourcePoint, sourceColor);
			drawTriangle(driver, Super::points[Super::indices[i*3]].measuredPoint, Super::points[Super::indices[i*3+1]].measuredPoint, Super::points[Super::indices[i*3+2]].measuredPoint, measColor);
		}
	}
	
};

typedef Vector3D<int64_t> TestVector;
typedef std::vector<TestVector> TestPath;

void drawPath(IVideoDriver* driver, SColor color, const TestPath& path){
	if(path.empty()){return;}
	auto last = &path[0];
	for(uint32_t i=1; i<path.size(); i++){
		driver->draw2DLine(vector2d<s32>((*last)[0], (*last)[1]), vector2d<s32>(path[i][0], path[i][1]), color);
		last = &path[i];
	}
}

class EventReceiver : public IEventReceiver{
	
	public:
	
	IPathTransform<TestPath>::Type type;
	IPathTransform<TestPath>* transformation;
	
	std::list<MatchedPoint<int64_t>> matchPoints;
	
	Vector2D<int64_t> sourcePoint;
	bool sourcePointSet;
	
	const TestPath& sourcePath;
	TestPath transformedPath;
	
	EventReceiver(const TestPath& sourcePath):type(IPathTransform<TestPath>::TRANSLATION),transformation(NULL),sourcePointSet(false),sourcePath(sourcePath){}

	bool OnEvent(const SEvent& event){
		if(event.EventType==EET_MOUSE_INPUT_EVENT){
			const SEvent::SMouseInput& m = event.MouseInput;
			if(m.Event==EMIE_LMOUSE_LEFT_UP){
				if(sourcePointSet){
					matchPoints.push_back(MatchedPoint<int64_t>{sourcePoint, {m.X,m.Y}});
					delete transformation;
					transformation = createPathTransform<TestPath, double>(type, matchPoints, LINE_DISTANCE, MIN_AREA_WITH_NEIGHBORS);
					if(transformation){
						IPathTransform<TestPath>* transformation2 = createPathTransform<TestPath, double>(transformation->createStringRepresentation());//test transformation en/decoding
						assert(transformation->createStringRepresentation().compare(transformation2->createStringRepresentation())==0);
						std::cout << "t: " << transformation->createStringRepresentation() << "\nt2: " << transformation2->createStringRepresentation() << std::endl;
						transformedPath = transformation2->transform(sourcePath);
						delete transformation2;
					}
					sourcePointSet = false;
				}else{
					sourcePoint[0] = m.X;
					sourcePoint[1] = m.Y;
					sourcePointSet = true;
				}
			}else if(m.Event==EMIE_RMOUSE_LEFT_UP){
				matchPoints.clear();
				sourcePointSet = false;
				transformedPath.clear();
				delete transformation;
				transformation = NULL;
			}else if(m.Event==EMIE_MOUSE_WHEEL){
				type = (IPathTransform<TestPath>::Type)Clamp(type + (int)m.Wheel, 0, IPathTransform<TestPath>::TYPE_COUNT-1);
				delete transformation;
				transformation = createPathTransform<TestPath, double>(type, matchPoints, LINE_DISTANCE, MIN_AREA_WITH_NEIGHBORS);
				if(transformation){
					std::cout << "new transformation: " << transformation->getIdentifier() << std::endl;
					transformedPath = transformation->transform(sourcePath);
				}else{
					std::cout << "new transformation: NULL" << std::endl;
				}
			}
		}
		return false;
	}
	
	void drawPoint(IVideoDriver* driver, IGUIFont* font, const Vector2D<int64_t>& point, int index, SColor color){
		driver->draw2DPolygon(vector2d<s32>(point[0], point[1]), 10, color);
		font->draw(convertToWString(index).c_str(), rect<s32>(point[0]-5, point[1]-5, point[0]+5, point[1]+5), color, true, true);
	}
	
	void drawPoint(IVideoDriver* driver, IGUIFont* font, const TestVector& point, int index, SColor color){
		drawPoint(driver, font, Vector2D<int64_t>(point[0], point[1]), index, color);
	}
	
	void draw(IrrlichtDevice* device){
		IVideoDriver* driver = device->getVideoDriver();
		IGUIFont* font = device->getGUIEnvironment()->getSkin()->getFont();
		drawPath(driver, SColor(255,0,0,0), sourcePath);
		if(!transformedPath.empty()){
			drawPath(driver, MEAS_COLOR, transformedPath);
		}
		if(type==IPathTransform<TestPath>::FULL_TRIANGULATED && transformation!=NULL){
			if(TriangulatedPathTransfrom<TestPath, double>::id.compare(transformation->getIdentifier())==0){
				auto dtpt = (DrawableTriangulatedPathTransform<TestPath, double>*)transformation;
				dtpt->drawTriangulation(driver, SOURCE_COLOR, MEAS_COLOR);
			}
		}
		int index = 0;
		for(auto it=matchPoints.begin(); it!=matchPoints.end(); ++it){
			drawPoint(driver, font, it->sourcePoint, index, SOURCE_COLOR);
			drawPoint(driver, font, it->measuredPoint, index, MEAS_COLOR);
			if(transformation){
				TestVector point(MatrixInit::ZERO);
				auto point2d = createSubMatrix(point,0,0,2,1);
				point2d = it->sourcePoint;
				//Vector2D<int64_t> point = it->sourcePoint;
				transformation->transformSingle(point);
				drawPoint(driver, font, point, index, SColor(255,0,0,0));
			}
			index++;
		}
		if(sourcePointSet){
			drawPoint(driver, font, sourcePoint, index, SOURCE_COLOR);
		}
	}
	
};

static bool isRadAngleEqual(double a1, double a2, double eps){
	double phi = fmod(a1-a2, 2*PI);
	if(phi>PI){
		phi = phi - 2*PI;
	}else if(phi<-PI){
		phi = phi + 2*PI;
	}
	return fabs(phi)<eps;
}

int main(int argc, char *argv[]){
	IrrlichtDevice* nulldevice = createDevice(EDT_NULL);
	core::dimension2d<u32> deskres = nulldevice->getVideoModeList()->getDesktopResolution();
	nulldevice->drop();
	
	SIrrlichtCreationParameters param;
	param.Bits = 32;
   param.DriverType = EDT_OPENGL;
	rect<irr::s32> winRect(0,0,9*deskres.Width/10,9*deskres.Height/10);
	param.WindowSize = winRect.getSize();
	
	IrrlichtDevice* device = createDeviceEx(param);
	IVideoDriver* driver = device->getVideoDriver();
	IGUIEnvironment* env = device->getGUIEnvironment();
	ISceneManager* smgr = device->getSceneManager();
	
	device->setWindowCaption(L"Manual Tests for Path Transformations");
	
	TestPath sourcePath{{0,100,0}, {800,100,0}, {600,300,0}, {450,150,0}, {300,300,0}};
	
	EventReceiver rcv(sourcePath);
	device->setEventReceiver(&rcv);
	
	//Unit tests: -----
	DDuernion test1(30/DEG_RAD);
	DDuernion test2(-30/DEG_RAD);
	DDuernion test3(90/DEG_RAD);
	
	assert(isRadAngleEqual((test1*test1).calcAngle(), 60/DEG_RAD, ANGLE_EPS));
	assert(isRadAngleEqual((test1*test2).calcAngle(), 0.0, ANGLE_EPS));
	assert(isRadAngleEqual((test3*test3.getInverse()).calcAngle(), 0.0, ANGLE_EPS));
	
	//assert(false);
	
	Matrix<2,2,int16_t> intIdentity(MatrixInit::IDENTITY);
	
	static_assert(std::is_same<ResultScalar<Matrix<3,3,int16_t>, Matrix<3,3,double>>::type, double>::value);
	
	Matrix<2,2,int16_t> testM({
		1, 2,
		3, 4});
		
	std::cout << "testM:\n" << testM << std::endl;
	
	Matrix<2,2,double> testM2({
		1.1, 2.1,
		3.1, 4.1});
	
	auto testM3 = testM*testM2;
	
	assert(!areMatricesEqual(testM, testM2));
	assert(areMatricesEqual(testM, testM));
	assert(areMatricesEqual(testM3, Matrix<2,2,double>{
		7.3,	10.3,
		15.7,	22.7}, MATRIX_EPS));
		
	std::cout << "testM3:\n" << testM3 << std::endl;
	
	Matrix<2,3,int> testM4{
		1,2,3,
		4,5,6};
	
	assert(areMatricesEqual(testM4 * Matrix<3,4,int>{1,2,3,4,4,5,5,7,7,8,9,10}, Matrix<2,4,int>{30,36,40,48,66,81,91,111}));
	assert(areMatricesEqual(testM4-testM4, Matrix<2,3,int>{0,0,0,0,0,0}));
	assert(areMatricesEqual(testM4+testM4, Matrix<2,3,int>{2,4,6,8,10,12}));
	assert(areMatricesEqual(2*testM4, Matrix<2,3,int>{2,4,6,8,10,12}));
	assert(areMatricesEqual(testM4*2, Matrix<2,3,int>{2,4,6,8,10,12}));
	
	
	auto testM5 = createTransposeMatrix(testM4);
	std::cout << "testM4:\n" << testM4 << std::endl;
	std::cout << "testM5:\n" << testM5 << std::endl;
	
	std::cout << "(testM4 * testM5):\n" << (testM4 * testM5) << std::endl;
	
	assert(areMatricesEqual(testM4 * testM5, Matrix<2,2,int>{14,32,32,77}));
	
	auto subMatrix = createSubMatrix(testM4,0,0,2,2);
	std::cout << "subMatrix:\n" << subMatrix << std::endl;
	subMatrix = Matrix<2,2,int>(MatrixInit::IDENTITY);
	std::cout << "testM4:\n" << testM4 << std::endl;
	
	assert(areMatricesEqual(testM4, Matrix<2,3,int>{1,0,3,0,1,6}));
	
	std::cout << "test3.convertToMatrix():\n" << test3.convertToMatrix() << std::endl;
	assert(areMatricesEqual(test3.convertToMatrix(), Matrix<2,2,double>{0, -1, 1, 0}, MATRIX_EPS));
	
	Vector2D<int> v0{1,0};
	std::cout << "transform:\n" << test3.transform(v0) << std::endl;
	assert(areMatricesEqual(test3.transform(v0), Vector2D<double>{0, 1}, MATRIX_EPS));
	
	Matrix<3,3,int> testM6{
		1,2,3,
		4,5,6,
		7,8,8};
	
	assert(calcDeterminant(testM)==-2);
	assert(calcDeterminant(testM6)==3);
	
	std::cout << "calcInverse(testM6):\n" << calcInverse(convertMatrix<double>(testM6)) << std::endl;
	std::cout << "calcInverse(testM):\n" << calcInverse(convertMatrix<double>(testM)) << std::endl;
	
	Matrix<4,4,int> testM7{
		1,2,3,4,
		5,6,7,8,
		8,10,11,12,
		13,14,15,15};
	
	std::cout << "calcDeterminant(testM7):\n" << calcDeterminant(testM7) << std::endl;
	std::cout << "calcInverse(testM7):\n" << calcInverse(convertMatrix<double>(testM7)) << std::endl;
	
	assert(areMatricesEqual(testM7*calcInverse(convertMatrix<double>(testM7)), Matrix<4,4,double>(MatrixInit::IDENTITY), MATRIX_EPS));
	assert(areMatricesEqual(testM6*calcInverse(convertMatrix<double>(testM6)), Matrix<3,3,double>(MatrixInit::IDENTITY), MATRIX_EPS));
	assert(areMatricesEqual(testM*calcInverse(convertMatrix<double>(testM)), Matrix<2,2,double>(MatrixInit::IDENTITY), MATRIX_EPS));
	
	assert(testM7[3]==13);
	assert(getMatrixMin(testM7)==1);
	assert(getMatrixMax(testM7)==15);
	
	assert(calcSquareSumNorm(testM)==30);
	assert(fabs(calcFrobeniusNorm<decltype(testM),double>(testM)-sqrt(30))<MATRIX_EPS);
	
	assert(areMatricesEqual(rdMatrix<int>(testM2), Matrix<2,2,int>{1,2,3,4}));
	assert(areMatricesEqual(rdMatrix<int64_t>(testM7), testM7));
	
	//-------
	
	while(device->run()){
		if(device->isWindowActive()){
			driver->beginScene(true, true, SColor(255,240,240,240));//SColor(0,200,200,200));
			
			rcv.draw(device);
			
			smgr->drawAll();
			env->drawAll();
			driver->endScene();
		}
	}
	
	device->drop();
	
	return 0;
}
