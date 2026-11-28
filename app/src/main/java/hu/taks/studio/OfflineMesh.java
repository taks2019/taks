package hu.taks.studio;

import java.io.*;
import java.nio.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

/** Indexed marching tetrahedra on a regular scalar grid; no Android dependency. */
final class OfflineMesh {
    static final float R=.87f;
    static final class Mesh {
        final float[] xyz, normals;
        final int[] triangles;
        Mesh(float[] xyz,int[] triangles) {
            this.xyz=xyz;this.triangles=triangles;normals=new float[xyz.length];
            for(int i=0;i<triangles.length;i+=3){
                int a=3*triangles[i],b=3*triangles[i+1],c=3*triangles[i+2];
                float ux=xyz[b]-xyz[a],uy=xyz[b+1]-xyz[a+1],uz=xyz[b+2]-xyz[a+2];
                float vx=xyz[c]-xyz[a],vy=xyz[c+1]-xyz[a+1],vz=xyz[c+2]-xyz[a+2];
                float x=uy*vz-uz*vy,y=uz*vx-ux*vz,z=ux*vy-uy*vx;
                for(int p:new int[]{a,b,c}){normals[p]+=x;normals[p+1]+=y;normals[p+2]+=z;}
            }
            for(int i=0;i<normals.length;i+=3){
                double len=Math.sqrt(normals[i]*normals[i]+normals[i+1]*normals[i+1]+normals[i+2]*normals[i+2]);
                if(len>1e-12){for(int j=0;j<3;j++)normals[i+j]/=(float)len;}
                else normals[i+1]=1;
            }
        }
    }
    static float coordinate(int i,int n){return -R+2*R*i/(n-1);}
    private static final int[][] TETS={{0,1,3,7},{0,3,2,7},{0,2,6,7},{0,6,4,7},{0,4,5,7},{0,5,1,7}};
    private static final class Builder {
        final int n;final float[] field;final float iso;
        final Map<Long,Integer> edges=new HashMap<>();
        final ArrayList<Float> xyz=new ArrayList<>();
        final ArrayList<Integer> tris=new ArrayList<>();
        Builder(int n,float[] f,float iso){this.n=n;field=f;this.iso=iso;}
        float p(int id,int axis){return coordinate(axis==0?id%n:axis==1?(id/n)%n:id/(n*n),n);}
        int edge(int a,int b){
            int lo=Math.min(a,b),hi=Math.max(a,b);
            float t=(iso-field[a])/(field[b]-field[a]);
            t=Math.max(0,Math.min(1,t));
            if(t<=1e-7f){lo=a;hi=a;}else if(t>=1-1e-7f){lo=b;hi=b;}
            long key=((long)lo<<32)|(hi&0xffffffffL);
            Integer old=edges.get(key);if(old!=null)return old;
            int k=xyz.size()/3;
            for(int j=0;j<3;j++)xyz.add(p(a,j)+(p(b,j)-p(a,j))*t);
            edges.put(key,k);return k;
        }
        void tri(int a,int b,int c,float[] out){
            if(a==b||b==c||c==a)return;
            int i=a*3,j=b*3,k=c*3;
            float ux=xyz.get(j)-xyz.get(i),uy=xyz.get(j+1)-xyz.get(i+1),uz=xyz.get(j+2)-xyz.get(i+2);
            float vx=xyz.get(k)-xyz.get(i),vy=xyz.get(k+1)-xyz.get(i+1),vz=xyz.get(k+2)-xyz.get(i+2);
            float nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
            if(nx*nx+ny*ny+nz*nz<1e-20f)return;
            if(nx*out[0]+ny*out[1]+nz*out[2]<0){int tmp=b;b=c;c=tmp;}
            tris.add(a);tris.add(b);tris.add(c);
        }
        void tet(int[] cube,int[] t){
            int[] in=new int[4],out=new int[4];int ni=0,no=0;
            for(int k:t){int id=cube[k];if(field[id]>iso)in[ni++]=id;else out[no++]=id;}
            if(ni==0||no==0)return;
            float[] direction=new float[3];
            for(int j=0;j<3;j++){for(int k=0;k<no;k++)direction[j]+=p(out[k],j)/no;for(int k=0;k<ni;k++)direction[j]-=p(in[k],j)/ni;}
            if(ni==1)tri(edge(in[0],out[0]),edge(in[0],out[1]),edge(in[0],out[2]),direction);
            else if(ni==3)tri(edge(out[0],in[0]),edge(out[0],in[1]),edge(out[0],in[2]),direction);
            else {int a=edge(in[0],out[0]),b=edge(in[0],out[1]),c=edge(in[1],out[0]),d=edge(in[1],out[1]);tri(a,b,c,direction);tri(b,d,c,direction);}
        }
    }
    static Mesh extract(int n,float[] f,float iso)throws IOException{
        if(n<2||n>128||f.length!=n*n*n||!Float.isFinite(iso))throw new IOException("Érvénytelen sűrűségi rács.");
        for(float v:f)if(!Float.isFinite(v))throw new IOException("Nem véges sűrűség.");
        Builder b=new Builder(n,f,iso);
        for(int z=0;z<n-1;z++){
            if(Thread.currentThread().isInterrupted())throw new InterruptedIOException("Megszakítva");
            for(int y=0;y<n-1;y++)for(int x=0;x<n-1;x++){
                int i=(z*n+y)*n+x,s=n*n;
                int[] c={i,i+1,i+n,i+n+1,i+s,i+s+1,i+s+n,i+s+n+1};
                boolean pos=false,neg=false;for(int j:c){if(f[j]>iso)pos=true;else neg=true;}
                if(pos&&neg)for(int[] t:TETS)b.tet(c,t);
            }
        }
        if(b.tris.isEmpty())throw new IOException("Nem keletkezett felület. Ellenőrizd a modellt és a referenciaképet.");
        float[] xyz=new float[b.xyz.size()];int[] tr=new int[b.tris.size()];
        for(int i=0;i<xyz.length;i++)xyz[i]=b.xyz.get(i);
        for(int i=0;i<tr.length;i++)tr[i]=b.tris.get(i);
        return new Mesh(xyz,tr);
    }
    static float linear(float c){c=Math.max(0,Math.min(1,c));return c<=.04045f?c/12.92f:(float)Math.pow((c+.055)/1.055,2.4);}
    static void writeGlb(File file,Mesh m,float[] rgb)throws IOException{
        if(rgb.length!=m.xyz.length)throw new IOException("Színhiba");
        int count=m.xyz.length/3,bytes=m.xyz.length*4,ib=m.triangles.length*4,bin=3*bytes+ib;
        float[] min={Float.POSITIVE_INFINITY,Float.POSITIVE_INFINITY,Float.POSITIVE_INFINITY},max={-Float.POSITIVE_INFINITY,-Float.POSITIVE_INFINITY,-Float.POSITIVE_INFINITY};
        for(int i=0;i<m.xyz.length;i++){int j=i%3;min[j]=Math.min(min[j],m.xyz[i]);max[j]=Math.max(max[j],m.xyz[i]);}
        String j="{\"asset\":{\"version\":\"2.0\",\"generator\":\"TAKS Studio offline\"},\"scene\":0,\"scenes\":[{\"nodes\":[0]}],\"nodes\":[{\"mesh\":0}],\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"NORMAL\":1,\"COLOR_0\":2},\"indices\":3,\"material\":0}]}],\"materials\":[{\"pbrMetallicRoughness\":{\"baseColorFactor\":[1,1,1,1],\"metallicFactor\":0,\"roughnessFactor\":1}}],\"buffers\":[{\"byteLength\":"+bin+"}],\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":"+bytes+",\"target\":34962},{\"buffer\":0,\"byteOffset\":"+bytes+",\"byteLength\":"+bytes+",\"target\":34962},{\"buffer\":0,\"byteOffset\":"+(2*bytes)+",\"byteLength\":"+bytes+",\"target\":34962},{\"buffer\":0,\"byteOffset\":"+(3*bytes)+",\"byteLength\":"+ib+",\"target\":34963}],\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":"+count+",\"type\":\"VEC3\",\"min\":"+Arrays.toString(min)+",\"max\":"+Arrays.toString(max)+"},{\"bufferView\":1,\"componentType\":5126,\"count\":"+count+",\"type\":\"VEC3\"},{\"bufferView\":2,\"componentType\":5126,\"count\":"+count+",\"type\":\"VEC3\"},{\"bufferView\":3,\"componentType\":5125,\"count\":"+m.triangles.length+",\"type\":\"SCALAR\"}]}";
        byte[] json=j.getBytes(StandardCharsets.UTF_8);int padded=(json.length+3)&~3;
        try(DataOutputStream o=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(file)))){
            put(o,0x46546c67);put(o,2);put(o,12+8+padded+8+bin);put(o,padded);put(o,0x4e4f534a);
            o.write(json);for(int i=json.length;i<padded;i++)o.writeByte(32);
            put(o,bin);put(o,0x004e4942);
            for(float v:m.xyz)put(o,Float.floatToIntBits(v));
            for(float v:m.normals)put(o,Float.floatToIntBits(v));
            for(float v:rgb){if(!Float.isFinite(v))throw new IOException("Érvénytelen szín");put(o,Float.floatToIntBits(linear(v)));}
            for(int v:m.triangles)put(o,v);
        }
    }
    private static void put(DataOutputStream o,int i)throws IOException{o.writeInt(Integer.reverseBytes(i));}
}
