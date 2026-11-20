package hu.taks.world;
import java.io.*;
import java.util.Arrays;
public final class ProceduralSoundscapeTest {
    static double energy(short[] a){double s=0;for(short v:a)s+=(double)v*v;return s/a.length;}
    public static void main(String[] args)throws Exception{
        ProceduralSoundscape s=new ProceduralSoundscape(1234);
        short[] pcm=new short[48000];s.render(pcm);
        if(energy(pcm)!=0)throw new AssertionError("Silence must be silent");
        float[] state=new float[13];state[0]=1;state[1]=.7f;state[2]=.5f;state[4]=.4f;
        s.update(state);s.render(pcm);double e=energy(pcm);
        if(e<100||e>100000000)throw new AssertionError("Unexpected output energy "+e);
        short[] previous=pcm.clone();s.render(pcm);
        if(Arrays.equals(previous,pcm))throw new AssertionError("Repeated audio block");
        // Rendering must preserve oscillator/filter state across arbitrary block boundaries.
        ProceduralSoundscape a=new ProceduralSoundscape(89),b=new ProceduralSoundscape(89);
        a.update(state);b.update(state);short[] whole=new short[48000],half=new short[24000];
        a.render(whole);b.render(half);
        if(!Arrays.equals(Arrays.copyOfRange(whole,0,24000),half))throw new AssertionError("First half");
        b.render(half);
        if(!Arrays.equals(Arrays.copyOfRange(whole,24000,48000),half))throw new AssertionError("Boundary discontinuity");
        s.update(new float[13]);for(int i=0;i<8;i++)s.render(pcm);
        if(energy(pcm)>1)throw new AssertionError("Did not fade to silence");
        // Travel, not a timer: idle is silent, actual motion triggers ground impact.
        ProceduralSoundscape steps=new ProceduralSoundscape(7);
        steps.update(new float[13]);steps.render(pcm);
        float[] moving=new float[13];moving[10]=.7f;steps.update(moving);steps.render(pcm);
        if(energy(pcm)==0)throw new AssertionError("No footstep after travel");
        if(args.length>0){
            s.update(state);try(DataOutputStream out=new DataOutputStream(new FileOutputStream(args[0]))){
                for(int block=0;block<20;block++){s.render(pcm);for(short v:pcm){out.writeByte(v&255);out.writeByte((v>>>8)&255);}}
            }
        }
        System.out.println("PASS silence, bounded signal, no repeated blocks, continuous block boundaries, fade, travel footsteps");
    }
}
