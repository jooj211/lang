cmake_minimum_required(VERSION 3.16)
project(LangCompiler CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
include_directories(src)
set(BISON_OUTPUT_CPP ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp)
set(BISON_OUTPUT_HPP ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.hpp)
add_custom_command(
    OUTPUT ${BISON_OUTPUT_CPP} ${BISON_OUTPUT_HPP}
    COMMAND ${BISON_EXECUTABLE} -o ${BISON_OUTPUT_CPP} -d ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parser.y
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parser.y
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set(FLEX_OUTPUT_CPP ${CMAKE_CURRENT_BINARY_DIR}/lexer.yy.cpp)
add_custom_command(
    OUTPUT ${FLEX_OUTPUT_CPP}
    COMMAND ${FLEX_EXECUTABLE} -o ${FLEX_OUTPUT_CPP} ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/lexer.l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/lexer.l ${BISON_OUTPUT_HPP}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
list(APPEND SRC_FILES ${BISON_OUTPUT_CPP})
list(APPEND SRC_FILES ${FLEX_OUTPUT_CPP})
list(APPEND SRC_FILES src/main.cpp)
list(APPEND SRC_FILES src/interpreter/Interpreter.cpp)
list(APPEND SRC_FILES src/typecheck/TypeChecker.cpp) # <-- ADICIONE ESTA LINHA
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(lang ${SRC_FILES})