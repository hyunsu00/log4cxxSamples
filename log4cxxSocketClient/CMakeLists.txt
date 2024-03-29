# CMakeList.txt: log4cxxSocketServer 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#
cmake_minimum_required (VERSION 2.8)

project("log4cxxSocketClient")
message(STATUS "\${PROJECT_NAME} = ${PROJECT_NAME}")

###
# pkg-config를 사용하여 log4cxx 패키지 찾기
# sudo apt install liblog4cxx-dev -y (데비안)
# sudo yum install liblog4cxx-devel -y (센트OS)
# 
find_package(PkgConfig REQUIRED)
pkg_check_modules(LOG4CXX liblog4cxx REQUIRED)
message(STATUS "\${LOG4CXX_LIBRARIES} = ${LOG4CXX_LIBRARIES}")
message(STATUS "\${LOG4CXX_INCLUDE_DIRS} = ${LOG4CXX_INCLUDE_DIRS}")
include_directories(${LOG4CXX_INCLUDE_DIRS})

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(TARGET_PLATFORM "x64")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(TARGET_PLATFORM "x86")
endif()
message(STATUS "\${TARGET_PLATFORM} = ${TARGET_PLATFORM}")

###
# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (${PROJECT_NAME} "log4cxxSocketClient.cpp")

###
# 실행파일 생성후에 지정
target_link_libraries(${PROJECT_NAME} ${LOG4CXX_LIBRARIES})

###
# C++ 11 사용
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")

add_custom_command(
	TARGET ${PROJECT_NAME} PRE_BUILD
	COMMENT "conf 파일을 복사합니다."
	COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/log4cxxSocketClient.conf" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
)
message(STATUS "\${CMAKE_COMMAND} = ${CMAKE_COMMAND}")

###
# 인스톨
install(TARGETS ${PROJECT_NAME} DESTINATION ${PROJECT_NAME})
install(FILES "${PROJECT_SOURCE_DIR}/log4cxxSocketClient.conf" DESTINATION ${PROJECT_NAME})

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
