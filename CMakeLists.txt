cmake_minimum_required(VERSION 3.10)
project(Mnsx_AchillesLink LANGUAGES CXX)

# ==========================================
# 全局设置
# ==========================================
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# ==========================================
# 引入基建 (强制依赖)
# ==========================================
if(NOT TARGET ArgusLog)
    add_subdirectory(third_party/argus_log)
endif()

# ==========================================
# 定义核心库 (AchillesLink)
# ==========================================
file(GLOB_RECURSE LIB_SRC_FILES "src/*.cpp")
add_library(AchillesLink STATIC ${LIB_SRC_FILES})

target_include_directories(AchillesLink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(AchillesLink PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(AchillesLink PRIVATE -fexec-charset=UTF-8)
endif()

target_link_libraries(AchillesLink PUBLIC Threads::Threads ArgusLog)

# ==========================================
# 示例程序 (Demo)
# ==========================================
add_executable(AchillesDemo examples/BasicUsage.cpp)

target_link_libraries(AchillesDemo PRIVATE AchillesLink)