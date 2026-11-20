#include <jni.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <algorithm>
#include "CauseWorld.h"
#include "AfricanClimate.h"
#include "FastScene.h"
#include "RealModels.h"
#include <vector>
#include <string>
#include <mutex>
#include <cmath>
#include <chrono>
#include <array>

namespace {
taks::World world;
std::mutex worldLock;
GLuint program=0, vertexArray=0;
float orbit=-0.785398f, pitch=0.615480f, zoom=19.0f;
float playerHeading=0.0f;
std::chrono::steady_clock::time_point playerMovementDeadline{};
std::chrono::steady_clock::time_point playerRunDeadline{};
std::array<float,8> npcPrevious{};
std::array<float,8> npcTarget{};
std::chrono::steady_clock::time_point npcSegmentStart{};
bool npcInterpolationReady=false;
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
    const std::string result=world.act(act);
    npcInterpolationReady=false;
    return toJava(env,result);
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeMove(JNIEnv* env,jobject,jfloat dx,jfloat dz){
    std::lock_guard<std::mutex> guard(worldLock);
    const double beforeX=world.playerX,beforeZ=world.playerZ;
    const std::string result=world.move(static_cast<double>(dx),static_cast<double>(dz));
    const double deltaX=world.playerX-beforeX,deltaZ=world.playerZ-beforeZ;
    if(std::hypot(deltaX,deltaZ)>.00001){
        playerHeading=static_cast<float>(std::atan2(deltaX,deltaZ));
        const auto now=std::chrono::steady_clock::now();
        playerMovementDeadline=now+std::chrono::milliseconds(130);
        if(std::hypot(dx,dz)>.075)playerRunDeadline=now+std::chrono::milliseconds(130);
    }
    return toJava(env,result);
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
    if(!npcInterpolationReady){
        for(int i=0;i<taks::World::npcCount;++i){
            npcPrevious[i*2]=npcTarget[i*2]=static_cast<float>(world.npcs[i].x);
            npcPrevious[i*2+1]=npcTarget[i*2+1]=static_cast<float>(world.npcs[i].z);
        }
        npcInterpolationReady=true;
    }else npcPrevious=npcTarget;
    world.step(0.04);
    for(int i=0;i<taks::World::npcCount;++i){
        npcTarget[i*2]=static_cast<float>(world.npcs[i].x);
        npcTarget[i*2+1]=static_cast<float>(world.npcs[i].z);
    }
    npcSegmentStart=std::chrono::steady_clock::now();
}
extern "C" JNIEXPORT jboolean JNICALL Java_hu_taks_world_MainActivity_nativeLogoUnlocked(JNIEnv*,jobject){
    std::lock_guard<std::mutex> guard(worldLock);
    return world.logoUnlocked()?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT jfloatArray JNICALL Java_hu_taks_world_MainActivity_nativePlayerCoordinates(JNIEnv* env,jobject){
    // Consistent world-space anchor for Android's separate eight-view PNG artwork.
    // No world state mutation and no approximation from the HUD text.
    std::lock_guard<std::mutex> guard(worldLock);
    jfloat coordinates[2]={static_cast<jfloat>(world.playerX),
                           static_cast<jfloat>(world.playerZ)};
    jfloatArray result=env->NewFloatArray(2);
    if(result)env->SetFloatArrayRegion(result,0,2,coordinates);
    return result;
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeNearby(JNIEnv* env,jobject){
    std::lock_guard<std::mutex> guard(worldLock);
    return toJava(env,world.nearby());
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeAdvance(JNIEnv*,jobject){
    std::lock_guard<std::mutex> guard(worldLock);world.step(1.0);npcInterpolationReady=false;
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
    std::lock_guard<std::mutex> guard(worldLock);
    const bool loaded=world.load(raw);
    if(loaded)npcInterpolationReady=false;
    return loaded?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeReset(JNIEnv*,jobject){
    std::lock_guard<std::mutex> guard(worldLock);world=taks::World();
    playerMovementDeadline=playerRunDeadline=std::chrono::steady_clock::time_point{};
    npcInterpolationReady=false;
}
extern "C" JNIEXPORT void JNICALL Java_hu_taks_world_MainActivity_nativeSetView(JNIEnv*,jobject,jfloat yaw,jfloat cameraPitch,jfloat scale){
    std::lock_guard<std::mutex> guard(worldLock);
    if(std::isfinite(yaw))orbit=yaw;
    if(std::isfinite(cameraPitch))pitch=std::fmax(.349066f,std::fmin(1.396263f,cameraPitch));
    if(std::isfinite(scale))zoom=std::fmax(6.f,std::fmin(26.f,scale));
}
// Upload a genuine triangulated CC0 photogrammetry model and its decoded photographic
// albedo texture from Android assets. All data remains offline in the APK.
extern "C" JNIEXPORT jboolean JNICALL Java_hu_taks_world_MainActivity_nativeUploadTerrainTexture(
    JNIEnv* env,jobject,jint id,jintArray pixels,jint width,jint height){
    if(!pixels||width<1||height<1||width>2048||height>2048)return JNI_FALSE;
    if(env->GetArrayLength(pixels)!=width*height)return JNI_FALSE;
    std::vector<std::uint32_t> data(static_cast<std::size_t>(width)*height);
    env->GetIntArrayRegion(pixels,0,width*height,reinterpret_cast<jint*>(data.data()));
    if(env->ExceptionCheck())return JNI_FALSE;
    return taks::FastScene::uploadTerrainTexture(id,data.data(),width,height)?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT jboolean JNICALL Java_hu_taks_world_MainActivity_nativeUploadRealModel(
    JNIEnv* env,jobject,jint id,jbyteArray geometry,jintArray pixels,jint width,jint height){
    if(!geometry||!pixels||width<1||height<1)return JNI_FALSE;
    const jsize length=env->GetArrayLength(geometry);
    if(length<8||length>30000000||env->GetArrayLength(pixels)!=width*height)return JNI_FALSE;
    std::vector<std::uint8_t> mesh(static_cast<std::size_t>(length));
    env->GetByteArrayRegion(geometry,0,length,reinterpret_cast<jbyte*>(mesh.data()));
    if(env->ExceptionCheck())return JNI_FALSE;
    std::vector<std::uint32_t> texture(static_cast<std::size_t>(width)*height);
    env->GetIntArrayRegion(pixels,0,width*height,reinterpret_cast<jint*>(texture.data()));
    if(env->ExceptionCheck())return JNI_FALSE;
    return taks::RealModels::upload(id,mesh.data(),mesh.size(),texture.data(),width,height)?JNI_TRUE:JNI_FALSE;
}
// Exactly the saved simulation's causal facts feed the prehistoric menu.
// Unknown fern, vine and rope species/skills remain locked; leaf resources
// do not magically become a discovered plant species.
extern "C" JNIEXPORT jfloatArray JNICALL Java_hu_taks_world_MainActivity_nativeMenuCues(
    JNIEnv* env,jobject){
    jfloat snapshot[11]={};
    {
        std::lock_guard<std::mutex> guard(worldLock);
        snapshot[0]=static_cast<jfloat>(world.hours);
        snapshot[1]=static_cast<jfloat>(world.rainIntensity);
        snapshot[2]=static_cast<jfloat>(world.groundMoisture);
        snapshot[3]=world.fireLit?1.f:0.f;
        // Real ecological biomass and verified plant knowledge, not fake unlocks.
        snapshot[4]=world.ecology.grassCover()>.15f?1.f:0.f;
        snapshot[5]=world.stoneDebris>0?1.f:0.f;
        snapshot[6]=(world.preparedPoles>0||world.shelterStage>0)?1.f:0.f;
        snapshot[7]=world.shelterStage>0?1.f:0.f;
        // The current World does not yet have a real fern, vine or rope system.
        snapshot[8]=world.ecology.verifiedFerns?1.f:0.f;
        snapshot[9]=world.ecology.verifiedVines?1.f:0.f;
        snapshot[10]=0.f; // Rope craft not yet implemented in this milestone.
    }
    jfloatArray arr=env->NewFloatArray(11);
    if(arr)env->SetFloatArrayRegion(arr,0,11,snapshot);
    return arr;
}
extern "C" JNIEXPORT jstring JNICALL Java_hu_taks_world_MainActivity_nativeRendererError(JNIEnv* env,jobject){
    const auto& e=taks::FastScene::lastError();
    if(!e.empty())return toJava(env,e);
    return toJava(env,taks::RealModels::errorMessage());
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
        const auto climate=taks::africanWeatherAt(world.hours);
        f.wetSeason=static_cast<float>(climate.wetSeason);
        f.windStrength=static_cast<float>(climate.windStrength);
        f.windX=static_cast<float>(climate.windX);
        f.windZ=static_cast<float>(climate.windZ);
        f.temperatureC=static_cast<float>(climate.temperatureC);
        f.mud=static_cast<float>(world.ecology.mud);
        f.hearth=world.hearthBuilt?1.f:0.f;
        f.fire=world.fireLit?1.f:0.f;
        f.playerX=static_cast<float>(world.playerX);
        f.playerZ=static_cast<float>(world.playerZ);
        f.walkPhase=static_cast<float>(world.walkPhase);
        f.playerHeading=playerHeading;
        const auto frameNow=std::chrono::steady_clock::now();
        f.animationTime=static_cast<float>(std::fmod(std::chrono::duration<double>(frameNow.time_since_epoch()).count(),4096.0));
        f.playerMoving=frameNow<playerMovementDeadline;
        f.playerRunning=f.playerMoving&&frameNow<playerRunDeadline;
        for(int i=0;i<taks::World::resourceCount;++i){
            f.nodes[i*4+0]=static_cast<float>(world.nodes[i].x);
            f.nodes[i*4+1]=static_cast<float>(world.nodes[i].z);
            f.nodes[i*4+2]=static_cast<float>(world.nodes[i].type);
            f.nodes[i*4+3]=static_cast<float>(world.nodes[i].amount);
        }
        for(int i=0;i<taks::LivingWorld::grassCount;++i){
            const auto& g=world.ecology.grass[i];
            f.grass[i*5+0]=static_cast<float>(g.x);
            f.grass[i*5+1]=static_cast<float>(g.z);
            f.grass[i*5+2]=static_cast<float>(g.biomass);
            f.grass[i*5+3]=static_cast<float>(g.health);
            f.grass[i*5+4]=static_cast<float>(g.trampling);
        }
        for(int i=0;i<taks::LivingWorld::treeCount;++i){
            const auto& t=world.ecology.trees[i];
            f.trees[i*4+0]=static_cast<float>(t.x);
            f.trees[i*4+1]=static_cast<float>(t.z);
            f.trees[i*4+2]=static_cast<float>(t.heightM);
            f.trees[i*4+3]=t.alive?1.f:0.f;
        }
        for(int i=0;i<taks::LivingWorld::footprintCount;++i){
            const auto& mark=world.ecology.footprints[i];
            f.footprints[i*4+0]=static_cast<float>(mark.x);
            f.footprints[i*4+1]=static_cast<float>(mark.z);
            f.footprints[i*4+2]=static_cast<float>(mark.heading);
            f.footprints[i*4+3]=static_cast<float>(mark.intensity);
        }
        for(int i=0;i<taks::LivingWorld::waterBodyCount;++i){
            const auto& water=world.ecology.waters[i];
            f.waters[i*4+0]=static_cast<float>(water.x);
            f.waters[i*4+1]=static_cast<float>(water.z);
            f.waters[i*4+2]=static_cast<float>(water.radius);
            f.waters[i*4+3]=static_cast<float>(water.level);
        }
        for(int i=0;i<taks::LivingWorld::animalCount;++i){
            const auto& animal=world.ecology.animals[i];
            f.animals[i*6+0]=static_cast<float>(animal.x);
            f.animals[i*6+1]=static_cast<float>(animal.z);
            f.animals[i*6+2]=static_cast<float>(animal.kind);
            f.animals[i*6+3]=animal.alive?1.f:0.f;
            f.animals[i*6+4]=static_cast<float>(animal.thirst);
            f.animals[i*6+5]=static_cast<float>(animal.hunger);
        }
        f.collectorStage=static_cast<float>(world.waterCollectorStage);
        f.collectorWater=static_cast<float>(world.storedRainwater/60.0);
        f.verifiedFerns=world.ecology.verifiedFerns;
        const auto now=std::chrono::steady_clock::now();
        if(!npcInterpolationReady){
            for(int i=0;i<taks::World::npcCount;++i){
                npcPrevious[i*2]=npcTarget[i*2]=static_cast<float>(world.npcs[i].x);
                npcPrevious[i*2+1]=npcTarget[i*2+1]=static_cast<float>(world.npcs[i].z);
            }
            npcSegmentStart=now;npcInterpolationReady=true;
        }
        const float npcBlend=std::clamp(
            std::chrono::duration<float>(now-npcSegmentStart).count()/1.8f,0.f,1.f);
        for(int i=0;i<taks::World::npcCount;++i){
            f.npcs[i*4+0]=npcPrevious[i*2]+(npcTarget[i*2]-npcPrevious[i*2])*npcBlend;
            f.npcs[i*4+1]=npcPrevious[i*2+1]+(npcTarget[i*2+1]-npcPrevious[i*2+1])*npcBlend;
            f.npcs[i*4+2]=static_cast<float>(world.npcs[i].task);
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
