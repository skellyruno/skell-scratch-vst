#include "../Source/ui/PresetBank.h"
#include <cassert>
#include <cmath>
int main(){
 assert(ScratchPresets::count()==36);
 for(int preset=0;preset<ScratchPresets::count();++preset){
  auto nodes=ScratchPresets::make(preset);
  assert(nodes.size()>=2 && nodes.size()<=512);
  assert(ScratchPresets::names[(size_t)preset][0]!=0);
  CurveMath::normalize(nodes);
  assert(nodes.front().x==0.f && nodes.back().x==1.f);
  for(size_t i=1;i<nodes.size();++i)assert(nodes[i-1].x<nodes[i].x);
  for(int i=0;i<=100;++i){
   const auto sample=CurveMath::evaluate(nodes,i/100.f);
   assert(std::isfinite(sample) && sample>=-2.f && sample<=2.f);
  }
 }
 return 0;
}
