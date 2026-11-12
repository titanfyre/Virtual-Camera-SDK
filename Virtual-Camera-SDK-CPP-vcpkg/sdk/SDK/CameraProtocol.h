#pragma once
// Fixed-width public protocol; no pointers or architecture-dependent fields.
#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include <windows.h>
#include <winioctl.h>
#endif
namespace CameraProtocol {
constexpr unsigned Version=1, Width=640, Height=480, Stride=Width*3, FrameBytes=Stride*Height;
constexpr unsigned FrameInterval100ns=333333;
constexpr unsigned Query=CTL_CODE(FILE_DEVICE_UNKNOWN,0x920,METHOD_BUFFERED,FILE_READ_DATA);
constexpr unsigned Submit=CTL_CODE(FILE_DEVICE_UNKNOWN,0x921,METHOD_BUFFERED,FILE_WRITE_DATA);
constexpr unsigned Challenge=CTL_CODE(FILE_DEVICE_UNKNOWN,0x922,METHOD_BUFFERED,FILE_READ_DATA);
constexpr unsigned Authorize=CTL_CODE(FILE_DEVICE_UNKNOWN,0x923,METHOD_BUFFERED,FILE_WRITE_DATA);
struct Header { unsigned version,bytes,width,height,stride,reserved; };
struct State { unsigned version,bytes,width,height,stride,frame_bytes,frame_interval_100ns,producer_active; unsigned long long submitted,delivered,sequence; };
constexpr bool Valid(const Header& h,unsigned bytes) noexcept {
 return h.version==Version && h.bytes==sizeof(Header) && h.width==Width && h.height==Height && h.stride==Stride && h.reserved==0 && bytes==sizeof(Header)+FrameBytes;
}
static_assert(sizeof(Header)==24 && sizeof(State)==56);
}
