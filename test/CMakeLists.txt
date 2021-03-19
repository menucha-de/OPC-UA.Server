set(testResourcesDirBase ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# installation directories
set(testInstallDirBase ${CMAKE_INSTALL_PREFIX}/serverTest-${arch}-${CMAKE_BUILD_TYPE})
set(testInstallDirLibs ${testInstallDirBase}/lib)
set(testInstallDirConf ${testInstallDirBase}/conf)
# add test libraries
install(DIRECTORY ${installDirLibs}/ DESTINATION ${testInstallDirLibs})

#---------- sub directories ----------
add_subdirectory(src)