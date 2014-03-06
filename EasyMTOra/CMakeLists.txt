# Set target
set(TARGET_NAME "EasyMTOra")

# Add libraries
find_package (MySQL REQUIRED)

# Add include dirs
if (MySQL_FOUND)
include_directories (${MySQL_INCLUDES})
endif (MySQL_FOUND)

include_directories ("${CMAKE_SOURCE_DIR}/ETO_Common/include")

# Add sources
set(SRC
    src/eto_dbs.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/dictionary.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/iniparser.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_log.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_util.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_main.c
)

# Add executable and dependancies
add_executable (${TARGET_NAME} ${SRC})

if (MySQL_FOUND)
  target_link_libraries (${TARGET_NAME} ${MySQL_LIBRARIES})
endif (MySQL_FOUND)
