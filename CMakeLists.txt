cmake_minimum_required(VERSION 3.12)
project(IPCCliChallenge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

set(LINK_LIBRARIES Threads::Threads)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND LINK_LIBRARIES rt)
endif()

add_executable(producer producer.cpp)
add_executable(consumer consumer.cpp)
target_link_libraries(producer PRIVATE ${LINK_LIBRARIES})
target_link_libraries(consumer PRIVATE ${LINK_LIBRARIES})
