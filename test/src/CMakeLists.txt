#---------- executable ----------
add_executable(ServerTest  
  common/logging/TestConsoleLogger.cpp
  common/logging/TestConsoleLoggerFactory.cpp
  common/logging/TestLoggerFactory.cpp
  provider/binary/common/TestClientSocket.cpp
  provider/binary/ioDataProvider/TestBinaryIODataProvider.cpp
  provider/binary/ioDataProvider/TestBinaryIODataProviderFactory.cpp
  provider/binary/messages/TestMessageQueue.cpp
  sasModelProvider/base/TestConverterUa2IO.cpp
  Env.cpp
  main.cpp
)
target_include_directories(ServerTest PRIVATE
  # inherits the include dirs from linked serverapi library (included in ${testLibraries})
)
target_link_libraries (ServerTest PRIVATE  
  ${testLibraries}
  ${CMAKE_THREAD_LIBS_INIT}  
)
# copy resources to binary directory for code coverage report creation
install(DIRECTORY ${testResourcesDirBase}/ DESTINATION ${CMAKE_BINARY_DIR}/test/src/)

install(TARGETS ServerTest DESTINATION ${testInstallDirBase})
install(PROGRAMS ${testResourcesDirBase}/ServerTest.sh DESTINATION ${testInstallDirBase})
install(DIRECTORY ${testResourcesDirBase}/conf/ DESTINATION ${testInstallDirConf})
configure_file(${resourcesDirBase}/changelog.in ${testInstallDirBase}/changelog)
