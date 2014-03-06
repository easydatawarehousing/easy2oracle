# Set target
set(TARGET_NAME "EasyTTOra")

# Add libraries
find_package (FreeTDS REQUIRED)

# Add include dirs
if (CURL_FOUND)
include_directories (FreeTDS_INCLUDES)
endif ()

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

if (FreeTDS_FOUND)
  target_link_libraries (${TARGET_NAME} ${FreeTDS_LIBRARIES})
endif (FreeTDS_FOUND)
