#include "../Source/CurveMath.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

static bool almost(float a,float b){return std::abs(a-b)<.00001f;}
int main(){
 std::vector<CurveNode> nodes={{1.f,0.f,0.f},{0.f,1.f,.5f},{.5f,0.f,0.f}};
 CurveMath::normalize(nodes);
 assert(nodes.size()==3 && nodes[0].x==0.f && nodes[2].x==1.f);
 assert(almost(CurveMath::evaluate(nodes,0.f),1.f));
 assert(almost(CurveMath::evaluate(nodes,.5f),0.f));
 assert(almost(CurveMath::evaluate(nodes,.25f),1.f)); // midpoint: average .5 + tension .5
 assert(almost(CurveMath::evaluate(nodes,-1.f),1.f));
 assert(almost(CurveMath::evaluate(nodes,100.f),0.f));
 nodes={{.5f,1.f,0.f},{.5f,-1.f,0.f}};
 CurveMath::normalize(nodes);
 assert(nodes.size()>=2 && nodes.front().x==0.f && nodes.back().x==1.f);
 for(size_t i=1;i<nodes.size();++i)assert(nodes[i].x>nodes[i-1].x);
 nodes={{.2f,std::numeric_limits<float>::infinity(),0.f},{.8f,0.f,NAN}};
 CurveMath::normalize(nodes);
 for(const auto& n:nodes){assert(std::isfinite(n.x)&&std::isfinite(n.y)&&std::isfinite(n.tension));}
 nodes={{0.f,-2.f,2.f},{1.f,2.f,0.f}};
 assert(CurveMath::evaluate(nodes,.5f)<=2.f);
 nodes.clear();CurveMath::normalize(nodes);
 assert(nodes.size()==2 && nodes.front().x==0.f && nodes.back().x==1.f);
 nodes.clear();for(int i=0;i<1000;++i)nodes.push_back({i/1000.f,0.f,0.f});
 CurveMath::normalize(nodes);assert(nodes.size()<=512);
 return 0;
}
