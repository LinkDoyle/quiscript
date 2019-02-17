cmake_minimum_required (VERSION 3.10)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

### Boost ###
find_package(Boost 1.68 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

### Google Test ###
set(GOOGLETEST_VERSION 1.9.0)
# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
enable_testing()
include(CMakeDependentOption)
include(GNUInstallDirs)
add_subdirectory(thirdparty/googletest)
set_target_properties(gtest PROPERTIES FOLDER "ThirdParty")
set_target_properties(gtest_main PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock_main PROPERTIES FOLDER "ThirdParty")

### LLVM ###
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
set_target_properties(intrinsics_gen PROPERTIES FOLDER "ThirdParty")

project(QUIScript)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include_directories(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(demo)
