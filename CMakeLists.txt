cmake_minimum_required(VERSION 2.6)
SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_CXX_COMPILER "clang++")

project(rvmparser)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
message("CMAKE_LIBRARY_OUTPUT_DIRECTORY: " ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
include_directories( 
  "${CMAKE_SOURCE_DIR}/src" 
  "${CMAKE_SOURCE_DIR}/libs/libtess2/Include" 
  "${CMAKE_SOURCE_DIR}/libs/libtess2/Source" 
  "${CMAKE_SOURCE_DIR}/libs/rapidjson/include" 
  "${CMAKE_SOURCE_DIR}/libs/cork" 
)
link_directories(
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
  "${CMAKE_SOURCE_DIR}/libs/cork" 
)

file(GLOB SOURCES
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_SOURCE_DIR}/libs/libtess2/Source/*.c" 
)
list(REMOVE_ITEM SOURCES
  "${CMAKE_SOURCE_DIR}/src/main.cpp"
  "${CMAKE_SOURCE_DIR}/src/ExportJson.cpp"
)


add_library (${PROJECT_NAME} STATIC ${SOURCES})
add_executable(${PROJECT_NAME}test "${CMAKE_SOURCE_DIR}/src/main.cpp")
target_link_libraries(${PROJECT_NAME}test  -l${PROJECT_NAME} -static -static-libgcc -static-libstdc++ -lpthread -lcork -lmpir)
