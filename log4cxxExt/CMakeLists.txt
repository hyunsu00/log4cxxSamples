# CMakeList.txt: log4cxxSocketServer 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#
cmake_minimum_required (VERSION 2.8)

project("log4cxxExt")
message(STATUS "\${PROJECT_NAME} = ${PROJECT_NAME}")

###
# pkg-config를 사용하여 log4cxx 패키지 찾기
# sudo apt install pkg-config -y (데비안)
# sudo apt install liblog4cxx-dev -y (데비안)
# sudo yum install liblog4cxx-devel -y (센트OS)
# 
find_package(PkgConfig REQUIRED)
pkg_check_modules(LOG4CXX liblog4cxx REQUIRED)
message(STATUS "\${LOG4CXX_LIBRARIES} = ${LOG4CXX_LIBRARIES}")
message(STATUS "\${LOG4CXX_INCLUDE_DIRS} = ${LOG4CXX_INCLUDE_DIRS}")
# include_directories(${LOG4CXX_INCLUDE_DIRS})

# sudo apt install libmsgpack-dev (데비안)
find_package(Msgpack REQUIRED)
message(STATUS "\${MSGPACK_LIBRARIES} = ${MSGPACK_LIBRARIES}")
message(STATUS "\${MSGPACK_INCLUDE_DIRS} = ${MSGPACK_INCLUDE_DIRS}")
#include_directories(${MSGPACK_INCLUDE_DIRS})

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(TARGET_PLATFORM "x64")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(TARGET_PLATFORM "x86")
endif()
message(STATUS "\${TARGET_PLATFORM} = ${TARGET_PLATFORM}")

###
#
set (LOADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Loader)
message(STATUS "\${LOADER_DIR} = ${LOADER_DIR}")
# target_include_directories 사용으로 불필요
# include_directories(${LOADER_DIR})

set (SOCKET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Socket)
message(STATUS "\${SOCKET_DIR} = ${SOCKET_DIR}")
# target_include_directories 사용으로 불필요
# include_directories(${SOCKET_DIR})

###
# 소스 파일 설정
set(LOADER_FILES 
	"${LOADER_DIR}/BytesObjectLoader.cpp" 
	"${LOADER_DIR}/BytesObjectLoader.h" 
	"${LOADER_DIR}/DefaultByteBufInputStream.cpp" 
	"${LOADER_DIR}/DefaultByteBufInputStream.h" 
	"${LOADER_DIR}/DefaultInputStreamDef.h" 
	"${LOADER_DIR}/DefaultObjectLoader.cpp" 
	"${LOADER_DIR}/DefaultObjectLoader.h" 
	"${LOADER_DIR}/DefaultSocketInputStream.cpp" 
	"${LOADER_DIR}/DefaultSocketInputStream.h"
	"${LOADER_DIR}/Exceptions.cpp"
	"${LOADER_DIR}/Exceptions.h"
	"${LOADER_DIR}/FileLoader.cpp"
	"${LOADER_DIR}/FileLoader.h"
	"${LOADER_DIR}/LoggingEventEx.cpp"
	"${LOADER_DIR}/LoggingEvnetEx.h"
	"${LOADER_DIR}/MsgpackObjectLoader.cpp"
	"${LOADER_DIR}/MsgpackObjectLoader.h"
	"${LOADER_DIR}/ObjectLoader.cpp"
	"${LOADER_DIR}/ObjectLoader.h"
)
message(STATUS "\${LOADER_FILES} = ${LOADER_FILES}")

set(SOCKET_FILES 
	"${SOCKET_DIR}/log4cxxClient.cpp" 
	"${SOCKET_DIR}/log4cxxClient.h" 
	"${SOCKET_DIR}/log4cxxSocket.h"
)
message(STATUS "\${SOCKET_FILES} = ${SOCKET_FILES}")

###
# 이 프로젝트의 스태틱 라이브러리에 소스를 추가합니다.
add_library(${PROJECT_NAME} STATIC ${LOADER_FILES} ${SOCKET_FILES})

###
# 라이브러리 생성후에 지정
target_include_directories(${PROJECT_NAME} PUBLIC ${LOADER_DIR} ${SOCKET_DIR})
target_link_libraries(${PROJECT_NAME} ${LOG4CXX_LIBRARIES} ${MSGPACK_LIBRARIES})

###
# C++ 11 사용
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")

###
# 인스톨
install(TARGETS ${PROJECT_NAME} DESTINATION ${PROJECT_NAME})

###
# cpack
set(_fmt TGZ)
if(WIN32)
  set(_fmt ZIP)
endif()

# 인스톨 파일 압축
set(CPACK_GENERATOR ${_fmt})
# 소스 파일 압축
# set(CPACK_SOURCE_GENERATOR ${_fmt})
set(CPACK_PACKAGE_VENDOR "Hancom")
set(CPACK_PACKAGE_CONTACT "Kim HyunSu")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")

message(STATUS "\${CPACK_OUTPUT_FILE_PREFIX} = ${CPACK_OUTPUT_FILE_PREFIX}")
message(STATUS "\${CPACK_PACKAGE_DIRECTORY} = ${CPACK_PACKAGE_DIRECTORY}")
message(STATUS "\${CPACK_PACKAGE_VERSION} = ${CPACK_PACKAGE_VERSION}")
message(STATUS "\${CPACK_PROJECT_NAME} = ${CPACK_PROJECT_NAME}")
message(STATUS "\${CPACK_PROJECT_VERSION} = ${CPACK_PROJECT_VERSION}")
