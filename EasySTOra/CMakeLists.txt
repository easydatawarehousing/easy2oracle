# Set target
set(TARGET_NAME "EasySTOra")

# Add libraries
#find_package (Threads REQUIRED)
set(LIBS pthread dl)

# Add include dirs
include_directories ("include")
include_directories ("${CMAKE_SOURCE_DIR}/ETO_Common/include")

# Add sources
set(SRC
    src/sqlite3.c
    src/eto_dbs.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/dictionary.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/iniparser.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_log.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_util.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_main.c
)

# Add executable and dependancies
add_executable (${TARGET_NAME} ${SRC})

#if (Threads_FOUND)
#  target_link_libraries (${TARGET_NAME} ${Threads_LIBRARIES})
#endif (Threads_FOUND)

target_link_libraries (${TARGET_NAME} ${LIBS})
