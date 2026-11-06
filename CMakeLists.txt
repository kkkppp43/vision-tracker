cmake_minimum_required(VERSION 3.16)

project(vision_tracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## 给 clangd / VSCode IntelliSense 生成 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(tracking
  src/TrackingBase/TrackingBase.cpp
  src/ArmorTarget/ArmorTarget.cpp
  src/SpinningTarget/SpinningTarget.cpp
  src/TargetManager/TargetManager.cpp
  include/TrackingBase/TrackingBase.hpp
  include/ArmorTarget/ArmorTarget.hpp
  include/SpinningTarget/SpinningTarget.hpp
  include/TargetManager/TargetManager.hpp
  include/data_types.hpp
)
target_include_directories(tracking PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(tracker src/main.cpp include/test_frames.hpp)
target_link_libraries(tracker PRIVATE tracking)
