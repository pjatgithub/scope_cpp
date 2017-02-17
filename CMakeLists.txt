cmake_minimum_required(VERSION 3.6)
project(scope)

set(CMAKE_CXX_STANDARD 11)

# Add Java JNI headers
include_directories($ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
# Add NI VISA headers (32bits)
include_directories($ENV{VXIPNPPATH}/WinNT/Include)

set(SOURCE_FILES org_usslab_scope_Scope.h org_usslab_scope_Scope.cpp)
add_library(scope SHARED ${SOURCE_FILES})