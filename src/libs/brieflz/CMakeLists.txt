cmake_minimum_required(VERSION 2.8.11)

project(BriefLZ C)

include(CTest)
include(GNUInstallDirs)

# Check if BriefLZ is the top-level project (standalone), or a subproject
set(brieflz_standalone FALSE)
get_directory_property(brieflz_parent_directory PARENT_DIRECTORY)
if(brieflz_parent_directory STREQUAL "")
  set(brieflz_standalone TRUE)
endif()
unset(brieflz_parent_directory)

#
# Options
#

# BRIEFLZ_BUILD_SHARED controls if BriefLZ libraries are built as shared or
# static
#
# It defaults to the value of BUILD_SHARED_LIBS if set, and in most cases
# that should be used instead. The purpose of BRIEFLZ_BUILD_SHARED is to allow
# overriding it when built as a subproject. For static libraries with position
# independent code, set CMAKE_POSITION_INDEPENDENT_CODE to True.
set(brieflz_shared_default OFF)
if(DEFINED BUILD_SHARED_LIBS)
  set(brieflz_shared_default ${BUILD_SHARED_LIBS})
endif()
option(BRIEFLZ_BUILD_SHARED "Build shared libraries" ${brieflz_shared_default})
unset(brieflz_shared_default)

# BRIEFLZ_BUILD_TESTING controls if BriefLZ adds testing support
#
# When built standalone, it defaults to the value of BUILD_TESTING if set.
# An optional prefix for the test names can be set with BRIEFLZ_TEST_PREFIX.
set(brieflz_testing_default ON)
if(brieflz_standalone)
  if(DEFINED BUILD_TESTING)
    set(brieflz_testing_default ${BUILD_TESTING})
  endif()
else()
  set(brieflz_testing_default OFF)
endif()
option(BRIEFLZ_BUILD_TESTING "Add testing support" ${brieflz_testing_default})
unset(brieflz_testing_default)

mark_as_advanced(BRIEFLZ_TEST_PREFIX)

# BRIEFLZ_BUILD_INSTALL controls if BriefLZ adds install support
#
# When built standalone or as a shared library subproject, the default is ON,
# and for static library subproject the default is OFF.
if(brieflz_standalone OR BRIEFLZ_BUILD_SHARED)
  option(BRIEFLZ_BUILD_INSTALL "Add install support" ON)
else()
  option(BRIEFLZ_BUILD_INSTALL "Add install support" OFF)
endif()

# BRIEFLZ_DEFAULT_RELEASE enables changing empty build type to Release
#
# Make based single-configuration generators default to an empty build type,
# which might be surprising, but could be useful if you want full control over
# compiler and linker flags. When BRIEFLZ_DEFAULT_RELEASE is ON, change an
# empty default build type to Release.
option(BRIEFLZ_DEFAULT_RELEASE "If CMAKE_BUILD_TYPE is empty, default to Release" ON)

if(brieflz_standalone AND BRIEFLZ_DEFAULT_RELEASE)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "CMAKE_BUILD_TYPE empty, defaulting to Release")
    set(CMAKE_BUILD_TYPE Release)
  endif()
endif()

#
# Library version
#
set(BRIEFLZ_VERSION_MAJOR 1)
set(BRIEFLZ_VERSION_MINOR 3)
set(BRIEFLZ_VERSION_PATCH 0)
set(BRIEFLZ_VERSION ${BRIEFLZ_VERSION_MAJOR}.${BRIEFLZ_VERSION_MINOR}.${BRIEFLZ_VERSION_PATCH})

if(BRIEFLZ_BUILD_SHARED)
  set(brieflz_library_type SHARED)
else()
  set(brieflz_library_type STATIC)
endif()

#
# brieflz
#
add_library(brieflz ${brieflz_library_type}
  src/brieflz.c
  src/brieflz_btparse.h
  src/brieflz_hashbucket.h
  src/brieflz_lazy.h
  src/brieflz_leparse.h
  src/depack.c
  src/depacks.c
  include/brieflz.h
)
target_include_directories(brieflz
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(brieflz PROPERTIES
  VERSION ${BRIEFLZ_VERSION}
  SOVERSION ${BRIEFLZ_VERSION_MAJOR}
)
if(BRIEFLZ_BUILD_SHARED)
  set_target_properties(brieflz PROPERTIES C_VISIBILITY_PRESET hidden)
  target_compile_definitions(brieflz
    PRIVATE BLZ_DLL_EXPORTS
    PUBLIC BLZ_DLL
  )
endif()
if(NOT CMAKE_VERSION VERSION_LESS 3.1)
  set_target_properties(brieflz PROPERTIES C_STANDARD 99)
endif()

#
# blzpack
#
add_executable(blzpack example/blzpack.c example/parg.c)
target_link_libraries(blzpack brieflz)
if(MSVC)
  target_compile_definitions(blzpack PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

#
# test_brieflz
#
add_executable(test_brieflz test/test_brieflz.c)
target_link_libraries(test_brieflz brieflz)
if(MSVC)
  target_compile_definitions(test_brieflz PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Create aliases
#
# Makes targets available to projects using BriefLZ as a subproject using the
# same names as in the config file package.
if(NOT CMAKE_VERSION VERSION_LESS 3.0)
  add_library(BriefLZ::brieflz ALIAS brieflz)
  add_executable(BriefLZ::blzpack ALIAS blzpack)
endif()

#
# Tests
#
if(BRIEFLZ_BUILD_TESTING)
  add_test("${BRIEFLZ_TEST_PREFIX}brieflz" test_brieflz)
endif()

#
# Install
#
if(BRIEFLZ_BUILD_INSTALL)
  install(TARGETS brieflz blzpack
    EXPORT brieflz_targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(FILES include/brieflz.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  # Install config file package
  #
  # This allows CMake based projects to use the installed libraries with
  # find_package(BriefLZ).
  if(NOT CMAKE_VERSION VERSION_LESS 3.0)
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/BriefLZConfigVersion.cmake
      VERSION ${BRIEFLZ_VERSION}
      COMPATIBILITY SameMajorVersion
    )
    # Since we have no dependencies, use export file directly as config file
    install(EXPORT brieflz_targets
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BriefLZ
      NAMESPACE BriefLZ::
      FILE BriefLZConfig.cmake
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BriefLZConfigVersion.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BriefLZ
    )
  endif()

  # Install pkg-config file
  configure_file(src/brieflz.pc.in brieflz.pc @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/brieflz.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )

  if(BRIEFLZ_BUILD_TESTING)
    install(TARGETS test_brieflz
      RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/installed-tests/brieflz
    )
  endif()
endif()
