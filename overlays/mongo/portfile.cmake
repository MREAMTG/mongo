# Get the current target architecture
if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(CPU_ARCH "x86_64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(CPU_ARCH "x86_64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(CPU_ARCH "aarch64")
else()
    message(FATAL_ERROR "Unsupported architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

cmake_host_system_information(RESULT DISTRO_ID QUERY DISTRIB_ID)
cmake_host_system_information(RESULT DISTRO_VERSION QUERY DISTRIB_VERSION_ID)

string(REGEX REPLACE "\\." "" DISTRO_VERSION "${DISTRO_VERSION}") # Remove quotes if present

message("===== SYSTEM INFO =====")
message(STATUS ${CPU_ARCH})
message(STATUS ${VCPKG_CMAKE_SYSTEM_NAME})
message(STATUS ${DISTRO_ID})
message(STATUS ${DISTRO_VERSION})
message("======================")

if(NOT VCPKG_CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Unsupported OS: ${VCPKG_CMAKE_SYSTEM_NAME}")
endif()

if(NOT DISTRO_ID STREQUAL "ubuntu")
    message(FATAL_ERROR "Unsupported Linux distro: ${DISTRO_ID}")
endif()

set(RELEASE_URL https://fastdl.mongodb.org/linux/mongodb-linux-${CPU_ARCH}-${DISTRO_ID}${DISTRO_VERSION}-${VERSION}.tgz)

message(STATUS ${RELEASE_URL})
set(REL_ID mongodb-linux-${CPU_ARCH}-${DISTRO_ID}${DISTRO_VERSION}-${VERSION})

vcpkg_download_distfile(
    MONGO_TAR_PATH
    URLS ${RELEASE_URL}
    FILENAME ${REL_ID}.tgz
    SKIP_SHA512
)

message("======================")
message(STATUS "Downloaded " "${MONGO_TAR_PATH}")
message("======================")

vcpkg_extract_archive(
    ARCHIVE "${MONGO_TAR_PATH}"
    DESTINATION "${CURRENT_PACKAGES_DIR}/mongo"
)

message("======================")
message(STATUS "Extracted " "${CURRENT_PACKAGES_DIR}/${REL_ID}")
message("======================")

file(RENAME "${CURRENT_PACKAGES_DIR}/mongo/${REL_ID}" "${CURRENT_PACKAGES_DIR}/mongo/release")