#include "../app/src/main/cpp/FastScene.h"
#include "../app/src/main/cpp/RealModels.h"
#include <png.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

namespace {
constexpr int W=640,H=360;
void check(bool ok,const std::string& reason){
    if(!ok){std::cerr<<"TAKS raster regression failed: "<<reason<<"\n";std::exit(1);}
}
std::vector<std::uint8_t> snapshot(const char* filename){
    std::vector<std::uint8_t> pixels(W*H*4);
    glFinish();glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    check(glGetError()==GL_NO_ERROR,"glReadPixels");
    std::ofstream out(std::string("build-previews/")+filename,std::ios::binary);
    check(bool(out),"could not create preview");
    out<<"P6\n"<<W<<" "<<H<<"\n255\n";
    for(int y=H-1;y>=0;--y)for(int x=0;x<W;++x){
        size_t i=4*(y*W+x);
        out.put(char(pixels[i]));out.put(char(pixels[i+1]));out.put(char(pixels[i+2]));
    }
    out.close();
    return pixels;
}
std::uint64_t pixelSum(const std::vector<std::uint8_t>& p) {
    std::uint64_t checksum=0;
    for(size_t i=0;i<p.size();i+=37)checksum=checksum*1315423911u+p[i];
    return checksum;
}
}

void loadRealModel(int id,const std::string& name){
    const std::string base="app/src/main/assets/models/"+name;
    std::ifstream stream(base+".tkm",std::ios::binary);
    check(bool(stream),"missing real 3D model: "+name);
    std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(stream)),std::istreambuf_iterator<char>());
    check(data.size()>50000,"the model must contain genuine detailed 3D triangles, not primitives: "+name);
    png_image picture{};
    picture.version=PNG_IMAGE_VERSION;
    check(png_image_begin_read_from_file(&picture,(base+".png").c_str())!=0,
          "missing photogrammetry texture for "+name);
    picture.format=PNG_FORMAT_RGBA;
    std::vector<std::uint8_t> rgba(PNG_IMAGE_SIZE(picture));
    check(png_image_finish_read(&picture,nullptr,rgba.data(),0,nullptr)!=0,
          "could not decode scanned texture for "+name);
    std::vector<std::uint32_t> argb(std::size_t(picture.width)*picture.height);
    for(std::size_t i=0;i<argb.size();++i){
        argb[i]=std::uint32_t(rgba[4*i+3])<<24|
                std::uint32_t(rgba[4*i])<<16|
                std::uint32_t(rgba[4*i+1])<<8|rgba[4*i+2];
    }
    check(taks::RealModels::upload(id,data.data(),data.size(),argb.data(),
                                   int(picture.width),int(picture.height)),
          std::string("real model import failed: ")+taks::RealModels::errorMessage());
    std::cout<<"Loaded actual CC0 textured scanned mesh: "<<name
             <<" (texture "<<picture.width<<"x"<<picture.height<<")\n";
    png_image_free(&picture);
}
int main(){
    setenv("EGL_PLATFORM","surfaceless",1);
    const EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    check(display!=EGL_NO_DISPLAY,"eglGetDisplay");
    EGLint major=0,minor=0;
    check(eglInitialize(display,&major,&minor)==EGL_TRUE,"eglInitialize");
    check(eglBindAPI(EGL_OPENGL_ES_API)==EGL_TRUE,"eglBindAPI");
    const EGLint cfg[]={
        EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,0x0040, // EGL_OPENGL_ES3_BIT
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,24,
        EGL_NONE
    };
    EGLConfig config=nullptr;EGLint found=0;
    check(eglChooseConfig(display,cfg,&config,1,&found)==EGL_TRUE&&found==1,"EGL config ES3");
    const EGLint surfaceAttrs[]={EGL_WIDTH,W,EGL_HEIGHT,H,EGL_NONE};
    const EGLSurface surface=eglCreatePbufferSurface(display,config,surfaceAttrs);
    check(surface!=EGL_NO_SURFACE,"eglCreatePbufferSurface");
    const EGLint ctxAttrs[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    const EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,ctxAttrs);
    check(context!=EGL_NO_CONTEXT,"eglCreateContext");
    check(eglMakeCurrent(display,surface,surface,context)==EGL_TRUE,"eglMakeCurrent");
    check(taks::FastScene::init(),taks::FastScene::lastError());
    loadRealModel(0,"rock_07");
    loadRealModel(1,"namaqualand_boulders_01");
    check(taks::RealModels::complete(),"the two authentic scanned 3D models must both be on GPU");

    taks::FastFrame frame;
    frame.playerX=0.f;frame.playerZ=-5.2f;frame.yaw=.785398f;frame.pitch=.615480f;
    frame.zoom=14.f;frame.clock=11.0f;frame.rain=.15f;
    const float nodes[48]={
        -2.f,-5.5f,0,3,3.f,-4.f,0,3,-4.f,-1.2f,0,4,5.f,.5f,0,4,
        1.f,-5.4f,1,3,3.f,-.8f,1,3,-2.f,1.5f,1,2,
        -1.f,-3.9f,2,2,4.f,1.1f,2,3,
        -2.8f,-4.f,3,6,4.1f,-3.3f,3,6,-4.f,0.f,3,6
    };
    std::copy(nodes,nodes+48,frame.nodes);
    const float npc[16]={
        -3.6f,-2.1f,0.f,0.f,2.7f,-1.1f,1.f,0.f,
        4.4f,-2.0f,2.f,0.f,-1.8f,-2.4f,3.f,0.f
    };
    std::copy(npc,npc+16,frame.npcs);
    for(int i=0;i<3;++i){frame.sites[4*i+2]=-1.f;frame.sites[4*i+3]=1.f;}
    frame.sites[0]=3.1f;frame.sites[1]=-1.55f;frame.sites[2]=3.f;
    frame.sites[4]=-3.1f;frame.sites[5]=-1.3f;frame.sites[6]=1.f;

    auto render=[&](const char* name){
        const auto start=std::chrono::steady_clock::now();
        taks::FastScene::render(W,H,frame);
        auto output=snapshot(name);
        double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        std::cout<<name<<"  CPU Mesa frame: "<<ms<<"ms\n";
        std::uint64_t hash=pixelSum(output);
        size_t nonBlank=0;
        for(size_t i=0;i<output.size();i+=4){
            if(output[i]!=143 || output[i+1]!=161 || output[i+2]!=171)++nonBlank;
        }
        check(nonBlank>W*H/20,"frame appears blank");
        return hash;
    };
    const auto iso=render("01-izometrikus-35fok.ppm");
    frame.yaw=2.3f;
    const auto rotated=render("02-forgatott-kamera.ppm");
    check(rotated!=iso,"yaw did not change the actual image");
    frame.pitch=1.25f;frame.yaw=.785398f;
    const auto overhead=render("03-felulnezet.ppm");
    check(overhead!=iso && overhead!=rotated,"pitch did not change the actual image");
    frame.pitch=.45f;
    const auto low=render("04-alacsony-3d-kamera.ppm");
    check(low!=overhead,"low 3D camera angle did not change the actual image");
    frame.pitch=.615480f;
    frame.yaw=.785398f;
    const auto scans=render("05-valodi-cc0-fotoalapu-kovek.ppm");
    check(scans!=0,"photogrammetry image rendered no pixels");
    for(int i=0;i<12;++i)if(int(frame.nodes[4*i+2]+.5f)==1)frame.nodes[4*i+3]=0;
    const auto mined=render("06-kitermelt-kovek.ppm");
    check(mined!=scans,"collecting rocks did not change photographic 3D mesh rendering");
    taks::FastScene::shutdown();
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(display,context);eglDestroySurface(display,surface);
    eglTerminate(display);
    std::cout<<"TAKS FAST TRIANGLE GPU + ORTHOGRAPHIC CAMERA TESTS PASSED\n";
}
