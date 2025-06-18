project(tiger-compiler)

cmake_minimum_required(VERSION 3.2)

if ("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif ()

find_program(FLEX flex)
if (NOT FLEX)
    message(FATAL_ERROR "Program flex not found!")
endif ()

find_program(BISON bison)
if (NOT BISON)
    message(FATAL_ERROR "Program bison not found!")
endif ()

set(CMAKE_CXX_STANDARD 17)

include_directories(src)
include_directories(src/tiger/lex)
include_directories(src/tiger/parse)
include_directories(${PROJECT_SOURCE_DIR}/src/tiger/parse)

file(GLOB SLP_SOURCES "src/straightline/*.cc")

file(GLOB TIGER_SOURCES
        "src/tiger/symbol/*.cc"
        "src/tiger/absyn/*.cc"
        "src/tiger/errormsg/*.cc"
        "src/tiger/env/*.cc"
        "src/tiger/escape/*.cc"
        "src/tiger/semant/*.cc"
        "src/tiger/frame/*.cc"
        "src/tiger/translate/*.cc"
        "src/tiger/canon/*.cc"
        "src/tiger/codegen/*.cc"
        "src/tiger/liveness/*.cc"
        "src/tiger/regalloc/*.cc"
        "src/tiger/output/*.cc"
        )

SET(TIGER_LEX_PARSE_SOURCES
        ${PROJECT_SOURCE_DIR}/src/tiger/lex/lex.yy.cc
        ${PROJECT_SOURCE_DIR}/src/tiger/parse/parse.tab.cc
        ${PROJECT_SOURCE_DIR}/src/tiger/parse/parse.tab.hh
        )

SET_SOURCE_FILES_PROPERTIES(${TIGER_LEX_PARSE_SOURCES} GENERATED)

# Create custom command for flex
add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/src/tiger/lex/lex.yy.cc
        COMMAND flex -o lex.yy.cc tiger.lex
        DEPENDS ${PROJECT_SOURCE_DIR}/src/tiger/lex/tiger.lex
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/tiger/lex
)

# Create custom command for bison
add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/src/tiger/parse/parse.tab.cc ${PROJECT_SOURCE_DIR}/src/tiger/parse/parse.tab.hh
        COMMAND bison -d -o parse.tab.cc tiger.y
        DEPENDS ${PROJECT_SOURCE_DIR}/src/tiger/parse/tiger.y
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/tiger/parse
)

# Create target for the lexer and the parser
add_custom_target(
        lex_parse_sources
        DEPENDS ${TIGER_LEX_PARSE_SOURCES}
)

# lab 1
add_executable(test_slp ${SLP_SOURCES})

# lab 2
add_executable(test_lex "src/tiger/main/test_lex.cc" ${TIGER_SOURCES} ${TIGER_LEX_PARSE_SOURCES})
add_dependencies(test_lex lex_parse_sources)

# lab 3
add_executable(test_parse "src/tiger/main/test_parse.cc" ${TIGER_SOURCES} ${TIGER_LEX_PARSE_SOURCES})
add_dependencies(test_parse lex_parse_sources)

# lab 4
add_executable(test_semant "src/tiger/main/test_semant.cc" ${TIGER_SOURCES} ${TIGER_LEX_PARSE_SOURCES})
add_dependencies(test_semant lex_parse_sources)

# lab5 part 1
add_executable(test_translate "src/tiger/main/test_translate.cc" ${TIGER_SOURCES} ${TIGER_LEX_PARSE_SOURCES})
add_dependencies(test_translate lex_parse_sources)

# lab 5 part 2
add_executable(test_codegen "src/tiger/main/test_codegen.cc" ${TIGER_SOURCES} ${TIGER_LEX_PARSE_SOURCES})
add_dependencies(test_codegen lex_parse_sources)

# lab 6
add_executable(tiger-compiler "src/tiger/main/main.cc" ${TIGER_SOURCES} ${TIGER_LEX_PARSE_SOURCES})
add_dependencies(tiger-compiler lex_parse_sources)
