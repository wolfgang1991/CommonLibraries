#ifndef MATHHELPERS_H_
#define MATHHELPERS_H_

#include <cmath>

const double PI = 3.141592654;
//factor * (m/s)/knots:
#define METER_S__KNOTS 0.514444444
//factor * m/feet
#define METER_FEET 0.3048
//factor * m/NM
#define METER_NM 1852.0
//factor * m/SM (statue mile)
#define METER_SM 1609.344
//factor * m/SM (statue mile)
#define METER_S__MPH 0.44704
//factor * °/rad
#define GRAD_RAD (180.0/3.141592654)
#define DEG_RAD (180.0/3.141592654)
#define DOUBLE_EPS 0.00000001
#define FLOAT_EPS 0.0001
//Erdradius in Meter
#define EARTHRADIUS 6371000.785
//WGS84EGM96 Ellipsoid major axis
#define WGS84EGM96_MAJOR 6378137.0
//WGS84EGM96 Ellipsoid minor axis
#define WGS84EGM96_MINOR 6356752.314
//Proportionalitätskonstante zur Korrektur zur mittleren Ausrichtung
#define LOC_GS_CORRECTION_FACTOR (1.0/1.24)
//Poroportionalität mbars Quecksilbersäule
#define HG_MBARS 0.02953
//1 pa = 1 N/m²; 1mbar = 100pa
#define PASCAL_MBARS 100

template <class T>
T Clamp(T value, T min, T max){
	return value<min?min:(value>max?max:value);
}

template <class T>
T Min(T value, T max){
	return value>max?max:value;
}

template <class T>
T Max(T value, T min){
	return value<min?min:value;
}

//! A (in), B (out)
//! round A to B
template <class A, class B>
B rd(A in){
	bool sign = in<(A)0.0;
	in = sign?-in:in;//abs
	B out = (B)(in+(A)0.5);
	return sign?-out:out;
}

template <typename T>
inline double celsiusToFahrenheit(T celcius){
	return static_cast<double>(celcius)*1.8 + 32.0;
}

template <typename T>
inline double fahrenheitToCelcius(T fahrenheit){
	return (static_cast<double>(fahrenheit)-32.0)/1.8;
}

template <typename T>
inline double HgtoHPa(T pressureInHg){
	return static_cast<double>(pressureInHg)*(1013.25/29.9213);
}

template <typename T>
inline double HPatoHg(T pressureInHpa){
	return static_cast<double>(pressureInHpa)*(29.9213/1013.25);
}

//! t in [0,1]
template<typename TEdge>
inline TEdge smoothstep(TEdge edge0, TEdge edge1, TEdge x){
	TEdge t = Clamp((x - edge0) / (edge1 - edge0), (TEdge)0.0, (TEdge)1.0);
	return t * t * ((TEdge)3.0 - (TEdge)2.0 * t);
}

//! ^2
template <class T>
T sq(T val){
	return val*val;
}

//! liefert die Differenz von a und b in mod max+1, wobei a "nach" b kommt
inline unsigned int Difference(unsigned int a, unsigned int b, unsigned int max){
	if(a<b){return max-b+a;}
	return a-b;
}

//! liefert true wenn sich zwei Zahlenbereiche a und b überlappen
template <class T>
inline bool NumbersOverlap(T a1, T a2, T b1, T b2){
	return (b1<=a1 && b2>=a1) || (b1<=a2 && b2>=a2) || (b1>=a1 && b2<=a2);
}

//! Exchanges a1 and a2 such that a1<=a2
template <class T>
inline void sortTwoValues(T& a1, T& a2){
	if(a1>a2){
		T intermediate = a1;
		a1 = a2;
		a2 = intermediate;
	}
}

//! in radians,  progress=0 => angle0, progress=1 => angle1
template <typename TAngle>
TAngle lerpAngle(TAngle angle0, TAngle angle1, TAngle progress){
	TAngle x0 = sin(angle0), y0 = cos(angle0), x1 = sin(angle1), y1 = cos(angle1);
	x0 = x0*(static_cast<TAngle>(1)-progress)+x1*progress;
	y0 = y0*(static_cast<TAngle>(1)-progress)+y1*progress;
	return atan2(x0,y0);
}

//! return angle between 0..360°
inline double Angle0to360(double phi){
	phi = fmod(phi, 360.0);
	return phi<0.0?(phi+360.0):phi;
}

//! return angle between -180...180°
inline double Angle180(double phi){
	phi = fmod(phi, 360.0);
	if(phi>180.0){
		phi = phi - 360.0;
	}else if(phi<-180.0){
		phi = phi + 360.0;
	}
	return phi;
}

#endif
