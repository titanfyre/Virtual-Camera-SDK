cmake_minimum_required(VERSION 3.24)
project(VirtualCameraSDK VERSION 0.1.1 LANGUAGES C CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "This SDK requires Windows x64. No driver is installed by this build.")
endif()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
add_library(TitanVirtualCameraSDK STATIC
  "sdk/SDK/Camera.cpp"
)
add_library(VirtualCameraSDK::sdk ALIAS TitanVirtualCameraSDK)
set_target_properties(TitanVirtualCameraSDK PROPERTIES EXPORT_NAME sdk)
target_compile_features(TitanVirtualCameraSDK PUBLIC cxx_std_20)
target_compile_definitions(TitanVirtualCameraSDK PUBLIC )
target_compile_definitions(TitanVirtualCameraSDK PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
if(MSVC)
  target_compile_options(TitanVirtualCameraSDK PRIVATE /utf-8 /EHsc /W4)
endif()
target_include_directories(TitanVirtualCameraSDK PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk>"
  "$<INSTALL_INTERFACE:include/>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk/SDK>"
  "$<INSTALL_INTERFACE:include/SDK>"
)
target_link_libraries(TitanVirtualCameraSDK PUBLIC advapi32 crypt32 gdi32 newdev ole32 setupapi user32 winhttp wintrust)
install(TARGETS TitanVirtualCameraSDK EXPORT VirtualCameraSDKTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY sdk/ DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl")
install(FILES LICENSE DESTINATION share/virtual-camera-sdk)
install(EXPORT VirtualCameraSDKTargets NAMESPACE VirtualCameraSDK:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VirtualCameraSDK)
configure_file("VirtualCameraSDKConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/VirtualCameraSDKConfig.cmake" @ONLY)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/VirtualCameraSDKConfigVersion.cmake" VERSION 0.1.1 COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VirtualCameraSDKConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/VirtualCameraSDKConfigVersion.cmake" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VirtualCameraSDK)
