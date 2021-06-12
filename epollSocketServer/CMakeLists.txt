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
# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (${PROJECT_NAME} "epollSocketServer.cpp" "ObjectFactory.cpp")

###
# 실행파일 생성후에 지정
target_link_libraries(${PROJECT_NAME} log4cxx)

add_custom_command(
	TARGET ${PROJECT_NAME} PRE_BUILD
	COMMENT "conf 파일을 복사합니다."
	COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/log4cxx.conf" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
)
message(STATUS "\${CMAKE_COMMAND} = ${CMAKE_COMMAND}")