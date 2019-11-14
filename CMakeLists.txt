cmake_minimum_required(VERSION 3.0)

# Nom du projet
project(SugarLibRetro)

set (USE_SHARED_CRT OFF)


set (CPCCORE			${PROJECT_SOURCE_DIR}/CPCCore/CPCCoreEmu 		CACHE STRING "CPCCoreEmu folder"		FORCE)
set (ZLIB				${PROJECT_SOURCE_DIR}/CPCCore/zlib 		CACHE STRING "zlib folder"		FORCE)

add_subdirectory(${CPCCORE})
add_subdirectory(${ZLIB})

add_subdirectory(SugarLibRetro)


