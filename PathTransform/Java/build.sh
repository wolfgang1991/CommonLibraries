#!/bin/bash -v

JAVA_HOME="$(readlink -f $(dirname $(readlink -f $(which javac)))/..)"
DIR=bin
mkdir -p "${DIR}"
rm -Rf "${DIR}"/*
javac -h . src/de/vhf/pathtransform/PathTransform.java -d "${DIR}"
jar cf PathTransform.jar -C "${DIR}" de
g++ -I"${JAVA_HOME}/include" -I"${JAVA_HOME}/include/linux" -I"../../Common" -I.. -DNO_OPENCV -shared -o libPathTransform.so PathTransform.cpp ../../Common/*.cpp -fPIC
