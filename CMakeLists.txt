CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
project(DEMO)

SET(CMAKE_BUILD_TYPE "DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#include
INCLUDE_DIRECTORIES(/usr/local/systemc-2.3.3/include)   #根据自己安装的systemc路径更改
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

#lib path
LINK_DIRECTORIES(/usr/local/systemc-2.3.3/lib)  #根据自己安装的systemc路径更改

#exec
ADD_EXECUTABLE(demo ${PROJECT_SOURCE_DIR}/learn_SystemC_with_Examples_CPlusPlus_Type/demo.cpp)
TARGET_LINK_LIBRARIES(demo 
                    -Wl,--start-group                
                    systemc
                    -Wl,--end-group)
