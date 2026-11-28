package hu.taks.studio;

import android.content.ContentResolver;
import android.graphics.*;
import android.net.Uri;
import ai.onnxruntime.*;
import java.io.*;
import java.nio.FloatBuffer;
import java.util.*;

/** Experimental TripoSR ONNX pipeline. Uses local files only, never a network fallback. */
final class OfflineGenerator {
    interface Progress {void update(String s,int p);}
    private final File encoder,decoder;
    private final Progress progress;
    private final OrtEnvironment env=OrtEnvironment.getEnvironment();
    OfflineGenerator(File a,File b,Progress p){encoder=a;decoder=b;progress=p;}
    private static void interrupt()throws IOException{if(Thread.currentThread().isInterrupted())throw new InterruptedIOException("Megszakítva");}
    private static TensorInfo info(NodeInfo n)throws IOException{
        if(n==null||!(n.getInfo() instanceof TensorInfo))throw new IOException("Nem támogatott ONNX-adattípus.");
        TensorInfo t=(TensorInfo)n.getInfo();if(t.type!=OnnxJavaType.FLOAT)throw new IOException("Csak float32 ONNX támogatott: "+t);return t;
    }
    private OrtSession.SessionOptions options()throws OrtException{
        OrtSession.SessionOptions o=new OrtSession.SessionOptions();
        o.setIntraOpNumThreads(2);o.setInterOpNumThreads(1);o.setMemoryPatternOptimization(false);
        o.setOptimizationLevel(OrtSession.SessionOptions.OptLevel.BASIC_OPT);
        return o;
    }
    private static float[] floats(OnnxValue value)throws IOException{
        if(!(value instanceof OnnxTensor))throw new IOException("Nem tenzor a modellkimenet.");
        FloatBuffer b=((OnnxTensor)value).getFloatBuffer();if(b==null)throw new IOException("Nem float kimenet.");
        float[] a=new float[b.remaining()];b.get(a);return a;
    }
    private static boolean dim(long v,int expected){return v<0||v==expected;}
    private static float[] image(ContentResolver resolver,Uri uri)throws IOException{
        BitmapFactory.Options o=new BitmapFactory.Options();o.inJustDecodeBounds=true;
        try(InputStream in=resolver.openInputStream(uri)){BitmapFactory.decodeStream(in,null,o);}
        if(o.outWidth<=0||o.outHeight<=0)throw new IOException("Nem olvasható kép.");
        o.inSampleSize=1;while(Math.max(o.outWidth,o.outHeight)/o.inSampleSize>2048)o.inSampleSize*=2;
        o.inJustDecodeBounds=false;o.inPreferredConfig=Bitmap.Config.ARGB_8888;
        Bitmap src;try(InputStream in=resolver.openInputStream(uri)){src=BitmapFactory.decodeStream(in,null,o);}
        if(src==null)throw new IOException("Nem nyitható meg a kép.");
        Bitmap dst=null;
        try{
            int w=src.getWidth(),h=src.getHeight(),x0=w,y0=h,x1=-1,y1=-1;
            int[] row=new int[w];boolean transparent=false;
            for(int y=0;y<h;y++){src.getPixels(row,0,w,0,y,w,1);for(int x=0;x<w;x++){
                int alpha=row[x]>>>24;if(alpha<250)transparent=true;
                if(alpha>16){x0=Math.min(x0,x);x1=Math.max(x1,x);y0=Math.min(y0,y);y1=Math.max(y1,y);}
            }}
            if(!transparent)throw new IOException("Az első offline változat átlátszó hátterű PNG-t igényel. A háttéreltávolítás még nincs beépítve.");
            if(x1<x0||y1<y0)throw new IOException("Üres alfa-kép.");
            int cw=x1-x0+1,ch=y1-y0+1;
            float scale=512*.85f/Math.max(cw,ch),dw=cw*scale,dh=ch*scale;
            dst=Bitmap.createBitmap(512,512,Bitmap.Config.ARGB_8888);
            Canvas canvas=new Canvas(dst);canvas.drawColor(Color.rgb(128,128,128));
            canvas.drawBitmap(src,new Rect(x0,y0,x1+1,y1+1),new RectF((512-dw)/2,(512-dh)/2,(512+dw)/2,(512+dh)/2),new Paint(Paint.FILTER_BITMAP_FLAG));
            int[] pix=new int[512*512];dst.getPixels(pix,0,512,0,0,512,512);
            float[] rgb=new float[pix.length*3];for(int i=0;i<pix.length;i++){rgb[3*i]=Color.red(pix[i])/255f;rgb[3*i+1]=Color.green(pix[i])/255f;rgb[3*i+2]=Color.blue(pix[i])/255f;}return rgb;
        }finally{src.recycle();if(dst!=null)dst.recycle();}
    }
    private float[] encode(float[] rgb)throws Exception{
        progress.update("1/4 • A kép elemzése helyben (ez több percig is tarthat)…",5);
        try(OrtSession.SessionOptions o=options();OrtSession s=env.createSession(encoder.getAbsolutePath(),o)){
            if(s.getInputInfo().size()!=1||s.getOutputInfo().size()!=1)throw new IOException("Nem támogatott képkódoló: "+s.getInputInfo()+" / "+s.getOutputInfo());
            String name=s.getInputNames().iterator().next();long[] sh=info(s.getInputInfo().get(name)).getShape();
            boolean channelsFirst;
            if(sh.length==4&&dim(sh[0],1)&&dim(sh[1],3)&&dim(sh[2],512)&&dim(sh[3],512)){channelsFirst=true;sh=new long[]{1,3,512,512};}
            else if(sh.length==4&&dim(sh[0],1)&&dim(sh[1],512)&&dim(sh[2],512)&&dim(sh[3],3)){channelsFirst=false;sh=new long[]{1,512,512,3};}
            else if(sh.length==5&&dim(sh[0],1)&&dim(sh[1],1)&&dim(sh[2],512)&&dim(sh[3],512)&&dim(sh[4],3)){channelsFirst=false;sh=new long[]{1,1,512,512,3};}
            else if(sh.length==5&&dim(sh[0],1)&&dim(sh[1],1)&&dim(sh[2],3)&&dim(sh[3],512)&&dim(sh[4],512)){channelsFirst=true;sh=new long[]{1,1,3,512,512};}
            else throw new IOException("Ismeretlen képkódoló bemenet: "+Arrays.toString(sh));
            float[] input=rgb;
            if(channelsFirst){input=new float[rgb.length];int n=512*512;for(int i=0;i<n;i++)for(int c=0;c<3;c++)input[c*n+i]=rgb[3*i+c];}
            try(OnnxTensor t=OnnxTensor.createTensor(env,FloatBuffer.wrap(input),sh);OrtSession.Result result=s.run(Collections.singletonMap(name,t))){
                interrupt();OnnxTensor p=(OnnxTensor)result.get(0);long[] actual=p.getInfo().getShape();
                if(!Arrays.equals(actual,new long[]{1,3,40,64,64})&&!Arrays.equals(actual,new long[]{3,40,64,64}))throw new IOException("Ismeretlen triplane forma: "+Arrays.toString(actual));
                return floats(p);
            }
        }
    }
    private static float at(float[] p,int plane,int c,int x,int y){if(x<0||x>=64||y<0||y>=64)return 0;return p[((plane*40+c)*64+y)*64+x];}
    private static void features(float[] p,float x,float y,float z,float[] f,int offset){
        float[] a={x,x,y},b={y,z,z};
        for(int plane=0;plane<3;plane++){
            float px=(a[plane]/OfflineMesh.R+1)*32-.5f,py=(b[plane]/OfflineMesh.R+1)*32-.5f;
            int ix=(int)Math.floor(px),iy=(int)Math.floor(py);float dx=px-ix,dy=py-iy;
            for(int c=0;c<40;c++)f[offset+plane*40+c]=(1-dy)*((1-dx)*at(p,plane,c,ix,iy)+dx*at(p,plane,c,ix+1,iy))+dy*((1-dx)*at(p,plane,c,ix,iy+1)+dx*at(p,plane,c,ix+1,iy+1));
        }
    }
    private final class Field implements AutoCloseable{
        final OrtSession.SessionOptions o;final OrtSession s;final String input;final int batch;final boolean batched;
        final String den,col;final boolean denActivated,colActivated;
        final float[] planes;
        Field(float[] planes)throws Exception{
            this.planes=planes;o=options();OrtSession local=null;
            try{
                local=env.createSession(decoder.getAbsolutePath(),o);s=local;
                if(s.getInputInfo().size()!=1)throw new IOException("Ismeretlen MLP bemenet: "+s.getInputInfo());
                input=s.getInputNames().iterator().next();long[] sh=info(s.getInputInfo().get(input)).getShape();
                batched=sh.length==3&&dim(sh[0],1);
                if(!(sh.length==2||batched)||sh[sh.length-1]!=120)throw new IOException("Ismeretlen MLP forma: "+Arrays.toString(sh));
                long fixed=sh[sh.length-2];if(fixed>8192||fixed==0)throw new IOException("Nem támogatott MLP köteg: "+fixed);
                batch=fixed>0?(int)fixed:2048;
                String dn=null,cn=null;
                for(String n:s.getOutputNames()){
                    String l=n.toLowerCase(Locale.ROOT);
                    if(l.equals("density")||l.equals("density_act")||l.equals("sigma"))dn=n;
                    if(l.equals("features")||l.equals("color")||l.equals("rgb")||l.equals("raw_rgb"))cn=n;
                }
                if(dn==null||cn==null)throw new IOException("Nem azonosítható MLP kimenet: "+s.getOutputInfo());
                den=dn;col=cn;denActivated=dn.equalsIgnoreCase("density_act")||dn.equalsIgnoreCase("sigma");
                colActivated=cn.equalsIgnoreCase("color")||cn.equalsIgnoreCase("rgb");
            }catch(Exception|Error e){if(local!=null)local.close();o.close();throw e;}
        }
        float sigmoid(float x){return (float)(1/(1+Math.exp(-x)));}
        void query(float[] positions,int start,int count,float[] densities,float[] colors)throws Exception{
            interrupt();float[] in=new float[batch*120];
            for(int i=0;i<count;i++){int j=(start+i)*3;features(planes,positions[j],positions[j+1],positions[j+2],in,120*i);}
            long[] sh=batched?new long[]{1,batch,120}:new long[]{batch,120};
            try(OnnxTensor t=OnnxTensor.createTensor(env,FloatBuffer.wrap(in),sh);OrtSession.Result r=s.run(Collections.singletonMap(input,t))){
                float[] d=floats(r.get(den).orElseThrow(()->new IOException("Nincs sűrűség")));
                float[] c=floats(r.get(col).orElseThrow(()->new IOException("Nincs szín")));
                if(d.length!=batch||c.length!=batch*3)throw new IOException("Nem támogatott MLP kimeneti forma.");
                for(int i=0;i<count;i++){
                    float v=denActivated?d[i]:(float)Math.exp(Math.min(20,d[i]-1));
                    if(!Float.isFinite(v))throw new IOException("Nem véges sűrűség.");
                    if(densities!=null)densities[start+i]=v;
                    if(colors!=null)for(int k=0;k<3;k++){float cv=c[3*i+k];if(!Float.isFinite(cv))throw new IOException("Nem véges szín");colors[(start+i)*3+k]=colActivated?Math.max(0,Math.min(1,cv)):sigmoid(cv);}
                }
            }
        }
        @Override public void close()throws OrtException{try{s.close();}finally{o.close();}}
    }
    File generate(ContentResolver resolver,Uri image,File library,int n)throws Exception{
        if(!encoder.isFile()||!decoder.isFile())throw new IOException("Először telepítsd a két AI-modellt.");
        if(n!=32&&n!=48&&n!=64)throw new IOException("Nem támogatott felbontás.");
        float[] planes=encode(image(resolver,image));interrupt();
        OfflineMesh.Mesh mesh;float[] colors;
        try(Field field=new Field(planes)){
            int total=n*n*n;float[] pts=new float[total*3],density=new float[total];
            for(int i=0;i<total;i++){pts[3*i]=OfflineMesh.coordinate(i%n,n);pts[3*i+1]=OfflineMesh.coordinate((i/n)%n,n);pts[3*i+2]=OfflineMesh.coordinate(i/(n*n),n);}
            for(int i=0;i<total;i+=field.batch){field.query(pts,i,Math.min(field.batch,total-i),density,null);progress.update("2/4 • Térbeli sűrűség: "+(100L*Math.min(total,i+field.batch)/total)+"%",10+50*Math.min(total,i+field.batch)/total);}
            progress.update("3/4 • Háromdimenziós térháló készítése…",65);
            mesh=OfflineMesh.extract(n,density,25f);colors=new float[mesh.xyz.length];
            int vertices=mesh.xyz.length/3;
            for(int i=0;i<vertices;i+=field.batch){field.query(mesh.xyz,i,Math.min(field.batch,vertices-i),null,colors);progress.update("3/4 • Felület színezése",70+20*Math.min(vertices,i+field.batch)/vertices);}
        }
        interrupt();progress.update("4/4 • GLB mentése…",95);
        if(!library.isDirectory()&&!library.mkdirs())throw new IOException("Nem hozható létre a modellkönyvtár.");
        File tmp=File.createTempFile("offline-",".tmp",library);
        try{OfflineMesh.writeGlb(tmp,mesh,colors);interrupt();File dest=new File(library,"Offline_"+UUID.randomUUID()+".glb");if(!tmp.renameTo(dest))throw new IOException("GLB mentési hiba.");progress.update("Elkészült: "+dest.getName()+" • "+mesh.triangles.length/3+" háromszög. Csúcsonkénti szín, nem UV-textúra. Ellenőrizd a modellkönyvtárban.",100);return dest;}finally{if(tmp.exists())tmp.delete();}
    }
}
