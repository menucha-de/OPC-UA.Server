add_library(uanodesetxml SHARED 
  sasModelProvider/HaNodeManagerNodeSetXml.cpp
  sasModelProvider/HaNodeManagerNodeSetXmlCreator.cpp
  sasModelProvider/HaNodeManagerNodeSetXmlException.cpp
  sasModelProvider/HaXmlUaNodeFactoryManager.cpp
  sasModelProvider/HaXmlUaNodeFactoryManagerSet.cpp
  sasModelProvider/HaXmlUaNodeFactoryNamespace.cpp
  sasModelProvider/ObjectTypeElement.cpp
  sasModelProvider/UaNodeSetXMLSASModelProvider.cpp
  sasModelProvider/UaNodeSetXMLSASModelProviderFactory.cpp
)
target_link_libraries (uanodesetxml PRIVATE
  serverapi
)
install(TARGETS uanodesetxml DESTINATION ${installDirLibs})
#install(DIRECTORY ${resourcesDirConf}/uanodesetxml DESTINATION ${installDirConf})

# add library to provider libraries in parent scope
set(providerLibraries
  ${providerLibraries}

  uanodesetxml

  PARENT_SCOPE
)
