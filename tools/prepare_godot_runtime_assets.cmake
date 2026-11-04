cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED DB_SOURCE_ROOT)
    get_filename_component(DB_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

set(source_root "${DB_SOURCE_ROOT}/models")
set(destination_root "${DB_SOURCE_ROOT}/godot/runtime-assets/models")
if(NOT EXISTS "${source_root}/phone.dbmesh" OR NOT EXISTS "${source_root}/human.dbhuman")
    message(FATAL_ERROR "DB_SOURCE_ROOT does not contain the authoritative models tree: ${DB_SOURCE_ROOT}")
endif()

set(runtime_assets
    phone.dbmesh
    human.dbhuman
    materials/photo-stone-runtime.rgb
    materials/photo-metal-runtime.rgb
    materials/cc0-fir-foliage.rgba
    environment/common-tree.dbmesh
    environment/twisted-tree.dbmesh
    environment/dead-tree.dbmesh
    environment/pine-tree.dbmesh
    environment/city/cornice.dbmesh
    environment/city/crown.dbmesh
    environment/city/door-bay.dbmesh
    environment/city/door-window-bay.dbmesh
    environment/city/wall-corner.dbmesh
    environment/city/wall-door-bay.dbmesh
    environment/city/wall-window-bay.dbmesh
    environment/city/window-bay-small.dbmesh
    environment/city/window-bay.dbmesh)

set(manifest "schema=digital-breakdown.godot-runtime-assets/v1\n")
foreach(relative_path IN LISTS runtime_assets)
    set(source "${source_root}/${relative_path}")
    set(destination "${destination_root}/${relative_path}")
    if(NOT EXISTS "${source}")
        message(FATAL_ERROR "Required Godot runtime asset is missing: ${source}")
    endif()
    get_filename_component(destination_directory "${destination}" DIRECTORY)
    file(MAKE_DIRECTORY "${destination_directory}")
    file(COPY_FILE "${source}" "${destination}" ONLY_IF_DIFFERENT)
    file(SHA256 "${source}" source_hash)
    file(SHA256 "${destination}" destination_hash)
    if(NOT source_hash STREQUAL destination_hash)
        message(FATAL_ERROR "Staged asset differs from its authority: ${relative_path}")
    endif()
    file(SIZE "${source}" source_size)
    string(APPEND manifest "${relative_path}|${source_size}|${source_hash}\n")
endforeach()

set(font_relative_path "fonts/SourceSans3-Semibold.ttf")
set(font_source "${DB_SOURCE_ROOT}/godot/${font_relative_path}")
set(font_destination "${DB_SOURCE_ROOT}/godot/runtime-assets/${font_relative_path}")
if(NOT EXISTS "${font_source}")
    message(FATAL_ERROR "Required native phone font is missing: ${font_source}")
endif()
get_filename_component(font_destination_directory "${font_destination}" DIRECTORY)
file(MAKE_DIRECTORY "${font_destination_directory}")
file(COPY_FILE "${font_source}" "${font_destination}" ONLY_IF_DIFFERENT)
file(SHA256 "${font_source}" font_source_hash)
file(SHA256 "${font_destination}" font_destination_hash)
if(NOT font_source_hash STREQUAL font_destination_hash)
    message(FATAL_ERROR "Staged font differs from its authority: ${font_relative_path}")
endif()
file(SIZE "${font_source}" font_source_size)
string(APPEND manifest "${font_relative_path}|${font_source_size}|${font_source_hash}\n")

file(WRITE "${DB_SOURCE_ROOT}/godot/runtime-assets/manifest.txt" "${manifest}")
list(LENGTH runtime_assets runtime_asset_count)
math(EXPR runtime_asset_count "${runtime_asset_count} + 1")
message(STATUS "GODOT_RUNTIME_ASSETS_OK count=${runtime_asset_count} destination=${destination_root}")
