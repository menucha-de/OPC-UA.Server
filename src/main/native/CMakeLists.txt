#---------- java-opcua library ----------
add_library(java-opcua SHARED
  opcua_OPCUA.cpp
)
target_include_directories(java-opcua PRIVATE
  ${JAVA_INCLUDE_PATH} # jni.h
  ${JAVA_INCLUDE_PATH2} # jni_m.h
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/target/include
)
target_link_libraries (java-opcua PRIVATE  
  opcua 
  binaryserver 
  serverapi 
  uastack
)
install(TARGETS java-opcua LIBRARY DESTINATION lib DESTINATION ${installDir})


#---------- java-opcua-provider library ----------
add_library(java-opcua-provider SHARED
  opcua_OPCUADataProvider.cpp
)
target_include_directories(java-opcua-provider PRIVATE
  ${JAVA_INCLUDE_PATH} # jni.h
  ${JAVA_INCLUDE_PATH2} # jni_m.h
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/target/include
)
target_link_libraries (java-opcua-provider PRIVATE
  opcua
  binaryserver
  server 
  serverapi 
  uastack
)
install(TARGETS java-opcua-provider LIBRARY DESTINATION lib DESTINATION ${installDir})