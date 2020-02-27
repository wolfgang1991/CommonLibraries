#!/bin/bash -v

DIR=./de/vhf/pathtransform
mkdir -p $DIR
rm $DIR/*
javac -h . PathTransform.java
mv *.class $DIR
jar cf PathTransform.jar ./de
g++ -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" -I"../../Common" -I.. -DNO_OPENCV -shared -o libPathTransform.so PathTransform.cpp ../../Common/*.cpp -fPIC
