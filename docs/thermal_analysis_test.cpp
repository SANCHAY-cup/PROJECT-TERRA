#include <cstdio>
#include <cmath>
#include "../include/thermal_analysis.h"
using namespace terra;
static int fails=0;
#define CHECK(c) do{ if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);fails++;} }while(0)
int main(){
  // Grid taken from the user's screenshot (person in the centre, room 24 C)
  float g[64]; for(int i=0;i<64;i++) g[i]=24;
  auto set=[&](int r,int c0,float a,float b,float c,float d,float e,float f){ float v[6]={a,b,c,d,e,f}; for(int k=0;k<6;k++) g[r*8+c0+k]=v[k]; };
  set(0,1,24,25,26,25,24,24); set(1,1,24,25,28,28,25,24); set(2,1,24,26,31,29,25,24);
  set(3,1,24,27,34,32,25,24); set(4,1,24,27,34,31,25,24); set(5,1,24,26,32,30,25,24);
  set(6,1,24,25,29,28,25,24); set(7,1,24,25,27,26,25,24);
  ThermalResult r=analyseThermal(g,60,3,28,8);
  printf("centre: found=%d max=%.1f base=%.1f ang=%.2f dir=%s act=%s hot=%d\n",r.found,r.maxC,r.baselineC,r.angleDeg,directionName(r.dir),actionFor(r),r.hotIndex);
  CHECK(r.found); CHECK(r.dir==Direction::Centre); CHECK(fabsf(r.angleDeg)<8); CHECK(r.maxC==34);
  // person at far right
  float h[64]; for(int i=0;i<64;i++) h[i]=24; for(int y=2;y<6;y++){h[y*8+6]=33;h[y*8+7]=31;}
  r=analyseThermal(h,60,3,28,8);
  printf("right : found=%d ang=%.2f dir=%s act=%s\n",r.found,r.angleDeg,directionName(r.dir),actionFor(r));
  CHECK(r.dir==Direction::Right); CHECK(r.angleDeg>15);
  // person at far left
  float l[64]; for(int i=0;i<64;i++) l[i]=24; for(int y=2;y<6;y++){l[y*8+0]=33;l[y*8+1]=31;}
  r=analyseThermal(l,60,3,28,8);
  printf("left  : found=%d ang=%.2f dir=%s act=%s\n",r.found,r.angleDeg,directionName(r.dir),actionFor(r));
  CHECK(r.dir==Direction::Left); CHECK(r.angleDeg<-15);
  // empty room with noise
  float n[64]; for(int i=0;i<64;i++) n[i]=24+((i*37)%5-2)*0.5f;
  r=analyseThermal(n,60,3,28,8);
  printf("empty : found=%d dir=%s act=%s\n",r.found,directionName(r.dir),actionFor(r));
  CHECK(!r.found); CHECK(r.dir==Direction::None);
  // hot room (all 35 C): no contrast -> no target
  float w[64]; for(int i=0;i<64;i++) w[i]=35;
  r=analyseThermal(w,60,3,28,8); CHECK(!r.found);
  printf(fails?"TESTS FAILED\n":"ALL TESTS PASSED\n"); return fails;
}
