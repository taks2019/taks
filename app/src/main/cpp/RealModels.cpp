#include "RealModels.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <string>
#include <vector>

namespace taks::RealModels {
namespace {
struct Vertex { float pos[3], normal[3], uv[2]; };
struct SkinnedVertex { float pos[3],normal[3],uv[2];std::uint16_t joints[4];float weights[4]; };
static_assert(sizeof(SkinnedVertex)==56,"TKS1 skinned vertex layout");
struct Model { GLuint vao=0,vbo=0,texture=0; GLsizei count=0;bool skinned=false; };
struct Joint { int parent=-1;float translation[3]={};float rotation[4]={0,0,0,1};float inverseBind[16]={}; };
struct Clip { std::string name;float fps=30;int frames=0;std::vector<float> rotations,root; };
struct HumanRig { std::array<Joint,53> joints{};std::vector<Clip> clips;bool valid=false; };
std::array<Model,7> models;
HumanRig human;
GLuint shader=0;
GLint uVP=-1,uModel=-1,uTexture=-1,uCutoff=-1,uDaylight=-1,uRain=-1,uTint=-1;
GLint uWindStrength=-1,uWindDirection=-1,uAnimationTime=-1,uFoliage=-1;
GLint uWetSeason=-1,uSoilMoisture=-1,uSwayAmount=-1;
GLint uBones=-1;
std::string error;
int renderedHumans=0;
constexpr float PI=3.14159265358979323846f;
const char* VS=R"GLSL(#version 300 es
precision highp float;
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in uvec4 aJoints;
layout(location=4) in vec4 aWeights;
uniform mat4 uViewProjection;
uniform mat4 uModel;
uniform mat4 uBones[53];
uniform float uFoliage;
uniform float uWindStrength;
uniform vec2 uWindDirection;
uniform float uSwayAmount;
uniform float uAnimationTime;
out highp vec3 vNormal;
out highp vec3 vWorld;
out highp vec2 vUV;
void main() {
 float totalWeight=dot(aWeights,vec4(1.0));
 mat4 skin=mat4(1.0);
 if(totalWeight>0.0001){
   skin=uBones[aJoints.x]*aWeights.x+uBones[aJoints.y]*aWeights.y+
        uBones[aJoints.z]*aWeights.z+uBones[aJoints.w]*aWeights.w;
 }
 vec4 skinned=skin*vec4(aPosition,1.0);
 vec4 p=uModel*skinned;
 if(uFoliage>0.5 && uWindStrength>0.001){
   // World-space bending keeps rotated clumps aligned with the same gust.
   // The planted base stays fixed. Trampled, shorter blades sway less.
   float height=max(p.y-uModel[3].y,0.0);
   vec2 direction=uWindDirection/max(length(uWindDirection),0.001);
   float phase=uAnimationTime*(1.35+uWindStrength*1.1)+dot(p.xz,direction)*1.65;
   float gust=0.28+0.72*sin(phase)+0.18*sin(phase*0.53+1.7);
   p.xz+=direction*(height*0.27*uSwayAmount*uWindStrength*gust);
 }
 vWorld=p.xyz;
 vNormal=normalize(mat3(uModel)*mat3(skin)*aNormal);
 vUV=aTexCoord;
 gl_Position=uViewProjection*p;
}
)GLSL";
const char* FS=R"GLSL(#version 300 es
precision highp float;
in highp vec3 vNormal;
in highp vec3 vWorld;
in highp vec2 vUV;
uniform sampler2D uBaseColor;
uniform float uAlphaCutoff;
uniform float uDaylight;
uniform float uRain;
uniform vec3 uTint;
uniform float uFoliage;
uniform float uWetSeason;
uniform float uSoilMoisture;
out vec4 fragColor;
void main(){
  // True photographic diffuse texture sampled at model UV coordinates.
  vec4 texel=texture(uBaseColor,vUV);
  // Alpha-masked photographic leaf/frond geometry, no solid billboard cards.
  if(texel.a<uAlphaCutoff)discard;
  vec3 albedo=texel.rgb*uTint;
  if(uFoliage>0.5){
    float drought=clamp((0.62-uWetSeason)*0.50+(0.34-uSoilMoisture)*0.36,0.0,0.52);
    albedo*=mix(vec3(1.0),vec3(1.10,0.84,0.61),drought);
  }
  vec3 n=normalize(vNormal);
  vec3 sun=normalize(vec3(-0.55,1.10,-0.7));
  float diffuse=max(dot(n,sun),0.0);
  // Energy-conserving-looking diffuse, no invented shiny metallic rocks.
  vec3 color=albedo*(0.18+0.92*diffuse*(0.20+0.80*uDaylight))*(1.0-0.28*uRain);
  color=color/(color+vec3(0.45));
  fragColor=vec4(pow(max(color,vec3(0.0)),vec3(1.0/2.2)),1.0);
}
)GLSL";
struct Quat { float x=0,y=0,z=0,w=1; };
struct Vec3 { float x=0,y=0,z=0; };
struct Mat4 { float a[16]{}; };
Quat qmul(Quat a,Quat b){return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
                                 a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
                                 a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
                                 a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
Quat qnorm(Quat q){float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    if(n<1.e-8f)return {};
    return {q.x/n,q.y/n,q.z/n,q.w/n};}
Quat qslerp(Quat a,Quat b,float t){
    float d=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
    if(d<0.f){b={-b.x,-b.y,-b.z,-b.w};d=-d;}
    if(d>.9995f)return qnorm({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,
                              a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t});
    const float angle=std::acos(std::clamp(d,-1.f,1.f));
    const float sa=std::sin((1.f-t)*angle)/std::sin(angle),sb=std::sin(t*angle)/std::sin(angle);
    return {a.x*sa+b.x*sb,a.y*sa+b.y*sb,a.z*sa+b.z*sb,a.w*sa+b.w*sb};
}
Mat4 multiply(const Mat4& a,const Mat4& b){Mat4 o;
    for(int c=0;c<4;++c)for(int r=0;r<4;++r)
        for(int k=0;k<4;++k)o.a[c*4+r]+=a.a[k*4+r]*b.a[c*4+k];
    return o;}
Mat4 localMatrix(const float* t,Quat q){q=qnorm(q);Mat4 m;
    const float x=q.x,y=q.y,z=q.z,w=q.w;
    m.a[0]=1-2*(y*y+z*z);m.a[1]=2*(x*y+z*w);m.a[2]=2*(x*z-y*w);
    m.a[4]=2*(x*y-z*w);m.a[5]=1-2*(x*x+z*z);m.a[6]=2*(y*z+x*w);
    m.a[8]=2*(x*z+y*w);m.a[9]=2*(y*z-x*w);m.a[10]=1-2*(x*x+y*y);
    m.a[12]=t[0];m.a[13]=t[1];m.a[14]=t[2];m.a[15]=1;return m;}
struct Pose { std::array<Quat,53> rotations{};Vec3 root{}; };
std::array<Mat4,53> boneMatrices{};
std::array<Pose,5> previousPose{};
std::array<int,5> previousClip{{-1,-1,-1,-1,-1}};
std::array<double,5> transitionStart{};
double animationNow(){using clock=std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();}
Pose sampleClip(int clipIndex,double time){
    Pose pose;for(auto& q:pose.rotations)q={};
    if(clipIndex<0||clipIndex>=static_cast<int>(human.clips.size()))return pose;
    const Clip& clip=human.clips[clipIndex];if(clip.frames<=0)return pose;
    const int loopFrames=std::max(1,clip.frames-1);
    const double frame=std::fmod(std::max(0.0,time)*clip.fps,double(loopFrames));
    const int a=static_cast<int>(frame),b=std::min(a+1,clip.frames-1);const float blend=float(frame-a);
    for(int joint=0;joint<53;++joint){
        const std::size_t ia=(std::size_t(a)*53+joint)*4,ib=(std::size_t(b)*53+joint)*4;
        const auto& q=clip.rotations;
        pose.rotations[joint]=qslerp({q[ia],q[ia+1],q[ia+2],q[ia+3]},
                                     {q[ib],q[ib+1],q[ib+2],q[ib+3]},blend);
    }
    const std::size_t ra=std::size_t(a)*3,rb=std::size_t(b)*3;
    pose.root={clip.root[ra]+(clip.root[rb]-clip.root[ra])*blend,
               clip.root[ra+1]+(clip.root[rb+1]-clip.root[ra+1])*blend,
               clip.root[ra+2]+(clip.root[rb+2]-clip.root[ra+2])*blend};
    return pose;
}
Pose poseForActor(int actor,int desired,double now){
    if(previousClip[actor]!=desired){
        previousPose[actor]=sampleClip(previousClip[actor],now);
        previousClip[actor]=desired;transitionStart[actor]=now;
    }
    Pose current=sampleClip(desired,now);
    const float t=std::clamp(float((now-transitionStart[actor])/.28),0.f,1.f);
    if(t<1.f){
        for(int j=0;j<53;++j)current.rotations[j]=qslerp(previousPose[actor].rotations[j],current.rotations[j],t);
        current.root.x=previousPose[actor].root.x+(current.root.x-previousPose[actor].root.x)*t;
        current.root.y=previousPose[actor].root.y+(current.root.y-previousPose[actor].root.y)*t;
        current.root.z=previousPose[actor].root.z+(current.root.z-previousPose[actor].root.z)*t;
    }
    return current;
}
void evaluateBones(const Pose& pose){
    std::array<Mat4,53> global{};
    for(int i=0;i<53;++i){
        const Joint& j=human.joints[i];
        float t[3]={j.translation[0],j.translation[1],j.translation[2]};
        if(i==0){t[0]+=pose.root.x;t[1]+=pose.root.y;t[2]+=pose.root.z;}
        Quat rest{j.rotation[0],j.rotation[1],j.rotation[2],j.rotation[3]};
        Mat4 local=localMatrix(t,qmul(rest,pose.rotations[i]));
        global[i]=j.parent<0?local:multiply(global[j.parent],local);
        Mat4 bind;std::copy(j.inverseBind,j.inverseBind+16,bind.a);
        boneMatrices[i]=multiply(global[i],bind);
    }
}
GLuint compile(GLenum type,const char* code){
 GLuint result=glCreateShader(type);
 glShaderSource(result,1,&code,nullptr);glCompileShader(result);
 GLint ok=GL_FALSE;glGetShaderiv(result,GL_COMPILE_STATUS,&ok);
 if(!ok){
    GLint len=0;glGetShaderiv(result,GL_INFO_LOG_LENGTH,&len);
    std::string log((std::size_t)std::max(len,1),'\0');
    glGetShaderInfoLog(result,len,nullptr,log.data());
    error += log;
    glDeleteShader(result);return 0;
 }
 return result;
}
void deleteModel(Model& model){
 if(model.vao)glDeleteVertexArrays(1,&model.vao);
 if(model.vbo)glDeleteBuffers(1,&model.vbo);
 if(model.texture)glDeleteTextures(1,&model.texture);
 model={};
}
template<class T> bool take(const std::uint8_t*& cursor,const std::uint8_t* end,T& value){
 if(std::size_t(end-cursor)<sizeof(T))return false;
 std::memcpy(&value,cursor,sizeof(T));cursor+=sizeof(T);return true;
}
bool uploadRigged(const std::uint8_t* bytes,std::size_t size,const std::uint32_t* argb,
                 int width,int height){
 if(size<16||std::memcmp(bytes,"TKS1",4)!=0){error="A riggelt embermodell fejléce hibás.";return false;}
 const std::uint8_t* cursor=bytes+4;const std::uint8_t* end=bytes+size;
 std::uint32_t count=0,jointCount=0,clipCount=0;
 if(!take(cursor,end,count)||!take(cursor,end,jointCount)||!take(cursor,end,clipCount)||
    count<15000||count>180000||jointCount!=53||clipCount<3||clipCount>8||count%3){
    error="A riggelt embermodell mérete vagy 53 csontos váza hibás.";return false;
 }
 const std::size_t vertexBytes=std::size_t(count)*sizeof(SkinnedVertex);
 if(vertexBytes>std::size_t(end-cursor)){error="A riggelt hálóadat csonka.";return false;}
 const std::uint8_t* vertexData=cursor;cursor+=vertexBytes;
 HumanRig next;
 for(int i=0;i<53;++i){
    Joint& j=next.joints[i];
    if(!take(cursor,end,j.parent)||!take(cursor,end,j.translation)||
       !take(cursor,end,j.rotation)||!take(cursor,end,j.inverseBind)||
       j.parent>=i||j.parent< -1){error="A humanoid csontváz kapcsolatai hibásak.";return false;}
 }
 for(std::uint32_t c=0;c<clipCount;++c){
    char name[16]{};std::uint32_t frameCount=0;float fps=0;
    if(!take(cursor,end,name)||!take(cursor,end,frameCount)||!take(cursor,end,fps)||
       frameCount<2||frameCount>600||!std::isfinite(fps)||fps<1||fps>120){
       error="A humanoid animációs klip fejléce hibás.";return false;
    }
    Clip clip;clip.name.assign(name,strnlen(name,sizeof(name)));clip.frames=int(frameCount);clip.fps=fps;
    const std::size_t rotationCount=std::size_t(frameCount)*53*4;
    const std::size_t rootCount=std::size_t(frameCount)*3;
    const std::size_t clipBytes=(rotationCount+rootCount)*sizeof(float);
    if(clipBytes>std::size_t(end-cursor)){error="A humanoid animációs klip csonka.";return false;}
    clip.rotations.resize(rotationCount);std::memcpy(clip.rotations.data(),cursor,rotationCount*sizeof(float));cursor+=rotationCount*sizeof(float);
    clip.root.resize(rootCount);std::memcpy(clip.root.data(),cursor,rootCount*sizeof(float));cursor+=rootCount*sizeof(float);
    if(!std::all_of(clip.rotations.begin(),clip.rotations.end(),[](float v){return std::isfinite(v);})||
       !std::all_of(clip.root.begin(),clip.root.end(),[](float v){return std::isfinite(v);})){
       error="A humanoid animációs klip nem véges számot tartalmaz.";return false;
    }
    next.clips.push_back(std::move(clip));
 }
 if(cursor!=end||width<128||height<128||width>2048||height>2048){
    error="A riggelt embermodell atlasza vagy fájlhossza hibás.";return false;
 }
 Model& model=models[6];deleteModel(model);
 glGenVertexArrays(1,&model.vao);glBindVertexArray(model.vao);
 glGenBuffers(1,&model.vbo);glBindBuffer(GL_ARRAY_BUFFER,model.vbo);
 glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(vertexBytes),vertexData,GL_STATIC_DRAW);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(SkinnedVertex),reinterpret_cast<const void*>(0));
 glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(SkinnedVertex),reinterpret_cast<const void*>(12));
 glEnableVertexAttribArray(2);glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(SkinnedVertex),reinterpret_cast<const void*>(24));
 glEnableVertexAttribArray(3);glVertexAttribIPointer(3,4,GL_UNSIGNED_SHORT,sizeof(SkinnedVertex),reinterpret_cast<const void*>(32));
 glEnableVertexAttribArray(4);glVertexAttribPointer(4,4,GL_FLOAT,GL_FALSE,sizeof(SkinnedVertex),reinterpret_cast<const void*>(40));
 glBindVertexArray(0);
 std::vector<std::uint8_t> rgba(std::size_t(width)*height*4);
 for(std::size_t i=0;i<std::size_t(width)*height;++i){
    const std::uint32_t color=argb[i];rgba[i*4]=std::uint8_t(color>>16);
    rgba[i*4+1]=std::uint8_t(color>>8);rgba[i*4+2]=std::uint8_t(color);
    rgba[i*4+3]=std::uint8_t(color>>24);
 }
 glGenTextures(1,&model.texture);glBindTexture(GL_TEXTURE_2D,model.texture);
 glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glGenerateMipmap(GL_TEXTURE_2D);
 glBindTexture(GL_TEXTURE_2D,0);
 if(glGetError()!=GL_NO_ERROR){error="A telefon GPU-ja nem tudta feltölteni a skinned humanoid hálót.";deleteModel(model);return false;}
 model.count=GLsizei(count);model.skinned=true;next.valid=true;human=std::move(next);
 previousClip.fill(-1);previousPose={};transitionStart.fill(animationNow());
 return true;
}
float surfaceHeight(float x,float z){
 auto saturate=[](float v){return std::max(0.f,std::min(1.f,v));};
 float e=saturate((11.f-std::abs(x))/2.4f) *
        saturate((z+11.f)/2.4f)*saturate((17.f-z)/2.4f);
 float relief=-.27f+.17f*std::sin(x*.21f)*std::cos(z*.22f)+
              .09f*std::sin(x*.87f+z*.57f)+.065f*std::cos(z*.73f-x*.42f);
 return -.48f+(relief+.48f)*e;
}
void draw(const Model& m,float x,float z,float scale,float radians){
 float c=std::cos(radians),s=std::sin(radians);
 const float matrix[16]={
    scale*c, 0.f,-scale*s,0.f,
    0.f,scale,0.f,0.f,
    scale*s,0.f,scale*c,0.f,
    x,surfaceHeight(x,z)-.014f,z,1.f};
 glUniformMatrix4fv(uModel,1,GL_FALSE,matrix);
 glBindTexture(GL_TEXTURE_2D,m.texture);
 glBindVertexArray(m.vao);
 if(!m.skinned){glVertexAttribI4ui(3,0,0,0,0);glVertexAttrib4f(4,0,0,0,0);}
 glDrawArrays(GL_TRIANGLES,0,m.count);
}
void drawHuman(float x,float z,float scale,float radians,int actor,int clipIndex,double now){
 const Model& model=models[6];if(!model.vao||!human.valid||uBones<0)return;
 const Pose pose=poseForActor(actor,clipIndex,now);evaluateBones(pose);
 glUniformMatrix4fv(uBones,53,GL_FALSE,boneMatrices[0].a);
 float c=std::cos(radians),s=std::sin(radians);
 const float matrix[16]={scale*c,0.f,-scale*s,0.f,0.f,scale,0.f,0.f,
                         scale*s,0.f,scale*c,0.f,x,surfaceHeight(x,z)-.014f,z,1.f};
 glUniformMatrix4fv(uModel,1,GL_FALSE,matrix);
 glBindTexture(GL_TEXTURE_2D,model.texture);glBindVertexArray(model.vao);
 glDrawArrays(GL_TRIANGLES,0,model.count);
 ++renderedHumans;
}
}
void shutdown(){
 for(auto& m:models)deleteModel(m);
 human=HumanRig{};previousClip.fill(-1);previousPose={};transitionStart.fill(0.0);
 if(shader)glDeleteProgram(shader);
 shader=0;
}
bool init(){
 shutdown();error.clear();
 GLuint vs=compile(GL_VERTEX_SHADER,VS),fs=compile(GL_FRAGMENT_SHADER,FS);
 if(!vs||!fs){if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);return false;}
 shader=glCreateProgram();
 glAttachShader(shader,vs);glAttachShader(shader,fs);
 glLinkProgram(shader);
 glDeleteShader(vs);glDeleteShader(fs);
 GLint linked=GL_FALSE;glGetProgramiv(shader,GL_LINK_STATUS,&linked);
 if(!linked){
    error="A valódi fotótextúrás 3D modellshader nem fordult le.";
    shutdown();return false;
 }
 uVP=glGetUniformLocation(shader,"uViewProjection");
 uModel=glGetUniformLocation(shader,"uModel");
 uTexture=glGetUniformLocation(shader,"uBaseColor");
 uCutoff=glGetUniformLocation(shader,"uAlphaCutoff");
 uDaylight=glGetUniformLocation(shader,"uDaylight");
 uRain=glGetUniformLocation(shader,"uRain");
 uTint=glGetUniformLocation(shader,"uTint");
 uWindStrength=glGetUniformLocation(shader,"uWindStrength");
 uSwayAmount=glGetUniformLocation(shader,"uSwayAmount");
 uWindDirection=glGetUniformLocation(shader,"uWindDirection");
 uAnimationTime=glGetUniformLocation(shader,"uAnimationTime");
 uFoliage=glGetUniformLocation(shader,"uFoliage");
 uWetSeason=glGetUniformLocation(shader,"uWetSeason");
 uSoilMoisture=glGetUniformLocation(shader,"uSoilMoisture");
 uBones=glGetUniformLocation(shader,"uBones[0]");
 if(uVP<0||uModel<0||uTexture<0||uCutoff<0||uDaylight<0||uRain<0||uTint<0||
    uSwayAmount<0||uWindStrength<0||uWindDirection<0||uAnimationTime<0||uFoliage<0||
    uWetSeason<0||uSoilMoisture<0||uBones<0){error="3D modellshader uniform hiány";shutdown();return false;}
 return true;
}
bool upload(int id,const std::uint8_t* bytes,std::size_t size,
            const std::uint32_t* argb,int width,int height){
 if(id<0||id>=static_cast<int>(models.size())||!shader||!bytes||!argb){
    error="Hiányzó vagy hibás valódi 3D hálómodell.";return false;
 }
 if(id==6&&size>=4&&std::memcmp(bytes,"TKS1",4)==0)
    return uploadRigged(bytes,size,argb,width,height);
 if(id==6){error="Az embermodell csak megőrzött bőrsúlyokkal és TKS1 csontvázzal tölthető be.";return false;}
 if(size<8 || std::memcmp(bytes,"TKM1",4)!=0){
    error="A 3D modellek TKM1 formátuma sérült.";return false;
 }
 std::uint32_t count=0;
 std::memcpy(&count,bytes+4,4);
 if(count<3||count>650000||count%3!=0||size!=8+std::size_t(count)*sizeof(Vertex)){
    error="A modellcsomag vertexmérete vagy geometriája hibás.";return false;
 }
 if(width<128||height<128||width>2048||height>2048){
    error="A fotótextúra mérete hibás.";return false;
 }
 Model& m=models[id];deleteModel(m);
 glGenVertexArrays(1,&m.vao);glBindVertexArray(m.vao);
 glGenBuffers(1,&m.vbo);glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
 glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(count*sizeof(Vertex)),bytes+8,GL_STATIC_DRAW);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(0));
 glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(12));
 glEnableVertexAttribArray(2);glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(24));
 glBindVertexArray(0);
 std::vector<std::uint8_t> rgba(std::size_t(width)*height*4);
 for(std::size_t i=0;i<std::size_t(width)*height;++i){
    std::uint32_t c=argb[i];
    rgba[i*4+0]=std::uint8_t((c>>16)&255);
    rgba[i*4+1]=std::uint8_t((c>>8)&255);
    rgba[i*4+2]=std::uint8_t(c&255);
    rgba[i*4+3]=std::uint8_t((c>>24)&255);
 }
 glGenTextures(1,&m.texture);glBindTexture(GL_TEXTURE_2D,m.texture);
 glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,width,height,0,GL_RGBA,
              GL_UNSIGNED_BYTE,rgba.data());
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,id>=3?GL_CLAMP_TO_EDGE:GL_REPEAT);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,id>=3?GL_CLAMP_TO_EDGE:GL_REPEAT);
 glGenerateMipmap(GL_TEXTURE_2D);
 glBindTexture(GL_TEXTURE_2D,0);
 m.count=GLsizei(count);
 if(glGetError()!=GL_NO_ERROR){
    error="Az Android GPU nem tudta feltölteni a valódi 3D modellt/textúrát.";
    deleteModel(m);return false;
 }
 return true;
}
bool complete(){return models[0].vao&&models[0].texture&&models[1].vao&&models[1].texture;}
bool hasRealHuman(){return models[6].vao!=0 && models[6].texture!=0 && models[6].count>=15000&&human.valid;}
int lastHumanDrawCount(){return renderedHumans;}
const char* errorMessage(){return error.c_str();}
void render(const float* viewProjection,const float* worldNodes,
            const float* grass,const float* trees,bool fernApproved,
            float worldClock,float rain,float playerX,float playerZ,
            float playerHeading,bool drawPlayer,bool playerMoving,bool playerRunning,
            const float* npcPositions,float animationTime,float windStrength,
            float windX,float windZ,float wetSeason,float soilMoisture){
 renderedHumans=0;
 if(!shader||!viewProjection||!worldNodes||!grass||!trees||!npcPositions||!complete())return;
 glUseProgram(shader);
 glUniformMatrix4fv(uVP,1,GL_FALSE,viewProjection);
 glUniform1i(uTexture,0);
 const float hour=std::fmod(std::fmod(worldClock,24.f)+24.f,24.f);
 const float sunlight=std::clamp(1.f-std::abs(hour-13.f)/12.f,0.f,1.f);
 glUniform1f(uDaylight,sunlight);
 glUniform1f(uRain,std::clamp(rain,0.f,1.f));
 glUniform1f(uAnimationTime,animationTime);
 glUniform1f(uWindStrength,std::clamp(windStrength,0.f,1.f));
 glUniform2f(uWindDirection,windX,windZ);
 glUniform1f(uWetSeason,std::clamp(wetSeason,0.f,1.f));
 glUniform1f(uSoilMoisture,std::clamp(soilMoisture,0.f,1.f));
 glUniform1f(uFoliage,0.f);
 glUniform1f(uCutoff,0.f);
 glUniform3f(uTint,1.f,1.f,1.f);
 glActiveTexture(GL_TEXTURE0);
 const double now=animationNow();
 // Smaller rocks follow real, finite, collectible world inventory.
 for(int i=0;i<12;++i){
    const float* n=worldNodes+i*4;
    if(int(n[2]+.5f)!=1||n[3]<.5f)continue;
    draw(models[0],n[0],n[1],1.4f,float(i)*1.15f);
 }
 // Three non-collectible geological outcrops, not part of inventory.
 // A later geology system must give the static rock formation physical identity.
 // Photo-scanned stones stay grounded and human-scaled. The old free-
 // standing giant cliff props looked suspended above the cave entrance:
 // use two lower side outcrops, then distribute smaller scans around the edge
 // of the explored ground. These remain visual geology; resource nodes are
 // still the only stones with collectible inventory state.
 if(models[2].vao&&models[2].texture){
    // Layer several scanned rock faces around the explored basin so the
    // walkable region reads as a cave clearing, not a lone wall prop.
    draw(models[2],-8.25f,2.15f,.48f,-.48f);
    draw(models[2],8.10f,2.45f,.44f,2.45f);
    draw(models[2],-8.30f,-5.70f,.31f,1.30f);
    draw(models[2],8.25f,-5.25f,.29f,-1.55f);
    draw(models[2],-7.90f,.10f,.26f,2.10f);
    draw(models[2],7.85f,-.45f,.28f,-.85f);
 }
 draw(models[1],-6.8f,1.9f,.88f,.6f);
 draw(models[1],5.9f,2.4f,.82f,2.1f);
 draw(models[1],6.8f,-2.8f,.95f,-.25f);
 for(int i=0;i<36;++i){
    const float phase=float(i)*2.39996323f;
    const float radius=3.7f+float((i*7)%11)*.48f;
    const float x=std::cos(phase)*radius;
    const float z=std::sin(phase)*(4.2f+float((i*3)%7)*.38f)-.8f;
    if(std::abs(x)<1.45f&&z>1.2f)continue;
    const float dx=x-playerX,dz=z-playerZ;
    if(dx*dx+dz*dz<1.8f)continue;
    const float scale=.62f+.26f*(.5f+.5f*std::sin(float(i)*1.73f));
    draw(models[0],x,z,scale,phase*.7f);
 }
 // Genuine published botanical geometry; no generated tube/ball foliage.
 if(models[3].vao && models[3].texture){
    glUniform1f(uFoliage,1.f);
    glUniform1f(uCutoff,.42f);
    glUniform1f(uSwayAmount,1.f);
    for(int i=0;i<4;++i){
        const float* g=grass+5*i;
        const float biomass=std::clamp(g[2],0.f,1.f);
        const float health=std::clamp(g[3],0.f,1.f);
        const float trampling=std::clamp(g[4],0.f,1.f);
        if(biomass<.02f||health<=0.f)continue;
        // An ecological clump is a distributed PATCH of the real scanned
        // plant, never an invented fifth or infinite independent resource.
        // Grazing shrinks the whole patch, drought reduces visual density.
        const int count=std::max(1,int(48.f*biomass*health*(1.f-.58f*trampling)));
        for(int j=0;j<count;++j){
            const float phase=float(j)*2.39996323f+float(i)*.87f;
            const float radial=std::sqrt((float(j)+.5f)/48.f);
            const float localR=2.05f*radial;
            const float x=g[0]+std::cos(phase)*localR;
            const float z=g[1]+std::sin(phase)*localR;
            const float scale=(.29f+.15f*std::sin(float(j)*1.23f+float(i)*2.1f)
                              +.24f*biomass)*(1.f-.38f*trampling);
            const float lean=trampling*.58f;
            const float c=std::cos(phase+.42f),s=std::sin(phase+.42f);
            const float matrix[16]={
                scale*c,0.f,-scale*s,0.f,
                lean*scale,scale*(1.f-.30f*trampling),lean*.55f*scale,0.f,
                scale*s,0.f,scale*c,0.f,
                x,surfaceHeight(x,z)-.014f,z,1.f};
            glUniformMatrix4fv(uModel,1,GL_FALSE,matrix);
            glBindTexture(GL_TEXTURE_2D,models[3].texture);
            glBindVertexArray(models[3].vao);
            glVertexAttribI4ui(3,0,0,0,0);glVertexAttrib4f(4,0,0,0,0);
            glDrawArrays(GL_TRIANGLES,0,models[3].count);
        }
    }
 }
 if(models[5].vao && models[5].texture){
    glUniform1f(uFoliage,1.f);
    glUniform1f(uCutoff,.42f);
    glUniform1f(uSwayAmount,.22f);
    for(int i=0;i<3;++i){
        const float* t=trees+4*i;
        if(t[3]<.5f||t[2]<.05f)continue;
        // Mesh normalized to 2 m height in build-time importer.
        draw(models[5],t[0],t[1],t[2]/2.f,.40f+float(i)*1.9f);
    }
 }
 if(fernApproved && models[4].vao && models[4].texture){
    glUniform1f(uFoliage,1.f);
    glUniform1f(uCutoff,.42f);
    glUniform1f(uSwayAmount,1.1f);
    // This biome plant is only visible when ecological verification is explicit.
    if(grass[2]>.12f)draw(models[4],grass[0]+.72f,grass[1]-.25f,.85f,.6f);
 }
 glUniform1f(uFoliage,0.f);
 // Real source skin weights and the 53-bone rig drive every person on the GPU.
 glUniform1f(uSwayAmount,0.f);
 // Clip changes blend over 280 ms; the source remains a general humanoid, not a
 // claimed archaeological reconstruction of Homo erectus.
 if(hasRealHuman()){
    glUniform1f(uCutoff,.34f);
    // Warm the extremely pale source diffuse map so exposed skin and hide
    // follow a more natural daylight range on phone displays.
    glUniform3f(uTint,1.02f,.90f,.77f);
    const int playerClip=playerRunning?4:(playerMoving?3:0);
    if(drawPlayer)drawHuman(playerX,playerZ,1.f,playerHeading,0,playerClip,now);
    // Each companion has its own gait phase; no synchronized zombie walk.
    for(int i=0;i<4;++i){
        const float* n=npcPositions+i*4;
        const float scale=.92f+.025f*float(i);
        const int task=std::clamp(int(n[2]+.5f),0,4);
        const bool carrying=n[3]>.5f;
        float targetX=-.30f,targetZ=-3.40f;
        if(!carrying){
            static constexpr int resourceTypeByRole[4]={0,1,3,2};
            const int resourceType=resourceTypeByRole[i];
            float best=1.e9f;
            for(int k=0;k<12;++k){const float* node=worldNodes+k*4;
                if(int(node[2]+.5f)!=resourceType||node[3]<.5f)continue;
                const float dx=node[0]-n[0],dz=node[1]-n[1],distance=dx*dx+dz*dz;
                if(distance<best){best=distance;targetX=node[0];targetZ=node[1];}
            }
        }
        const float heading=std::atan2(targetX-n[0],targetZ-n[1]);
        const int clip=(task==1||task==2)?3:(task==3?1:0);
        static constexpr float skinTints[4][3]={
            {1.04f,.92f,.80f},{.93f,.84f,.74f},
            {.98f,.89f,.78f},{.91f,.82f,.71f}
        };
        glUniform3f(uTint,skinTints[i][0],skinTints[i][1],skinTints[i][2]);
        drawHuman(n[0],n[1],scale,heading,i+1,clip,now+double(i+1)*.63);
    }
 }
 glUniform1f(uCutoff,0.f);
 glBindVertexArray(0);
 glBindTexture(GL_TEXTURE_2D,0);
}
}
