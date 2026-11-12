vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/titanfyre/Virtual-Camera-SDK/releases/download/sdk-v0.1.1/virtual-camera-sdk-sdk-0.1.1.tar.gz"
    FILENAME "virtual-camera-sdk-sdk-0.1.1.tar.gz"
    SHA512 817f3d128b21d3735aa5e4e219efbeab5c4d694082c195f711158e10ebd7b45df93c25389b1a0d566108bb7d2c7686a7e486f0fd2a818cfd125132de5cf2d757
)
vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}" NO_REMOVE_ONE_LEVEL)
vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME VirtualCameraSDK CONFIG_PATH lib/cmake/VirtualCameraSDK)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
