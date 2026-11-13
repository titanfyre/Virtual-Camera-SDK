# Virtual-Camera-SDK Ruby SDK

`gem install virtual-camera-sdk`

Windows x64 Ruby3.1+.

```ruby
require "virtual_camera_sdk"
VirtualCameraSDKSDK::Client.open { |client| p client.call("status") }
```

[API and operation schema](docs/API.md). Matching driver installed separately.
