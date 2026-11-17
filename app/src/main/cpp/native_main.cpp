#include <jni.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include "CauseWorld.h"
#include <string>
#include <mutex>
#include <cmath>

namespace {
taks::World world;
std::mutex worldLock;
GLuint program=0, vertexArray=0;
float orbit=0.0f, zoom=14.0f;
std::string rendererError;
jstring toJava(JNIEnv* env,const std::string& s){return env->NewStringUTF(s.c_str());}
GLuint compile(GLenum type,const char* source){
    GLuint s=glCreateShader(type);
    glShaderSource(s,1,&source,nullptr);
    glCompileShader(s);
    GLint ok=GL_FALSE;
    glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){
        GLint length=0;glGetShaderiv(s,GL_INFO_LOG_LENGTH,&length);
        std::string log((size_t)(length>0?length:1),'\0');
        glGetShaderInfoLog(s,length,nullptr,&log[0]);
        rendererError += log;
        __android_log_print(ANDROID_LOG_ERROR,"TAKS_RENDER","GLSL: %s",log.c_str());
        glDeleteShader(s);return 0;
    }
    return s;
}
void u1(const char* name,float v){
    const GLint loc=glGetUniformLocation(program,name);
    if(loc>=0)glUniform1f(loc,v);
}
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeDo(JNIEnv* env,jobject,jstring command){
    if(!command)return toJava(env,"Hiányzó cselekvés.");
    const char* txt=env->GetStringUTFChars(command,nullptr);
    const std::string act=txt?txt:"";
    if(txt)env->ReleaseStringUTFChars(command,txt);
    std::lock_guard<std::mutex> guard(worldLock);
    return toJava(env,world.act(act));
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeMove(JNIEnv* env,jobject,jfloat dx,jfloat dz){
    std::lock_guard<std::mutex> guard(worldLock);
    return toJava(env,world.move(static_cast<double>(dx),static_cast<double>(dz)));
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeGoal(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(worldLock);
    return toJava(env,world.nextObjective());
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeNpcStatus(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(worldLock);
    return toJava(env,world.npcStatus());
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeCampStatus(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(worldLock);
    return toJava(env,world.campStatus());
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeTick(JNIEnv*,jobject){
    std::lock_guard<std::mutex> guard(worldLock);
    world.step(0.04);
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeNearby(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(worldLock);
    return toJava(env,world.nearby());
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeAdvance(JNIEnv*,jobject){
    std::lock_guard<std::mutex> guard(worldLock);world.step(1.0);
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeState(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(worldLock);return toJava(env,world.summary());
}
extern "C" JNIEXPORT jint JNICALL Java_hu_taks_world_MainActivity_nativeSiteStage(JNIEnv*,jobject){
    std::lock_guard<std::mutex> guard(worldLock);
    return world.nearbySiteStage();
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeBuildState(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(worldLock);return toJava(env,world.shelterStatus());
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeSave(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(worldLock);return toJava(env,world.save());
}
extern "C" JNIEXPORT jboolean JNICALL Java_hu_taks_world_MainActivity_nativeLoad(JNIEnv* env,jobject,jstring payload){
    if(!payload)return JNI_FALSE;
    const char* txt=env->GetStringUTFChars(payload,nullptr);
    std::string raw=txt?txt:"";
    if(txt)env->ReleaseStringUTFChars(payload,txt);
    std::lock_guard<std::mutex> guard(worldLock);return world.load(raw)?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeReset(JNIEnv*,jobject){
    std::lock_guard<std::mutex> guard(worldLock);world=taks::World();
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeSetView(JNIEnv*,jobject,jfloat angle,jfloat distance){
    std::lock_guard<std::mutex> guard(worldLock);
    if(std::isfinite(angle))orbit=angle;
    if(std::isfinite(distance))zoom=std::fmax(7.f,std::fmin(26.f,distance));
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeRendererError(JNIEnv* env,jobject){
    return toJava(env,rendererError);
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeOnSurfaceCreated(JNIEnv* env,jobject,jstring fragmentSource){
    rendererError.clear();
    if(program){glDeleteProgram(program);program=0;}
    if(vertexArray){glDeleteVertexArrays(1,&vertexArray);vertexArray=0;}
    const char* vert=R"(#version 300 es
precision highp float;
void main(){
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    gl_Position=vec4(p*2.-1.,0.,1.);
})";
    if(!fragmentSource){rendererError="A 3D shader-forrás hiányzik.";return;}
    const char* fragment=env->GetStringUTFChars(fragmentSource,nullptr);
    GLuint vs=compile(GL_VERTEX_SHADER,vert);
    GLuint fs=compile(GL_FRAGMENT_SHADER,fragment?fragment:"");
    if(fragment)env->ReleaseStringUTFChars(fragmentSource,fragment);
    if(!vs||!fs){
        if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);
        return;
    }
    program=glCreateProgram();
    glAttachShader(program,vs);glAttachShader(program,fs);
    glLinkProgram(program);
    glDeleteShader(vs);glDeleteShader(fs);
    GLint linked=GL_FALSE;glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if(!linked){
        GLint length=0;glGetProgramiv(program,GL_INFO_LOG_LENGTH,&length);
        std::string err((size_t)(length>0?length:1),'\0');
        glGetProgramInfoLog(program,length,nullptr,&err[0]);
        rendererError=err;
        __android_log_print(ANDROID_LOG_ERROR,"TAKS_RENDER","LINK: %s",err.c_str());
        glDeleteProgram(program);program=0;return;
    }
    glGenVertexArrays(1,&vertexArray);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeRender(JNIEnv*,jobject){
    if(!program){
        glClearColor(.065f,.1f,.12f,1.f);glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    float angle,dist,hours,rain,wet,hearth,fire,shelterStage;
    float px,pz,walkPhase,nodes[4*taks::World::resourceCount],npcs[4*taks::World::npcCount],sites[4*taks::World::maxBuildSites];
    {
        std::lock_guard<std::mutex> guard(worldLock);
        angle=orbit;dist=zoom;
        hours=(float)world.hours;rain=(float)world.rainIntensity;
        wet=(float)world.groundMoisture;hearth=world.hearthBuilt?1.f:0.f;
        fire=world.fireLit?1.f:0.f;
        shelterStage=static_cast<float>(world.shelterStage);
        px=static_cast<float>(world.playerX);
        pz=static_cast<float>(world.playerZ);
        walkPhase=static_cast<float>(world.walkPhase);
        for(int i=0;i<taks::World::resourceCount;++i){
            nodes[4*i]=static_cast<float>(world.nodes[i].x);
            nodes[4*i+1]=static_cast<float>(world.nodes[i].z);
            nodes[4*i+2]=static_cast<float>(world.nodes[i].type);
            nodes[4*i+3]=static_cast<float>(world.nodes[i].amount);
        }
        for(int i=0;i<taks::World::npcCount;++i){
            npcs[4*i]=static_cast<float>(world.npcs[i].x);
            npcs[4*i+1]=static_cast<float>(world.npcs[i].z);
            npcs[4*i+2]=static_cast<float>(world.npcs[i].role);
            npcs[4*i+3]=static_cast<float>(world.npcs[i].carrying);
        }
        for(int i=0;i<taks::World::maxBuildSites;++i){
            sites[4*i]=static_cast<float>(world.sites[i].x);
            sites[4*i+1]=static_cast<float>(world.sites[i].z);
            sites[4*i+2]=i<world.siteCount?static_cast<float>(world.sites[i].stage):-1.0f;
            sites[4*i+3]=static_cast<float>(world.sites[i].condition/100.0);
        }
    }
    GLint vp[4]={0,0,1,1};
    glGetIntegerv(GL_VIEWPORT,vp);
    glUseProgram(program);
    glBindVertexArray(vertexArray);
    GLint loc=glGetUniformLocation(program,"u_resolution");
    if(loc>=0)glUniform2f(loc,(float)vp[2],(float)vp[3]);
    u1("u_orbit",angle);
    u1("u_zoom",dist);
    u1("u_clock",hours);
    u1("u_rain",rain);
    u1("u_moisture",wet);
    u1("u_hearth",hearth);
    u1("u_fire",fire);
    u1("u_shelterStage",shelterStage);
    const GLint playerLoc=glGetUniformLocation(program,"u_player");
    if(playerLoc>=0)glUniform2f(playerLoc,px,pz);
    u1("u_walkPhase",walkPhase);
    const GLint nodeLoc=glGetUniformLocation(program,"u_nodes[0]");
    if(nodeLoc>=0)glUniform4fv(nodeLoc,taks::World::resourceCount,nodes);
    const GLint npcLoc=glGetUniformLocation(program,"u_npcs[0]");
    if(npcLoc>=0)glUniform4fv(npcLoc,taks::World::npcCount,npcs);
    const GLint siteLoc=glGetUniformLocation(program,"u_sites[0]");
    if(siteLoc>=0)glUniform4fv(siteLoc,taks::World::maxBuildSites,sites);
    u1("u_quality",1.f);
    glDrawArrays(GL_TRIANGLES,0,3);
    glBindVertexArray(0);
}
