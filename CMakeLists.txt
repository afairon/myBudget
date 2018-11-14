cmake_minimum_required(VERSION 3.0.0)
project(myBudget VERSION 0.1.0)

include(CTest)
enable_testing()

set(SQLITE_SOURCE_FILES
    lib/sqlite/sqlite3.c
)

add_library(sqliteModule ${SQLITE_SOURCE_FILES})

set(SRCS
    src/main.c
)

add_executable(myBudget ${SRCS})
target_link_libraries(myBudget sqliteModule)
target_link_libraries(myBudget pthread)
target_link_libraries(myBudget dl)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
