CMAKE_MINIMUM_REQUIRED(VERSION 3.1...)

SET(CMAKE_BUILD_TYPE "DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

#include
INCLUDE_DIRECTORIES(/usr/local/systemc-2.3.1/include)   #根据自己安装的systemc路径更改

#lib path
LINK_DIRECTORIES(/usr/local/systemc-2.3.1/lib-linux64)  #根据自己安装的systemc路径更改

#exec
ADD_EXECUTABLE(demo ${PROJECT_SOURCE_DIR}/learn_SystemC_with_Examples_CPlusPlus_Type/demo.cpp)
TARGET_LINK_LIBRARIES(demo 
                    -Wl,--start-group                
                    systemc
                    -Wl,--end-group)
