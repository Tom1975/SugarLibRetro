cmake_minimum_required(VERSION 3.0)
project(SugarLibRetro)

file( GLOB SRCS
	"*.cpp"
	)

file( GLOB HEADERS
	"*.h"
)

add_library(SugarLibRetro SHARED ${SRCS} ${HEADERS}  )
target_include_directories( SugarLibRetro PUBLIC ${CPCCORE})
set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )

set(CMAKE_CXX_STANDARD 17)
 
IF(MSVC)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")  
ENDIF(MSVC)

if(CMAKE_COMPILER_IS_GNUCC)
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z )
endif()

foreach(CompilerFlag ${CompilerFlags})
	string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

IF(CMAKE_BUILD_TYPE MATCHES "Debug")
include_directories (SugarLibRetro
					 ${ZLIBDIR}/lib
                ${CpcCoreEmu_BINARY_DIR}/Debug
					 )
ELSE(CMAKE_BUILD_TYPE MATCHES "Release")
include_directories (SugarLibRetro
					 ${ZLIBDIR}/lib
                ${CpcCoreEmu_BINARY_DIR}/Release
					 )
ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")

if(MSVC)
	target_link_libraries ( SugarLibRetro zlibstatic CPCCoreEmu)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")  
	ADD_DEFINITIONS(/arch:SSE2)
else ()
	target_link_libraries ( SugarLibRetro  m stdc++fs CPCCoreEmu zlib)
endif()

