add_library(binary SHARED  
  common/ConversionException.cpp
  common/TimeoutException.cpp 
  jDataProvider/JDataProvider.cpp
  jDataProvider/JDataProviderFactory.cpp
  messages/ConverterBin2IO.cpp
  messages/dto/Array.cpp
  messages/dto/Call.cpp
  messages/dto/CallResponse.cpp
  messages/dto/Event.cpp
  messages/dto/Message.cpp
  messages/dto/MessageHeader.cpp
  messages/dto/Notification.cpp
  messages/dto/ParamId.cpp
  messages/dto/ParamIdMessage.cpp
  messages/dto/ParamList.cpp
  messages/dto/ParamMap.cpp
  messages/dto/Read.cpp
  messages/dto/ReadResponse.cpp
  messages/dto/Scalar.cpp
  messages/dto/StatusMessage.cpp
  messages/dto/Subscribe.cpp
  messages/dto/SubscribeResponse.cpp
  messages/dto/Struct.cpp
  messages/dto/Unsubscribe.cpp
  messages/dto/UnsubscribeResponse.cpp
  messages/dto/Variant.cpp
  messages/dto/Write.cpp
  messages/dto/WriteResponse.cpp
)
target_include_directories(binary PRIVATE
  ${LIBXML2_INCLUDE_DIR}
  ${OPCUA_CLIENT_INCLUDE_DIRS}
)
target_link_libraries (binary PRIVATE
  serverapi
)
install(TARGETS binary DESTINATION ${installDirLibs})
#install(DIRECTORY ${resourcesDirConf}/binary DESTINATION ${installDirConf})

# add library to provider libraries in parent scope
set(providerLibraries
  ${providerLibraries}

  binary

  PARENT_SCOPE
)

