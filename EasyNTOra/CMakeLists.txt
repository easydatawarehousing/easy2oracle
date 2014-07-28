# Set target
set(TARGET_NAME "EasyNTOra")

# Add libraries
set(ORACLE_OCI_NAMES libclntsh.so)
set(ORACLE_LIB_DIR "${CMAKE_SOURCE_DIR}/bin/lib")

find_library(ORACLE_OCI_LIBRARY
  NAMES ${ORACLE_OCI_NAMES}
  PATHS ${ORACLE_LIB_DIR}
  NO_DEFAULT_PATH
)

set(LIBS ${ORACLE_OCI_LIBRARY})

# Add include dirs
include_directories ("${CMAKE_SOURCE_DIR}/ETO_Common/include")
include_directories ("${CMAKE_SOURCE_DIR}/EasyNTOra/instantclient/include")

# Add sources
set(SRC
    src/eto_dbs.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/dictionary.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/iniparser.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_log.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_util.c
    ${CMAKE_SOURCE_DIR}/ETO_Common/src/eto_main.c
)

# Add executable and dependancies. Setup RPATH to the executable (lib) directory
add_executable (${TARGET_NAME} ${SRC})

SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,'$ORIGIN:$ORIGIN/lib'" )

target_link_libraries (${TARGET_NAME} ${LIBS})