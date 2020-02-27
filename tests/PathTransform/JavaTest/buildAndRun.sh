#!/bin/bash -v

javac -classpath ../../../PathTransform/Java/PathTransform.jar src/Test.java
java -Djava.library.path=../../../PathTransform/Java/ -classpath ../../../PathTransform/Java/PathTransform.jar:./src Test
