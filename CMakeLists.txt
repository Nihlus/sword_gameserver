cmake_minimum_required(VERSION 3.6)
project(Sword_GameServer)

set(${CMAKE_PROJECT_NAME}_VERSION_MAJOR 1)
set(${CMAKE_PROJECT_NAME}_VERSION_MINOR 0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_compile_options(-std=gnu++14 -Wno-c++11-narrowing)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../OpenCLRenderer/cmake_modules" ${CMAKE_MODULE_PATH})

find_package(Threads)
find_package(SFML 2 REQUIRED system)

set(SWORD_GAME_SERVER_SOURCES
		${PROJECT_SOURCE_DIR}/game_modes.cpp
		${PROJECT_SOURCE_DIR}/game_state.cpp
		${PROJECT_SOURCE_DIR}/main.cpp
		${PROJECT_SOURCE_DIR}/../OpenCLRenderer/logging.cpp
		${PROJECT_SOURCE_DIR}/../SwordFight/sword_server/reliability_shared.cpp)

set(SWORD_GAME_SERVER_HEADERS
		${PROJECT_SOURCE_DIR}/game_modes.hpp
		${PROJECT_SOURCE_DIR}/game_state.hpp
		${PROJECT_SOURCE_DIR}/../sword_masterserver/network_messages.hpp
		${PROJECT_SOURCE_DIR}/../SwordFight/sword_server/reliability_shared.hpp)

include_directories(
		${PROJECT_SOURCE_DIR}/
		${PROJECT_SOURCE_DIR}/../libraries
)

add_executable(${PROJECT_NAME} ${SWORD_GAME_SERVER_SOURCES} ${SWORD_GAME_SERVER_HEADERS})

target_link_libraries(
		${PROJECT_NAME}
		${SFML_LIBRARIES}
		${CMAKE_THREAD_LIBS_INIT})