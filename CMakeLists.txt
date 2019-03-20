cmake_minimum_required(VERSION 2.6)
SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_CXX_COMPILER "clang++")

project(rvmparser)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
message(STATUS "projectdir: ${CMAKE_SOURCE_DIR}")
include_directories( 
  "${CMAKE_SOURCE_DIR}/src" 
  "${CMAKE_SOURCE_DIR}/libs/libtess2/Include" 
  "${CMAKE_SOURCE_DIR}/libs/libtess2/Source" 
  "${CMAKE_SOURCE_DIR}/libs/rapidjson/include" 
)
link_directories(
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

file(GLOB SOURCES
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_SOURCE_DIR}/libs/libtess2/Source/*.c" 
)
list(REMOVE_ITEM SOURCES
  "${CMAKE_SOURCE_DIR}/src/main.cpp"
  "${CMAKE_SOURCE_DIR}/src/ExportJson.cpp"
)


add_library (${PROJECT_NAME} SHARED ${SOURCES})
add_executable(${PROJECT_NAME}test "${CMAKE_SOURCE_DIR}/src/main.cpp")
target_link_libraries(${PROJECT_NAME}  -static-libgcc -static-libstdc++)
target_link_libraries(${PROJECT_NAME}test  -l${PROJECT_NAME})