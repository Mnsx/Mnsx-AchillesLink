cmake_minimum_required(VERSION 3.10)
project(Mnsx-AchillesLink LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexec-charset=UTF-8")

file(GLOB_RECURSE LIB_SRC_FILES "src/*.cpp")

add_library(AchillesLink STATIC ${LIB_SRC_FILES})

target_include_directories(AchillesLink PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(AchillesLink PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(AchillesLink PUBLIC pthread)

# 示例程序
add_executable(AchillesDemo examples/BasicUsage.cpp)
target_link_libraries(AchillesDemo PRIVATE AchillesLink)