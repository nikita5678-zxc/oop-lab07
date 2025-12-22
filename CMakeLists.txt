cmake_minimum_required(VERSION 3.14)
project(NPC_Dungeon_Editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS
    include/npc_system.hpp
)

set(SOURCES
    src/npc_system.cpp
)

add_executable(main
    main.cpp
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(tests
    tests/test.cpp
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(tests PRIVATE gtest gtest_main)