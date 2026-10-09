cmake_minimum_required(VERSION 3.24)

project(DataIOS LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The real desktop renderer is reused; DB_USE_GL_SHIM routes its fixed-function
# GL calls into GLShim.cpp, which main.mm presents through Metal.
add_executable(DataIOS MACOSX_BUNDLE
    main.mm
    GLShim.cpp
    TouchOverlay.cpp
    ../desktop/DesktopRenderer.cpp
    ../game/Game.cpp
)

target_include_directories(DataIOS PRIVATE
    .
    ..
    ../game
    ../desktop
    ../desktop/third_party/stb
)

target_compile_definitions(DataIOS PRIVATE DB_USE_GL_SHIM)

# Models, fonts and TV clips are bundled under assets/ with the layout
# DesktopRenderer::setAssetRoot expects (assets/models, assets/fonts, assets/tv-gifs).
set(DB_IOS_ASSET_DIRS
    "models|${CMAKE_CURRENT_SOURCE_DIR}/../models"
    "fonts|${CMAKE_CURRENT_SOURCE_DIR}/../desktop/fonts"
    "tv-gifs|${CMAKE_CURRENT_SOURCE_DIR}/../tv"
)
foreach(entry IN LISTS DB_IOS_ASSET_DIRS)
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 name)
    list(GET parts 1 dir)
    file(GLOB asset_files CONFIGURE_DEPENDS "${dir}/*")
    foreach(file IN LISTS asset_files)
        if(NOT IS_DIRECTORY "${file}")
            target_sources(DataIOS PRIVATE "${file}")
            set_source_files_properties("${file}" PROPERTIES MACOSX_PACKAGE_LOCATION "assets/${name}")
        endif()
    endforeach()
endforeach()

target_link_libraries(DataIOS PRIVATE
    "-framework UIKit"
    "-framework Metal"
    "-framework MetalKit"
    "-framework QuartzCore"
    "-framework CoreGraphics"
)

set_target_properties(DataIOS PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.indrolend.data.ios"
    MACOSX_BUNDLE_BUNDLE_NAME "DATA"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.indrolend.data.ios"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "17.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
)
