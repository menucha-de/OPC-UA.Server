set(resourcesDirConf ${resourcesDirConf}/provider)
set(installDirLibs ${installDirLibs}/provider)
set(installDirConf ${installDirConf}/provider)

add_subdirectory(binary)
add_subdirectory(buildingautomation)
add_subdirectory(uaNodeSetXml)

# add libraries to provider libraries in parent scope
set(providerLibraries
  ${providerLibraries}

  PARENT_SCOPE
)
