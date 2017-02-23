#!/bin/sh

#  buildscript.sh
#  mmdagentwrap
#
#  Created by h4x on 14/12/16.
#

WORKING_DIR=`pwd`

# build
cd $WORKING_DIR
export NDK_ROOT=/Users/h4x/android-ndk
export PATH=$PATH:$NDK_ROOT
ndk-build NDK_DEBUG=1

echo Copying sphinx native libraries to libs
cp ../sphinx/android/pocketsphinx-android-demo/app/src/main/jniLibs/armeabi-v7a/* ./libs/armeabi-v7a/
