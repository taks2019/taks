#include <jni.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include "CauseWorld.h"
#include "FastScene.h"
#include <string>
#include <mutex>
#include <cmath>

namespace {
taks::World world;
std::mutex worldLock;
GLuint program=0, vertexArray=0;
float orbit=0.785398f, pitch=0.615480f, zoom=14.0f;
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
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeSetView(JNIEnv*,jobject,jfloat yaw,jfloat cameraPitch,jfloat scale){
    std::lock_guard<std::mutex> guard(worldLock);
    if(std::isfinite(yaw))orbit=yaw;
    if(std::isfinite(cameraPitch))pitch=std::fmax(.349066f,std::fmin(1.396263f,cameraPitch));
    if(std::isfinite(scale))zoom=std::fmax(6.f,std::fmin(26.f,scale));
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeRendererError(JNIEnv* env,jobject){
    return toJava(env,taks::FastScene::lastError());
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeOnSurfaceCreated(JNIEnv*,jobject,jstring){
    // Kizárólag a natív GLES3 háló-/háromszögmegjelenítő. Nincs képpontonkénti sugármenetelés.
    if(!taks::FastScene::init()){
        __android_log_print(ANDROID_LOG_ERROR,"TAKS_RENDER","Fast GLES3 init: %s",
                            taks::FastScene::lastError().c_str());
    }
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeRender(JNIEnv*,jobject){
    taks::FastFrame f;
    {
        std::lock_guard<std::mutex> guard(worldLock);
        f.yaw=orbit;f.pitch=pitch;f.zoom=zoom;
        f.clock=static_cast<float>(world.hours);
        f.rain=static_cast<float>(world.rainIntensity);
        f.moisture=static_cast<float>(world.groundMoisture);
        f.hearth=world.hearthBuilt?1.f:0.f;
        f.fire=world.fireLit?1.f:0.f;
        f.playerX=static_cast<float>(world.playerX);
        f.playerZ=static_cast<float>(world.playerZ);
        f.walkPhase=static_cast<float>(world.walkPhase);
        for(int i=0;i<taks::World::resourceCount;++i){
            f.nodes[i*4+0]=static_cast<float>(world.nodes[i].x);
            f.nodes[i*4+1]=static_cast<float>(world.nodes[i].z);
            f.nodes[i*4+2]=static_cast<float>(world.nodes[i].type);
            f.nodes[i*4+3]=static_cast<float>(world.nodes[i].amount);
        }
        for(int i=0;i<taks::World::npcCount;++i){
            f.npcs[i*4+0]=static_cast<float>(world.npcs[i].x);
            f.npcs[i*4+1]=static_cast<float>(world.npcs[i].z);
            f.npcs[i*4+2]=static_cast<float>(world.npcs[i].role);
            f.npcs[i*4+3]=static_cast<float>(world.npcs[i].carrying);
        }
        for(int i=0;i<taks::World::maxBuildSites;++i){
            f.sites[i*4+0]=static_cast<float>(world.sites[i].x);
            f.sites[i*4+1]=static_cast<float>(world.sites[i].z);
            f.sites[i*4+2]=i<world.siteCount?static_cast<float>(world.sites[i].stage):-1.f;
            f.sites[i*4+3]=static_cast<float>(world.sites[i].condition/100.0);
        }
    }
    GLint vp[4]={0,0,1,1};
    glGetIntegerv(GL_VIEWPORT,vp);
    taks::FastScene::render(vp[2],vp[3],f);
}
