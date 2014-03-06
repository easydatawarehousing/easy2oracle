# - Find mysqlclient
# Find the native MySQL includes and library
#
#  MySQL_INCLUDES    - where to find mysql.h, etc.
#  MySQL_LIBRARIES   - List of libraries when using MySQL.
#  MySQL_FOUND       - True if MySQL found.

# Silent mode ?
IF (MySQL_INCLUDE_DIR AND MySQL_LIBRARY)
  # Already in cache, be silent
  SET(MySQL_FIND_QUIETLY TRUE)
ENDIF (MySQL_INCLUDE_DIR AND MySQL_LIBRARY)

# Find include dir
FIND_PATH(MySQL_INCLUDE_DIR mysql.h
  /usr/local/include/mysql
  /usr/include/mysql
)

# Find library
SET(MySQL_NAMES mysqlclient mysqlclient_r)
FIND_LIBRARY(MySQL_LIBRARY
  NAMES ${MySQL_NAMES}
  PATHS /usr/lib /usr/local/lib
  PATH_SUFFIXES mysql
)

# Set variables
IF (MySQL_INCLUDE_DIR AND MySQL_LIBRARY)
  SET(MySQL_FOUND TRUE)
  SET(MySQL_LIBRARIES ${MySQL_LIBRARY})
  SET(MySQL_INCLUDES ${MySQL_INCLUDE_DIR})
ELSE (MySQL_INCLUDE_DIR AND MySQL_LIBRARY)
  SET(MySQL_FOUND FALSE)
  SET(MySQL_LIBRARIES)
  SET(MySQL_INCLUDES)
ENDIF (MySQL_INCLUDE_DIR AND MySQL_LIBRARY)

# Report
IF (MySQL_FOUND)
  IF (NOT MySQL_FIND_QUIETLY)
    MESSAGE(STATUS "Found MySQL: ${MySQL_LIBRARY}")
    MESSAGE(STATUS "MySQL include_dir: ${MySQL_INCLUDES}")
  ENDIF (NOT MySQL_FIND_QUIETLY)
ELSE (MySQL_FOUND)
  IF (MySQL_FIND_REQUIRED)
    MESSAGE(STATUS "Looked for MySQL libraries named ${MySQL_NAMES}.")
    MESSAGE(FATAL_ERROR "Could NOT find MySQL library")
  ENDIF (MySQL_FIND_REQUIRED)
ENDIF (MySQL_FOUND)

MARK_AS_ADVANCED(
  MySQL_LIBRARY
  MySQL_INCLUDE_DIR
)
