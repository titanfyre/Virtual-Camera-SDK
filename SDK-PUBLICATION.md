# SDK publication status

Current source layout and registry coverage. A GitHub download is distinct from publication on a central registry.

| Target | Status | Package / documentation |
| --- | --- | --- |
| CPP/vcpkg | published_custom_registry | virtual-camera-sdk |
| CPP/conan | public_recipe_builds_locally |  [Source](packaging/native/) |
| CPP/nuget | paused_pending_owner_email_confirmation | Virtual-Camera-SDK.Native |
| NET/nuget | built_verified_publication_on_hold_by_owner | VirtualCameraSDK.SDK [Source](Virtual-Camera-SDK-NET-nuget/) |
| Python/pypi | built_fixture_and_production_verified | virtual-camera-sdk [Source](Virtual-Camera-SDK-Python-pypi/) |
| Python/conda-forge | built_tested_requires_source_publication_and_official_review | virtual-camera-sdk [Source](packaging/conda/) |
| Rust/cargo | published_install_verified | [virtual-camera-sdk](https://crates.io/crates/virtual-camera-sdk) [Source](Virtual-Camera-SDK-Rust-cargo/) |
| JavaScript/npm | awaiting_browser_publication_authentication | [virtual-camera-sdk](https://www.npmjs.com/package/virtual-camera-sdk) [Source](Virtual-Camera-SDK-JavaScript-npm/) |
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
