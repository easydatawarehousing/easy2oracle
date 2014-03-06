# Set target
set(TARGET_NAME "EasyPTOra")

# Add libraries
find_package (CURL REQUIRED)

# Add include dirs
include_directories (prestoclient)

if (CURL_FOUND)
include_directories (CURL_INCLUDES)
else ()
include_directories (prestoclient/curl)
endif ()

include_directories ("${CMAKE_SOURCE_DIR}/ETO_Common/include")

# Add sources
set(SRC
    prestoclient/prestoclientutils.c
    prestoclient/prestoclientjsonstream.c
    prestoclient/prestoclientcurl.c
    prestoclient/prestoclient.c
    src/eto_dbs.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/dictionary.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/iniparser.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_log.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_util.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_main.c
)

# Add executable and dependancies
add_executable (${TARGET_NAME} ${SRC})

if (CURL_FOUND)
  target_link_libraries (${TARGET_NAME} ${CURL_LIBRARIES})
endif (CURL_FOUND)
