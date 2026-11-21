#include "CaveSprite.h"
#include "CaveBillboardGeometry.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>
namespace taks::CaveSprite {
namespace {
constexpr float pi = 3.14159265358979323846f;
struct Vertex { float x,y,z,u,v; };
GLuint program=0,vao=0,vbo=0,textures[8]={},shadowTexture=0;
GLint mvpUniform=-1,alphaUniform=-1,tintUniform=-1,blendUniform=-1;
std::string error;
bool initialized=false;
const char* VERT=R"GLSL(#version 300 es
layout(location=0) in vec3 a_position;
layout(location=1) in vec2 a_uv;
uniform mat4 u_viewProjection;
out vec2 v_uv;
void main(){
    gl_Position=u_viewProjection*vec4(a_position,1.0);
    v_uv=a_uv;
}
)GLSL";
const char* FRAG=R"GLSL(#version 300 es
precision mediump float;
in vec2 v_uv;
uniform sampler2D u_image;
uniform sampler2D u_image_next;
uniform float u_viewBlend;
uniform float u_opacity;
uniform vec3 u_tint;
out vec4 color;
void main(){
    // Blend the two viewpoints in ONE fragment pass. Two separate alpha
    // draws made the cave briefly transparent and appear to jump on rotation.
    vec4 first=texture(u_image,v_uv);
    vec4 second=texture(u_image_next,v_uv);
    vec4 texel=mix(first,second,clamp(u_viewBlend,0.0,1.0));
    if(texel.a<0.016)discard;
    color=vec4(texel.rgb*u_tint, texel.a*u_opacity);
}
)GLSL";
GLuint shader(GLenum kind,const char* source){
    GLuint id=glCreateShader(kind);
    glShaderSource(id,1,&source,nullptr);
    glCompileShader(id);
    GLint ok=GL_FALSE;
    glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
    if(!ok){
        GLint length=0;
        glGetShaderiv(id,GL_INFO_LOG_LENGTH,&length);
        std::string diagnostics(static_cast<std::size_t>(std::max(length,1)),'\0');
        glGetShaderInfoLog(id,length,nullptr,diagnostics.data());
        error="Cave PNG shader: "+diagnostics;
        glDeleteShader(id);
        return 0;
    }
    return id;
}
void drawTexture(GLuint texture,const Vertex* data,float alpha,float tint){
    if(!texture||alpha<.002f)return;
    glBindTexture(GL_TEXTURE_2D,texture);
    glUniform1f(alphaUniform,std::clamp(alpha,0.f,1.f));
    glUniform3f(tintUniform,tint,tint*.985f,tint*.955f);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,6*sizeof(Vertex),data,GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}
}
const std::string& lastError(){return error;}
void shutdown(){
    if(vbo)glDeleteBuffers(1,&vbo);
    if(vao)glDeleteVertexArrays(1,&vao);
    if(program)glDeleteProgram(program);
    if(shadowTexture)glDeleteTextures(1,&shadowTexture);
    glDeleteTextures(8,textures);
    vbo=vao=program=shadowTexture=0;
    std::fill(std::begin(textures),std::end(textures),0);
    initialized=false;
}
bool init(){
    shutdown();error.clear();
    // Android GLES error state is shared across all render modules. The
    // prior 3D model uploader may leave a stale error. It is NOT evidence
    // that this shader/PNG renderer failed; isolate this module's operations.
    for(int i=0;i<16 && glGetError()!=GL_NO_ERROR;++i){}
    GLuint vs=shader(GL_VERTEX_SHADER,VERT),fs=shader(GL_FRAGMENT_SHADER,FRAG);
    if(!vs||!fs){
        if(vs)glDeleteShader(vs);
        if(fs)glDeleteShader(fs);
        return false;
    }
    program=glCreateProgram();
    glAttachShader(program,vs);
    glAttachShader(program,fs);
    glLinkProgram(program);
    glDeleteShader(vs);glDeleteShader(fs);
    GLint ok=GL_FALSE;glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if(!ok){error="Cave PNG shader program failed to link";shutdown();return false;}
    mvpUniform=glGetUniformLocation(program,"u_viewProjection");
    alphaUniform=glGetUniformLocation(program,"u_opacity");
    tintUniform=glGetUniformLocation(program,"u_tint");
    blendUniform=glGetUniformLocation(program,"u_viewBlend");
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,6*sizeof(Vertex),nullptr,GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex,x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex,u)));
    glBindVertexArray(0);
    glGenTextures(8,textures);
    // Procedural soft ground contact shadow, not a new photographic asset.
    constexpr int size=64;
    std::array<std::uint8_t,size*size*4> shadow{};
    for(int y=0;y<size;++y)for(int x=0;x<size;++x){
        float u=(float(x)+.5f-size*.5f)/(size*.5f);
        float v=(float(y)+.5f-size*.5f)/(size*.5f);
        float distance=std::sqrt(u*u+v*v);
        float coverage=std::clamp((1.f-distance)/.67f,0.f,1.f);
        std::size_t at=(y*size+x)*4;
        shadow[at+0]=17;shadow[at+1]=11;shadow[at+2]=5;
        shadow[at+3]=static_cast<std::uint8_t>(std::round(90.f*coverage*coverage));
    }
    glGenTextures(1,&shadowTexture);
    glBindTexture(GL_TEXTURE_2D,shadowTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,size,size,0,GL_RGBA,GL_UNSIGNED_BYTE,shadow.data());
    const GLenum textureStatus=glGetError();
    initialized=(textureStatus==GL_NO_ERROR && glIsProgram(program) &&
                 glIsTexture(shadowTexture));
    if(!initialized){
        error="PNG barlang: GPU inicializálási hiba (GL="
             +std::to_string(static_cast<unsigned>(textureStatus))+")";
        shutdown();
    }
    return initialized;
}
bool upload(int direction,const std::uint32_t* argb,int width,int height){
    if(!initialized||direction<0||direction>=8||!argb||
       width<128||height<128||width>2048||height>2048)return false;
    std::vector<std::uint8_t> rgba(std::size_t(width)*height*4);
    for(std::size_t i=0;i<std::size_t(width)*height;++i){
        auto c=argb[i];
        rgba[4*i+0]=std::uint8_t(c>>16);
        rgba[4*i+1]=std::uint8_t(c>>8);
        rgba[4*i+2]=std::uint8_t(c);
        rgba[4*i+3]=std::uint8_t(c>>24);
    }
    glBindTexture(GL_TEXTURE_2D,textures[direction]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    if(glGetError()!=GL_NO_ERROR){
        error="Wonderwerk 8 PNG: nem sikerült a GPU feltöltés";return false;
    }
    return true;
}
bool ready(){
    if(!initialized)return false;
    for(GLuint texture:textures)if(!texture)return false;
    return true;
}
void render(const float* viewProjection,float yaw,float pitch,float zoom,
            float caveGroundY,float clock,float rain,float alpha){
    if(!ready()||!viewProjection||!std::isfinite(yaw)||alpha<.002f)return;
    // The whole cave is ONE ground-anchored world object, not a screen-space
    // image. GLES depth testing lets nearer actors and rocks occlude it.
    // At very close zoom, the existing interactive interior is revealed.
    constexpr float footprintX=0.f, footprintZ=2.2f;
    constexpr float widthM=16.f;
    // The actual 512x512 PNG source alpha-foot row is 444. Earlier,
    // the 16 x 10.7 metre vertical card was ALSO foreshortened by the
    // isometric camera. Result: a visibly flattened, stretched cave.
    // This camera-aligned full-size 16 x 16 card respects every pixel's
    // original 1:1 proportions on all views, while its visible feet remain
    // grounded at precisely the same world coordinate.
    constexpr float sourceFootRow=444.f/512.f;
    const auto aligned=taks::CaveBillboard::aligned(
        yaw,pitch,footprintX,caveGroundY,footprintZ,widthM,sourceFootRow);
    Vertex billboard[6]{};
    for(std::size_t k=0;k<6;++k){
        const auto& p=aligned.vertices[k];
        billboard[k]={p.x,p.y,p.z,p.u,p.v};
    }

    // A crossfade between incompatible 45-degree painted silhouettes
    // creates TWO cave roofs and doubled rocks. Pick the closest existing
    // angle instead. Geometry never jumps, and each PNG is drawn once.
    double direction=double(yaw)/(double(pi)/4.);
    int index0=(static_cast<int>(std::floor(direction+.5))%8+8)%8;
    int index1=index0;
    const float fade=0.f;
    float time=std::fmod(std::fmod(clock,24.f)+24.f,24.f);
    float daylight=std::clamp(1.f-std::abs(time-13.f)/12.f,.24f,1.f);
    float tint=(.46f+.54f*daylight)*(1.f-.25f*std::clamp(rain,0.f,1.f));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(program);
    glUniformMatrix4fv(mvpUniform,1,GL_FALSE,viewProjection);
    glActiveTexture(GL_TEXTURE0);
    GLint sampler=glGetUniformLocation(program,"u_image");
    glUniform1i(sampler,0);
    glUniform1i(glGetUniformLocation(program,"u_image_next"),1);
    glUniform1f(blendUniform,0.f);

    // Small soft shadow makes the cave touch the SAME ground as its characters.
    if(shadowTexture){
        const float gy=caveGroundY+.045f;
        const float spanX=7.8f,spanZ=4.3f;
        Vertex shadow[6]={
            {-spanX,gy,footprintZ-spanZ,0,0},
            {-spanX,gy,footprintZ+spanZ,0,1},
            { spanX,gy,footprintZ+spanZ,1,1},
            {-spanX,gy,footprintZ-spanZ,0,0},
            { spanX,gy,footprintZ+spanZ,1,1},
            { spanX,gy,footprintZ-spanZ,1,0}
        };
        drawTexture(shadowTexture,shadow,alpha,.65f);
    }
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,textures[index1]);
    glActiveTexture(GL_TEXTURE0);
    glUniform1f(blendUniform,fade);
    drawTexture(textures[index0],billboard,alpha,tint);
    glUniform1f(blendUniform,0.f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,0);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,0);
    glUseProgram(0);
}
}
