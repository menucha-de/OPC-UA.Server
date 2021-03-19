#!/usr/bin/env bash
# The default properties for building the server in build.properties.example
# can be overridden with a custom file build.properties or by setting environment variables.
# Environment variables override the properties from existing files.
# The server is created in directory 
#   target/server-<DEB_HOST_GNU_TYPE>-<CMAKE_BUILD_TYPE>

function getProperty {
  VAR_NAME=$1
  # if environment variable does not exist
  if [[ ! -v $VAR_NAME ]]; then
    # if custom properties file exists
    if [ -f build.properties ]; then
      PROPS_FILE=build.properties
    else
      PROPS_FILE=build.properties.example
    fi
    GET_PROPERTY_RESULT=`grep ^$VAR_NAME $PROPS_FILE | cut -d'=' -f2`
  else
    GET_PROPERTY_RESULT=${!VAR_NAME}
  fi
}

# get properties
getProperty CMAKE_BUILD_TYPE
CMAKE_BUILD_TYPE=$GET_PROPERTY_RESULT
getProperty DEB_HOST_GNU_TYPE
DEB_HOST_GNU_TYPE=$GET_PROPERTY_RESULT
getProperty CMAKE_TOOLCHAIN_FILE
CMAKE_TOOLCHAIN_FILE=$GET_PROPERTY_RESULT
if [ -z $CMAKE_TOOLCHAIN_FILE ]; then
  TOOLCHAIN=
else
  TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE 
fi
getProperty IVY_URL
IVY_URL=$GET_PROPERTY_RESULT
getProperty IVY_SETTINGS
IVY_SETTINGS=$GET_PROPERTY_RESULT
getProperty IVY_RESOLVER
IVY_RESOLVER=$GET_PROPERTY_RESULT

# use ant script to get libraries from ivy repository
CMD="ant -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DDEB_HOST_GNU_TYPE=$DEB_HOST_GNU_TYPE $TOOLCHAIN \
         -Divy.url=$IVY_URL -Divy.settings=$IVY_SETTINGS -Divy.resolver=$IVY_RESOLVER retrieve"
echo $CMD && eval $CMD

# create build directory
BUILD_DIR=target/build-$DEB_HOST_GNU_TYPE-$CMAKE_BUILD_TYPE 
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# configure the build with cmake
CMD="DEB_HOST_GNU_TYPE=$DEB_HOST_GNU_TYPE cmake ../.. $TOOLCHAIN -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_INSTALL_PREFIX=.."
echo $CMD && eval $CMD

# compile and install the server
if [ $CMAKE_BUILD_TYPE = "Release" ]; then
  CMD="make install/strip"
  echo $CMD && eval $CMD
else
  CMD="make install"
#  CMD="make install VERBOSE=1"
  echo $CMD && eval $CMD
fi
