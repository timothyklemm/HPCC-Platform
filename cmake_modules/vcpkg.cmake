if ("${VCPKG_DONE}" STREQUAL "")
  set (VCPKG_DONE 1)

set(VCPKG_FILES_DIR "${CMAKE_BINARY_DIR}" CACHE STRING "Folder for vcpkg download, build and installed files")
set(CMAKE_TOOLCHAIN_FILE ${HPCC_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(VCPKG_ROOT ${HPCC_SOURCE_DIR}/vcpkg)
set(VCPKG_INSTALLED_DIR "${VCPKG_FILES_DIR}/vcpkg_installed")
set(VCPKG_INSTALL_OPTIONS "--x-abi-tools-use-exact-versions;--downloads-root=${VCPKG_FILES_DIR}/vcpkg_downloads;--x-buildtrees-root=${VCPKG_FILES_DIR}/vcpkg_buildtrees;--x-packages-root=${VCPKG_FILES_DIR}/vcpkg_packages")
set(VCPKG_VERBOSE OFF)

if(APPLE OR WIN32)
elseif(UNIX)
    set(VCPKG_TARGET_TRIPLET "x64-linux-dynamic" CACHE STRING "target triplet")
endif()

#  Create a catalog of the vcpkg dependencies ---
file(GLOB VCPKG_PACKAGES ${VCPKG_FILES_DIR}/vcpkg_packages/*/CONTROL)
list(APPEND VCPKG_PACKAGE_LIST "-----------------\n")
foreach(VCPKG_PACKAGE ${VCPKG_PACKAGES})
    file(READ ${VCPKG_PACKAGE} VCPKG_PACKAGE_CONTENTS)
    list(APPEND VCPKG_PACKAGE_LIST ${VCPKG_PACKAGE_CONTENTS})
    list(APPEND VCPKG_PACKAGE_LIST "-----------------\n")
endforeach()
file(WRITE ${CMAKE_BINARY_DIR}/vcpkg-catalog.txt ${VCPKG_PACKAGE_LIST})
if (INSTALL_VCPKG_CATALOG)
    install(FILES ${CMAKE_BINARY_DIR}/vcpkg-catalog.txt DESTINATION "." COMPONENT Runtime)
endif()

endif ()
