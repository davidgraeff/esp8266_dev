cmake_minimum_required(VERSION 3.0)

project(pc_simpleudp)
add_definitions(-Wall -Werror -std=gnu11 -DDEBUG=1)
file(GLOB SRC "*.c")
add_executable(pc_simpleudp "${SRC}")