cmake_minimum_required(VERSION 3.10)
project(hook_system)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")

add_executable(${PROJECT_NAME} src/main.cpp
  minhook/src/buffer.c
  minhook/src/hook.c
  minhook/src/trampoline.c
  minhook/src/hde/hde32.c
  minhook/src/hde/hde64.c
)

target_include_directories(${PROJECT_NAME} PRIVATE minhook/include)