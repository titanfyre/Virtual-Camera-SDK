// Titan Virtual Camera, explicit fixed-format software capture.
// AVStream descriptor layout follows Microsoft's avssamp reference (MS-PL).
// Copyright (c) 1999-2001 Microsoft Corporation, retained for that layout.
// Copyright (c) 2026 Titanfyre. No hooks or arbitrary kernel-memory access.
#include <ntddk.h>
#include <windef.h>
#include <unknown.h>
#include <ks.h>
#include <ksmedia.h>
#include <wdmsec.h>
#include "CameraProtocol.h"
#include "DriverAuthorization.h"
using namespace CameraProtocol;
namespace {
constexpr ULONG Tag='maCT';
const GUID CameraReference={0x9ae0237b,0x6238,0x464b,{0xb4,0x1a,0x27,0x95,0x38,0xd8,0x95,0xa3}};
const GUID RGB24={0xe436eb7d,0x524f,0x11ce,{0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70}};
const GUID ControlClass={0x6de02086,0xea4c,0x45ae,{0x9c,0x5d,0xd8,0xd2,0x7c,0xf8,0x45,0x28}};
FAST_MUTEX FrameLock;
KMUTEX SessionMutex;
TITAN_AUTHORIZATION_STATE Authorization;
#define SessionLock() KeWaitForSingleObject(&SessionMutex,Executive,KernelMode,FALSE,nullptr)
#define SessionUnlock() KeReleaseMutex(&SessionMutex,FALSE)
PUCHAR Frame;
PFILE_OBJECT Producer;
PDEVICE_OBJECT Control;
PDRIVER_DISPATCH Previous[IRP_MJ_MAXIMUM_FUNCTION+1];
PDRIVER_UNLOAD PreviousUnload;
ULONGLONG Submitted,Delivered,Sequence;
volatile LONG Filters;
volatile LONG PinDiagnostics[32];
constexpr ULONG DiagnosticQuery=CTL_CODE(FILE_DEVICE_UNKNOWN,0x924,METHOD_BUFFERED,FILE_READ_DATA);
struct FilterContext { PKSFILTER filter; KTIMER timer; KDPC dpc; KSPIN_LOCK timer_lock; BOOLEAN running; ULONGLONG next; };
struct PinContext { IKsReferenceClock* clock; ULONGLONG frames; };
#define Lock() ExAcquireFastMutex(&FrameLock)
#define Unlock() ExReleaseFastMutex(&FrameLock)
NTSTATUS Complete(PIRP irp,NTSTATUS s,ULONG_PTR n=0){irp->IoStatus.Status=s;irp->IoStatus.Information=n;IoCompleteRequest(irp,IO_NO_INCREMENT);return s;}
_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CREATE_NAMED_PIPE)
_Dispatch_type_(IRP_MJ_CLOSE)
_Dispatch_type_(IRP_MJ_READ)
_Dispatch_type_(IRP_MJ_WRITE)
_Dispatch_type_(IRP_MJ_QUERY_INFORMATION)
_Dispatch_type_(IRP_MJ_SET_INFORMATION)
_Dispatch_type_(IRP_MJ_QUERY_EA)
_Dispatch_type_(IRP_MJ_SET_EA)
_Dispatch_type_(IRP_MJ_FLUSH_BUFFERS)
_Dispatch_type_(IRP_MJ_QUERY_VOLUME_INFORMATION)
_Dispatch_type_(IRP_MJ_SET_VOLUME_INFORMATION)
_Dispatch_type_(IRP_MJ_DIRECTORY_CONTROL)
_Dispatch_type_(IRP_MJ_FILE_SYSTEM_CONTROL)
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
_Dispatch_type_(IRP_MJ_INTERNAL_DEVICE_CONTROL)
_Dispatch_type_(IRP_MJ_SHUTDOWN)
_Dispatch_type_(IRP_MJ_LOCK_CONTROL)
_Dispatch_type_(IRP_MJ_CLEANUP)
_Dispatch_type_(IRP_MJ_CREATE_MAILSLOT)
_Dispatch_type_(IRP_MJ_QUERY_SECURITY)
_Dispatch_type_(IRP_MJ_SET_SECURITY)
_Dispatch_type_(IRP_MJ_POWER)
_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
_Dispatch_type_(IRP_MJ_DEVICE_CHANGE)
_Dispatch_type_(IRP_MJ_QUERY_QUOTA)
_Dispatch_type_(IRP_MJ_SET_QUOTA)
_Dispatch_type_(IRP_MJ_PNP)
DRIVER_DISPATCH Dispatch;
NTSTATUS Dispatch(PDEVICE_OBJECT device,PIRP irp){
 auto stack=IoGetCurrentIrpStackLocation(irp);
 if(device!=Control){auto forward=Previous[stack->MajorFunction];return forward?forward(device,irp):Complete(irp,STATUS_INVALID_DEVICE_REQUEST);}
 if(KeGetCurrentIrql()!=PASSIVE_LEVEL)return Complete(irp,STATUS_INVALID_DEVICE_STATE);
 auto file=stack->FileObject;
 switch(stack->MajorFunction){
 case IRP_MJ_CREATE:{
  if(file->FileName.Length)return Complete(irp,STATUS_OBJECT_NAME_INVALID);
  if(!stack->Parameters.Create.SecurityContext)return Complete(irp,STATUS_ACCESS_DENIED);
  ACCESS_MASK requested=stack->Parameters.Create.SecurityContext->DesiredAccess;
  RtlMapGenericMask(&requested,IoGetFileObjectGenericMapping());
  if((requested&(FILE_READ_DATA|FILE_WRITE_DATA))!=(FILE_READ_DATA|FILE_WRITE_DATA))return Complete(irp,STATUS_ACCESS_DENIED);
  SessionLock();Lock();if(Producer){Unlock();SessionUnlock();return Complete(irp,STATUS_SHARING_VIOLATION);}
  TitanAuthorizationInitialize(&Authorization,TITAN_AUTHORIZATION_CAMERA);Producer=file;RtlZeroMemory(Frame,FrameBytes);Unlock();SessionUnlock();return Complete(irp,STATUS_SUCCESS);
 }
 case IRP_MJ_CLEANUP:
  SessionLock();Lock();if(Producer==file){Producer=nullptr;InterlockedExchange64(&Authorization.deadline,0);RtlZeroMemory(Frame,FrameBytes);++Sequence;}Unlock();SessionUnlock();return Complete(irp,STATUS_SUCCESS);
 case IRP_MJ_CLOSE:return Complete(irp,STATUS_SUCCESS);
 case IRP_MJ_DEVICE_CONTROL:break;
 default:return Complete(irp,STATUS_INVALID_DEVICE_REQUEST);
 }
 const auto code=stack->Parameters.DeviceIoControl.IoControlCode;
 const ULONG input=stack->Parameters.DeviceIoControl.InputBufferLength,output=stack->Parameters.DeviceIoControl.OutputBufferLength;
 auto data=irp->AssociatedIrp.SystemBuffer;
 if(code==DiagnosticQuery){
  if(input || output!=sizeof(PinDiagnostics) || !data)return Complete(irp,STATUS_INVALID_BUFFER_SIZE);
  auto values=static_cast<ULONG*>(data);for(unsigned i=0;i<RTL_NUMBER_OF(PinDiagnostics);++i)values[i]=static_cast<ULONG>(InterlockedCompareExchange(&PinDiagnostics[i],0,0));return Complete(irp,STATUS_SUCCESS,sizeof(PinDiagnostics));
 }
 if(code==Challenge || code==Authorize){
  if(!data || (code==Challenge && (input || output!=sizeof(TITAN_AUTHORIZATION_CHALLENGE))) || (code==Authorize && (output || input!=sizeof(TITAN_AUTHORIZATION_TOKEN))))return Complete(irp,STATUS_INVALID_BUFFER_SIZE);
  SessionLock();if(Producer!=file){SessionUnlock();return Complete(irp,STATUS_ACCESS_DENIED);}
  NTSTATUS result=code==Challenge?TitanAuthorizationChallenge(&Authorization,static_cast<TITAN_AUTHORIZATION_CHALLENGE*>(data)):TitanAuthorizationAccept(&Authorization,static_cast<const TITAN_AUTHORIZATION_TOKEN*>(data));
  SessionUnlock();return Complete(irp,result,NT_SUCCESS(result)&&code==Challenge?sizeof(TITAN_AUTHORIZATION_CHALLENGE):0);
 }
 if(code==Query){
  if(input || output!=sizeof(State) || !data)return Complete(irp,STATUS_INVALID_BUFFER_SIZE);
  State state{};state.version=Version;state.bytes=sizeof(state);state.width=Width;state.height=Height;state.stride=Stride;state.frame_bytes=FrameBytes;state.frame_interval_100ns=FrameInterval100ns;
  Lock();state.producer_active=Producer==file && TitanAuthorizationAllowed(&Authorization);state.submitted=Submitted;state.delivered=Delivered;state.sequence=Sequence;Unlock();
  RtlCopyMemory(data,&state,sizeof(state));return Complete(irp,STATUS_SUCCESS,sizeof(state));
 }
 if(code==Submit){
  if(input!=sizeof(Header)+FrameBytes || output || !data)return Complete(irp,STATUS_INVALID_BUFFER_SIZE);
  const auto header=static_cast<const Header*>(data);
  if(!Valid(*header,input))return Complete(irp,STATUS_INVALID_PARAMETER);
  SessionLock();Lock();if(Producer!=file || !TitanAuthorizationAllowed(&Authorization)){Unlock();SessionUnlock();return Complete(irp,STATUS_ACCESS_DENIED);}
  RtlCopyMemory(Frame,static_cast<const UCHAR*>(data)+sizeof(Header),FrameBytes);++Submitted;++Sequence;Unlock();SessionUnlock();return Complete(irp,STATUS_SUCCESS);
 }
 return Complete(irp,STATUS_INVALID_DEVICE_REQUEST);
}
KDEFERRED_ROUTINE TimerDpc;
void TimerDpc(PKDPC,PVOID object,PVOID,PVOID){
 if(!object)return;
 auto c=static_cast<FilterContext*>(object);
 InterlockedIncrement(&PinDiagnostics[19]);
 KsFilterAttemptProcessing(c->filter,TRUE); // AVStream queues PASSIVE_LEVEL processing.
 KIRQL irql;KeAcquireSpinLock(&c->timer_lock,&irql);
 if(c->running){
  auto now=KeQueryInterruptTime();c->next+=FrameInterval100ns;if(c->next<=now)c->next=now+FrameInterval100ns;
  LARGE_INTEGER due;due.QuadPart=-static_cast<LONGLONG>(c->next-now);KeSetTimer(&c->timer,due,&c->dpc);
 }
 KeReleaseSpinLock(&c->timer_lock,irql);
}
void TimerStop(FilterContext* c){
 InterlockedIncrement(&PinDiagnostics[24]);PinDiagnostics[25]=1;
 KIRQL irql;KeAcquireSpinLock(&c->timer_lock,&irql);c->running=FALSE;KeCancelTimer(&c->timer);KeReleaseSpinLock(&c->timer_lock,irql);
 PinDiagnostics[25]=2;KeFlushQueuedDpcs();PinDiagnostics[25]=3; // Only on state/handle teardown; never per frame.
}
void FreeFilter(PVOID object){InterlockedIncrement(&PinDiagnostics[31]);auto c=static_cast<FilterContext*>(object);TimerStop(c);ExFreePoolWithTag(c,Tag);InterlockedDecrement(&Filters);}
NTSTATUS FilterCreate(PKSFILTER filter,PIRP){
 InterlockedIncrement(&PinDiagnostics[1]);PinDiagnostics[4]=1;
 if(InterlockedIncrement(&Filters)>4){InterlockedDecrement(&Filters);return STATUS_INSUFFICIENT_RESOURCES;}
 auto c=static_cast<FilterContext*>(ExAllocatePool2(POOL_FLAG_NON_PAGED,sizeof(FilterContext),Tag));
 if(!c){InterlockedDecrement(&Filters);return STATUS_INSUFFICIENT_RESOURCES;}
 c->filter=filter;KeInitializeTimer(&c->timer);KeInitializeDpc(&c->dpc,TimerDpc,c);KeInitializeSpinLock(&c->timer_lock);
 NTSTATUS s=KsAddItemToObjectBag(filter->Bag,c,FreeFilter);if(!NT_SUCCESS(s)){ExFreePoolWithTag(c,Tag);InterlockedDecrement(&Filters);return s;}
 filter->Context=c;return STATUS_SUCCESS;
}
NTSTATUS FilterClose(PKSFILTER filter,PIRP){InterlockedIncrement(&PinDiagnostics[30]);auto c=static_cast<FilterContext*>(filter->Context);if(c)TimerStop(c);return STATUS_SUCCESS;}
NTSTATUS Process(PKSFILTER,PKSPROCESSPIN_INDEXENTRY entries){
 // This timer-driven filter must stop processing after one tick. SUCCESS
 // with FRAMES_NOT_REQUIRED would immediately reschedule an empty queue.
 InterlockedIncrement(&PinDiagnostics[20]);PinDiagnostics[23]=1;PinDiagnostics[28]=KeGetCurrentIrql();PinDiagnostics[21]=entries?entries[0].Count:-1;
 if(!entries || !entries[0].Count || !entries[0].Pins)return STATUS_PENDING;
 auto p=entries[0].Pins[0];
 PinDiagnostics[22]=p?p->BytesAvailable:0;
 if(!p || !p->BytesAvailable)return STATUS_PENDING;
 if(p->BytesAvailable<FrameBytes || !p->Data || !p->StreamPointer){p->BytesUsed=0;p->Terminate=TRUE;return STATUS_PENDING;}
 auto context=static_cast<PinContext*>(p->Pin->Context);
 PinDiagnostics[23]=2;Lock();if(Producer && TitanAuthorizationAllowed(&Authorization))RtlCopyMemory(p->Data,Frame,FrameBytes);else RtlZeroMemory(p->Data,FrameBytes);++Delivered;Unlock();PinDiagnostics[23]=3;
 p->BytesUsed=FrameBytes;p->Terminate=TRUE;
 auto h=p->StreamPointer->StreamHeader;h->Duration=FrameInterval100ns;
 h->OptionsFlags=KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
 if(context->clock){h->PresentationTime.Time=context->clock->GetTime();h->PresentationTime.Numerator=1;h->PresentationTime.Denominator=1;h->OptionsFlags|=KSSTREAM_HEADER_OPTIONSF_TIMEVALID;}
 ++context->frames;
 if(h->Size>=sizeof(KSSTREAM_HEADER)+sizeof(KS_FRAME_INFO)){
  auto info=reinterpret_cast<PKS_FRAME_INFO>(h+1);RtlZeroMemory(info,sizeof(*info));info->ExtendedHeaderSize=sizeof(*info);info->PictureNumber=context->frames;
 }
 PinDiagnostics[23]=4;return STATUS_PENDING;
}
void FreePin(PVOID object){auto c=static_cast<PinContext*>(object);if(c->clock)c->clock->Release();ExFreePoolWithTag(c,Tag);}
NTSTATUS PinCreate(PKSPIN pin,PIRP){
 InterlockedIncrement(&PinDiagnostics[3]);PinDiagnostics[4]=4;
 auto c=static_cast<PinContext*>(ExAllocatePool2(POOL_FLAG_NON_PAGED,sizeof(PinContext),Tag));if(!c)return STATUS_INSUFFICIENT_RESOURCES;
 auto s=KsAddItemToObjectBag(pin->Bag,c,FreePin);if(!NT_SUCCESS(s)){ExFreePoolWithTag(c,Tag);return s;}
 pin->Context=c;pin->StreamHeaderSize=sizeof(KSSTREAM_HEADER)+sizeof(KS_FRAME_INFO);PinDiagnostics[4]=5;PinDiagnostics[5]=STATUS_SUCCESS;return STATUS_SUCCESS;
}
NTSTATUS PinState(PKSPIN pin,KSSTATE to,KSSTATE from){
 InterlockedIncrement(&PinDiagnostics[16]);PinDiagnostics[17]=to;PinDiagnostics[18]=from;PinDiagnostics[29]=1;
 auto c=static_cast<FilterContext*>(KsPinGetParentFilter(pin)->Context);auto p=static_cast<PinContext*>(pin->Context);
 if(to==KSSTATE_ACQUIRE && from==KSSTATE_STOP){PinDiagnostics[29]=2;KsPinGetReferenceClockInterface(pin,&p->clock);}
 PinDiagnostics[29]=3;
 if(to==KSSTATE_RUN){
  KIRQL irql;KeAcquireSpinLock(&c->timer_lock,&irql);if(!c->running){InterlockedIncrement(&PinDiagnostics[26]);c->running=TRUE;c->next=KeQueryInterruptTime()+FrameInterval100ns;LARGE_INTEGER due;due.QuadPart=-static_cast<LONGLONG>(FrameInterval100ns);KeSetTimer(&c->timer,due,&c->dpc);}KeReleaseSpinLock(&c->timer_lock,irql);
 } else TimerStop(c);
 if(to==KSSTATE_STOP && p->clock){p->clock->Release();p->clock=nullptr;}
 PinDiagnostics[29]=4;return STATUS_SUCCESS;
}
bool VideoValid(const KS_VIDEOINFOHEADER& v){
 return v.bmiHeader.biSize==sizeof(KS_BITMAPINFOHEADER) && v.bmiHeader.biWidth==Width && v.bmiHeader.biHeight==Height && v.bmiHeader.biPlanes==1 && v.bmiHeader.biBitCount==24 && v.bmiHeader.biCompression==KS_BI_RGB && v.bmiHeader.biSizeImage==FrameBytes && v.AvgTimePerFrame==FrameInterval100ns &&
 v.rcSource.left==0 && v.rcSource.top==0 && (v.rcSource.right==0 || v.rcSource.right==Width) && (v.rcSource.bottom==0 || v.rcSource.bottom==Height) && v.rcTarget.left==0 && v.rcTarget.top==0 && (v.rcTarget.right==0 || v.rcTarget.right==Width) && (v.rcTarget.bottom==0 || v.rcTarget.bottom==Height);
}
NTSTATUS PinFormat(PKSPIN pin,PKSDATAFORMAT old,PKSMULTIPLE_ITEM,const KSDATARANGE*,const KSATTRIBUTE_LIST*){
 InterlockedIncrement(&PinDiagnostics[2]);PinDiagnostics[4]=2;PinDiagnostics[7]=sizeof(KS_DATAFORMAT_VIDEOINFOHEADER);PinDiagnostics[14]=old!=nullptr;
 if(old && pin->DeviceState!=KSSTATE_STOP){PinDiagnostics[5]=STATUS_INVALID_DEVICE_STATE;return STATUS_INVALID_DEVICE_STATE;}
 auto f=pin->ConnectionFormat;
 if(f){PinDiagnostics[6]=f->FormatSize;PinDiagnostics[8]=f->SampleSize;}
 if(!f || f->FormatSize<sizeof(KS_DATAFORMAT_VIDEOINFOHEADER) || !IsEqualGUID(f->MajorFormat,KSDATAFORMAT_TYPE_VIDEO) || !IsEqualGUID(f->SubFormat,RGB24) || !IsEqualGUID(f->Specifier,KSDATAFORMAT_SPECIFIER_VIDEOINFO)){PinDiagnostics[5]=STATUS_NO_MATCH;return STATUS_NO_MATCH;}
 const auto& v=reinterpret_cast<PKS_DATAFORMAT_VIDEOINFOHEADER>(f)->VideoInfoHeader;
 PinDiagnostics[9]=v.bmiHeader.biWidth;PinDiagnostics[10]=v.bmiHeader.biHeight;PinDiagnostics[11]=v.bmiHeader.biSize;PinDiagnostics[12]=(v.bmiHeader.biPlanes<<16)|v.bmiHeader.biBitCount;PinDiagnostics[13]=static_cast<LONG>(v.AvgTimePerFrame);PinDiagnostics[15]=VideoValid(v);
 PinDiagnostics[5]=VideoValid(v)?STATUS_SUCCESS:STATUS_NO_MATCH;if(VideoValid(v))PinDiagnostics[4]=3;
 return static_cast<NTSTATUS>(PinDiagnostics[5]);
}

const 
KS_DATARANGE_VIDEO 
FormatRGB24Bpp_Capture = {

    //
    // KSDATARANGE
    //
    {   
        sizeof (KS_DATARANGE_VIDEO),                // FormatSize
        0,                                          // Flags
        Width * Height * 3,                              // SampleSize
        0,                                          // Reserved

        STATICGUIDOF (KSDATAFORMAT_TYPE_VIDEO),     // aka. MEDIATYPE_Video
        0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 
            0xaf, 0x0b, 0xa7, 0x70,                 // aka. MEDIASUBTYPE_RGB24,
        STATICGUIDOF (KSDATAFORMAT_SPECIFIER_VIDEOINFO) // aka. FORMAT_VideoInfo
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    0,                  // Reserved (was StreamDescriptionFlags)
    0,                  // Reserved (was MemoryAllocationFlags   
                        //           (KS_VIDEO_ALLOC_*))

    //
    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    //
    {
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VIDEOINFO ), // GUID
        0,                    // AnalogVideoStandard
        Width,Height,        // InputSize, (the inherent size of the incoming signal
                        //             with every digitized pixel unique)
        Width,Height,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        Width,Height,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        Width, Height,       // MinOutputSize, smallest bitmap stream can produce
        Width, Height,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY 
        FrameInterval100ns,         // MinFrameInterval, 100 nS units
        FrameInterval100ns,      // MaxFrameInterval, 100 nS units
        8 * 3 * 30 * Width * Height,  // MinBitsPerSecond;
        8 * 3 * 30 * Width * Height   // MaxBitsPerSecond;
    }, 
        
    //
    // KS_VIDEOINFOHEADER (default format)
    //
    {
        0,0,0,0,                            // RECT  rcSource; 
        0,0,0,0,                            // RECT  rcTarget; 
        Width * Height * 3 * 30 * 8,                 // DWORD dwBitRate;
        0L,                                 // DWORD dwBitErrorRate; 
        FrameInterval100ns,                             // REFERENCE_TIME  AvgTimePerFrame;   
        sizeof (KS_BITMAPINFOHEADER),       // DWORD biSize;
        Width,                                // LONG  biWidth;
        Height,                               // LONG  biHeight;
        1,                                  // WORD  biPlanes;
        24,                                 // WORD  biBitCount;
        KS_BI_RGB,                          // DWORD biCompression;
        Width * Height * 3,                      // DWORD biSizeImage;
        0,                                  // LONG  biXPelsPerMeter;
        0,                                  // LONG  biYPelsPerMeter;
        0,                                  // DWORD biClrUsed;
        0                                   // DWORD biClrImportant;
    }
}; 


NTSTATUS Intersect(PVOID,PIRP,PKSP_PIN,PKSDATARANGE caller,PKSDATARANGE descriptor,ULONG size,PVOID data,PULONG bytes){
 if(!caller || !descriptor || caller->FormatSize<sizeof(KS_DATARANGE_VIDEO) || !IsEqualGUID(caller->MajorFormat,descriptor->MajorFormat) || !IsEqualGUID(caller->SubFormat,descriptor->SubFormat) || !IsEqualGUID(caller->Specifier,descriptor->Specifier))return STATUS_NO_MATCH;
 auto range=reinterpret_cast<PKS_DATARANGE_VIDEO>(caller);
 if(!VideoValid(range->VideoInfoHeader))return STATUS_NO_MATCH;
 *bytes=sizeof(KS_DATAFORMAT_VIDEOINFOHEADER);
 if(!size)return STATUS_BUFFER_OVERFLOW;
 if(size<*bytes || !data)return STATUS_BUFFER_TOO_SMALL;
 auto format=static_cast<PKS_DATAFORMAT_VIDEOINFOHEADER>(data);RtlZeroMemory(format,*bytes);format->DataFormat=FormatRGB24Bpp_Capture.DataRange;format->DataFormat.FormatSize=*bytes;format->VideoInfoHeader=FormatRGB24Bpp_Capture.VideoInfoHeader;return STATUS_SUCCESS;
}
const KSPIN_DISPATCH PinDispatch={PinCreate,nullptr,nullptr,nullptr,PinFormat,PinState,nullptr,nullptr,nullptr,nullptr};
DECLARE_SIMPLE_FRAMING_EX(Framing,STATICGUIDOF(KSMEMORY_TYPE_KERNEL_NONPAGED),KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY|KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY,3,0,FrameBytes,FrameBytes);
const PKSDATARANGE Ranges[]={const_cast<PKSDATARANGE>(&FormatRGB24Bpp_Capture.DataRange)};
const GUID PinName={STATIC_PINNAME_VIDEO_CAPTURE};
const KSPIN_DESCRIPTOR_EX Pins[]={
 {&PinDispatch,nullptr,{0,nullptr,0,nullptr,1,Ranges,KSPIN_DATAFLOW_OUT,KSPIN_COMMUNICATION_BOTH,&KSCATEGORY_VIDEO,&PinName,0},KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING|KSPIN_FLAG_DO_NOT_INITIATE_PROCESSING|KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY,1,1,&Framing,Intersect}
};
const KSFILTER_DISPATCH FilterDispatch={FilterCreate,FilterClose,Process,nullptr};
const GUID Categories[]={STATICGUIDOF(KSCATEGORY_VIDEO),STATICGUIDOF(KSCATEGORY_CAPTURE),STATICGUIDOF(KSCATEGORY_VIDEO_CAMERA)};
const KSFILTER_DESCRIPTOR FilterDescriptor={&FilterDispatch,nullptr,KSFILTER_DESCRIPTOR_VERSION,KSFILTER_FLAG_CRITICAL_PROCESSING,&CameraReference,DEFINE_KSFILTER_PIN_DESCRIPTORS(Pins),DEFINE_KSFILTER_CATEGORIES(Categories),DEFINE_KSFILTER_NODE_DESCRIPTORS_NULL,DEFINE_KSFILTER_DEFAULT_CONNECTIONS,nullptr};
DEFINE_KSFILTER_DESCRIPTOR_TABLE(FilterDescriptors){&FilterDescriptor};
const KSDEVICE_DESCRIPTOR DeviceDescriptor={nullptr,SIZEOF_ARRAY(FilterDescriptors),FilterDescriptors,KSDEVICE_DESCRIPTOR_VERSION};
DRIVER_UNLOAD Unload;
void Unload(PDRIVER_OBJECT driver){
 UNICODE_STRING link=RTL_CONSTANT_STRING(L"\\DosDevices\\TitanVirtualCamera");IoDeleteSymbolicLink(&link);if(Control){IoDeleteDevice(Control);Control=nullptr;}
 if(PreviousUnload)PreviousUnload(driver);
 if(Frame){RtlSecureZeroMemory(Frame,FrameBytes);ExFreePoolWithTag(Frame,Tag);Frame=nullptr;}
}
}
extern "C" DRIVER_INITIALIZE DriverEntry;
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver,PUNICODE_STRING registry){
 PinDiagnostics[0]=1;KeInitializeMutex(&SessionMutex,0);ExInitializeFastMutex(&FrameLock);Frame=static_cast<PUCHAR>(ExAllocatePool2(POOL_FLAG_NON_PAGED,FrameBytes,Tag));if(!Frame)return STATUS_INSUFFICIENT_RESOURCES;
 auto status=KsInitializeDriver(driver,registry,&DeviceDescriptor);if(!NT_SUCCESS(status)){ExFreePoolWithTag(Frame,Tag);Frame=nullptr;return status;}
 PreviousUnload=driver->DriverUnload;
 UNICODE_STRING name=RTL_CONSTANT_STRING(L"\\Device\\TitanVirtualCameraControl");UNICODE_STRING acl=RTL_CONSTANT_STRING(L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
 status=IoCreateDeviceSecure(driver,0,&name,FILE_DEVICE_UNKNOWN,FILE_DEVICE_SECURE_OPEN,FALSE,&acl,&ControlClass,&Control);
 if(!NT_SUCCESS(status)){if(PreviousUnload)PreviousUnload(driver);ExFreePoolWithTag(Frame,Tag);Frame=nullptr;return status;}
 UNICODE_STRING link=RTL_CONSTANT_STRING(L"\\DosDevices\\TitanVirtualCamera");status=IoCreateSymbolicLink(&link,&name);
 if(!NT_SUCCESS(status)){IoDeleteDevice(Control);Control=nullptr;if(PreviousUnload)PreviousUnload(driver);ExFreePoolWithTag(Frame,Tag);Frame=nullptr;return status;}
 for(unsigned i=0;i<=IRP_MJ_MAXIMUM_FUNCTION;++i)Previous[i]=driver->MajorFunction[i];
 // Every major function must route the standalone control object away from
 // AVStream routines, whose device-extension contract belongs only to KS FDOs.
 for(unsigned i=0;i<=IRP_MJ_MAXIMUM_FUNCTION;++i)driver->MajorFunction[i]=Dispatch;
 driver->DriverUnload=Unload;Control->Flags&=~DO_DEVICE_INITIALIZING;return STATUS_SUCCESS;
}
