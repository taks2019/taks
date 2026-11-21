#include "FastScene.h"
#include "RealModels.h"
#include "CaveSprite.h"
#include "RoofCutaway.h"
#include "WorldTerrain.h"
#include "CaveVolumeReview.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <cstddef>

namespace taks::FastScene {
namespace {
constexpr float PI=3.14159265358979323846f;
struct Vec3 { float x,y,z; };
struct Vertex { float x,y,z,nx,ny,nz,kind; };
struct Mat4 { float a[16]{}; };
std::string error;
GLuint program=0,staticVAO=0,staticVBO=0,caveVAO=0,caveVBO=0,roofVAO=0,roofVBO=0,dynamicVAO=0,dynamicVBO=0;
GLint uVP=-1,uRain=-1,uMud=-1,uClock=-1,uAnimation=-1,uWind=-1,uGround=-1,uCliff=-1,uRoofOpacity=-1;
GLint uApprovedTerrain=-1;
GLint uApprovedSampler[6]={-1,-1,-1,-1,-1,-1};
GLuint terrainTexture[8]={0,0,0,0,0,0,0,0};
bool approvedTerrainActive=false;
std::vector<Vertex> terrainMesh,caveMesh,roofMesh,objectsMesh,weatherMesh;
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 mul(Vec3 v,float s){return {v.x*s,v.y*s,v.z*s};}
float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3 norm(Vec3 v){float d=std::sqrt(std::max(dot(v,v),.00000001f));return mul(v,1.f/d);}
float surfaceHeight(float x,float z){
    return taks::WorldTerrain::height(x,z);
}
float top(float x){
    const float taper=std::sqrt(std::max(0.f,1.f-x*x/(9.4f*9.4f)));
    return 5.15f*taper+.24f*taper*std::sin(x*.83f);
}
void triangle(std::vector<Vertex>& v,Vec3 a,Vec3 b,Vec3 c,float kind){
    Vec3 n=norm(cross(sub(b,a),sub(c,a)));
    v.push_back({a.x,a.y,a.z,n.x,n.y,n.z,kind});
    v.push_back({b.x,b.y,b.z,n.x,n.y,n.z,kind});
    v.push_back({c.x,c.y,c.z,n.x,n.y,n.z,kind});
}
void quad(std::vector<Vertex>& v,Vec3 a,Vec3 b,Vec3 c,Vec3 d,float kind){
    triangle(v,a,b,c,kind);triangle(v,a,c,d,kind);
}
// Smooth-shaded 3D triangles: every ground vertex gets the true slope normal,
// unlike flat cards or the old per-triangle-faceted brown plane.
void smoothTerrainVertex(std::vector<Vertex>& vertices,Vec3 p) {
    float nx=0.f,ny=1.f,nz=0.f;
    taks::WorldTerrain::normal(p.x,p.z,nx,ny,nz);
    vertices.push_back({p.x,p.y,p.z,nx,ny,nz,0.f});
}
void sculptedTerrainGrid(std::vector<Vertex>& dst,float x0,float x1,
                         float z0,float z1,int cols,int rows,
                         bool skipCentralRegion) {
    struct TerrainPoint {Vec3 p;Vec3 n;};
    std::vector<TerrainPoint> grid(std::size_t(cols+1)*std::size_t(rows+1));
    auto index=[cols](int i,int j){return std::size_t(j)*std::size_t(cols+1)+std::size_t(i);};
    for(int j=0;j<=rows;++j)for(int i=0;i<=cols;++i){
        const float x=x0+(x1-x0)*float(i)/float(cols);
        const float z=z0+(z1-z0)*float(j)/float(rows);
        float nx,ny,nz;
        taks::WorldTerrain::normal(x,z,nx,ny,nz);
        grid[index(i,j)]={{x,surfaceHeight(x,z),z},{nx,ny,nz}};
    }
    auto push=[&dst](const TerrainPoint& p){
        dst.push_back({p.p.x,p.p.y,p.p.z,p.n.x,p.n.y,p.n.z,0.f});
    };
    for(int j=0;j<rows;++j)for(int i=0;i<cols;++i){
        const float xa=x0+(x1-x0)*float(i)/float(cols);
        const float xb=x0+(x1-x0)*float(i+1)/float(cols);
        const float za=z0+(z1-z0)*float(j)/float(rows);
        const float zb=z0+(z1-z0)*float(j+1)/float(rows);
        // Far and near tiles join on exact x=-28/+28,z=-28/+40
        // boundaries; there is no 160m flat quad showing through.
        if(skipCentralRegion && xa>=-28.f && xb<=28.f &&
           za>=-28.f && zb<=40.f)continue;
        const auto& a=grid[index(i,j)];
        const auto& b=grid[index(i+1,j)];
        const auto& c=grid[index(i+1,j+1)];
        const auto& d=grid[index(i,j+1)];
        // A 4m outer cell meets eight 0.5m edges. Stitch every edge
        // vertex to the exact analytic height: no gaps in any camera view.
        const float eps=.0001f;
        const bool west=skipCentralRegion && std::abs(xb-(-28.f))<eps &&
                         za>=-28.f && zb<=40.f;
        const bool east=skipCentralRegion && std::abs(xa-28.f)<eps &&
                         za>=-28.f && zb<=40.f;
        const bool south=skipCentralRegion && std::abs(zb-(-28.f))<eps &&
                          xa>=-28.f && xb<=28.f;
        const bool north=skipCentralRegion && std::abs(za-40.f)<eps &&
                          xa>=-28.f && xb<=28.f;
        if(west||east||south||north){
            const int segments=8;
            const TerrainPoint& farCorner=(west||south)?a:(east?b:d);
            auto boundaryPoint=[&](int step){
                const float t=float(step)/float(segments);
                const float x=(west?xb:(east?xa:xa+(xb-xa)*t));
                const float z=(south?zb:(north?za:za+(zb-za)*t));
                float nx,ny,nz;
                taks::WorldTerrain::normal(x,z,nx,ny,nz);
                return TerrainPoint{{x,surfaceHeight(x,z),z},{nx,ny,nz}};
            };
            for(int k=0;k<segments;++k){
                const auto p0=boundaryPoint(k),p1=boundaryPoint(k+1);
                push(farCorner);push(p0);push(p1);
            }
            // The remaining triangle keeps the outer coarse edge whole.
            if(west){push(a);push(d);push(c);}
            else if(east){push(b);push(c);push(d);}
            else if(south){push(a);push(c);push(b);}
            else {push(d);push(c);push(b);}
            continue;
        }
        push(a);push(d);push(c);
        push(a);push(c);push(b);
    }
}
void axisBar(std::vector<Vertex>& v,Vec3 a,Vec3 b,float radius,float kind,int sides=7){
    Vec3 axis=norm(sub(b,a));
    Vec3 reference=std::abs(axis.y)>.90f?Vec3{1,0,0}:Vec3{0,1,0};
    Vec3 u=norm(cross(axis,reference)),w=norm(cross(axis,u));
    for(int i=0;i<sides;++i){
        float t0=2.f*PI*float(i)/float(sides),t1=2.f*PI*float(i+1)/float(sides);
        Vec3 r0=mul(add(mul(u,std::cos(t0)),mul(w,std::sin(t0))),radius);
        Vec3 r1=mul(add(mul(u,std::cos(t1)),mul(w,std::sin(t1))),radius);
        quad(v,add(a,r0),add(a,r1),add(b,r1),add(b,r0),kind);
        triangle(v,a,add(a,r1),add(a,r0),kind);
        triangle(v,b,add(b,r0),add(b,r1),kind);
    }
}
void muddyFootprint(std::vector<Vertex>& v,float x,float z,float heading,float strength){
    if(strength<.025f)return;
    // A low, ground-conforming sole impression. It appears only after a step
    // in soft mud and fades with the same saved ground-state simulation.
    constexpr float halfWidth=.052f,halfLength=.125f;
    constexpr float shape[10][2]={
        {-.54f,-.72f},{.54f,-.72f},{.77f,-.49f},{.72f,-.05f},{.61f,.42f},
        {.34f,.78f},{-.34f,.78f},{-.61f,.42f},{-.72f,-.05f},{-.77f,-.49f}
    };
    const float c=std::cos(heading),s=std::sin(heading);
    std::array<Vec3,10> points{};
    Vec3 center{};
    for(int i=0;i<10;++i){
        const float lx=shape[i][0]*halfWidth,lz=shape[i][1]*halfLength;
        const float wx=x+lx*c+lz*s,wz=z-lx*s+lz*c;
        points[i]={wx,surfaceHeight(wx,wz)+.008f,wz};
        center=add(center,points[i]);
    }
    center=mul(center,.1f);
    const float kind=12.f+std::clamp(strength,0.f,1.f)*.49f;
    for(int i=0;i<10;++i)triangle(v,center,points[(i+1)%10],points[i],kind);
}
void waterFan(std::vector<Vertex>& v,float x,float z,float radius,float y,float phase){
    constexpr int sides=28;
    const Vec3 center{x,y,z};
    for(int i=0;i<sides;++i){
        const float a=2.f*PI*float(i)/float(sides);
        const float b=2.f*PI*float(i+1)/float(sides);
        const float ra=radius*(.91f+.06f*std::sin(a*3.f+phase));
        const float rb=radius*(.91f+.06f*std::sin(b*3.f+phase));
        Vec3 p0{x+std::cos(a)*ra,y+.012f*std::sin(a*4.f+phase),z+std::sin(a)*ra*.82f};
        Vec3 p1{x+std::cos(b)*rb,y+.012f*std::sin(b*4.f+phase),z+std::sin(b)*rb*.82f};
        triangle(v,center,p1,p0,11.f);
    }
}
void addMudFootprints(const FastFrame& f){
    for(int i=0;i<32;++i){
        const int at=i*4;
        muddyFootprint(objectsMesh,f.footprints[at],f.footprints[at+1],
                       f.footprints[at+2],f.footprints[at+3]);
    }
}
void irregularRock(std::vector<Vertex>& v,float x,float z,float radius,float scale,float kind,int seed=0){
    constexpr int sides=9;
    float h=surfaceHeight(x,z);
    Vec3 centerTop{x,h+radius*scale*.85f,z};
    for(int i=0;i<sides;++i){
        float a=2.f*PI*i/sides,b=2.f*PI*(i+1)/sides;
        float r0=radius*(.77f+.17f*std::sin(i*4.1f+seed*2.71f));
        float r1=radius*(.77f+.17f*std::sin((i+1)*4.1f+seed*2.71f));
        Vec3 base0{x+std::cos(a)*r0,h-.012f,z+std::sin(a)*r0*.8f};
        Vec3 base1{x+std::cos(b)*r1,h-.012f,z+std::sin(b)*r1*.8f};
        Vec3 shoulder0{x+std::cos(a)*r0*.7f,h+radius*scale*(.45f+.05f*std::sin(float(i)+seed)),z+std::sin(a)*r0*.65f};
        Vec3 shoulder1{x+std::cos(b)*r1*.7f,h+radius*scale*(.45f+.05f*std::sin(float(i+1)+seed)),z+std::sin(b)*r1*.65f};
        quad(v,base0,base1,shoulder1,shoulder0,kind);
        triangle(v,shoulder0,shoulder1,centerTop,kind);
    }
}
void addWater(const FastFrame& scene){
    for(int i=0;i<2;++i){
        const float* water=scene.waters+i*4;
        const float level=std::clamp(water[3],0.f,1.f);
        if(level<.015f)continue;
        const float x=water[0],z=water[1];
        const float radius=water[2]*(.40f+.60f*level);
        const float y=surfaceHeight(x,z)+.10f+.08f*level;
        waterFan(objectsMesh,x,z,radius,y,scene.animationTime*.55f+float(i));
        // Loose limestone at the changing shoreline gives the pool a bank.
        for(int k=0;k<7;++k){
            const float a=2.f*PI*float(k)/7.f+float(i)*.4f;
            const float r=radius*1.12f;
            irregularRock(objectsMesh,x+std::cos(a)*r,z+std::sin(a)*r*.82f,
                          .10f,.42f,2.f,k+i*7);
        }
    }
}
Vec3 animalPoint(float x,float z,float groundY,float heading,float lx,float ly,float lz){
    const float c=std::cos(heading),s=std::sin(heading);
    return {x+lx*c+lz*s,groundY+ly,z-lx*s+lz*c};
}
void ellipsoid(std::vector<Vertex>& v,float x,float z,float groundY,float heading,
               float lx,float ly,float lz,float rx,float ry,float rz,float material){
    constexpr int rings=8,sides=16;
    Vec3 points[rings+1][sides]{};
    for(int r=0;r<=rings;++r){
        const float latitude=-.5f*PI+PI*float(r)/float(rings);
        const float radial=std::cos(latitude),vertical=std::sin(latitude);
        for(int s=0;s<sides;++s){
            const float longitude=2.f*PI*float(s)/float(sides);
            points[r][s]=animalPoint(x,z,groundY,heading,
                lx+rx*radial*std::cos(longitude),
                ly+ry*vertical,
                lz+rz*radial*std::sin(longitude));
        }
    }
    for(int r=0;r<rings;++r)for(int s=0;s<sides;++s){
        const int next=(s+1)%sides;
        const Vec3 a=points[r][s],b=points[r+1][s];
        const Vec3 c=points[r+1][next],d=points[r][next];
        triangle(v,a,b,c,material);triangle(v,a,c,d,material);
    }
}
void addWildlife(const FastFrame& scene){
    for(int i=0;i<8;++i){
        const float* animal=scene.animals+i*6;
        if(animal[3]<.5f)continue;
        const float x=animal[0],z=animal[1],kind=animal[2];
        const bool thirsty=animal[4]>35.f,moving=thirsty||animal[5]>45.f;
        float targetX=x,targetZ=z+1.f,best=1.0e12f;
        if(thirsty){
            for(int w=0;w<2;++w){
                const float* water=scene.waters+w*4;
                if(water[3]<=.015f)continue;
                const float dx=water[0]-x,dz=water[1]-z,d=dx*dx+dz*dz;
                if(d<best){best=d;targetX=water[0];targetZ=water[1];}
            }
        }else if(kind<.5f){
            for(int g=0;g<4;++g){
                const float* grass=scene.grass+g*5;
                if(grass[2]<=.01f)continue;
                const float dx=grass[0]-x,dz=grass[1]-z,d=dx*dx+dz*dz;
                if(d<best){best=d;targetX=grass[0];targetZ=grass[1];}
            }
        }else{
            for(int j=0;j<8;++j){
                const float* prey=scene.animals+j*6;
                if(prey[3]<.5f||prey[2]>.5f)continue;
                const float dx=prey[0]-x,dz=prey[1]-z,d=dx*dx+dz*dz;
                if(d>1.0e-6f&&d<best){best=d;targetX=prey[0];targetZ=prey[1];}
            }
        }
        const float heading=std::atan2(targetX-x,targetZ-z);
        const float ground=surfaceHeight(x,z);
        const float scale=kind>.5f?.88f:1.0f;
        const float coat=kind>.5f?5.f:7.f;
        const float sway=moving?.19f:.025f;
        const float phase=scene.animationTime*4.1f+float(i)*1.7f;
        ellipsoid(objectsMesh,x,z,ground,heading,0.f,.70f,0.f,
                  .38f*scale,.29f*scale,.67f*scale,coat);
        axisBar(objectsMesh,animalPoint(x,z,ground,heading,0,.72f,.40f*scale),
                animalPoint(x,z,ground,heading,0,.88f,.63f*scale),.14f*scale,coat,7);
        ellipsoid(objectsMesh,x,z,ground,heading,0.f,.94f,.75f*scale,
                  .22f*scale,.20f*scale,.28f*scale,coat);
        ellipsoid(objectsMesh,x,z,ground,heading,0.f,.84f,1.00f*scale,
                  .12f*scale,.10f*scale,.12f*scale,2.f);
        for(int side=-1;side<=1;side+=2){
            ellipsoid(objectsMesh,x,z,ground,heading,.17f*scale*side,.99f,.86f*scale,
                      .028f,.028f,.028f,2.f);
            axisBar(objectsMesh,animalPoint(x,z,ground,heading,.13f*scale*side,1.08f,.65f*scale),
                    animalPoint(x,z,ground,heading,.18f*scale*side,1.19f,.62f*scale),
                    .035f*scale,coat,5);
        }
        axisBar(objectsMesh,animalPoint(x,z,ground,heading,0,.69f,-.61f*scale),
                animalPoint(x,z,ground,heading,0,.40f,-.91f*scale),.033f*scale,coat,5);
        for(int leg=0;leg<4;++leg){
            const float side=(leg%2==0)?-1.f:1.f;
            const bool front=leg<2;
            const float hipZ=(front?.38f:-.40f)*scale;
            const float stride=std::sin(phase+(front?0.f:PI)+(side>0?PI:0.f))*sway;
            const Vec3 hip=animalPoint(x,z,ground,heading,side*.23f*scale,.53f,hipZ);
            const Vec3 knee=animalPoint(x,z,ground,heading,side*.24f*scale,.29f,hipZ+stride*.45f);
            const Vec3 hoof=animalPoint(x,z,ground,heading,side*.24f*scale,.045f,hipZ+stride);
            axisBar(objectsMesh,hip,knee,.071f*scale,coat,6);
            axisBar(objectsMesh,knee,hoof,.046f*scale,coat,6);
        }
    }
}
float cliffZ(float x,float y){return 3.63f+.13f*std::sin(x*.74f+y*.28f)+.12f*std::sin(x*2.3f+y*2.1f);}
void buildStatic(){
    terrainMesh.clear();
    caveMesh.clear();
    roofMesh.clear();
    // FULLY GEOMETRIC TERRAIN. A fine 0.5m tessellated foreground with
    // smoothly varying slopes joins a coarser 4m 3D horizon. The original
    // fake flat 320x320 brown quad is not drawn ANYWHERE in this renderer.
    // Both grids use exactly the same deterministic CPU height function.
    sculptedTerrainGrid(terrainMesh,-120.f,120.f,-120.f,120.f,
                       60,60,true);
    sculptedTerrainGrid(terrainMesh,-28.f,28.f,-28.f,40.f,
                       112,136,false);
    // Native XYZ rock mesh: curved entrance, full inner tunnel, true side depth.
    // Review geometry only; NOT an archaeological Wonderwerk survey mesh.
    // The 8-angle photographic PNG remains optional and never used by default.
    taks::CaveVolume::build(
        [](taks::CaveVolume::P a,taks::CaveVolume::P b,
           taks::CaveVolume::P c,float material) {
            triangle(caveMesh,{a.x,a.y,a.z},{b.x,b.y,b.z},
                     {c.x,c.y,c.z},material);
        },
        [](float x,float z){return surfaceHeight(x,z);});
    // Close the full rock formation down to the ground behind the entrance.
    constexpr int depthSegments=30;
    for(int i=0;i<80;++i){
        const float x=-9.4f+18.8f*i/80.f,xx=-9.4f+18.8f*(i+1)/80.f;
        auto crest=[](float x,float t){
            const float y=top(x);
            const float z=cliffZ(x,y)+(12.2f-cliffZ(x,y))*t;
            const float ground=surfaceHeight(x,z);
            const float ripple=.24f*std::sin(x*1.7f+t*31.f)*std::sin(t*PI);
            return Vec3{x,ground+(y-ground)*std::cos(t*PI*.5f)+ripple,z};
        };
        for(int j=0;j<depthSegments;++j){
            const float a=float(j)/depthSegments,b=float(j+1)/depthSegments;
            // The rock ceiling is separate from the permanent cave walls / floor.
            // Zoom only affects the rendered ceiling, never collision or inhabitants.
            quad(roofMesh,crest(x,a),crest(x,b),crest(xx,b),crest(xx,a),1.f);
        }
    }
    // The previous 8 angular polygon-stones are removed: natural stones are now true photogrammetry meshes.
}
Mat4 multiply(const Mat4& l,const Mat4& r){
    Mat4 m;
    for(int col=0;col<4;++col)for(int row=0;row<4;++row){
        float n=0.f;
        for(int k=0;k<4;++k)n+=l.a[row+4*k]*r.a[k+4*col];
        m.a[row+4*col]=n;
    }
    return m;
}
Mat4 lookAt(Vec3 eye,Vec3 center){
    Vec3 f=norm(sub(center,eye)),s=norm(cross(f,{0,1,0})),u=cross(s,f);
    Mat4 m;float out[16]={s.x,u.x,-f.x,0,s.y,u.y,-f.y,0,s.z,u.z,-f.z,0,
                         -dot(s,eye),-dot(u,eye),dot(f,eye),1};
    std::copy(out,out+16,m.a);return m;
}
Mat4 orthographic(float width,float height){
    Mat4 p;const float znear=.1f,zfar=140.f;
    p.a[0]=2.f/width;p.a[5]=2.f/height;
    p.a[10]=-2.f/(zfar-znear);p.a[14]=-(zfar+znear)/(zfar-znear);
    p.a[15]=1;return p;
}
GLuint compile(GLenum type,const char* src){
    GLuint id=glCreateShader(type);
    glShaderSource(id,1,&src,nullptr);glCompileShader(id);
    GLint ok=0;glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
    if(!ok){
        GLint len=0;glGetShaderiv(id,GL_INFO_LOG_LENGTH,&len);
        std::string log((size_t)std::max(len,1),'\0');
        glGetShaderInfoLog(id,len,nullptr,log.data());
        error+=log;
        glDeleteShader(id);return 0;
    }
    return id;
}
void setupBuffer(GLuint& vao,GLuint& vbo,GLenum usage,const std::vector<Vertex>& data){
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);
    glGenBuffers(1,&vbo);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(data.size()*sizeof(Vertex)),
                 data.empty()?nullptr:data.data(),usage);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,x)));
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,nx)));
    glEnableVertexAttribArray(2);glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,kind)));
    glBindVertexArray(0);
}
void addResources(const FastFrame& scene){
    for(int i=0;i<12;++i){
        const float* n=scene.nodes+i*4;
        if(n[3]<.5f)continue;
        float x=n[0],z=n[1],h=surfaceHeight(x,z);
        int type=(int)(n[2]+.5f);
        if(type==0){ // faágak
            axisBar(objectsMesh,{x-.32f,h+.07f,z-.16f},{x+.32f,h+.11f,z+.16f},.045f,3.f,6);
            axisBar(objectsMesh,{x-.24f,h+.06f,z+.18f},{x+.19f,h+.12f,z-.13f},.035f,3.f,6);
        }else if(type==1){
            // The authentic CC0 UV-mapped scanned stone is drawn by RealModels at this exact finite resource node.
        }else if(type==2){
            axisBar(objectsMesh,{x-.2f,h+.05f,z},{x+.19f,h+.08f,z+.08f},.045f,3.f,5);
            axisBar(objectsMesh,{x,h+.08f,z-.2f},{x-.1f,h+.15f,z+.16f},.028f,4.f,5);
        }else{
            for(int j=0;j<3;++j) {
                float xx=x+(.12f*(j-1)),zz=z+(.075f*(j-1));
                quad(objectsMesh,{xx-.22f,h+.05f,zz-.14f},{xx+.22f,h+.06f,zz-.14f},
                     {xx+.20f,h+.05f,zz+.14f},{xx-.20f,h+.05f,zz+.14f},4.f);
            }
        }
    }
}
void addBuildings(const FastFrame& scene){
    for(int i=0;i<3;++i){
        const float* s=scene.sites+i*4;
        if(s[2]<-.5f)continue;
        float x=s[0],z=s[1],h=surfaceHeight(x,z),stage=s[2],integrity=s[3];
        if(stage<.5f){irregularRock(objectsMesh,x,z,.18f,.9f,2.f,i);continue;}
        for(int k=0;k<2;++k){
            float xx=x-1.05f+2.10f*k;
            irregularRock(objectsMesh,xx,z,.25f,1.0f,2.f,k);
            axisBar(objectsMesh,{xx,surfaceHeight(xx,z)+.11f,z},{xx,h+2.25f,z},.10f,3.f);
        }
        if(stage>1.5f){
            axisBar(objectsMesh,{x-1.10f,h+2.25f,z},{x+1.10f,h+2.25f,z},.11f,3.f);
            for(int k=0;k<2;++k){
                float xx=x-1.05f+2.10f*k;
                axisBar(objectsMesh,{xx,h+2.23f,z},
                        {xx,surfaceHeight(xx,z+2.4f)+.40f,z+2.4f},.085f,3.f);
            }
        }
        if(stage>2.5f){
            for(int k=0;k<6;++k){
                float off=.15f+float(k)*.36f;
                float y=h+2.24f-off*.72f;
                float zz=z+off;
                float wear=(1.f-integrity)*.12f;
                quad(objectsMesh,{x-1.13f,y,zz-.19f},{x+1.13f,y,zz-.19f},
                     {x+1.14f,y-.18f-wear,zz+.22f},{x-1.14f,y-.18f-wear,zz+.22f},4.f);
            }
        }
    }
}
void addWaterCollector(const FastFrame& scene){
    if(scene.collectorStage<.5f)return;
    constexpr float x=1.0f,z=-3.4f;
    const float h=surfaceHeight(x,z);
    for(int ix=0;ix<2;++ix)for(int iz=0;iz<2;++iz){
        const float px=x-.66f+float(ix)*1.32f;
        const float pz=z-.43f+float(iz)*.86f;
        axisBar(objectsMesh,{px,surfaceHeight(px,pz)+.08f,pz},
                {px,h+1.48f,pz},.075f,3.f,6);
    }
    axisBar(objectsMesh,{x-.72f,h+1.43f,z-.46f},{x+.72f,h+1.43f,z-.46f},.07f,3.f,6);
    axisBar(objectsMesh,{x-.72f,h+1.43f,z+.46f},{x+.72f,h+1.43f,z+.46f},.07f,3.f,6);
    if(scene.collectorStage>1.5f){
        // The leaf roof slopes toward a shallow stone trough; rain collection
        // and its stored-water amount are simulated by the same World state.
        quad(objectsMesh,{x-.79f,h+1.48f,z-.55f},{x+.79f,h+1.48f,z-.55f},
             {x+.79f,h+1.24f,z+.57f},{x-.79f,h+1.24f,z+.57f},4.f);
        irregularRock(objectsMesh,x+.98f,z+.34f,.31f,.24f,2.f,11);
        if(scene.collectorWater>.005f)
            waterFan(objectsMesh,x+.98f,z+.34f,.12f,
                     surfaceHeight(x+.98f,z+.34f)+.075f,scene.animationTime*.8f);
    }
}
void addWeather(const FastFrame& scene,Vec3 right){
    weatherMesh.clear();
    const float gust=scene.windStrength;
    const int drops=int(scene.rain*260.f);
    const int dust=int(100.f*gust*(1.f-scene.rain)*
                       std::clamp((.30f-scene.moisture)*4.f,0.f,1.f));
    const float time=scene.animationTime;
    auto fraction=[](float value){return value-std::floor(value);};
    for(int i=0;i<drops+dust;++i){
        const bool rain=i<drops;
        const float seed=float(i+1);
        const float age=fraction(seed*.618034f+time*(rain?1.24f:.23f));
        const float travel=age*(rain?1.6f:5.2f)*gust;
        float x=scene.playerX+24.f*(fraction(seed*.7548777f)-.5f)+scene.windX*travel;
        float z=scene.playerZ+4.f+22.f*(fraction(seed*.5698403f)-.5f)+scene.windZ*travel;
        const float y=surfaceHeight(x,z)+(rain?.07f+5.2f*(1.f-age):.08f+.32f*age);
        if(z>3.7f&&z<12.2f&&std::abs(x)<9.4f&&y<top(x))continue;
        const Vec3 p{x,y,z};
        const Vec3 extent=mul(right,rain?.013f:.13f);
        const Vec3 tip=add(p,rain?Vec3{-scene.windX*gust*.18f,.30f,-scene.windZ*gust*.18f}
                                 :Vec3{0,.06f,0});
        quad(weatherMesh,sub(p,extent),add(p,extent),add(tip,extent),sub(tip,extent),
             rain?13.f:14.f);
    }
}

void addHearth(const FastFrame& scene,Vec3 right){
    // No white mannequins or rod people. The animated, source-skinned humanoids
    // are drawn by RealModels; a missing asset reports an import error.
    if(scene.hearth>.5f){
        float x=-1.15f,z=-1.8f,h=surfaceHeight(x,z);
        for(int k=0;k<7;++k){
            float a=2*PI*k/7;
            irregularRock(objectsMesh,x+.39f*std::cos(a),z+.37f*std::sin(a),.18f,.6f,2.f,k);
        }
        axisBar(objectsMesh,{x-.33f,h+.12f,z-.18f},{x+.31f,h+.16f,z+.16f},.056f,3.f,6);
        if(scene.fire>.5f){
            const float gust=scene.windStrength;
            const Vec3 base{x,h+.16f,z};
            const float flicker=.42f+.12f*std::sin(scene.animationTime*13.f);
            const Vec3 tip=add(base,{scene.windX*.46f*gust,flicker,scene.windZ*.46f*gust});
            const Vec3 width=mul(right,.115f);
            triangle(objectsMesh,sub(base,width),add(base,width),tip,10.f);
            for(int i=9;i>=0;--i){
                const float age=std::fmod(scene.animationTime*.48f+float(i)*.1f,1.f);
                const float drift=1.7f*age*age*gust;
                const Vec3 center{x+scene.windX*drift,h+.56f+1.85f*age,z+scene.windZ*drift};
                const float radius=.061f+.16f*age;
                const Vec3 side=mul(right,radius);
                const Vec3 up{0,radius*.54f,0};
                constexpr int segments=8;
                for(int j=0;j<segments;++j){
                    const float a=2.f*PI*j/segments,b=2.f*PI*(j+1)/segments;
                    const Vec3 edgeA=add(center,add(mul(side,std::cos(a)),mul(up,std::sin(a))));
                    const Vec3 edgeB=add(center,add(mul(side,std::cos(b)),mul(up,std::sin(b))));
                    triangle(weatherMesh,center,edgeA,edgeB,15.f);
                    weatherMesh[weatherMesh.size()-3].kind+=.44f*(1.f-age);
                }
            }
        }
    }
}
const char* VERT=R"GLSL(#version 300 es
precision highp float;
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in float materialId;
uniform mat4 u_viewProjection;
out vec3 wPos;
out vec3 wNorm;
out float materialValue;
flat out int matId;
void main(){
    wPos=position;
    wNorm=normal;
    materialValue=materialId;
    matId=int(materialId+.5);
    gl_Position=u_viewProjection*vec4(position,1.);
})GLSL";
const char* FRAG=R"GLSL(#version 300 es
precision highp float;
in vec3 wPos;in vec3 wNorm;in float materialValue;flat in int matId;
uniform float u_rain;uniform float u_mud;uniform float u_clock;uniform float u_animationTime;
uniform vec2 u_wind;
uniform sampler2D u_groundTexture;
uniform sampler2D u_cliffTexture;
// Six photographically detailed surfaces come from the *approved user's*
// SuperImage 4x2 PNG, not a generator or unapproved procedural brown slab.
uniform sampler2D u_approvedDry;
uniform sampler2D u_approvedRed;
uniform sampler2D u_approvedRock;
uniform sampler2D u_approvedSand;
uniform sampler2D u_approvedMud;
uniform sampler2D u_approvedTransition;
uniform float u_hasApprovedTerrain;
uniform float u_roofOpacity;
out vec4 fragColor;
float hash(vec3 p){p=fract(p*.1031);p+=dot(p,p.yzx+33.33);return fract((p.x+p.y)*p.z);}
// The 512px gravel picture previously tiled every 0.7 metres, resulting in
// a giant repeated brown wallpaper. Blend four DIFFERENT continuous
// sample offsets over large world cells, and retain fine soil detail.
vec2 gravelOffset(vec2 i){
    return vec2(hash(vec3(i,1.17)),hash(vec3(i,7.31)))*11.23;
}
vec3 gravelNoTile(vec2 world){
    vec2 cell=world*.145;
    vec2 tile=floor(cell);
    vec2 f=fract(cell);
    vec2 w=f*f*(3.0-2.0*f);
    vec2 uv=world*.39;
    vec3 a=texture(u_groundTexture,uv+gravelOffset(tile)).rgb;
    vec3 b=texture(u_groundTexture,uv+gravelOffset(tile+vec2(1.,0.))).rgb;
    vec3 c=texture(u_groundTexture,uv+gravelOffset(tile+vec2(0.,1.))).rgb;
    vec3 d=texture(u_groundTexture,uv+gravelOffset(tile+vec2(1.,1.))).rgb;
    return mix(mix(a,b,w.x),mix(c,d,w.x),w.y);
}
// Existing photographic dolomite is sampled in three physical dimensions;
// the same stone material transitions from the real cave wall into its
// 3D scree apron, without adding a new invented ground colour slab.
vec3 photoDolomite(vec3 p,vec3 n){
    vec3 weights=pow(abs(normalize(n)),vec3(4.0));
    weights/=max(dot(weights,vec3(1.0)),.0001);
    const float scale=.44;
    vec3 sideX=texture(u_cliffTexture,p.zy*scale+vec2(.13,.27)).rgb;
    vec3 topY=texture(u_cliffTexture,p.xz*scale+vec2(.19,.43)).rgb;
    vec3 sideZ=texture(u_cliffTexture,p.xy*scale+vec2(.34,.31)).rgb;
    return sideX*weights.x+topY*weights.y+sideZ*weights.z;
}
// Real spatially aligned rock: same approved sample covers vertical and
// horizontal cliff faces, with smooth normals (not front-facing billboards).
vec3 approvedTriplanarRock(vec3 p,vec3 n){
    vec3 w=pow(abs(normalize(n)),vec3(4.));
    w/=max(dot(w,vec3(1.)),.0001);
    const float scale=.22;
    return texture(u_approvedRock,p.zy*scale+vec2(.21,.67)).rgb*w.x+
           texture(u_approvedRock,p.xz*scale+vec2(.34,.17)).rgb*w.y+
           texture(u_approvedRock,p.xy*scale+vec2(.79,.38)).rgb*w.z;
}
// Ground material masks follow WORLD coordinates, not isometric tile images.
// A camera orbit sees the same real XYZ surface from any of its 8 directions.
float soilNoise(vec2 p){
    vec2 i=floor(p),f=fract(p);
    vec2 t=f*f*(3.0-2.0*f);
    float a=hash(vec3(i,2.93));
    float b=hash(vec3(i+vec2(1.,0.),2.93));
    float c=hash(vec3(i+vec2(0.,1.),2.93));
    float d=hash(vec3(i+vec2(1.,1.),2.93));
    return mix(mix(a,b,t.x),mix(c,d,t.x),t.y);
}
float soilField(vec2 p){
    return .53*soilNoise(p*.115)+.30*soilNoise(p*.34+vec2(17.,-9.))
          +.17*soilNoise(p*.91+vec2(-22.,8.));
}
float geologicalScree(vec3 p,vec3 n){
    // Same footprint as WorldTerrain::height: stone shoulder mounds
    // around both sides of the 3 metre cave opening.
    vec2 l=(p.xz-vec2(-6.8,6.2))/vec2(5.0,8.0);
    vec2 r=(p.xz-vec2(6.5,6.8))/vec2(5.5,8.5);
    vec2 c=(p.xz-vec2(0.0,9.1))/vec2(9.0,12.0);
    float exposed=exp(-.5*dot(l,l))+exp(-.5*dot(r,r))
                  +.43*exp(-.5*dot(c,c));
    float steepness=1.0-clamp(normalize(n).y,0.0,1.0);
    vec2 opening=(p.xz-vec2(0.0,3.5))/vec2(2.6,6.8);
    float door=exp(-.5*dot(opening,opening));
    return clamp((exposed*.72+steepness*1.35)*(1.0-.82*door),0.,1.);
}
void main(){
    float grain=hash(floor(wPos*12.));
    float lines=.5+.5*sin(wPos.y*8.5+sin(wPos.x*.69+wPos.z*.21)*1.8);
    vec3 base;
    if(matId==13){fragColor=vec4(.63,.73,.78,.36);return;}
    if(matId==14){fragColor=vec4(.63,.49,.28,.15);return;}
    if(matId==15){fragColor=vec4(.61,.59,.54,(materialValue-15.)*.39);return;}
    if(matId==0){
        // Real photographed gravel: world-space stochastic anti-tiling,
        // no invented textures and no repeating diagonal wallpaper.
        vec3 micro=texture(u_groundTexture,
                          mat2(.809,.588,-.588,.809)*wPos.xz*1.05).rgb;
        vec3 macro=gravelNoTile(wPos.xz);
        float blendWeight=.18+.13*grain;
        base=mix(macro,micro,blendWeight)*(.94+.06*grain);
        // Dry sand and walked compact soil blend in large, irregular
        // physically world-aligned patches, without square tile cut lines.
        float broad=soilField(wPos.xz);
        float fine=soilNoise(wPos.xz*2.30+vec2(3.7,8.1));
        float soilMix=smoothstep(.28,.73,broad+.12*(fine-.5));
        vec3 paleDry=base*vec3(1.14,1.10,1.01);
        vec3 compactEarth=base*vec3(.88,.83,.76);
        base=mix(paleDry,compactEarth,soilMix);
        base*=.96+.08*fine;

        // Water from real weather accumulates in low basins and the actual
        // winding drainage line, not in cut-out rectangular mud squares.
        float channelX=-9.2+.087*(wPos.z+10.)+.7*sin(wPos.z*.12+2.);
        float wetChannel=exp(-.5*pow((wPos.x-channelX)/1.45,2.))*
                         exp(-.5*pow((wPos.z+9.5)/24.,2.));
        float basin=1.0-smoothstep(.14,.43,broad);
        float waterPotential=clamp(.55*u_rain+.66*u_mud,0.,1.);
        float mudCoverage=smoothstep(.25,.79,waterPotential)*
                          (.22+.55*basin+.38*wetChannel);
        mudCoverage=clamp(mudCoverage,0.,1.);
        base=mix(base,base*vec3(.55,.52,.47),mudCoverage);
        base+=vec3(.026,.025,.021)*mudCoverage*pow(fine,6.);

        // Exposed rock responds continuously to geological position,
        // 3D slope and mineral erosion. Same dolomite texture as the cave.
        float exposed=geologicalScree(wPos,wNorm);
        float slope=clamp(1.-normalize(wNorm).y,0.,1.);
        float weathering=soilField(wPos.xz+vec2(26.2,17.4));
        float stone=smoothstep(.17,.81,exposed+.42*slope+
                               .22*(weathering-.5));
        float grit=hash(floor(wPos*19.7));
        vec3 limestone=photoDolomite(wPos,wNorm);
        vec3 weathered=limestone*(.87+.12*grit);
        base=mix(base,weathered,stone*.92);
        if(u_hasApprovedTerrain>.5){
            // No tile atlas borders, captions, or labels are GPU resources.
            // Physical ground and water stay in their existing 3D locations.
            // Different UV scales and smooth noise fields prevent the eight
            // approved photographic ground kinds from appearing as squares.
            vec2 p=wPos.xz;
            vec3 dry=texture(u_approvedDry,p*.193+vec2(.43,.15)).rgb;
            vec3 red=texture(u_approvedRed,p*.174+vec2(.09,.67)).rgb;
            vec3 sand=texture(u_approvedSand,p*.149+vec2(.29,.46)).rgb;
            vec3 wet=texture(u_approvedMud,p*.178+vec2(.74,.22)).rgb;
            vec3 transition=texture(u_approvedTransition,p*.186+vec2(.37,.91)).rgb;
            vec3 rock=approvedTriplanarRock(wPos,wNorm);
            float dryness=soilField(p+vec2(13.0,-19.0));
            float redEarth=smoothstep(.44,.78,soilField(p+vec2(32.,8.)));
            float sandPatch=1.-smoothstep(.31,.67,dryness);
            vec3 realSurface=mix(dry,sand,sandPatch*.84);
            realSurface=mix(realSurface,red,redEarth*.59);
            realSurface=mix(realSurface,transition,.12+.21*soilField(p+vec2(-5.,21.)));
            realSurface=mix(realSurface,rock,stone*.92);
            realSurface=mix(realSurface,wet,mudCoverage*.84);
            base=realSurface; // Replace rejected brown base completely, not 96%.
        }
        // The same physical terrain becomes darker UNDER the cave overhang;
        // no painted 2D black rectangle on top of world geometry.
        float caveX=1.0-smoothstep(1.52,2.10,abs(wPos.x));
        float caveZ=smoothstep(3.45,6.10,wPos.z)*(1.0-smoothstep(11.5,12.5,wPos.z));
        base*=1.0-.55*caveX*caveZ;
    }else if(matId==1){
        base=photoDolomite(wPos,wNorm)*(.90+.10*lines);
        if(u_hasApprovedTerrain>.5)
            base=approvedTriplanarRock(wPos,wNorm);
    }else if(matId==16){
        // Actual concave 3D tunnel stone, triplanar texture with natural darkness
        // increasing with distance from the daylight opening.
        float depth=clamp((wPos.z-3.65)/8.1,0.0,1.0);
        base=photoDolomite(wPos,wNorm)*mix(.84,.19,depth);
        if(u_hasApprovedTerrain>.5)
            base=approvedTriplanarRock(wPos,wNorm)*mix(.80,.16,depth);
        base*=.93+.07*grain;
    }else if(matId==17){
        base=photoDolomite(wPos,wNorm)*.13;
    }else if(matId==2){
        base=mix(vec3(.32,.30,.27),vec3(.59,.53,.46),grain*.7);
    }else if(matId==3){
        base=mix(vec3(.27,.17,.10),vec3(.49,.31,.17),.5+.5*sin(wPos.y*8.+wPos.x*4.));
    }else if(matId==4){
        base=mix(vec3(.21,.28,.13),vec3(.43,.39,.19),grain);
    }else if(matId==5){
        base=vec3(.40,.29,.22);
    }else if(matId==6){
        float shade=mix(.54,.15,smoothstep(3.7,6.7,wPos.z));
        base=vec3(.21,.16,.115)*(shade+grain*.035);
    }else if(matId==7){base=vec3(.70,.61,.39);}
    else if(matId==8){base=vec3(.46,.62,.52);}
    else if(matId==9){base=vec3(.57,.60,.73);}
    else if(matId==11){
        float windSpeed=length(u_wind);
        vec2 direction=windSpeed>.001?u_wind/windSpeed:vec2(1.,0.);
        float crosswind=dot(wPos.xz,vec2(-direction.y,direction.x));
        float ripple=.5+.5*sin(dot(wPos.xz,direction)*12.-u_animationTime*3.+sin(crosswind*2.1));
        ripple*=.24+.76*max(windSpeed,u_rain);
        float glint=pow(max(0.,sin(u_animationTime*1.2+wPos.x*1.7-wPos.z*2.1)),10.);
        base=mix(vec3(.035,.115,.12),vec3(.12,.31,.29),ripple*.72);
        base+=vec3(.18,.21,.17)*glint;
    }
    else if(matId==12){
        float imprint=clamp((materialValue-12.0)/.49,0.0,1.0);
        base=mix(vec3(.34,.24,.15),vec3(.15,.10,.065),imprint);
        base*=1.0-.12*clamp(u_mud,0.0,1.0);
    }
    else {base=vec3(1.4,.50,.15);}
    vec3 normal=normalize(wNorm);
    float daylight=clamp(1.-abs(mod(u_clock,24.)-13.)/12.,.2,1.);
    vec3 sun=normalize(vec3(-.55,1.1,-.70));
    float diffuse=max(dot(normal,sun),0.);
    float light=.35+.63*diffuse*daylight;
    float wetness=clamp(u_rain*.37,0.,.33);
    base*=light*(1.-wetness);
    // Finom önárnyékolt rétegek, tónustérkép helyett valódi geometriai fényválasz.
    base*=.94+.10*grain;
    vec3 mapped=base/(base+vec3(.43));
    mapped=pow(max(mapped,vec3(0.)),vec3(.91));
    fragColor=vec4(mapped,u_roofOpacity);
})GLSL";
}
const std::string& lastError(){return error;}
void shutdown(){
    glDeleteTextures(8,terrainTexture);
    std::fill(std::begin(terrainTexture),std::end(terrainTexture),0);
    approvedTerrainActive=false;
    if(staticVBO)glDeleteBuffers(1,&staticVBO);
    if(caveVBO)glDeleteBuffers(1,&caveVBO);
    if(roofVBO)glDeleteBuffers(1,&roofVBO);
    if(dynamicVBO)glDeleteBuffers(1,&dynamicVBO);
    if(staticVAO)glDeleteVertexArrays(1,&staticVAO);
    if(caveVAO)glDeleteVertexArrays(1,&caveVAO);
    if(roofVAO)glDeleteVertexArrays(1,&roofVAO);
    if(dynamicVAO)glDeleteVertexArrays(1,&dynamicVAO);
    if(program)glDeleteProgram(program);
    staticVBO=dynamicVBO=staticVAO=dynamicVAO=caveVAO=caveVBO=roofVAO=roofVBO=program=0;
    taks::CaveSprite::shutdown();
    taks::RealModels::shutdown();
}
bool init(){
    // OpenGL erőforrásait csak az aktív grafikus kontextus szálán kezeljük.
    shutdown();error.clear();
    GLuint vertex=compile(GL_VERTEX_SHADER,VERT),fragment=compile(GL_FRAGMENT_SHADER,FRAG);
    if(!vertex||!fragment){
        if(vertex)glDeleteShader(vertex);
        if(fragment)glDeleteShader(fragment);
        return false;
    }
    program=glCreateProgram();
    glAttachShader(program,vertex);glAttachShader(program,fragment);
    glLinkProgram(program);glDeleteShader(vertex);glDeleteShader(fragment);
    GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if(!linked){error="A gyors 3D motor shaderprogramja nem kapcsolódott.";shutdown();return false;}
    uVP=glGetUniformLocation(program,"u_viewProjection");
    uRain=glGetUniformLocation(program,"u_rain");
    uWind=glGetUniformLocation(program,"u_wind");
    uMud=glGetUniformLocation(program,"u_mud");
    uClock=glGetUniformLocation(program,"u_clock");
    uAnimation=glGetUniformLocation(program,"u_animationTime");
    uGround=glGetUniformLocation(program,"u_groundTexture");
    uCliff=glGetUniformLocation(program,"u_cliffTexture");
    uApprovedTerrain=glGetUniformLocation(program,"u_hasApprovedTerrain");
    const char* approvedNames[6]={
        "u_approvedDry","u_approvedRed","u_approvedRock",
        "u_approvedSand","u_approvedMud","u_approvedTransition"
    };
    for(int i=0;i<6;i++)uApprovedSampler[i]=glGetUniformLocation(program,approvedNames[i]);
    uRoofOpacity=glGetUniformLocation(program,"u_roofOpacity");
    buildStatic();
    setupBuffer(staticVAO,staticVBO,GL_STATIC_DRAW,terrainMesh);
    setupBuffer(caveVAO,caveVBO,GL_STATIC_DRAW,caveMesh);
    setupBuffer(roofVAO,roofVBO,GL_STATIC_DRAW,roofMesh);
    setupBuffer(dynamicVAO,dynamicVBO,GL_STREAM_DRAW,objectsMesh);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    if(!taks::RealModels::init()){error=taks::RealModels::errorMessage();return false;}
    if(!taks::CaveSprite::init()){error=taks::CaveSprite::lastError();return false;}
    return glGetError()==GL_NO_ERROR;
}
bool uploadTerrainTexture(int slot,const std::uint32_t* argb,int width,int height){
    if(slot<0||slot>7||!argb||width<128||height<128||width>2048||height>2048){
        error="A természetes talaj- vagy sziklafaltextúra hibás.";return false;
    }
    if(terrainTexture[slot])glDeleteTextures(1,&terrainTexture[slot]);
    std::vector<std::uint8_t> rgba(std::size_t(width)*height*4);
    for(std::size_t i=0;i<std::size_t(width)*height;++i){
        const std::uint32_t color=argb[i];
        rgba[i*4]=std::uint8_t(color>>16);
        rgba[i*4+1]=std::uint8_t(color>>8);
        rgba[i*4+2]=std::uint8_t(color);
        rgba[i*4+3]=std::uint8_t(color>>24);
    }
    glGenTextures(1,&terrainTexture[slot]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,terrainTexture[slot]);
    glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0);
    if(glGetError()!=GL_NO_ERROR){
        error="A telefon GPU-ja nem tudta feltölteni a valódi PBR alapanyagtextúrát.";return false;
    }
    return true;
}
void setApprovedTerrainActive(bool enable){
    approvedTerrainActive=enable;
    if(enable){
        for(int slot=2;slot<=7;++slot){
            if(!terrainTexture[slot]){
                approvedTerrainActive=false;
                error="A jóváhagyott SuperImage készlet hiányos.";
                return;
            }
        }
    }
}
void render(int width,int height,const FastFrame& f){
    if(!program)return;
    width=std::max(width,1);height=std::max(height,1);
    float aspect=float(width)/float(height);
    float yaw=f.yaw,pitch=std::clamp(f.pitch,.349066f,1.396263f);
    // View-only cave inspection; world physics, actor location and saves
    // remain untouched. The preview can look INSIDE the cave by hiding
    // exterior PNG and rock roof at the effective orthographic zoom.
    const float effectiveZoom=f.inspectCave?7.5f:f.zoom;
    float distance=std::max(18.f,effectiveZoom*2.f);
    Vec3 focus=f.inspectCave
        ?Vec3{0.f,surfaceHeight(0.f,4.8f)+.95f,4.8f}
        :Vec3{f.playerX,surfaceHeight(f.playerX,f.playerZ)+.90f,
              f.playerZ+1.65f+std::clamp((f.zoom-10.f)*.29f,0.f,3.f)};
    Vec3 delta{std::sin(yaw)*std::cos(pitch)*distance,
               std::sin(pitch)*distance,
               -std::cos(yaw)*std::cos(pitch)*distance};
    Vec3 eye=add(focus,delta);
    Mat4 vp=multiply(orthographic(effectiveZoom*.99f*aspect,effectiveZoom*.99f),lookAt(eye,focus));

    objectsMesh.clear();
    addWater(f);addWildlife(f);addMudFootprints(f);addResources(f);addBuildings(f);
    const Vec3 right{std::cos(yaw),0,std::sin(yaw)};
    addWeather(f,right);
    addWaterCollector(f);addHearth(f,right);
    glViewport(0,0,width,height);
    // Stone Age menu/game atmosphere derives from the actual clock and weather.
    // Never use a fixed blue/neon backdrop: nighttime charcoal, daylight dust,
    // and rain-darkened limestone are natural environmental consequences.
    const float timeOfDay=std::fmod(std::fmod(f.clock,24.f)+24.f,24.f);
    const float daylight=std::clamp(1.f-std::abs(timeOfDay-13.f)/9.f,0.f,1.f);
    const float rainfall=std::clamp(f.rain,0.f,1.f);
    const float duskR=.085f, duskG=.074f, duskB=.058f;
    const float dayR=.56f, dayG=.46f, dayB=.34f;
    const float skyR=(duskR+(dayR-duskR)*daylight)*(1.f-.35f*rainfall);
    const float skyG=(duskG+(dayG-duskG)*daylight)*(1.f-.29f*rainfall);
    const float skyB=(duskB+(dayB-duskB)*daylight)*(1.f-.24f*rainfall);
    glClearColor(skyR,skyG,skyB,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    if(uRoofOpacity>=0)glUniform1f(uRoofOpacity,1.f);
    glUniformMatrix4fv(uVP,1,GL_FALSE,vp.a);
    if(uRain>=0)glUniform1f(uRain,f.rain);
    glUniform2f(uWind,f.windX*f.windStrength,f.windZ*f.windStrength);
    if(uMud>=0)glUniform1f(uMud,f.mud);
    if(uClock>=0)glUniform1f(uClock,f.clock);
    if(uAnimation>=0)glUniform1f(uAnimation,f.animationTime);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,terrainTexture[0]);
    if(uGround>=0)glUniform1i(uGround,0);
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,terrainTexture[1]);
    if(uCliff>=0)glUniform1i(uCliff,1);
    for(int i=0;i<6;i++){
        glActiveTexture(GL_TEXTURE0+2+i);
        glBindTexture(GL_TEXTURE_2D,terrainTexture[i+2]);
        if(uApprovedSampler[i]>=0)glUniform1i(uApprovedSampler[i],i+2);
    }
    if(uApprovedTerrain>=0)
        glUniform1f(uApprovedTerrain,approvedTerrainActive?1.f:0.f);
    glBindVertexArray(staticVAO);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)terrainMesh.size());

    // The original placeholder cave is not a second cave: it is hidden while
    // the eight-view PNG preview covers the world, then progressively returns
    // when zooming inside. Ground, resources, NPCs and saves are untouched.
    const float pngCover = f.pngCavePreview && !f.inspectCave
            ? taks::RoofCutaway::visibility(effectiveZoom) : 0.f;
    const float oldCaveAlpha = 1.f - pngCover;
    if(oldCaveAlpha > .001f && !caveMesh.empty()){
        if(uRoofOpacity>=0)glUniform1f(uRoofOpacity,oldCaveAlpha);
        if(oldCaveAlpha < .999f){
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }
        glBindVertexArray(caveVAO);
        glDrawArrays(GL_TRIANGLES,0,(GLsizei)caveMesh.size());
        if(oldCaveAlpha < .999f){
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
        if(uRoofOpacity>=0)glUniform1f(uRoofOpacity,1.f);
    }

    glBindVertexArray(dynamicVAO);
    glBindBuffer(GL_ARRAY_BUFFER,dynamicVBO);
    glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(objectsMesh.size()*sizeof(Vertex)),
                 objectsMesh.empty()?nullptr:objectsMesh.data(),GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)objectsMesh.size());
    glBindVertexArray(0);
    taks::RealModels::render(vp.a,f.nodes,f.grass,f.trees,f.verifiedFerns,
        f.clock,f.rain,f.playerX,f.playerZ,f.playerHeading,
        f.playerMoving,f.playerRunning,f.npcs,f.animationTime,f.windStrength,
        f.windX,f.windZ,f.wetSeason,f.moisture);

    // Native depth-tested sprite: belongs to actual world coordinates.
    // Foreground actors/rocks occlude the sprite, unlike screen-space Canvas.
    const float caveCover = f.pngCavePreview && !f.inspectCave
            ? taks::RoofCutaway::visibility(effectiveZoom) : 0.f;
    if(caveCover>.001f)
        taks::CaveSprite::render(vp.a,f.yaw,f.pitch,effectiveZoom,
            surfaceHeight(0.f,2.2f),f.clock,f.rain,caveCover);

    // Draw the rock roof on top of the permanent scene. At close zoom, expose
    // the SAME cave interior and actors; neither simulation state nor the
    // original outside geometry is replaced or reset.
    const float roofOpacity=f.inspectCave?0.f:
          (f.pngCavePreview?0.f:taks::RoofCutaway::visibility(effectiveZoom));
    if(roofOpacity>.001f && !roofMesh.empty()){
        glUseProgram(program);
        if(uRoofOpacity>=0)glUniform1f(uRoofOpacity,roofOpacity);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,terrainTexture[0]);
        glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,terrainTexture[1]);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glBindVertexArray(roofVAO);
        glDrawArrays(GL_TRIANGLES,0,(GLsizei)roofMesh.size());
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        if(uRoofOpacity>=0)glUniform1f(uRoofOpacity,1.f);
    }
    if(!weatherMesh.empty()){
        glUseProgram(program);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glBindVertexArray(dynamicVAO);
        glBindBuffer(GL_ARRAY_BUFFER,dynamicVBO);
        glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(weatherMesh.size()*sizeof(Vertex)),
                     weatherMesh.data(),GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES,0,GLsizei(weatherMesh.size()));
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}
}
