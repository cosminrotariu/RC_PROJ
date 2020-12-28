cmake_minimum_required(VERSION 3.19)
project(marele_zid_chinezesc)

add_executable(client client.c shared.h)
target_compile_options(client PRIVATE -Wall -pedantic -Werror=discarded-qualifiers)

add_executable(server server.c sqlite/sqlite3.c)

target_compile_options(server PRIVATE -Wall -pedantic -Werror=discarded-qualifiers)
target_include_directories(server PRIVATE sqlite)

find_package(Threads)
target_link_libraries(server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(server ${CMAKE_DL_LIBS})