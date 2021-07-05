# CMakeList.txt
cmake_minimum_required (VERSION 2.8)

#
# 프로젝트 이름 지정
project ("log4cxxSample")
message(STATUS "\${PROJECT_NAME} = ${PROJECT_NAME}")

option(BUILD_SOCKET_CLIENT "Enable building log4cxxSocketClient" ON)
option(BUILD_SOCKET_SERVER "Enable building log4cxxSocketServer" ON)
option(BUILD_SECECT_SOCKET_SERVER "Enable building log4cxxSelectSocketServer" ON)
option(BUILD_EPOLL_SOCKET_SERVER "Enable building log4cxxEpollSocketClient" ON)
option(BUILD_POLL_SOCKET_SERVER "Enable building log4cxxPollSocketClient" ON)

if(BUILD_SOCKET_CLIENT)
    add_subdirectory(log4cxxSocketClient)
endif()
if(BUILD_SOCKET_SERVER)
    add_subdirectory(log4cxxSocketServer)
endif()
if(BUILD_SECECT_SOCKET_SERVER)
    add_subdirectory(log4cxxSelectSocketServer)
endif()
if(BUILD_POLL_SOCKET_SERVER)
    add_subdirectory(log4cxxPollSocketServer)
endif()
if(BUILD_EPOLL_SOCKET_SERVER)
    add_subdirectory(log4cxxEpollSocketServer)
endif()

set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
install(FILES ${CPACK_RESOURCE_FILE_README} DESTINATION .)
