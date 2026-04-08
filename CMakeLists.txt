cmake_minimum_required(VERSION 3.10)
project(Mnsx-AchillesLink LANGUAGES CXX)

# 全局设置
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexec-charset=UTF-8")

# 定义核心库
 add_library(AchillesLink STATIC
         src/net/InetAddress.cpp
         src/net/Socket.cpp
 )

# 配置库的头文件路径
include_directories(AchillesLink PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 定义实例程序
add_executable(AchillesDemo examples/BasicUsage.cpp)

# 建立连接关系
target_link_libraries(AchillesDemo PRIVATE AchillesLink)

if(WIN32)
 target_link_libraries(AchillesLink PUBLIC ws2_32)
endif()