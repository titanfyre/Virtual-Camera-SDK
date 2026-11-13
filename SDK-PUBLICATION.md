# SDK publication status

Current source layout and registry coverage. A GitHub download is distinct from publication on a central registry.

| Target | Status | Package / documentation |
| --- | --- | --- |
| CPP/vcpkg | published_custom_registry | virtual-camera-sdk |
| CPP/conan | published_custom_source_index | [virtual-camera-sdk](https://github.com/TitanCSA/sdk-packages/tree/089bc73095aa8eda89bacbe10d0b9e58394b0207/conan-index) [Source](packaging/native/) |
| CPP/nuget | published_payload_verified | [Virtual-Camera-SDK.Native](https://www.nuget.org/packages/Virtual-Camera-SDK.Native/0.1.1) |
| NET/nuget | published_payload_verified | [VirtualCameraSDK.SDK](https://www.nuget.org/packages/VirtualCameraSDK.SDK/1.0.0) |
| Python/pypi | built_fixture_and_production_verified | virtual-camera-sdk [Source](Virtual-Camera-SDK-Python-pypi/) |
| Python/conda-forge | custom_channel_install_verified_requires_conda_forge_review | [virtual-camera-sdk](https://raw.githubusercontent.com/TitanCSA/sdk-packages/089bc73095aa8eda89bacbe10d0b9e58394b0207/conda-channel) [Source](packaging/conda/) |
| Rust/cargo | published_install_verified | [virtual-camera-sdk](https://crates.io/crates/virtual-camera-sdk) [Source](Virtual-Camera-SDK-Rust-cargo/) |
| JavaScript/npm | stopped_before_submission | virtual-camera-sdk [Source](Virtual-Camera-SDK-JavaScript-npm/) |
| Java/maven | published_install_verified | [virtual-camera-sdk](https://central.sonatype.com/artifact/io.github.titancsa/virtual-camera-sdk/1.0.0) [Source](Virtual-Camera-SDK-Java-maven/) |
| Java/jitpack | published_install_verified | [virtual-camera-sdk](https://jitpack.io/#titanfyre/Virtual-Camera-SDK/java-sdk-v1.0.0) [Source](Virtual-Camera-SDK-Java-maven/) |
| Go/modules | published_install_verified | [virtual-camera-sdk](https://pkg.go.dev/github.com/titanfyre/Virtual-Camera-SDK/Virtual-Camera-SDK-Go-modules@v1.0.0) [Source](Virtual-Camera-SDK-Go-modules/) |
| Go/pkg-go-dev | indexed_verified | [Documentation](https://pkg.go.dev/github.com/titanfyre/Virtual-Camera-SDK/Virtual-Camera-SDK-Go-modules@v1.0.0) |
| PowerShell/gallery | published_install_verified | [VirtualCameraSDK](https://www.powershellgallery.com/packages/VirtualCameraSDK/1.0.0) [Source](Virtual-Camera-SDK-PowerShell-gallery/) |
| Dart/pub-dev | built_fixture_and_production_verified | virtual_camera_sdk [Source](Virtual-Camera-SDK-Dart-pub-dev/) |
| Ruby/rubygems | published_install_verified | [virtual-camera-sdk](https://rubygems.org/gems/virtual-camera-sdk) [Source](Virtual-Camera-SDK-Ruby-rubygems/) |
| PHP/packagist | composer_source_and_tag_published | [titancsa/virtual-camera-sdk](https://packagist.org/packages/titancsa/virtual-camera-sdk) |

Native SDK source and CMake/Conan/vcpkg/NuGet recipes share [`packaging/native`](packaging/native/LAYOUT.md). Previous release archives and tags remain unchanged.

Application and driver prerequisites and original licensing remain those documented by each SDK. No driver installer is bundled with these source packages.
