cmake_minimum_required(VERSION 3.22)
project(DigitalBreakdownGodot LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(parent_reference tests/parent_reference.cpp parent/game/Game.cpp)
target_include_directories(parent_reference PRIVATE parent/game)
# Supply a matching checkout of godot-cpp using -DGODOT_CPP_DIR=...
if(NOT GODOT_CPP_DIR)
  message(FATAL_ERROR "Set -DGODOT_CPP_DIR to the official godot-cpp checkout matching your Godot 4 version")
endif()
add_subdirectory("${GODOT_CPP_DIR}" "${CMAKE_BINARY_DIR}/godot-cpp")
add_library(digital_breakdown_parent SHARED
  native/register_types.cpp native/parent_game_node.cpp parent/game/Game.cpp)
target_include_directories(digital_breakdown_parent PRIVATE native parent/game)
target_link_libraries(digital_breakdown_parent PRIVATE godot-cpp)
set_target_properties(digital_breakdown_parent PROPERTIES
  PREFIX "" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
 string(TOUPPER "${config}" uc)
 set_target_properties(digital_breakdown_parent PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY_${uc} "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin"
   LIBRARY_OUTPUT_DIRECTORY_${uc} "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin")
endforeach()
