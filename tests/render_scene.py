#!/usr/bin/env python3
"""TAKS WORLD native OpenGL ES 3 shader smoke-test, using Mesa/EGL without a window.
This checks the real APK shader source on a software GLES renderer. No mock images.
"""
import ctypes as C
import os
import pathlib
import struct
import zlib

os.environ.setdefault("EGL_PLATFORM", "surfaceless")
W, H = 480, 270
egl = C.CDLL("libEGL.so.1")
gl = C.CDLL("libGL.so.1")
NONE=0x3038; SURFACE=0x3033; PBUFFER=0x0001
RED=0x3024;GREEN=0x3023;BLUE=0x3022;RENDERABLE=0x3040;ES3=0x0040
WIDTH=0x3057;HEIGHT=0x3056;CONTEXTVER=0x3098
def signature(lib, name, args, result):
    fn=getattr(lib,name);fn.argtypes=args;fn.restype=result;return fn
ptr=C.c_void_p; ci=C.c_int; cu=C.c_uint; cf=C.c_float
display=signature(egl,"eglGetDisplay",[ptr],ptr)(None)
v1=ci();v2=ci()
assert signature(egl,"eglInitialize",[ptr,C.POINTER(ci),C.POINTER(ci)],ci)(display,C.byref(v1),C.byref(v2))
choose=signature(egl,"eglChooseConfig",[ptr,C.POINTER(ci),C.POINTER(ptr),ci,C.POINTER(ci)],ci)
attrs=(ci*11)(SURFACE,PBUFFER,RED,8,GREEN,8,BLUE,8,RENDERABLE,ES3,NONE)
config=ptr();num=ci()
assert choose(display,attrs,C.byref(config),1,C.byref(num)) and num.value
surface=signature(egl,"eglCreatePbufferSurface",[ptr,ptr,C.POINTER(ci)],ptr)(display,config,(ci*5)(WIDTH,W,HEIGHT,H,NONE))
context=signature(egl,"eglCreateContext",[ptr,ptr,ptr,C.POINTER(ci)],ptr)(display,config,None,(ci*3)(CONTEXTVER,3,NONE))
assert context and surface
assert signature(egl,"eglMakeCurrent",[ptr,ptr,ptr,ptr],ci)(display,surface,surface,context)

for name,args,ret in [
 ("glCreateShader",[cu],cu),("glShaderSource",[cu,ci,C.POINTER(C.c_char_p),C.POINTER(ci)],None),
 ("glCompileShader",[cu],None),("glGetShaderiv",[cu,cu,C.POINTER(ci)],None),
 ("glGetShaderInfoLog",[cu,ci,C.POINTER(ci),C.c_char_p],None),
 ("glCreateProgram",[],cu),("glAttachShader",[cu,cu],None),("glLinkProgram",[cu],None),
 ("glGetProgramiv",[cu,cu,C.POINTER(ci)],None),("glGetProgramInfoLog",[cu,ci,C.POINTER(ci),C.c_char_p],None),
 ("glUseProgram",[cu],None),("glGetUniformLocation",[cu,C.c_char_p],ci),
 ("glUniform1f",[ci,cf],None),("glUniform2f",[ci,cf,cf],None),
 ("glUniform4fv",[ci,ci,C.POINTER(cf)],None),
 ("glGenVertexArrays",[ci,C.POINTER(cu)],None),("glBindVertexArray",[cu],None),
 ("glDrawArrays",[cu,ci,ci],None),("glViewport",[ci,ci,ci,ci],None),
 ("glReadPixels",[ci,ci,ci,ci,cu,cu,ptr],None),("glGetError",[],cu)
]:
    signature(gl,name,args,ret)

def shader(kind,content):
    target=gl.glCreateShader(kind);p=(C.c_char_p*1)(content)
    gl.glShaderSource(target,1,p,None);gl.glCompileShader(target)
    status=ci();gl.glGetShaderiv(target,0x8B81,C.byref(status))
    if not status.value:
        log=C.create_string_buffer(100000)
        gl.glGetShaderInfoLog(target,100000,None,log)
        raise RuntimeError("Shader compilation failed: "+log.value.decode("utf-8","replace"))
    return target

vertex=shader(0x8B31,b"#version 300 es\nprecision highp float;\nvoid main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));gl_Position=vec4(p*2.-1.,0.,1.);}\n")
source=pathlib.Path("app/src/main/assets/scene.frag").read_bytes()
fragment=shader(0x8B30,source)
program=gl.glCreateProgram();gl.glAttachShader(program,vertex);gl.glAttachShader(program,fragment)
gl.glLinkProgram(program)
status=ci();gl.glGetProgramiv(program,0x8B82,C.byref(status))
if not status.value:
    log=C.create_string_buffer(100000);gl.glGetProgramInfoLog(program,100000,None,log)
    raise RuntimeError("Link error: "+log.value.decode("utf-8","replace"))
gl.glUseProgram(program);vao=cu()
gl.glGenVertexArrays(1,C.byref(vao));gl.glBindVertexArray(vao.value)
gl.glViewport(0,0,W,H)
nodes=[
 (-2.0,-5.5,0,3),(3.0,-4.0,0,3),(-4.0,-1.2,0,4),(5.0,.5,0,4),
 (1.0,-5.4,1,3),(3.0,-.8,1,3),(-2.0,1.5,1,2),
 (-1.0,-3.9,2,2),(4.0,1.1,2,3),
 (-2.8,-4.0,3,6),(4.1,-3.3,3,6),(-4.,0.,3,6)
]
values=[f for node in nodes for f in node]
array=(cf*len(values))(*values)
pLoc=gl.glGetUniformLocation(program,b"u_nodes[0]")
assert pLoc>=0, "Nodes uniform was optimized out: spatial resources not visible."
gl.glUniform4fv(pLoc,len(nodes),array)

def scalar(name,value):
    idx=gl.glGetUniformLocation(program,name.encode())
    if idx>=0:gl.glUniform1f(idx,value)
def pair(name,x,y):
    idx=gl.glGetUniformLocation(program,name.encode())
    if idx>=0:gl.glUniform2f(idx,x,y)

def save_png(path,pixels):
    stride=W*4
    scanlines=b"".join(b"\x00"+pixels[y*stride:(y+1)*stride] for y in range(H-1,-1,-1))
    def chunk(kind,data):return struct.pack(">I",len(data))+kind+data+struct.pack(">I",zlib.crc32(kind+data)&0xffffffff)
    png=b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",W,H,8,6,0,0,0))+chunk(b"IDAT",zlib.compress(scanlines,6))+chunk(b"IEND",b"")
    pathlib.Path(path).write_bytes(png)

def render(name,px,pz,stage):
    pair("u_resolution",W,H);pair("u_player",px,pz)
    for n,v in {"u_orbit":0.0,"u_zoom":14.,"u_clock":9.2,"u_rain":0.0,
                "u_moisture":.15,"u_hearth":0.,"u_fire":0.,
                "u_quality":1.,"u_shelterStage":stage,"u_walkPhase":3.}.items():scalar(n,v)
    gl.glDrawArrays(0x0004,0,3)
    error=gl.glGetError()
    if error:raise RuntimeError("OpenGL draw failure: "+hex(error))
    raw=(C.c_ubyte*(W*H*4))()
    gl.glReadPixels(0,0,W,H,0x1908,0x1401,C.cast(raw,ptr))
    output=bytes(raw)
    assert len(set(output[::113]))>5, "Rendered frame appears blank/flat"
    outputDir=pathlib.Path("build-previews");outputDir.mkdir(exist_ok=True)
    save_png(outputDir/name,output)
    print("Saved",outputDir/name,"min",min(output),"max",max(output))
    return output

initial=render("01-kezdo-terulet.png",0.,-5.2,0.)
moved=render("02-menedek-fazis3.png",3.1,-1.55,3.)
assert initial!=moved, "Player movement and shelter construction did not change the image"
print("TAKS GLES3 SPATIAL RENDER TEST PASSED")
