cmake_minimum_required(VERSION 3.10)
project(microcli_examples)

add_executable(interpreter_demo interpreter_demo.c ../microcli.c)
target_include_directories(interpreter_demo PRIVATE ../)
