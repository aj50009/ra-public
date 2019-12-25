find_path(
	assimp_INCLUDE_DIRS
	NAMES assimp/postprocess.h assimp/scene.h assimp/version.h assimp/config.h assimp/cimport.h
	PATHS /usr/local/include
	PATHS /usr/include/
	PATHS "${ASSIMP_ROOT_DIR}/include"
)

find_library(
	assimp_LIBRARY
	NAMES assimp
	PATHS /usr/local/lib/
	PATHS /usr/lib64/
	PATHS /usr/lib/
	PATHS "${ASSIMP_ROOT_DIR}/build/code"
)

find_library(
	IrrXML_LIBRARY
	NAMES IrrXML
	PATHS /usr/local/lib/
	PATHS /usr/lib64/
	PATHS /usr/lib/
	PATHS "${ASSIMP_ROOT_DIR}/build/contrib/irrXML"
)

find_library(
	zlib_LIBRARY
	NAMES zlibstatic zlib
	PATHS /usr/local/lib/
	PATHS /usr/lib64/
	PATHS /usr/lib/
	PATHS "${ASSIMP_ROOT_DIR}/build/contrib/zlib"
)

set(assimp_LIBRARIES ${assimp_LIBRARY} CACHE INTERNAL "" FORCE)
if(IrrXML_LIBRARY)
	set(assimp_LIBRARIES ${assimp_LIBRARIES} ${IrrXML_LIBRARY})
endif()
if(zlib_LIBRARY)
	set(assimp_LIBRARIES ${assimp_LIBRARIES} ${zlib_LIBRARY})
endif()


if (assimp_INCLUDE_DIRS AND assimp_LIBRARIES)
	SET(assimp_FOUND TRUE)
endif(assimp_INCLUDE_DIRS AND assimp_LIBRARIES)

if (assimp_FOUND)
	if (NOT assimp_FIND_QUIETLY)
		message(STATUS "Found asset importer library: ${assimp_LIBRARIES}")
	endif (NOT assimp_FIND_QUIETLY)
else (assimp_FOUND)
	if (assimp_FIND_REQUIRED)
		message(FATAL_ERROR "Could not find asset importer library")
	endif (assimp_FIND_REQUIRED)
endif (assimp_FOUND)
