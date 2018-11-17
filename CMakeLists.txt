cmake_minimum_required(VERSION 3.0.0)
project(myBudget VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories(lib/rxi/log)
include_directories(lib/sqlite)

set(RXI_LOG_SOURCE_FILES
    lib/rxi/log/log.c
)

add_library(rxiModule ${RXI_LOG_SOURCE_FILES})
if (UNIX)
    target_compile_definitions(rxiModule PUBLIC -DLOG_USE_COLOR)
endif()

set(SQLITE_SOURCE_FILES
    lib/sqlite/sqlite3.c
)

add_library(sqliteModule ${SQLITE_SOURCE_FILES})

set(SRCS
    src/main.c
    src/db.c
)

add_executable(myBudget ${SRCS})
target_link_libraries(myBudget sqliteModule)
target_link_libraries(myBudget rxiModule)
if (UNIX)
    target_link_libraries(myBudget pthread)
    target_link_libraries(myBudget dl)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
