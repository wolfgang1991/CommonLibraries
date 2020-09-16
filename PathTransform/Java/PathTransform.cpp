#include "de_vhf_pathtransform_PathTransform.h"
#include "de_vhf_pathtransform_PathTransform_Path.h"

#include <PathTransformCreation.h>
#include <StringHelpers.h>

#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

typedef std::vector<Vector3D<double>> Path;
typedef double PathFloat;
typedef IPathTransform<Path> PathTransform;

static std::wstring Java_To_WStr(JNIEnv* env, jstring string){
    std::wstring value;
    const jchar* raw = env->GetStringChars(string, 0);
    jsize len = env->GetStringLength(string);
    value.assign(raw, raw + len);
    env->ReleaseStringChars(string, raw);
    return value;
}

//PathTransform:

JNIEXPORT jlong JNICALL Java_de_vhf_pathtransform_PathTransform_createPathTransform(JNIEnv* env, jobject jthis, jstring representation){
	std::string r = convertWStringToString(Java_To_WStr(env, representation));
	return (jlong)createPathTransform<Path,PathFloat>(r);
}

JNIEXPORT void JNICALL Java_de_vhf_pathtransform_PathTransform_printPathTransform(JNIEnv* env, jobject jthis, jlong ptr){
	std::cout << ((PathTransform*)ptr)->createStringRepresentation() << std::endl;
}

JNIEXPORT void JNICALL Java_de_vhf_pathtransform_PathTransform_deletePathTransform(JNIEnv* env, jobject jthis, jlong ptr){
	delete (PathTransform*)ptr;
}

JNIEXPORT void JNICALL Java_de_vhf_pathtransform_PathTransform_transformInPlace(JNIEnv* env, jobject jthis, jlong ptr, jlong pathPtr){
	PathTransform* transform = (PathTransform*)ptr;
	Path* path = (Path*)pathPtr;
	transform->transformInPlace(*path);
}

JNIEXPORT jlong JNICALL Java_de_vhf_pathtransform_PathTransform_transform(JNIEnv* env, jobject jthis, jlong ptr, jlong pathPtr){
	PathTransform* transform = (PathTransform*)ptr;
	Path* path = (Path*)pathPtr;
	Path newPath = transform->transform(*path);
	return (jlong)(new Path(std::move(newPath)));
}

//Path:

JNIEXPORT jlong JNICALL Java_de_vhf_pathtransform_PathTransform_00024Path_createPath(JNIEnv* env, jobject jthis, jint size){
	Path* p = new Path();
	p->reserve(size);
	return (jlong)p;
}

JNIEXPORT void JNICALL Java_de_vhf_pathtransform_PathTransform_00024Path_deletePath(JNIEnv* env, jobject jthis, jlong ptr){
	delete (Path*)ptr;
}

JNIEXPORT void JNICALL Java_de_vhf_pathtransform_PathTransform_00024Path_pushBack(JNIEnv* env, jobject jthis, jlong ptr, jlong x, jlong y, jlong z){
	Path* p = (Path*)ptr;
	p->push_back(Vector3D<double>{static_cast<double>(x)/1000.0,static_cast<double>(y)/1000.0,static_cast<double>(z)/1000.0});
}

JNIEXPORT jlong JNICALL Java_de_vhf_pathtransform_PathTransform_00024Path_getPointPtr(JNIEnv* env, jobject jthis, jlong ptr, jint index){
	Path* p = (Path*)ptr;
	return (jlong)(&((*p)[index]));
}

JNIEXPORT jlong JNICALL Java_de_vhf_pathtransform_PathTransform_00024Path_getPointX(JNIEnv* env, jobject jthis, jlong ptr){
	return ((Vector3D<double>*)ptr)->get(0)*1000.0;
}

JNIEXPORT jlong JNICALL Java_de_vhf_pathtransform_PathTransform_00024Path_getPointY(JNIEnv* env, jobject jthis, jlong ptr){
	return ((Vector3D<double>*)ptr)->get(1)*1000.0;
}

JNIEXPORT jlong JNICALL Java_de_vhf_pathtransform_PathTransform_00024Path_getPointZ(JNIEnv* env, jobject jthis, jlong ptr){
	return ((Vector3D<double>*)ptr)->get(2)*1000.0;
}

JNIEXPORT jint JNICALL Java_de_vhf_pathtransform_PathTransform_00024Path_getPointCount(JNIEnv* env, jobject jthis, jlong ptr){
	return ((Path*)ptr)->size();
}

#ifdef __cplusplus
}
#endif
