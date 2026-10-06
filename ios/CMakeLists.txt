cmake_minimum_required(VERSION 3.24)

project(DataIOS LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(DataIOS MACOSX_BUNDLE
    main.mm
    ../game/Game.cpp
)

target_include_directories(DataIOS PRIVATE
    ..
    ../game
)

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
