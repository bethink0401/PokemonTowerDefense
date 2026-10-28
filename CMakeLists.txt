cmake_minimum_required(VERSION 3.16)

project(PokemonTowerDefense LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 本工程刻意不使用 Q_OBJECT/信号槽（见 README），无需 moc；
# 也避免了 AUTOMOC 对 moc.exe 的环境自检在某些终端下误报 DLL 缺失的问题。
set(CMAKE_AUTOMOC OFF)

# 同时兼容 Qt6 与 Qt5（优先 Qt6）
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE PROJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/entity/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/battle/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/strategy/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/persistence/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp
)

add_library(pokemon_core STATIC ${PROJECT_SOURCES})
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE pokemon_core)

target_include_directories(pokemon_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/battle
    ${CMAKE_CURRENT_SOURCE_DIR}/strategy
    ${CMAKE_CURRENT_SOURCE_DIR}/event
    ${CMAKE_CURRENT_SOURCE_DIR}/persistence
    ${CMAKE_CURRENT_SOURCE_DIR}/template
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

target_link_libraries(pokemon_core PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)

include(CTest)
if(BUILD_TESTING)
    foreach(check map_rules persistence_rules unit_insights position_rules render_layout balance_rules)
        add_executable(${check} tests/${check}.cpp)
        target_link_libraries(${check} PRIVATE pokemon_core)
        add_test(NAME ${check} COMMAND ${check})
        set_tests_properties(${check} PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endforeach()
endif()
