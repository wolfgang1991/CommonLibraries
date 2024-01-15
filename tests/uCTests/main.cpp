#include <uCMatrix.h>

#include <iostream>
#include <cassert>


int main(int argc, char *argv[]){
	
	ucstd::Matrix3DF I(ucstd::Matrix3DF::IDENTITY);
	ucstd::Matrix3DF T{
		1, 2, 3,
		4, 5, 6,
		7, 8, 9};
		
	auto res = I*T;
	std::cout << "I:\n" << I << "\nT:\n" << T << "\nres:\n" << res << std::endl;
	
	ucstd::Matrix<float, 3, 4> T2{
		10, 20, 30, 40,
		50, 60, 70, 80,
		90,100,110,120};
	
	std::cout << T*T2 << std::endl;
	
	ucstd::Vector3DF v{1, 2, 3};
	
	std::cout << T*v << std::endl;
	
	//TODO more tests
	
	assert(I==I);
	assert(res==T);
	
	return 0;
}
