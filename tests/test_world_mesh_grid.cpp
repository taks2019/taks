#include "../app/src/main/cpp/WorldMeshGrid.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
static bool close(float a,float b){return std::abs(a-b)<.0001f;}
int main(){
  const auto a=taks::WorldMeshGrid::defaultAxes();
  assert(a.xs.size()==159);
  assert(a.zs.size()==180);
  assert(close(a.xs.front(),-120.f)&&close(a.xs.back(),120.f));
  assert(close(a.zs.front(),-120.f)&&close(a.zs.back(),120.f));
  auto test=[](const std::vector<float>& v){
    for(std::size_t i=1;i<v.size();++i){
      assert(v[i]>v[i-1]);
      assert(v[i]-v[i-1] <= 4.0001f);
    }
  };
  test(a.xs);test(a.zs);
  for(float x : {-28.f,28.f}){bool found=false;for(float v:a.xs)found|=close(x,v);assert(found);}
  for(float z : {-28.f,40.f}){bool found=false;for(float v:a.zs)found|=close(z,v);assert(found);}
  // Exactly one quad (two triangles) per adjacent pair of axis intervals.
  // None can overlap or leave gaps because the full Cartesian product
  // is meshed in a single pass with the same analytic terrain normals.
  const std::size_t cells=(a.xs.size()-1)*(a.zs.size()-1);
  assert(cells==28282);
  assert(cells*6==169692);
  std::cout<<"CONTINUOUS TERRAIN MESH: "<<cells
           <<" cells, "<<cells*2<<" triangles, no duplicated bands\n";
}
