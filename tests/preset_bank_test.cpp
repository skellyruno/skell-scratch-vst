#include "TestCheck.h"
#include "../Source/ui/PresetBank.h"
#include <cmath>
int main(){
 REQUIRE(ScratchPresets::count()==36);
 for(int preset=0;preset<ScratchPresets::count();++preset){
  auto nodes=ScratchPresets::make(preset);
  REQUIRE(nodes.size()>=2 && nodes.size()<=512);
  REQUIRE(ScratchPresets::names[(size_t)preset][0]!=0);
  CurveMath::normalize(nodes);
  REQUIRE(nodes.front().x==0.f && nodes.back().x==1.f);
  for(size_t i=1;i<nodes.size();++i)REQUIRE(nodes[i-1].x<nodes[i].x);
  for(int i=0;i<=100;++i){
   const auto sample=CurveMath::evaluate(nodes,i/100.f);
   REQUIRE(std::isfinite(sample) && sample>=-2.f && sample<=2.f);
  }
 }
 return 0;
}
