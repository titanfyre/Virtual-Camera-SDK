# Validation

Application/SDK 0.1.1; returned Microsoft-signed driver 0.1.0.0, Windows x64, AVStream. The signed INF matches the submission, the SYS protected-image digest matches the submitted candidate, and Microsoft catalog trust plus INF/SYS membership pass. Driver source remains private.

## Signed distribution and offline checks

| Check | Result |
| --- | --- |
| Kernel C++ /W4 /WX + PREfast | Pass, zero reported defects |
| INF Windows Driver/Universal checks and Inf2Cat | Pass for the submitted package |
| Public app, SDK, example and both installers | Build with /W4 /WX, static runtime and CFG |
| Exact signed resources in both setups | INF/CAT/SYS match the verified return byte for byte; executables loaded only as data |
| Fixed frame protocol headers and malformed lengths | 100,010 bounded offline checks |
| Basic package identity and unsigned-file refusal | 11 read-only checks |
| Actual returned package and tamper refusal | 25 read-only checks; modified SYS, INF and CAT refused |
| Signed-build CTest | 3/3 pass |
| Separate AddressSanitizer configuration | Instrumented protocol and offline identity tests pass |
| Windows 11 and capture-application compatibility | Not yet qualified |

Catalog acceptance requires a valid Windows trust chain, Microsoft root public-key policy and exact Microsoft hardware-publisher identity, followed by INF/SYS catalog membership. A certificate display name alone is insufficient. Source builds contain no driver payload unless the matching signed package is supplied explicitly. No installer, driver or certificate was installed on the development host.

## Microsoft-signed package VM campaigns

The complete signed-console installation, runtime and uninstallation campaign passed on **Windows 10 build 19045 x64**, normally and again under standard Driver Verifier targeted at both SDK drivers before load. Test-signing was disabled; no guest test certificate was used. The installed Driver Store SYS hash matched the exact returned Microsoft-signed camera image.

Each campaign passed **499 real-KS/frame/lifecycle checks** and **67 signed-permission expiry checks**. The validator opened the actual KS capture pin and checked every byte of captured images, fixed frame size, black fallback before authorization and after producer close, exclusive producer ownership, unsupported read/write/flush refusal, malformed IOCTL rejection, wrong-product/unsigned/replayed permissions, 256 repeated submissions and 16 capture open/close cycles. A separate two-second permission confirmed that further producer injection is refused and capture returns black after expiry. Genuine signed test grants came from the private server; its private signing key was never transferred to the guest.

Restricted-token producer access, including a zero-access overlapped request, was refused. Administrator zero-access, read-only and write-only control opens were also refused before ownership or state changes. Producer control requires mapped FILE_READ_DATA and FILE_WRITE_DATA; the SDK opens it with both rights. This applies to producer control, not the independent KS capture path.

The bundled console installer installed the exact signed package and completed removal of the owned camera device and its unused package. The camera driver unloaded; no new minidump was observed. The original VM baseline and all nine historical checkpoints were restored with the VM off. Host security settings were not changed. UI setup shares the tested installation core and embeds the same verified package, but these campaigns automated the console setup rather than manually interacting with the UI.

Earlier pre-submission campaigns exercised the same protected driver image with a temporary guest-only lab signature. They remain separate private evidence; the results above are from the actual returned Microsoft-signed package and final console setup.

## Runtime and timing boundary

The driver keeps one bounded 921,600-byte latest frame, processes frame copies at PASSIVE_LEVEL and schedules a small timer DPC only while a capture pin is running. Its filter processing callback returns STATUS_PENDING after one tick, following the AVStream scheduling contract, so an empty queue cannot cause continuous processing. Bounded administrator diagnostics expose lifecycle/format stages and inexpensive counters, never pixels or access keys. Cryptography and HTTPS do not execute for each frame. No zero-latency, throughput or CPU-use benchmark is claimed. Camera consumers may buffer, convert, schedule or reject this one format.

Further qualification includes Windows 11, Secure Boot/HVCI, representative capture applications and browser camera permission paths, standard-user capture/privacy policy, multiple concurrent consumers, suspend/resume, device surprise removal/reinstallation during active streams and prolonged workloads. No test installs the driver on the development host. Bounded VM tests and a Microsoft signature do not establish universal reliability, WHQL certification or absence of defects.

References: [AVStream samples](https://learn.microsoft.com/en-us/windows-hardware/drivers/stream/avstream-samples), [AVStream filter descriptor and passive processing](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ks/ns-ks-_ksfilter_descriptor), [Windows virtual-camera alternative](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera).
