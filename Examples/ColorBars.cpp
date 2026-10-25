#include "Camera.h"
#include <conio.h>
#include <iostream>
#include <vector>
int main(){
 std::cout<<"Send a raw BGR24 color-bar frame to Titan Virtual Camera.\nAccess key (not echoed): ";
 std::string key;for(int c;(c=_getch())!='\r';){if(c==3)return 1;if(c=='\b'){if(!key.empty())key.pop_back();}else if(c>=32&&c<127&&key.size()<128)key.push_back(static_cast<char>(c));}
 Camera camera;auto error=camera.Activate(key);SecureZeroMemory(key.data(),key.size());
 if(!error.empty()){std::cout<<"\n"<<error<<"\n";return 1;}
 std::vector<unsigned char> frame(CameraProtocol::FrameBytes);
 for(unsigned y=0;y<CameraProtocol::Height;++y)for(unsigned x=0;x<CameraProtocol::Width;++x){unsigned i=y*CameraProtocol::Stride+x*3;frame[i]=static_cast<unsigned char>(x*255/CameraProtocol::Width);frame[i+1]=static_cast<unsigned char>(y*255/CameraProtocol::Height);frame[i+2]=200;}
 if(DWORD code=camera.Send(frame)){std::cout<<Camera::Error(code)<<"\n";return 1;}
 std::cout<<"\nFrame sent. Select Titan Virtual Camera in your capture app. Enter closes the producer and returns its output to black.\n";std::cin.get();
}
