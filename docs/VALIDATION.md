# Validation

Candidate 0.1.0.0, Windows x64, AVStream. Microsoft signing is pending.

| Check | Result |
| --- | --- |
| Kernel C++ /W4 /WX + PREfast | Pass, zero reported defects |
| INF Windows Driver and Universal checks | Pass |
| Inf2Cat Windows 10/11 x64 | Pass |
| Public app, SDK, console and installer sources | Compile with /W4 /WX, static runtime and CFG |
| Frame protocol headers/lengths | 100,010 bounded offline checks |
| Exact package identity and unsigned-file refusal | 11 offline checks |
| Protocol AddressSanitizer | Pass, 100,010 checks |
| Windows 10 build 19045 x64, isolated VM | Pass: 499 checks, real KS pixels, 256 submissions and 16 capture open/close cycles |
| Two-second signed permission expiry | Pass: 67 checks; further injection refused and real capture returned black |
| Exact device/package removal and driver unload | Pass, no restart needed in the tested campaign |
| Same candidate with Driver Verifier enabled before load | Pass: 499 real-frame/lifecycle checks plus 67 expiry checks |
| Restricted-token producer access, including zero-access overlapped opens | Refused as required |
| Administrator zero-access, read-only and write-only control opens | Refused before producer ownership or state changes |
| Returned Microsoft-signed package | Pending signing |
| Windows 11 and application compatibility | Not yet qualified |

Both the normal campaign and the same-image Driver Verifier campaign opened the actual KS capture pin and checked every byte of the captured image, fixed frame size, black fallback before authorization and after producer close, exclusive producer ownership, unsupported read/write/flush refusal, malformed IOCTL rejection, wrong-product/unsigned/replayed permissions, 256 repeated submissions and 16 capture open/close cycles. A separate two-second permission confirmed that the driver refuses further injection and returns black frames after expiry. Genuine signed test grants came from the private server; its private signing key was never transferred to the guest.

Both campaigns passed exact package removal and driver unload without a restart. These tests used a temporary lab certificate and enabled test-signing only inside the disposable Windows 10 guest. They do not establish final Microsoft-signed installer behavior. The exact candidate image and runtime executable were pinned for each campaign. Restricted-token tests confirmed that unprivileged clients cannot open producer control, including a zero-access overlapped request. Administrator zero-access, read-only and write-only opens were also refused. The control endpoint requires mapped FILE_READ_DATA and FILE_WRITE_DATA before it can claim producer ownership or alter frame/session state; the SDK already opens it with both rights. This check applies only to producer control, not the independent KS capture path. The exact lab device and driver package were removed, the driver unloaded, and the original VM and all nine historical checkpoints were restored with the VM off. No driver was installed on the host and no host security setting was changed.

The driver keeps one bounded 921,600-byte latest frame, processes frame copies at PASSIVE_LEVEL and schedules only a small timer DPC while a capture pin is running. Its filter processing callback returns STATUS_PENDING after one tick, following the AVStream scheduling contract, so an empty queue cannot cause continuous processing. Bounded administrator diagnostics expose lifecycle/format stages and inexpensive counters, never pixels or access keys. Cryptography and HTTPS do not execute for each frame. No zero-latency, throughput or CPU-use benchmark is claimed. Camera consumers may buffer, convert, schedule or reject this one format.

Further qualification includes the final Microsoft-signed package, Windows 11, representative capture applications and browser camera permission paths, standard-user capture/privacy policy, multiple concurrent consumers, suspend/resume, device surprise removal/reinstall during active streams and prolonged workloads. No test installs this driver on the development host.

References: [AVStream samples](https://learn.microsoft.com/en-us/windows-hardware/drivers/stream/avstream-samples), [AVStream filter descriptor and passive processing](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ks/ns-ks-_ksfilter_descriptor), [Windows virtual-camera alternative](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera).
