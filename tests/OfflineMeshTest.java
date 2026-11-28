package hu.taks.studio;
import java.io.*;
import java.nio.*;
import java.nio.file.*;
import java.util.*;
public class OfflineMeshTest {
 public static void main(String[] args)throws Exception{
  int n=24;float[] f=new float[n*n*n];
  for(int i=0;i<f.length;i++){float x=OfflineMesh.coordinate(i%n,n),y=OfflineMesh.coordinate((i/n)%n,n),z=OfflineMesh.coordinate(i/(n*n),n);f[i]=.36f-x*x-y*y-z*z;}
  OfflineMesh.Mesh m=OfflineMesh.extract(n,f,0);
  if(m.xyz.length<300||m.triangles.length<300)throw new AssertionError("empty mesh");
  Map<Long,Integer> edges=new HashMap<>();
  for(int i=0;i<m.triangles.length;i+=3)for(int j=0;j<3;j++){
   int a=m.triangles[i+j],b=m.triangles[i+(j+1)%3];
   if(a<0||a>=m.xyz.length/3||b<0||b>=m.xyz.length/3||a==b)throw new AssertionError("index");
   long k=((long)Math.min(a,b)<<32)|(Math.max(a,b)&0xffffffffL);edges.merge(k,1,Integer::sum);
  }
  for(int k:edges.values())if(k!=2)throw new AssertionError("nonmanifold edge");
  for(int i=0;i<m.xyz.length;i+=3){float dot=m.xyz[i]*m.normals[i]+m.xyz[i+1]*m.normals[i+1]+m.xyz[i+2]*m.normals[i+2];if(dot<=0)throw new AssertionError("normal direction");}
  float[] rgb=new float[m.xyz.length];Arrays.fill(rgb,.5f);
  File out=File.createTempFile("mesh-test",".glb");try{
   OfflineMesh.writeGlb(out,m,rgb);ByteBuffer b=ByteBuffer.wrap(Files.readAllBytes(out.toPath())).order(ByteOrder.LITTLE_ENDIAN);
   if(b.getInt()!=0x46546c67||b.getInt()!=2||b.getInt()!=out.length())throw new AssertionError("GLB");
   int len=b.getInt();if(b.getInt()!=0x4e4f534a)throw new AssertionError("JSON");
   b.position(20+len);int bin=b.getInt();if(b.getInt()!=0x004e4942||b.remaining()!=bin)throw new AssertionError("BIN");
  }finally{out.delete();}
  System.out.println("Mesh test passed: "+m.xyz.length/3+" vertices, "+m.triangles.length/3+" triangles");
 }
}
