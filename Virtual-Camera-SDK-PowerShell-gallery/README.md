# Virtual-Camera-SDK PowerShell SDK

PowerShell7.4+ Windows x64. `Install-Module VirtualCameraSDK -Scope CurrentUser`.

```powershell
$client=Connect-VirtualCameraSDKSDK
try { Invoke-VirtualCameraSDKSDK -Client $client -Operation status } finally { Disconnect-VirtualCameraSDKSDK -Client $client }
```

Use -Arguments with a hashtable matching [schema](docs/API.md). Native errors retain their Windows code through the inner exception. Matching driver installed separately.
