#include "../Source/dsp/SceneDSP.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 afar::SceneDSP e;e.prepare(48000.,2,0);
 afar::SceneControls p;p.scene=5;p.distance=3.f;
 e.setScene(5);
 float mx=0.f;
 for(int i=0;i<200000;++i){
   auto f=e.processFrame({i==0?1.f:0.f,i==0?1.f:0.f},p,1.f,0.f);
   assert(std::isfinite(f.left) && std::isfinite(f.right));
   mx=std::max(mx,std::abs(f.left));
 }
 assert(mx>.01f);
 std::cout<<"Signal path is finite and emits audio in headless stub compilation\n";
}
