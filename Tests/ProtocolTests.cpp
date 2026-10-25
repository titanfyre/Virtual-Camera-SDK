#include "CameraProtocol.h"
#include "DriverAuthorizationProtocol.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
void Require(bool value){if(!value)std::abort();}
int main(){
 using namespace CameraProtocol;Header h{Version,sizeof(Header),Width,Height,Stride,0};unsigned checks=0;
 Require(Valid(h,sizeof(Header)+FrameBytes));++checks;
 for(unsigned n=0;n<100000;++n){Require(!Valid(h,n));++checks;}
 for(unsigned f=0;f<6;++f){auto copy=h;reinterpret_cast<unsigned*>(&copy)[f]^=0x80000000u;Require(!Valid(copy,sizeof(Header)+FrameBytes));++checks;}
 Require(!Valid(h,0xffffffffu));++checks;Require(FrameBytes==921600);++checks;
 Require(sizeof(TITAN_AUTHORIZATION_TOKEN)==112);++checks;
 std::cout<<"Protocol tests passed: "<<checks<<" checks. No driver or camera was opened.\n";
}
