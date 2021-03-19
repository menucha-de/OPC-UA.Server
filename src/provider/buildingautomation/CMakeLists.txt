add_library(buildingautomation SHARED 
  ioDataProvider/bacommunicationinterface.cpp
  ioDataProvider/bacontrollersimulation.cpp
  ioDataProvider/BaIODataProvider.cpp
  ioDataProvider/BaIODataProviderFactory.cpp
  sasModelProvider/airconditionercontrollerobject.cpp
  sasModelProvider/BaNodeManager.cpp
  sasModelProvider/BaSASModelProvider.cpp
  sasModelProvider/BaSASModelProviderFactory.cpp
  sasModelProvider/controllerobject.cpp
  sasModelProvider/furnacecontrollerobject.cpp
)
target_link_libraries (buildingautomation PRIVATE
  serverapi
)
install(TARGETS buildingautomation DESTINATION ${installDirLibs})

# add library to provider libraries in parent scope
set(providerLibraries
  ${providerLibraries}

  buildingautomation

  PARENT_SCOPE
)
