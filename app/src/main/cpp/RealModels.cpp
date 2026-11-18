#include "RealModels.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace taks::RealModels {
namespace {
struct Vertex { float pos[3], normal[3], uv[2]; };
struct Model { GLuint vao=0,vbo=0,texture=0; GLsizei count=0; };
std::array<Model,7> models;
GLuint shader=0;
GLint uVP=-1,uModel=-1,uTexture=-1,uCutoff=-1,uDaylight=-1,uRain=-1,uTint=-1;
std::string error;
constexpr float PI=3.14159265358979323846f;
const char* VS=R"GLSL(#version 300 es
precision highp float;
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
uniform mat4 uViewProjection;
uniform mat4 uModel;
out highp vec3 vNormal;
out highp vec3 vWorld;
out highp vec2 vUV;
void main() {
 vec4 p=uModel*vec4(aPosition,1.0);
 vWorld=p.xyz;
 vNormal=normalize(mat3(uModel)*aNormal);
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
out vec4 fragColor;
void main(){
  // True photographic diffuse texture sampled at model UV coordinates.
  vec4 texel=texture(uBaseColor,vUV);
  // Alpha-masked photographic leaf/frond geometry, no solid billboard cards.
  if(texel.a<uAlphaCutoff)discard;
  vec3 albedo=texel.rgb*uTint;
  vec3 n=normalize(vNormal);
  vec3 sun=normalize(vec3(-0.55,1.10,-0.7));
  float diffuse=max(dot(n,sun),0.0);
  // Energy-conserving-looking diffuse, no invented shiny metallic rocks.
  vec3 color=albedo*(0.18+0.92*diffuse*(0.20+0.80*uDaylight))*(1.0-0.28*uRain);
  color=color/(color+vec3(0.45));
  fragColor=vec4(pow(max(color,vec3(0.0)),vec3(1.0/2.2)),1.0);
}
)GLSL";
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
 glDrawArrays(GL_TRIANGLES,0,m.count);
}
}
void shutdown(){
 for(auto& m:models)deleteModel(m);
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
 if(uVP<0||uModel<0||uTexture<0||uCutoff<0||uDaylight<0||uRain<0||uTint<0){error="3D modellshader uniform hiány";shutdown();return false;}
 return true;
}
bool upload(int id,const std::uint8_t* bytes,std::size_t size,
            const std::uint32_t* argb,int width,int height){
 if(id<0||id>=static_cast<int>(models.size())||!shader||!bytes||!argb){
    error="Hiányzó vagy hibás valódi 3D hálómodell.";return false;
 }
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
bool hasRealHuman(){return models[6].vao!=0 && models[6].texture!=0 && models[6].count>=15000;}
const char* errorMessage(){return error.c_str();}
void render(const float* viewProjection,const float* worldNodes,
            const float* grass,const float* trees,bool fernApproved,
            float worldClock,float rain,float playerX,float playerZ,
            float playerHeading,const float* npcPositions){
 if(!shader||!viewProjection||!worldNodes||!grass||!trees||!npcPositions||!complete())return;
 glUseProgram(shader);
 glUniformMatrix4fv(uVP,1,GL_FALSE,viewProjection);
 glUniform1i(uTexture,0);
 const float hour=std::fmod(std::fmod(worldClock,24.f)+24.f,24.f);
 const float sunlight=std::clamp(1.f-std::abs(hour-13.f)/12.f,0.f,1.f);
 glUniform1f(uDaylight,sunlight);
 glUniform1f(uRain,std::clamp(rain,0.f,1.f));
 glUniform1f(uCutoff,0.f);
 glUniform3f(uTint,1.f,1.f,1.f);
 glActiveTexture(GL_TEXTURE0);
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
 // remove them until a physically registered geological scene is available.
 draw(models[1],-6.8f,1.9f,.88f,.6f);
 draw(models[1],5.9f,2.4f,.82f,2.1f);
 draw(models[1],6.8f,-2.8f,.95f,-.25f);
 // Genuine published botanical geometry; no generated tube/ball foliage.
 if(models[3].vao && models[3].texture){
    glUniform1f(uCutoff,.42f);
    for(int i=0;i<4;++i){
        const float* g=grass+4*i;
        const float biomass=std::clamp(g[2],0.f,1.f);
        const float health=std::clamp(g[3],0.f,1.f);
        if(biomass<.02f||health<=0.f)continue;
        // An ecological clump is a distributed PATCH of the real scanned
        // plant, never an invented fifth or infinite independent resource.
        // Grazing shrinks the whole patch, drought reduces visual density.
        const int count=std::max(1,int(30.f*biomass*health));
        for(int j=0;j<count;++j){
            const float phase=float(j)*2.39996323f+float(i)*.87f;
            const float radial=std::sqrt((float(j)+.5f)/30.f);
            const float localR=1.5f*radial;
            const float x=g[0]+std::cos(phase)*localR;
            const float z=g[1]+std::sin(phase)*localR;
            const float scale=(.23f+.12f*std::sin(float(j)*1.23f+float(i)*2.1f)
                              +.22f*biomass);
            draw(models[3],x,z,std::max(.10f,scale),phase+.42f);
        }
    }
 }
 if(models[5].vao && models[5].texture){
    glUniform1f(uCutoff,.42f);
    for(int i=0;i<3;++i){
        const float* t=trees+4*i;
        if(t[3]<.5f||t[2]<.05f)continue;
        // Mesh normalized to 2 m height in build-time importer.
        draw(models[5],t[0],t[1],t[2]/2.f,.40f+float(i)*1.9f);
    }
 }
 if(fernApproved && models[4].vao && models[4].texture){
    glUniform1f(uCutoff,.42f);
    // This biome plant is only visible when ecological verification is explicit.
    if(grass[2]>.12f)draw(models[4],grass[0]+.72f,grass[1]-.25f,.85f,.6f);
 }
 // Anatomical human meshes are real CC0 source geometry, not rod people.
 // This still tests a static neutral pose, not historical Homo erectus identity
 // or a finished skeletal walking animation.
 if(hasRealHuman()){
    glUniform1f(uCutoff,.34f);
    // Warm, sunlit skin material correction for the source's pale neutral
    // anatomical texture, while retaining authored face/eye/hair UV detail.
    glUniform3f(uTint,.80f,.56f,.43f);
    draw(models[6],playerX,playerZ,.99f,playerHeading);
    for(int i=0;i<4;++i){
        const float* n=npcPositions+i*4;
        const float scale=.92f+.025f*float(i);
        const float t=float(i);
        glUniform3f(uTint,.83f-.04f*t,.60f-.025f*t,.47f-.02f*t);
        draw(models[6],n[0],n[1],scale,float(i)*1.33f+.47f);
    }
    glUniform3f(uTint,1.f,1.f,1.f);
 }
 glUniform1f(uCutoff,0.f);
 glBindVertexArray(0);
 glBindTexture(GL_TEXTURE_2D,0);
}
}
