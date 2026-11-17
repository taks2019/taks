#include "FastScene.h"
#include "RealModels.h"
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
GLuint program=0,staticVAO=0,staticVBO=0,dynamicVAO=0,dynamicVBO=0;
GLint uVP=-1,uRain=-1,uClock=-1,uEye=-1,uGround=-1,uCliff=-1;
GLuint terrainTexture[2]={0,0};
std::vector<Vertex> terrainMesh,objectsMesh;
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 mul(Vec3 v,float s){return {v.x*s,v.y*s,v.z*s};}
float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3 norm(Vec3 v){float d=std::sqrt(std::max(dot(v,v),.00000001f));return mul(v,1.f/d);}
float surfaceHeight(float x,float z){
    // A részletgazdag belső régió lágyan csatlakozik a távoli talajhoz;
    // a korábbi négyzet alakú térképlap éle így nem látszik a kamerából.
    auto saturate=[](float v){return std::max(0.f,std::min(1.f,v));};
    float e=saturate((11.f-std::abs(x))/2.4f) *
            saturate((z+11.f)/2.4f)*saturate((17.f-z)/2.4f);
    float relief=-.27f+.17f*std::sin(x*.21f)*std::cos(z*.22f)+
                 .09f*std::sin(x*.87f+z*.57f)+.065f*std::cos(z*.73f-x*.42f);
    return -.48f+(relief+.48f)*e;
}
float top(float x){return 4.45f+.62f*std::sin(x*.29f+.48f)+.37f*std::sin(x*.81f);}
void triangle(std::vector<Vertex>& v,Vec3 a,Vec3 b,Vec3 c,float kind){
    Vec3 n=norm(cross(sub(b,a),sub(c,a)));
    v.push_back({a.x,a.y,a.z,n.x,n.y,n.z,kind});
    v.push_back({b.x,b.y,b.z,n.x,n.y,n.z,kind});
    v.push_back({c.x,c.y,c.z,n.x,n.y,n.z,kind});
}
void quad(std::vector<Vertex>& v,Vec3 a,Vec3 b,Vec3 c,Vec3 d,float kind){
    triangle(v,a,b,c,kind);triangle(v,a,c,d,kind);
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
float cliffZ(float x,float y){return 3.63f+.13f*std::sin(x*.74f+y*.28f)+.12f*std::sin(x*2.3f+y*2.1f);}
bool cliffOpen(float x,float y) {
    if(y<.08f)return false;
    float xn=x/1.65f,yn=(y-1.34f)/1.55f;
    return xn*xn+yn*yn<.96f && y<2.85f;
}
void buildStatic(){
    terrainMesh.clear();
    // Nagyméretű háttértalaj, valódi mélységi teszttel; nincs látható "lebegő táblaszél".
    quad(terrainMesh,{-160.f,-.485f,-160.f},{-160.f,-.485f,160.f},
         {160.f,-.485f,160.f},{160.f,-.485f,-160.f},0.f);
    constexpr int nx=72,nz=92;
    const float x0=-11.f,x1=11.f,z0=-11.f,z1=17.f;
    for(int j=0;j<nz;++j)for(int i=0;i<nx;++i){
        float x=x0+(x1-x0)*float(i)/float(nx),xx=x0+(x1-x0)*float(i+1)/float(nx);
        float z=z0+(z1-z0)*float(j)/float(nz),zz=z0+(z1-z0)*float(j+1)/float(nz);
        Vec3 a{x,surfaceHeight(x,z),z},b{xx,surfaceHeight(xx,z),z},c{xx,surfaceHeight(xx,zz),zz},d{x,surfaceHeight(x,zz),zz};
        quad(terrainMesh,a,d,c,b,0.f);
    }
    // Természetes sziklafal, valódi 3D elülső és oldalsó mélységgel.
    const int mx=95,my=34;
    for(int j=0;j<my;++j)for(int i=0;i<mx;++i){
        float x=-9.4f+18.8f*float(i)/mx,xx=-9.4f+18.8f*float(i+1)/mx;
        float y=5.5f*float(j)/my,yy=5.5f*float(j+1)/my;
        float midX=(x+xx)*.5f,midY=(y+yy)*.5f;
        if(cliffOpen(midX,midY))continue;
        if(midY>top(midX))continue;
        Vec3 a{x,y,cliffZ(x,y)},b{xx,y,cliffZ(xx,y)},c{xx,yy,cliffZ(xx,yy)},d{x,yy,cliffZ(x,yy)};
        quad(terrainMesh,a,b,c,d,1.f);
    }
    // A fal kap egy felső felszínt és valódi oldalakat.
    for(int i=0;i<80;++i){
        float x=-9.4f+18.8f*i/80.f,xx=-9.4f+18.8f*(i+1)/80.f;
        float y=top(x),yy=top(xx);
        quad(terrainMesh,{x,y,cliffZ(x,y)},{xx,yy,cliffZ(xx,yy)},{xx,yy-.25f,12.2f},{x,y-.25f,12.2f},1.f);
    }
    // A barlang sötét belseje az elülső arch mögött található.
    for(int i=0;i<13;++i){
        float angle=PI*float(i)/12.f,next=PI*float(i+1)/12.f;
        Vec3 a{1.5f*std::cos(angle),1.35f+1.5f*std::sin(angle),3.73f};
        Vec3 b{1.5f*std::cos(next),1.35f+1.5f*std::sin(next),3.73f};
        Vec3 aa{a.x*.86f,a.y,6.7f},bb{b.x*.86f,b.y,6.7f};
        quad(terrainMesh,a,b,bb,aa,6.f);
    }
    quad(terrainMesh,{-1.45f,surfaceHeight(-1.45f,3.75f),3.75f},{1.45f,surfaceHeight(1.45f,3.75f),3.75f},
        {1.45f,surfaceHeight(1.45f,6.7f),6.7f},{-1.45f,surfaceHeight(-1.45f,6.7f),6.7f},1.f);
    quad(terrainMesh,{-1.1f,0.0f,6.7f},{1.1f,0.0f,6.7f},
        {1.1f,2.7f,6.7f},{-1.1f,2.7f,6.7f},6.f);
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
void addPlayerAndNpcs(const FastFrame& scene){
    auto addCharacter=[](float x,float z,float phase,float accent){
        float h=surfaceHeight(x,z);
        // A végleges embermodell helyett az animált mozgási és pozíciós teszt geometria.
        float w=.10f*std::sin(phase);
        axisBar(objectsMesh,{x,h+.9f,z},{x,h+1.40f,z},.135f,5.f,8);
        irregularRock(objectsMesh,x,z,.13f,1.05f,5.f,3);
        axisBar(objectsMesh,{x-.095f,h+.88f,z},{x-.13f,h+.1f,z+w},.06f,5.f,7);
        axisBar(objectsMesh,{x+.095f,h+.88f,z},{x+.13f,h+.1f,z-w},.06f,5.f,7);
        axisBar(objectsMesh,{x-.14f,h+1.27f,z},{x-.28f,h+.84f,z-w*.5f},.047f,5.f,6);
        axisBar(objectsMesh,{x+.14f,h+1.27f,z},{x+.28f,h+.84f,z+w*.5f},.047f,5.f,6);
        // Azonosító gyűrűként nem lebegő UI elem: a talajra fektetett 3D geometria.
        for(int j=0;j<16;++j){
            float a=2*PI*j/16,b=2*PI*(j+1)/16;
            Vec3 p{x+.33f*std::cos(a),h+.025f,z+.33f*std::sin(a)};
            Vec3 q{x+.33f*std::cos(b),h+.025f,z+.33f*std::sin(b)};
            axisBar(objectsMesh,p,q,.018f,accent,4);
        }
    };
    addCharacter(scene.playerX,scene.playerZ,scene.walkPhase,7.f);
    for(int i=0;i<4;++i){
        const float* n=scene.npcs+i*4;
        float x=n[0],z=n[1],h=surfaceHeight(x,z),c=8.f+float(i%2);
        // A társak még nem fotórealisztikusak: egyszerű térbeli helyzetjelölő.
        axisBar(objectsMesh,{x,h+.05f,z},{x,h+.46f,z},.045f,c,5);
        for(int j=0;j<12;++j){
            float a=2*PI*j/12,b=2*PI*(j+1)/12;
            Vec3 p{x+.22f*std::cos(a),h+.025f,z+.22f*std::sin(a)};
            Vec3 q{x+.22f*std::cos(b),h+.025f,z+.22f*std::sin(b)};
            axisBar(objectsMesh,p,q,.016f,c,3);
        }
    }
    if(scene.hearth>.5f){
        float x=-1.15f,z=-1.8f,h=surfaceHeight(x,z);
        for(int k=0;k<7;++k){
            float a=2*PI*k/7;
            irregularRock(objectsMesh,x+.39f*std::cos(a),z+.37f*std::sin(a),.18f,.6f,2.f,k);
        }
        axisBar(objectsMesh,{x-.33f,h+.12f,z-.18f},{x+.31f,h+.16f,z+.16f},.056f,3.f,6);
        if(scene.fire>.5f){
            // Kis lángsík csak valósan égő tűznél jelenik meg.
            quad(objectsMesh,{x-.09f,h+.16f,z},{x+.09f,h+.16f,z},
                 {x+.01f,h+.66f,z},{x-.01f,h+.64f,z},10.f);
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
flat out int matId;
void main(){
    wPos=position;
    wNorm=normal;
    matId=int(materialId+.5);
    gl_Position=u_viewProjection*vec4(position,1.);
})GLSL";
const char* FRAG=R"GLSL(#version 300 es
precision mediump float;
in vec3 wPos;in vec3 wNorm;flat in int matId;
uniform float u_rain;uniform float u_clock;
uniform sampler2D u_groundTexture;
uniform sampler2D u_cliffTexture;
out vec4 fragColor;
float hash(vec3 p){p=fract(p*.1031);p+=dot(p,p.yzx+33.33);return fract((p.x+p.y)*p.z);}
void main(){
    float grain=hash(floor(wPos*12.));
    float lines=.5+.5*sin(wPos.y*8.5+sin(wPos.x*.69+wPos.z*.21)*1.8);
    vec3 base;
    if(matId==0){
        base=texture(u_groundTexture,wPos.xz*.45).rgb * (0.85+0.15*grain);
    }else if(matId==1){
        base=texture(u_cliffTexture,vec2(wPos.x*.25,wPos.y*.34)).rgb * (0.88+0.12*lines);
    }else if(matId==2){
        base=mix(vec3(.32,.30,.27),vec3(.59,.53,.46),grain*.7);
    }else if(matId==3){
        base=mix(vec3(.27,.17,.10),vec3(.49,.31,.17),.5+.5*sin(wPos.y*8.+wPos.x*4.));
    }else if(matId==4){
        base=mix(vec3(.21,.28,.13),vec3(.43,.39,.19),grain);
    }else if(matId==5){
        base=vec3(.40,.29,.22);
    }else if(matId==6){
        base=vec3(.055,.052,.046);
    }else if(matId==7){base=vec3(.70,.61,.39);}
    else if(matId==8){base=vec3(.46,.62,.52);}
    else if(matId==9){base=vec3(.57,.60,.73);}
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
    fragColor=vec4(mapped,1.);
})GLSL";
}
const std::string& lastError(){return error;}
void shutdown(){
    if(terrainTexture[0]||terrainTexture[1])glDeleteTextures(2,terrainTexture);
    terrainTexture[0]=terrainTexture[1]=0;
    if(staticVBO)glDeleteBuffers(1,&staticVBO);
    if(dynamicVBO)glDeleteBuffers(1,&dynamicVBO);
    if(staticVAO)glDeleteVertexArrays(1,&staticVAO);
    if(dynamicVAO)glDeleteVertexArrays(1,&dynamicVAO);
    if(program)glDeleteProgram(program);
    staticVBO=dynamicVBO=staticVAO=dynamicVAO=program=0;
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
    uClock=glGetUniformLocation(program,"u_clock");
    uGround=glGetUniformLocation(program,"u_groundTexture");
    uCliff=glGetUniformLocation(program,"u_cliffTexture");
    buildStatic();
    setupBuffer(staticVAO,staticVBO,GL_STATIC_DRAW,terrainMesh);
    setupBuffer(dynamicVAO,dynamicVBO,GL_STREAM_DRAW,objectsMesh);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    if(!taks::RealModels::init()){error=taks::RealModels::errorMessage();return false;}
    return glGetError()==GL_NO_ERROR;
}
bool uploadTerrainTexture(int slot,const std::uint32_t* argb,int width,int height){
    if(slot<0||slot>1||!argb||width<128||height<128||width>2048||height>2048){
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
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0);
    if(glGetError()!=GL_NO_ERROR){
        error="A telefon GPU-ja nem tudta feltölteni a valódi PBR alapanyagtextúrát.";return false;
    }
    return true;
}
void render(int width,int height,const FastFrame& f){
    if(!program)return;
    width=std::max(width,1);height=std::max(height,1);
    float aspect=float(width)/float(height);
    float yaw=f.yaw,pitch=std::clamp(f.pitch,.349066f,1.396263f);
    float distance=std::max(18.f,f.zoom*2.f);
    Vec3 focus{f.playerX,surfaceHeight(f.playerX,f.playerZ)+.90f,f.playerZ+1.2f};
    Vec3 delta{std::sin(yaw)*std::cos(pitch)*distance,
               std::sin(pitch)*distance,
               -std::cos(yaw)*std::cos(pitch)*distance};
    Vec3 eye=add(focus,delta);
    Mat4 vp=multiply(orthographic(f.zoom*.99f*aspect,f.zoom*.99f),lookAt(eye,focus));

    objectsMesh.clear();
    addResources(f);addBuildings(f);addPlayerAndNpcs(f);
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
    glUniformMatrix4fv(uVP,1,GL_FALSE,vp.a);
    if(uRain>=0)glUniform1f(uRain,f.rain);
    if(uClock>=0)glUniform1f(uClock,f.clock);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,terrainTexture[0]);
    if(uGround>=0)glUniform1i(uGround,0);
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,terrainTexture[1]);
    if(uCliff>=0)glUniform1i(uCliff,1);
    glBindVertexArray(staticVAO);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)terrainMesh.size());
    glBindVertexArray(dynamicVAO);
    glBindBuffer(GL_ARRAY_BUFFER,dynamicVBO);
    glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(objectsMesh.size()*sizeof(Vertex)),
                 objectsMesh.empty()?nullptr:objectsMesh.data(),GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)objectsMesh.size());
    glBindVertexArray(0);
    taks::RealModels::render(vp.a,f.nodes);
}
}
