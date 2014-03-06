# - Find FreeTDS
# Find the native FreeTDS includes and library
#
#  FreeTDS_INCLUDES    - where to find ctpublic.h
#  FreeTDS_LIBRARIES   - List of libraries when using FreeTDS
#  FreeTDS_FOUND       - True if FreeTDS found.

# Silent mode ?
IF (FreeTDS_INCLUDE_DIR AND FreeTDS_LIBRARY)
  # Already in cache, be silent
  SET(FreeTDS_FIND_QUIETLY TRUE)
ENDIF (FreeTDS_INCLUDE_DIR AND FreeTDS_LIBRARY)

# Find include dir
FIND_PATH(FreeTDS_INCLUDE_DIR ctpublic.h
  /usr/local/include
  /usr/include
)

# Find library
SET(FreeTDS_NAMES ct)
FIND_LIBRARY(FreeTDS_LIBRARY
  NAMES ${FreeTDS_NAMES}
  PATHS /usr/lib /usr/local/lib
)

# Set variables
IF (FreeTDS_INCLUDE_DIR AND FreeTDS_LIBRARY)
  SET(FreeTDS_FOUND TRUE)
  SET(FreeTDS_LIBRARIES ${FreeTDS_LIBRARY})
  SET(FreeTDS_INCLUDES ${FreeTDS_INCLUDE_DIR})
ELSE (FreeTDS_INCLUDE_DIR AND FreeTDS_LIBRARY)
  SET(FreeTDS_FOUND FALSE)
  SET(FreeTDS_LIBRARIES)
  SET(FreeTDS_INCLUDES)
ENDIF (FreeTDS_INCLUDE_DIR AND FreeTDS_LIBRARY)

# Report
IF (FreeTDS_FOUND)
  IF (NOT FreeTDS_FIND_QUIETLY)
    MESSAGE(STATUS "Found FreeTDS: ${FreeTDS_LIBRARY}")
    MESSAGE(STATUS "FreeTDS include_dir: ${FreeTDS_INCLUDES}")
  ENDIF (NOT FreeTDS_FIND_QUIETLY)
ELSE (FreeTDS_FOUND)
  IF (FreeTDS_FIND_REQUIRED)
    MESSAGE(STATUS "Looked for FreeTDS libraries named ${FreeTDS_NAMES}.")
    MESSAGE(FATAL_ERROR "Could NOT find FreeTDS library")
  ENDIF (FreeTDS_FIND_REQUIRED)
ENDIF (FreeTDS_FOUND)

MARK_AS_ADVANCED(
  FreeTDS_LIBRARY
  FreeTDS_INCLUDE_DIR
)
