cmake_minimum_required(VERSION 2.6)
project(tomonoid)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(tomonoid tomonoid_assigning.cpp tomonoid_reader.cpp tomonoid.cpp element_creator.cpp table_element.cpp element.cpp main.cpp tomonoid_generating.cpp)


find_package (Threads)
target_link_libraries (tomonoid ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS tomonoid RUNTIME DESTINATION bin)
