#include "SDK/Camera.h"
#include "WireTypes.h"
#include <map>
struct ProductSession{
Camera client;
Json Invoke(const std::string& op,const Json& a){

if(op=="check"){auto data=Bytes(a.at("data"),CameraProtocol::FrameBytes);return {{"valid",data.size()==CameraProtocol::FrameBytes},{"width",CameraProtocol::Width},{"height",CameraProtocol::Height},{"stride",CameraProtocol::Stride},{"frame_bytes",CameraProtocol::FrameBytes}};}
if(op=="activate"){CheckMessage(client.Activate(a.at("key").get<std::string>()));return Json::object();}
if(op=="send"){auto data=Bytes(a.at("data"),CameraProtocol::FrameBytes);Require(data.size()==CameraProtocol::FrameBytes,"Expected a complete BGR24 frame");CheckCode(client.Send(std::span<const unsigned char>(data)));return Json::object();}
if(op=="status"){CameraProtocol::State s{};CheckCode(client.Query(s));return {{"version",s.version},{"width",s.width},{"height",s.height},{"stride",s.stride},{"frame_bytes",s.frame_bytes},{"frame_interval_100ns",s.frame_interval_100ns},{"producer_active",s.producer_active},{"submitted",std::to_string(s.submitted)},{"delivered",std::to_string(s.delivered)},{"sequence",std::to_string(s.sequence)}};}
if(op=="close"){client.Close();return Json::object();}

throw SdkError(50,"Unknown operation");}
};
static Runtime<ProductSession> runtime;
extern "C" __declspec(dllexport) unsigned virtual_camera_sdk_json_open(std::uint64_t* id)noexcept{return runtime.Open(id);}
extern "C" __declspec(dllexport) unsigned virtual_camera_sdk_json_close(std::uint64_t id)noexcept{return runtime.Close(id);}
extern "C" __declspec(dllexport) char* virtual_camera_sdk_json_call(std::uint64_t id,const char* request,std::uint32_t length)noexcept{return runtime.Call(id,request,length);}
extern "C" __declspec(dllexport) void virtual_camera_sdk_json_free(char* response)noexcept{delete[] response;}
