cmake_minimum_required(VERSION 3.0.0)
project(myBudget VERSION 0.1.0)

include(CTest)
enable_testing()

set(SRCS
    src/main.c
)

add_executable(myBudget ${SRCS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
