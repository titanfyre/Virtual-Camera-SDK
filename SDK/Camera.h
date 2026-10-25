#pragma once
#include "CameraProtocol.h"
#include "DriverAuthorizationProtocol.h"
#include <span>
#include <string>
#include <string_view>
#include <vector>

// One producer per machine. Serialize calls on each Camera instance.
// A session access key is requested once; no HTTP occurs for each frame.
class Camera {
 HANDLE handle_=INVALID_HANDLE_VALUE;
 std::vector<unsigned char> packet_;
public:
 Camera()=default;
 Camera(const Camera&)=delete;
 Camera& operator=(const Camera&)=delete;
 ~Camera();
 std::string Activate(std::string_view accessKey);
 DWORD Send(std::span<const unsigned char> bgr24);
 DWORD Query(CameraProtocol::State& state) const;
 void Close() noexcept;
 static std::string Error(DWORD code);
};
