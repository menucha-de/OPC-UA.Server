cmake_minimum_required(VERSION 3.0)
project(Server)

# load properties
include(cmake.properties)

set(CMAKE_CXX_FLAGS "-std=c++98 -gdwarf-4")

set(SERVER_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/src # files created with configure_file
)

set(resourcesDirBase ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(resourcesDirConf ${resourcesDirBase}/conf)

set(installDirBase ${CMAKE_INSTALL_PREFIX})
set(installDirLibs ${installDirBase}/lib)
set(installDirConf ${installDirBase}/conf)

# export properties "ant"
configure_file(${resourcesDirBase}/ant.properties.in ${CMAKE_CURRENT_BINARY_DIR}/ant.properties)

#---------- debug ----------
set(CMAKE_DEBUG_POSTFIX d)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG=1)
endif()
 
#---------- system libraries ----------
# libraries: ${CMAKE_THREAD_LIBS_INIT}
find_package(Threads REQUIRED) 
# libraries: ${OPENSSL_LIBRARIES}
find_package(OpenSSL REQUIRED)
# libraries: ${LIBXML2_LIBRARIES} 
# includes: ${LIBXML2_INCLUDE_DIR}
find_package(LibXml2 REQUIRED)
# includes: ${JAVA_INCLUDE_PATH}
set(JAVA_AWT_LIBRARY NotNeeded)
set(JAVA_JVM_LIBRARY NotNeeded)
set(JAVA_AWT_INCLUDE_PATH NotNeeded)
find_package(JNI REQUIRED)




#---------- OPC UA libraries ----------
add_definitions(
  -DOPCUA_SUPPORT_SECURITYPOLICY_BASIC128RSA15=1
  -DOPCUA_SUPPORT_SECURITYPOLICY_BASIC256=1
  -DOPCUA_SUPPORT_SECURITYPOLICY_NONE=1
  -DOPCUA_SUPPORT_PKI=1
  -DSUPPORT_XML_PARSER=1
  -D_UA_STACK_USE_DLL
)
set(OPCUA_INCLUDE_DIRS    
  /usr/include/uastack
  /usr/include/uabase  
  /usr/include/uapki
  /usr/include/xmlparser
)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_library(uamoduleLib NAMES uamoduled PATHS ${libDirBase})
    find_library(coremoduleLib NAMES coremoduled PATHS ${libDirBase})
    find_library(uapkiLib NAMES uapkid PATHS ${libDirBase})
    find_library(uabaseLib NAMES uabased PATHS ${libDirBase})
    find_library(uastackLib NAMES uastackd PATHS ${libDirBase})
    find_library(xmlparserLib NAMES xmlparserd PATHS ${libDirBase})
else(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_library(uamoduleLib NAMES uamodule PATHS ${libDirBase})
    find_library(coremoduleLib NAMES coremodule PATHS ${libDirBase})
    find_library(uapkiLib NAMES uapki PATHS ${libDirBase})
    find_library(uabaseLib NAMES uabase PATHS ${libDirBase})
    find_library(uastackLib NAMES uastack PATHS ${libDirBase})
    find_library(xmlparserLib NAMES xmlparser PATHS ${libDirBase})
endif()
set(OPCUA_LIBRARIES    
  ${uamoduleLib} # static
  ${coremoduleLib} # static
  ${uapkiLib}  # static
  ${uabaseLib}  # static
  ${uastackLib}
  ${xmlparserLib} # static
)
foreach(opcuaLib ${OPCUA_LIBRARIES})
  message("Found OPC UA: ${opcuaLib}")
endforeach()

#---------- OPC UA server libraries ----------
set(OPCUA_SERVER_INCLUDE_DIRS
  /usr/include/uaserver
)

#---------- Playground
add_executable(playground test/src/playground.cpp)
target_include_directories(playground PRIVATE
  src
  include
  /usr/include/uabase
  /usr/include/uastack
)
target_link_libraries (playground PRIVATE
  opcua
  binary
  binaryserver
  serverapi
  uastack
)

#---------- OPC UA client libraries ----------
set(OPCUA_CLIENT_INCLUDE_DIRS
  /usr/include/uaclient
)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_library(uaclientLib NAMES uaclientd PATHS ${libDirBase})
else(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_library(uaclientLib NAMES uaclient PATHS ${libDirBase})
endif()
set(OPCUA_CLIENT_LIBRARIES
  ${uaclientLib} # static
)
foreach(opcuaLib ${OPCUA_CLIENT_LIBRARIES})
  message("Found OPC UA: ${opcuaLib}")
endforeach()

#---------- cpputest libraries ----------
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(cpputestDir ${CMAKE_CURRENT_SOURCE_DIR}/cpputest-${cpputestVersion})
  set(CPPUTEST_INCLUDE_DIR ${cpputestDir}/include)
  set(cpputestLibDir ${cpputestDir}/${arch}/lib)
  find_library(cpputestLib NAMES CppUTest PATHS ${cpputestLibDir})
  find_library(cpputestextLib NAMES CppUTestExt PATHS ${cpputestLibDir})
  set(CPPUTEST_LIBRARIES
    ${cpputestLib} # static
    ${cpputestextLib} # static
  )
  foreach(testLib ${CPPUTEST_LIBRARIES})
    message("Found CppUTest: ${testLib}")
  endforeach()

  if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 --coverage")
  endif()
  if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 --coverage")
  endif()

# cd target/build-*
# execute test/src/ServerTest

# lcov --capture --directory src --output-file coverage.info
# genhtml coverage.info --output-directory out

# gcovr -r . --html -o a.html
# gcovr -r . --xml-pretty          # Cobertura XML, Cobertura Plugin for Jenkins

endif()

#---------- sub directories ----------
# list for test libraries
set(testLibraries)
add_subdirectory(src)
add_subdirectory(src/main/native)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_subdirectory(test)
endif()
