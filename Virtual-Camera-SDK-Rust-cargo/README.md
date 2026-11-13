# Virtual-Camera-SDK Rust SDK

`cargo add virtual-camera-sdk`

Windows x64 MSVC Rust1.82+. Bundled DLL/import library; copy native DLL beside deployed executable.

```rust,no_run
use virtual_camera_sdk::Client;
let mut client=Client::new()?;
let result=client.call("status",serde_json::json!({}))?;
# Ok::<(),Box<dyn std::error::Error>>(())
```

[API](docs/API.md). Drop closes owned session; original driver and licensing requirements unchanged.
