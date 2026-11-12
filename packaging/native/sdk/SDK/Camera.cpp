#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Camera.h"
#include <winhttp.h>
#include <wincrypt.h>
#include <array>
#include <cstring>
#include <utility>
#pragma comment(lib,"winhttp.lib")
#pragma comment(lib,"crypt32.lib")

namespace {
struct Http {
    HINTERNET value = nullptr;
    explicit Http(HINTERNET handle) : value(handle) {}
    ~Http() { if (value) WinHttpCloseHandle(value); }
    Http(const Http&) = delete;
    Http& operator=(const Http&) = delete;
};
std::string Failure(const char* stage) {
    return std::string(stage) + " failed (Windows error " +
        std::to_string(GetLastError()) + ").";
}
bool ValidKey(std::string_view key) {
    if (key.size() != 81 || key.substr(0, 5) != "app1_" || key[37] != '_')
        return false;
    for (size_t i = 5; i != 37; ++i)
        if (!((key[i] >= '0' && key[i] <= '9') ||
              (key[i] >= 'a' && key[i] <= 'f'))) return false;
    for (size_t i = 38; i != key.size(); ++i)
        if (!((key[i] >= 'a' && key[i] <= 'z') ||
              (key[i] >= 'A' && key[i] <= 'Z') ||
              (key[i] >= '0' && key[i] <= '9') ||
               key[i] == '_' || key[i] == '-')) return false;
    return true;
}
}

namespace {
std::string RequestDriverAuthorization(std::string_view key,
    const TITAN_AUTHORIZATION_CHALLENGE& challenge,
    TITAN_AUTHORIZATION_TOKEN& token) {
    token = {};
    if (!ValidKey(key)) return "Enter a valid application access key (app1_...).";
    if (challenge.magic != TITAN_AUTHORIZATION_MAGIC ||
        challenge.version != TITAN_AUTHORIZATION_VERSION ||
        challenge.duration_seconds != 0 ||
        (challenge.product != TITAN_AUTHORIZATION_NETWORK &&
         challenge.product != TITAN_AUTHORIZATION_CAMERA))
        return "The driver returned an invalid authorization challenge.";
    const char* sku = challenge.product == TITAN_AUTHORIZATION_NETWORK ?
        "virtual-network-adapter-sdk" : "virtual-camera-sdk";
    std::string nonce;
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char value : challenge.nonce) {
        nonce += hex[value >> 4]; nonce += hex[value & 15];
    }
    std::string body = "{\"key\":\"" + std::string(key) +
        "\",\"sku\":\"" + sku + "\",\"nonce\":\"" + nonce + "\"}";
    Http session(WinHttpOpen(L"Titan Driver SDK/0.1",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.value) { SecureZeroMemory(body.data(), body.size()); return Failure("HTTPS initialization"); }
    WinHttpSetTimeouts(session.value, 5000, 5000, 10000, 10000);
    Http connection(WinHttpConnect(session.value, L"licences.configcsa.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection.value) { SecureZeroMemory(body.data(), body.size()); return Failure("HTTPS connection"); }
    Http request(WinHttpOpenRequest(connection.value, L"POST",
        L"/portfolio/api/v1/driver-authorize", nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request.value) { SecureZeroMemory(body.data(), body.size()); return Failure("Authorization request"); }
    DWORD disabled = WINHTTP_DISABLE_REDIRECTS | WINHTTP_DISABLE_COOKIES;
    if (!WinHttpSetOption(request.value, WINHTTP_OPTION_DISABLE_FEATURE,
        &disabled, sizeof(disabled))) {
        SecureZeroMemory(body.data(), body.size()); return Failure("HTTPS policy");
    }
    const BOOL sent = WinHttpSendRequest(request.value,
        L"Content-Type: application/json\r\n", static_cast<DWORD>(-1),
        body.data(), static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()), 0);
    const DWORD sendError = sent ? ERROR_SUCCESS : GetLastError();
    SecureZeroMemory(body.data(), body.size());
    if (!sent) return "Authorization request failed (Windows error " + std::to_string(sendError) + ").";
    if (!WinHttpReceiveResponse(request.value, nullptr)) return Failure("Authorization response");
    DWORD status = 0, length = sizeof(status);
    if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE |
        WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status,
        &length, WINHTTP_NO_HEADER_INDEX)) return Failure("HTTP status");
    std::string response;
    std::array<char, 1024> chunk{};
    for (;;) {
        DWORD count = 0;
        if (!WinHttpReadData(request.value, chunk.data(),
            static_cast<DWORD>(chunk.size()), &count)) return Failure("Response read");
        if (!count) break;
        if (response.size() + count > 8192) return "Authorization response is too large.";
        response.append(chunk.data(), count);
    }
    if (status != 200) {
        for (char& c : response) if (static_cast<unsigned char>(c) < 32) c = ' ';
        return "License server refused authorization (HTTP " +
            std::to_string(status) + "): " + response;
    }
    // Only extract a bounded base64 field. The kernel verifies every byte of
    // the decoded permission and its signature before enabling any producer.
    const auto field = response.find("\"grant_base64\"");
    if (field == std::string::npos) return "Missing authorization grant in server response.";
    const auto colon = response.find(':', field + 14);
    const auto start = colon == std::string::npos ? std::string::npos : response.find('"', colon + 1);
    const auto end = start == std::string::npos ? std::string::npos : response.find('"', start + 1);
    if (end == std::string::npos || end - start - 1 != 152)
        return "Invalid authorization grant encoding.";
    const std::string grant = response.substr(start + 1, end - start - 1);
    DWORD bytes = sizeof(token);
    if (!CryptStringToBinaryA(grant.c_str(), static_cast<DWORD>(grant.size()),
        CRYPT_STRING_BASE64 | CRYPT_STRING_STRICT,
        reinterpret_cast<BYTE*>(&token), &bytes, nullptr, nullptr) ||
        bytes != sizeof(token)) {
        token = {}; return "Invalid authorization grant length or encoding.";
    }
    if (token.permission.magic != challenge.magic ||
        token.permission.version != challenge.version ||
        token.permission.product != challenge.product ||
        token.permission.duration_seconds == 0 ||
        token.permission.duration_seconds > TITAN_AUTHORIZATION_MAX_SECONDS ||
        std::memcmp(token.permission.nonce, challenge.nonce, sizeof(challenge.nonce))) {
        token = {}; return "Authorization grant does not match this driver session.";
    }
    return {};
}

} // Authorization transport is private to this SDK translation unit.
Camera::~Camera(){Close();}
void Camera::Close() noexcept {if(handle_!=INVALID_HANDLE_VALUE){CloseHandle(handle_);handle_=INVALID_HANDLE_VALUE;}if(!packet_.empty())SecureZeroMemory(packet_.data(),packet_.size());packet_.clear();}
std::string Camera::Error(DWORD code){
 char* message=nullptr;FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,code,MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),reinterpret_cast<LPSTR>(&message),0,nullptr);
 std::string result=message?message:"Windows operation failed";if(message)LocalFree(message);while(!result.empty()&&(result.back()=='\r'||result.back()=='\n'))result.pop_back();return result+" (Windows error "+std::to_string(code)+").";
}
std::string Camera::Activate(std::string_view key){
 if(handle_==INVALID_HANDLE_VALUE){
  handle_=CreateFileW(L"\\\\.\\TitanVirtualCamera",GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
  if(handle_==INVALID_HANDLE_VALUE)return "Open virtual camera producer: "+Error(GetLastError())+" Install the signed camera driver and run as administrator.";
 }
 TITAN_AUTHORIZATION_CHALLENGE challenge{};DWORD bytes=0;
 if(!DeviceIoControl(handle_,CameraProtocol::Challenge,nullptr,0,&challenge,sizeof(challenge),&bytes,nullptr)||bytes!=sizeof(challenge))return "Read session challenge: "+Error(GetLastError()?GetLastError():ERROR_INVALID_DATA);
 TITAN_AUTHORIZATION_TOKEN token{};auto failure=RequestDriverAuthorization(key,challenge,token);if(!failure.empty())return failure;
 BOOL ok=DeviceIoControl(handle_,CameraProtocol::Authorize,&token,sizeof(token),nullptr,0,&bytes,nullptr);DWORD error=ok?0:GetLastError();SecureZeroMemory(&token,sizeof(token));
 if(error)return "The driver rejected the session permission: "+Error(error);
 if(!packet_.empty())return {}; // Renewal does not touch the live frame packet.
 try{packet_.resize(sizeof(CameraProtocol::Header)+CameraProtocol::FrameBytes);}catch(...){Close();return "Not enough memory for the fixed camera frame.";}
 CameraProtocol::Header header{CameraProtocol::Version,sizeof(CameraProtocol::Header),CameraProtocol::Width,CameraProtocol::Height,CameraProtocol::Stride,0};std::memcpy(packet_.data(),&header,sizeof(header));return {};
}
DWORD Camera::Send(std::span<const unsigned char> frame){
 if(handle_==INVALID_HANDLE_VALUE||packet_.empty())return ERROR_INVALID_HANDLE;
 if(frame.size()!=CameraProtocol::FrameBytes)return ERROR_INVALID_PARAMETER;
 std::memcpy(packet_.data()+sizeof(CameraProtocol::Header),frame.data(),frame.size());DWORD bytes=0;
 return DeviceIoControl(handle_,CameraProtocol::Submit,packet_.data(),static_cast<DWORD>(packet_.size()),nullptr,0,&bytes,nullptr)?ERROR_SUCCESS:GetLastError();
}
DWORD Camera::Query(CameraProtocol::State& state) const {
 state={};if(handle_==INVALID_HANDLE_VALUE)return ERROR_INVALID_HANDLE;DWORD bytes=0;
 if(!DeviceIoControl(handle_,CameraProtocol::Query,nullptr,0,&state,sizeof(state),&bytes,nullptr))return GetLastError();
 return bytes==sizeof(state)&&state.version==CameraProtocol::Version&&state.bytes==sizeof(state)?ERROR_SUCCESS:ERROR_INVALID_DATA;
}
