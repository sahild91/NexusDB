file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/nexusdb/*.h" "include/nexusdb/utils/*.h")

# Add library target
add_library(nexusdb_core ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(nexusdb_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add executable target for the main application
add_executable(nexusdb_app src/main.cpp)

# Link the library to the executable
target_link_libraries(nexusdb_app PRIVATE nexusdb_core)

# Installation rules
include(GNUInstallDirs)

install(TARGETS nexusdb_core nexusdb_app
    EXPORT NexusDBTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nexusdb)

# Generate and install export file
install(EXPORT NexusDBTargets
    FILE NexusDBTargets.cmake
    NAMESPACE NexusDB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NexusDB
)

# Generate the config file that includes the exports
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/NexusDBConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NexusDB
)

# Generate the version file for the config file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NexusDBConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

# Install the configuration files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/NexusDBConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/NexusDBConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NexusDB
)