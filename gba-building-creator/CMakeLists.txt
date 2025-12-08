# CMakeList.txt: CMake-Projekt für "gba-building-creator". Schließen Sie die Quelle ein, und definieren Sie
# projektspezifische Logik hier.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

add_library(gba-building-creator SHARED
            gbaBuildingCreator.cpp
            gbaBuildingCreator.h
            BuildingCreator.cpp
            BuildingCreator.h
            PluginObserver.cpp
            PluginObserver.h
            BuildingParameter.h)

target_link_directories(gba-building-creator PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(gba-building-creator PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_options(gba-building-creator PRIVATE /utf-8)

target_link_libraries(gba-building-creator PRIVATE ifcexplorer_plugin stdutils ifcdb gps)

set_target_properties(gba-building-creator PROPERTIES VCPKG_MANIFEST_MODE TRUE)
