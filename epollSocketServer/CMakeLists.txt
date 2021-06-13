# CMakeList.txt: pdfiumSamples에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#
cmake_minimum_required (VERSION 2.8)

project ("epollSocketServer")
message(STATUS "\${PROJECT_NAME} = ${PROJECT_NAME}")

###
# 패키지 찾기 - log4cxxConfig.cmake 파일 인 클루드
# sudo apt install liblog4cxx-dev -y (데비안)
# sudo yum install liblog4cxx-devel -y (센트OS)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
message(STATUS "\${CMAKE_MODULE_PATH} = ${CMAKE_MODULE_PATH}")
find_package(log4cxx REQUIRED)

###
#
set (COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common)
message(STATUS "\${COMMON_DIR} = ${COMMON_DIR}")
include_directories(${COMMON_DIR})

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(TARGET_PLATFORM "x64")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(TARGET_PLATFORM "x86")
endif()
message(STATUS "\${TARGET_PLATFORM} = ${TARGET_PLATFORM}")

###
# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (${PROJECT_NAME} "epollSocketServer.cpp" "${COMMON_DIR}/ObjectLoader.cpp")

###
# 실행파일 생성후에 지정
target_link_libraries(${PROJECT_NAME} log4cxx)

add_custom_command(
	TARGET ${PROJECT_NAME} PRE_BUILD
	COMMENT "conf 파일을 복사합니다."
	COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/epollSocketServer.conf" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
	COMMENT "samples 폴더를 복사합니다."
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/../samples" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/samples/"
)
message(STATUS "\${CMAKE_COMMAND} = ${CMAKE_COMMAND}")