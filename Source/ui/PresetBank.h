#pragma once
#include "../CurveMath.h"
#include <vector>
#include <array>
#include <cmath>
namespace ScratchPresets {
inline constexpr std::array<const char*,36> names{{"Skelly Scratch", "Classic Cut", "Tape Brake", "Reverse Rise", "Baby Scratch", "Stutter 1/8", "Stutter 1/16", "Transformer", "Crab Scratch", "Chirp", "Orbit Reverse", "Vinyl Spin Up", "Stop & Go", "Half-Time", "Double-Time", "Triplet Chop", "Wobble", "Ramp Down", "Ramp Up", "Freeze Frames", "Punch In", "Reverse Tape", "Flow Scratch", "Funk Bounce", "Triple Kill", "2-Step Cut", "Slip Back", "Vinyl Stop", "Platter Push", "Sharp Cut", "Lo-Fi Tremor", "Beat Freeze", "Counter Spin", "Mirror Steps", "Roller Coaster", "Glitch Dive"}};
inline std::vector<CurveNode> make(int id) {
 std::vector<CurveNode> v;
 auto put=[&](float x,float y,float t=0.f){v.push_back({x,y,t});};
 switch(id) {
 case 0: for(int k=0;k<=12;++k)put(k/24.f,(k%2)?-1.f:1.f);put(.68f,.8f,-.12f);put(1,0);break;
 case 1: for(int k=0;k<=8;++k)put(k/8.f,(k%2)?-1.f:1.f);break;
 case 2: put(0,1);put(.28f,.75f,-.12f);put(.65f,.25f);put(1,0);break;
 case 3: put(0,-1);put(.4f,-.75f);put(.7f,.0f,.12f);put(1,1);break;
 case 4: for(int k=0;k<=8;++k)put(k/8.f,(k%2)?-.5f:.5f);break;
 case 5: case 6: case 7: case 8: case 15: {int cuts=id==5?8:id==6?16:id==7?12:id==8?24:12; for(int k=0;k<=cuts;++k)put((float)k/cuts,(k%2)?-1.f:1.f);break;}
 case 9: put(0,1);put(.13f,1);put(.24f,-1);put(.33f,-1);put(.47f,1);put(.66f,.5f);put(1,0);break;
 case 10: put(0,-1);put(.24f,-1);put(.48f,0);put(.73f,-1);put(1,0);break;
 case 11: put(0,0);put(.3f,.2f);put(.65f,.7f);put(1,1);break;
 case 12: for(int k=0;k<4;++k){put(k*.25f,1);put(k*.25f+.12f,0);}put(1,0);break;
 case 13: put(0,.5f);put(1,.5f);break;
 case 14: put(0,2);put(1,2);break;
 case 16: for(int k=0;k<=16;++k)put(k/16.f,std::sin(k*3.14159265358979323846f/4));break;
 case 17: put(0,1);put(1,0);break;
 case 18: put(0,0);put(1,1);break;
 case 19: for(int k=0;k<=8;++k)put(k/8.f,(k%3==0)?0.f:(k%3==1?.8f:-.8f));break;
 case 20: put(0,0);put(.12f,0);put(.125f,1.6f);put(.30f,1.6f);put(.32f,0);put(1,0);break;
 case 21: put(0,-1.7f);put(.3f,-1.2f,.1f);put(.7f,-.5f);put(1,0);break;
 case 22: put(0,1);put(.15f,.3f,.13f);put(.34f,-.5f,-.15f);put(.55f,.8f);put(.7f,-.8f);put(1,0);break;
 case 23: put(0,1);put(.125f,1);put(.25f,0);put(.375f,-1);put(.5f,1);put(.625f,0);put(.75f,1);put(.875f,-.25f);put(1,0);break;
 case 24: for(int k=0;k<=12;++k)put(k/12.f,(k%3==2)?-1.5f:1.5f);break;
 case 25: put(0,1);put(.24f,1);put(.25f,-1);put(.49f,-1);put(.5f,1);put(.74f,1);put(.75f,-1);put(1,-1);break;
 case 26: put(0,.6f);put(.125f,-1.4f);put(.25f,.4f);put(.5f,-1.f);put(.7f,.6f);put(1,0);break;
 case 27: put(0,1.9f,-.3f);put(.25f,1.5f,-.3f);put(.65f,.35f,-.1f);put(1,0);break;
 case 28: put(0,0);put(.22f,.3f,.14f);put(.44f,.8f,.12f);put(.7f,1.4f);put(1,2);break;
 case 29: for(int k=0;k<=8;++k)put(k/8.f,(k%2)?-2.f:2.f);break;
 case 30: for(int k=0;k<=24;++k)put(k/24.f,.55f+.25f*std::sin(k*.94f));break;
 case 31: put(0,1);put(.24f,1);put(.25f,0);put(.75f,0);put(.76f,1);put(1,1);break;
 case 32: put(0,-2);put(.25f,-1);put(.50f,1);put(.75f,-2);put(1,0);break;
 case 33: put(0,1);put(.2f,-1);put(.4f,1);put(.6f,-1);put(.8f,1);put(1,-1);break;
 case 34: for(int k=0;k<=16;++k)put(k/16.f,1.7f*std::sin(k*3.14159265358979323846f/8));break;
 case 35: for(int k=0;k<=32;++k)put(k/32.f,(k%8==0)?0.f:((k%4<2)?1.5f:-1.5f));break;
 default: put(0,1);put(1,0);break;
 }
 return v;
}
inline int count(){return (int)names.size();}
}
