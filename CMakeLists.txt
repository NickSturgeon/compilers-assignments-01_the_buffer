cmake_minimum_required(VERSION 3.14)
project(01_the_buffer C)

set(CMAKE_C_STANDARD 90)

set(CMAKE_C_FLAGS "-pedantic -Wall -Wextra")

add_executable(01_the_buffer platy_bt.c buffer.c)