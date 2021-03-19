
#---------- server interface library ----------
add_library(serverapi SHARED
  common/Exception.cpp
  common/Mutex.cpp
  common/MutexException.cpp
  common/MutexLock.cpp
  common/logging/ConsoleLogger.cpp
  common/logging/ConsoleLoggerFactory.cpp
  common/logging/ILoggerFactory.cpp
  common/logging/Logger.cpp
  common/logging/LoggerFactory.cpp
  common/logging/LoggingException.cpp
  ioDataProvider/Array.cpp
  ioDataProvider/Event.cpp
  ioDataProvider/IODataProvider.cpp
  ioDataProvider/IODataProviderException.cpp
  ioDataProvider/IODataProviderFactory.cpp
  ioDataProvider/MethodData.cpp
  ioDataProvider/NodeData.cpp
  ioDataProvider/NodeId.cpp
  ioDataProvider/NodeProperties.cpp
  ioDataProvider/OpcUaEventData.cpp
  ioDataProvider/Scalar.cpp
  ioDataProvider/Structure.cpp
  ioDataProvider/SubscriberCallback.cpp
  ioDataProvider/SubscriberCallbackException.cpp
  ioDataProvider/Variant.cpp
  sasModelProvider/base/CodeNodeManagerBase.cpp
  sasModelProvider/base/ConversionException.cpp
  sasModelProvider/base/ConverterUa2IO.cpp
  sasModelProvider/base/EventTypeData.cpp
  sasModelProvider/base/EventTypeRegistry.cpp
  sasModelProvider/base/HaNodeManager.cpp
  sasModelProvider/base/HaNodeManagerException.cpp
  sasModelProvider/base/HaNodeManagerIODataProviderBridge.cpp
  sasModelProvider/base/HaNodeManagerIODataProviderBridgeException.cpp
  sasModelProvider/base/IODataManager.cpp
  sasModelProvider/base/IODataProviderSubscriberCallback.cpp
  sasModelProvider/base/NodeBrowser.cpp
  sasModelProvider/base/NodeBrowserException.cpp
  sasModelProvider/base/generator/DataGenerator.cpp
  sasModelProvider/base/generator/GeneratorException.cpp
  sasModelProvider/base/generator/GeneratorIODataProvider.cpp  
  sasModelProvider/SASModelProvider.cpp
  sasModelProvider/SASModelProviderException.cpp
  sasModelProvider/SASModelProviderFactory.cpp
  utilities/linux.cpp
)
target_include_directories(serverapi PUBLIC
  ${SERVER_INCLUDE_DIRS} # #include <common/Exception.h>
  ${OPCUA_SERVER_INCLUDE_DIRS}
  ${OPCUA_INCLUDE_DIRS}
  ${CPPUTEST_INCLUDE_DIR} # #include "CppUTest/MemoryLeakDetectorNewMacros.h"
)
install(TARGETS serverapi DESTINATION ${installDirLibs})

#---------- providers ----------
# list for provider libraries (each provider creates an own shared library)
set(providerLibraries)
add_subdirectory(provider)

#---------- server library ----------
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_library(server SHARED    
  OpcUaServerCore.cpp  
  Server.cpp
  ServerException.cpp
  logging/ServerSdkLogger.cpp
  logging/ServerSdkLoggerFactory.cpp
)
target_include_directories(server PRIVATE
  # inherits the include dirs from linked serverapi library  
)
set(cpputestLibs ${CPPUTEST_LIBRARIES})
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # provide all symbols for testing
  # -Wl: option for linker; -whole-archive: link the whole static library and not only the used symbols
  set(cpputestLibs -Wl,-whole-archive ${cpputestLibs} -Wl,-no-whole-archive)
endif()
target_link_libraries (server PRIVATE  
  opcua
  serverapi
  ${CMAKE_DL_LIBS}
  ${cpputestLibs} # static
)
install(TARGETS server DESTINATION ${installDirLibs})

#---------- server executable ----------
add_executable(Server         
  main.cpp
)
set_target_properties(Server PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_include_directories(Server PRIVATE
  # inherits the include dirs from linked serverapi library
)
target_link_libraries (Server PRIVATE
  opcua
  serverapi
  ${CMAKE_DL_LIBS}
  server
)
install(TARGETS Server DESTINATION ${installDirBase})
install(PROGRAMS ${resourcesDirBase}/Server.sh DESTINATION ${installDirBase} CONFIGURATIONS Release)
install(PROGRAMS ${resourcesDirBase}/Serverd.sh DESTINATION ${installDirBase} CONFIGURATIONS Debug)
install(FILES ${resourcesDirConf}/ServerConfig.xml DESTINATION ${installDirConf})
configure_file(${resourcesDirBase}/changelog.in ${installDirBase}/changelog)

#---------- binary server library ----------
add_library(binaryserver SHARED
  binaryServer/CachedConverterCallback.cpp
  binaryServer/ConfigLoader.cpp
  binaryServer/Event.cpp
  binaryServer/EventField.cpp
  binaryServer/HaSession.cpp
  binaryServer/HaSessionException.cpp
  binaryServer/HaSubscription.cpp
  binaryServer/HaSubscriptionException.cpp
  binaryServer/NodeAttributes.cpp  
  binaryServer/Server.cpp
  binaryServer/ServerException.cpp
  binaryServer/logging/ClientSdkLogger.cpp
  binaryServer/logging/ClientSdkLoggerFactory.cpp
)
target_include_directories(binaryserver PRIVATE
  # inherits the include dirs from linked serverapi library
  ${LIBXML2_INCLUDE_DIR}
  ${OPCUA_CLIENT_INCLUDE_DIRS}
)
set(cpputestLibs ${CPPUTEST_LIBRARIES})
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # -Wl: option for linker; -whole-archive: link the whole static library and not only the used symbols
  set(cpputestLibs -Wl,-whole-archive ${cpputestLibs} -Wl,-no-whole-archive)
endif()
target_link_libraries (binaryserver PRIVATE
  opcua
  serverapi
  binary
  ${OPCUA_CLIENT_LIBRARIES} # static
  ${cpputestLibs} # static
)
install(TARGETS binaryserver DESTINATION ${installDirLibs})

#---------- binary server executable ----------
add_executable(BinaryServer
  binaryServer/main.cpp
)
set_target_properties(BinaryServer PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_include_directories(BinaryServer PRIVATE
  # inherits the include dirs from linked serverapi library
)
target_link_libraries (BinaryServer PRIVATE
  opcua
  serverapi
  binaryserver
)
install(TARGETS BinaryServer DESTINATION ${installDirBase})
install(PROGRAMS ${resourcesDirBase}/BinaryServer.sh DESTINATION ${installDirBase} CONFIGURATIONS Release)
install(PROGRAMS ${resourcesDirBase}/BinaryServerd.sh DESTINATION ${installDirBase} CONFIGURATIONS Debug)
install(FILES ${resourcesDirConf}/BinaryServerConfig.xml DESTINATION ${installDirConf})

# add libraries to test libraries in parent scope
set(testLibraries
  opcua
  serverapi
  ${providerLibraries}
  ${CMAKE_DL_LIBS}
  server
  binaryserver

  PARENT_SCOPE
)
