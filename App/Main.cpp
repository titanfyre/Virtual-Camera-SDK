#include "Camera.h"
#include <commctrl.h>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
namespace {
HWND window,keybox,start,stop,status;HANDLE stopEvent;std::thread worker;HFONT font;HBRUSH background;std::atomic_bool running=false;bool closing=false,stopRequested=false;
void Report(std::wstring message){auto value=new(std::nothrow)std::wstring(std::move(message));if(value&&!PostMessageW(window,WM_APP+1,0,reinterpret_cast<LPARAM>(value)))delete value;}
std::wstring Wide(const std::string& s){if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),nullptr,0);std::wstring w(n,L' ');MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),w.data(),n);return w;}
void Start(){
 if(running.exchange(true))return;if(worker.joinable())worker.join();ResetEvent(stopEvent);stopRequested=false;
 char raw[129]{};GetWindowTextA(keybox,raw,129);std::string key(raw);SecureZeroMemory(raw,sizeof(raw));SetWindowTextW(keybox,L"");EnableWindow(start,FALSE);EnableWindow(stop,TRUE);SetWindowTextW(status,L"Requesting a session permission over HTTPS...");
 try{worker=std::thread([key=std::move(key)]()mutable{
  try{
   Camera camera;auto error=camera.Activate(key);
   if(!error.empty())Report(Wide(error));
   else {
    std::vector<unsigned char> frame(CameraProtocol::FrameBytes);unsigned tick=0;
    std::thread renewal([&]{
     DWORD delay=45*60*1000;
     while(WaitForSingleObject(stopEvent,delay)==WAIT_TIMEOUT){
      try{
       auto failure=camera.Activate(key);
       if(failure.empty()){delay=45*60*1000;continue;}
       Report(L"Authorization renewal: "+Wide(failure));
       const bool transient=failure.find("Windows error")!=std::string::npos || failure.find("HTTP 429")!=std::string::npos || failure.find("HTTP 5")!=std::string::npos;
       if(!transient){SetEvent(stopEvent);break;}
       delay=60000;
      }catch(...){PostMessageW(window,WM_APP+3,0,0);SetEvent(stopEvent);break;}
     }
    });
    struct JoinRenewal{std::thread& thread;~JoinRenewal(){SetEvent(stopEvent);if(thread.joinable())thread.join();}} joinRenewal{renewal};
    while(WaitForSingleObject(stopEvent,33)==WAIT_TIMEOUT){
     for(unsigned y=0;y<CameraProtocol::Height;++y)for(unsigned x=0;x<CameraProtocol::Width;++x){unsigned i=y*CameraProtocol::Stride+x*3;frame[i]=static_cast<unsigned char>((x+tick)%256);frame[i+1]=static_cast<unsigned char>((y+tick)%256);frame[i+2]=static_cast<unsigned char>(180+(tick%60));}
     if(DWORD code=camera.Send(frame)){Report(L"Frame submission stopped: "+Wide(Camera::Error(code)));SetEvent(stopEvent);break;}
     if((tick++%15)==0){CameraProtocol::State state{};if(!camera.Query(state))Report(L"Live synthetic frames  |  640 x 480 BGR24  |  Submitted: "+std::to_wstring(state.submitted)+L"  |  Delivered to capture clients: "+std::to_wstring(state.delivered));}
    }
    SetEvent(stopEvent);if(renewal.joinable())renewal.join();
   }
  }catch(...){PostMessageW(window,WM_APP+3,0,0);}
  SecureZeroMemory(key.data(),key.size());key.clear();running=false;PostMessageW(window,WM_APP+2,0,0);
 });}catch(...){running=false;EnableWindow(start,TRUE);EnableWindow(stop,FALSE);SetWindowTextW(status,L"Not enough resources to start the producer.");}
}

LRESULT CALLBACK Proc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){
case WM_CREATE:{
 font=CreateFontW(19,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 auto label=CreateWindowW(L"STATIC",L"VIRTUAL CAMERA SDK",WS_CHILD|WS_VISIBLE,28,26,600,32,h,nullptr,nullptr,nullptr);
 auto subtitle=CreateWindowW(L"STATIC",L"One raw frame API. A standard Windows camera stream.",WS_CHILD|WS_VISIBLE,28,65,680,30,h,nullptr,nullptr,nullptr);
 keybox=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,28,122,480,36,h,nullptr,nullptr,nullptr);SendMessageW(keybox,EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(L"Evaluation / OEM application access key"));SendMessageW(keybox,EM_SETLIMITTEXT,128,0);
 start=CreateWindowW(L"BUTTON",L"Start color bars",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,28,178,195,44,h,reinterpret_cast<HMENU>(1),nullptr,nullptr);
 stop=CreateWindowW(L"BUTTON",L"Stop producer",WS_CHILD|WS_VISIBLE|WS_TABSTOP,236,178,175,44,h,reinterpret_cast<HMENU>(2),nullptr,nullptr);EnableWindow(stop,FALSE);
 status=CreateWindowW(L"EDIT",L"Install the signed driver, enter an access key and start. Select Titan Virtual Camera in your capture application.\r\n\r\nOnly generated pixels are sent. Closing the producer returns the camera to black. No physical camera, desktop or audio is captured.",WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,28,250,675,150,h,nullptr,nullptr,nullptr);
 for(auto child:{label,subtitle,keybox,start,stop,status})SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return 0;
}
case WM_COMMAND:if(LOWORD(w)==1)Start();else if(LOWORD(w)==2){stopRequested=true;SetEvent(stopEvent);SetWindowTextW(status,L"Stopping producer...");}return 0;
case WM_APP+1:{std::unique_ptr<std::wstring> s(reinterpret_cast<std::wstring*>(l));SetWindowTextW(status,s->c_str());return 0;}
case WM_APP+3:SetWindowTextW(status,L"The producer stopped because Windows could not provide the required resources.");return 0;
case WM_APP+2:if(worker.joinable())worker.join();EnableWindow(start,TRUE);EnableWindow(stop,FALSE);if(stopRequested)SetWindowTextW(status,L"Producer stopped. Capture clients now receive black frames.");if(closing)DestroyWindow(h);return 0;
case WM_CTLCOLOREDIT:case WM_CTLCOLORSTATIC:SetTextColor(reinterpret_cast<HDC>(w),RGB(232,238,248));SetBkColor(reinterpret_cast<HDC>(w),RGB(18,25,39));return reinterpret_cast<LRESULT>(background);
case WM_ERASEBKGND:{RECT r;GetClientRect(h,&r);FillRect(reinterpret_cast<HDC>(w),&r,background);return 1;}
case WM_SETCURSOR:if(reinterpret_cast<HWND>(w)==start||reinterpret_cast<HWND>(w)==stop){SetCursor(LoadCursorW(nullptr,IDC_HAND));return TRUE;}break;
case WM_CLOSE:SetEvent(stopEvent);if(running){closing=true;EnableWindow(start,FALSE);EnableWindow(stop,FALSE);SetWindowTextW(status,L"Stopping. Finishing the current bounded authorization request...");return 0;}if(worker.joinable())worker.join();DestroyWindow(h);return 0;
case WM_DESTROY:{MSG pending;while(PeekMessageW(&pending,h,WM_APP+1,WM_APP+1,PM_REMOVE))delete reinterpret_cast<std::wstring*>(pending.lParam);DeleteObject(font);PostQuitMessage(0);return 0;}
}return DefWindowProcW(h,m,w,l);}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show){
 SetProcessDPIAware();background=CreateSolidBrush(RGB(18,25,39));stopEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!stopEvent)return 1;
 WNDCLASSW c{};c.lpfnWndProc=Proc;c.hInstance=instance;c.lpszClassName=L"TitanCameraStudio";c.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&c);
 window=CreateWindowW(c.lpszClassName,L"Virtual Camera SDK - Frame Producer",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,750,465,nullptr,nullptr,instance,nullptr);if(!window){CloseHandle(stopEvent);DeleteObject(background);return 1;}
 ShowWindow(window,show);MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0)if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}CloseHandle(stopEvent);DeleteObject(background);return 0;
}
