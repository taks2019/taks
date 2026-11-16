#include <jni.h>
#include <GLES3/gl3.h>
#include "CauseWorld.h"
#include <string>
#include <mutex>

namespace {
    taks::World world;
    std::mutex lock;
    jstring toJava(JNIEnv* env, const std::string& s){return env->NewStringUTF(s.c_str());}
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeDo(JNIEnv* env, jobject, jstring command){
    const char* txt=env->GetStringUTFChars(command,nullptr);
    const std::string act=txt?txt:"";
    if(txt) env->ReleaseStringUTFChars(command,txt);
    std::lock_guard<std::mutex> guard(lock);
    return toJava(env, world.act(act));
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeAdvance(JNIEnv*,jobject){
    std::lock_guard<std::mutex> guard(lock);
    world.step(1.0);
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeState(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(lock);
    return toJava(env,world.summary());
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeSave(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(lock);
    return toJava(env,world.save());
}
extern "C" JNIEXPORT jboolean JNICALL Java_hu_taks_world_MainActivity_nativeLoad(JNIEnv* env,jobject,jstring payload){
    const char* txt=env->GetStringUTFChars(payload,nullptr);
    std::string raw=txt?txt:"";
    if(txt) env->ReleaseStringUTFChars(payload,txt);
    std::lock_guard<std::mutex> guard(lock);
    return world.load(raw)?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeReset(JNIEnv*,jobject){
    std::lock_guard<std::mutex> guard(lock);
    world=taks::World();
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeOnSurfaceCreated(JNIEnv*,jobject){
    glDisable(GL_DITHER);
    glEnable(GL_DEPTH_TEST);
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeRender(JNIEnv*,jobject){
    // A GPU-felület még nem 3D Wonderwerk jelenet.
    glClearColor(0.038f,0.071f,0.073f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}
