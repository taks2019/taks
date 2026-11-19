package hu.taks.world;

/** Immutable data adapted from the SAVED native World, never fake weather. */
public final class MenuClimate {
    public final float hour,rain,soilWater,wind,windX,windZ,waterLevel;
    public final boolean fern;
    private static float clamp(float x){ return Math.max(0f,Math.min(1f,x)); }
    private MenuClimate(float h,float r,float m,float w,float wx,float wz,float level,boolean f){
        hour=h;rain=r;soilWater=m;wind=w;windX=wx;windZ=wz;waterLevel=level;fern=f;
    }
    /** hour, rain, ground moisture, wind strength, wind X, wind Z, water level, verified fern. */
    public static MenuClimate fromNative(float[] raw){
        if(raw==null||raw.length!=8)return null;
        for(float x:raw)if(!Float.isFinite(x))return null;
        if(raw[0]<0f||raw[0]>100000000f||raw[1]<0f||raw[1]>1f||
           raw[2]<0f||raw[2]>1f||raw[3]<0f||raw[3]>1f||
           raw[6]<0f||raw[6]>1f||Math.abs(raw[4])>1.01f||Math.abs(raw[5])>1.01f)
            return null;
        return new MenuClimate((raw[0]%24f+24f)%24f,raw[1],raw[2],raw[3],
                               raw[4],raw[5],raw[6],raw[7]>.5f);
    }
    public float waterMotion(){return clamp(.10f+wind*.65f+rain*.45f) * (.35f+.65f*waterLevel);}
    public float vegetationMotion(){return clamp(wind)*.94f;}
    public float wetStoneDarkening(){return clamp(soilWater*.75f+rain*.23f);}
    public float daylight(){
        float h=hour;
        if(h<5.5f||h>19.5f)return .03f;
        if(h<7.25f)return .03f+(h-5.5f)/1.75f*.97f;
        if(h>17.75f)return .03f+(19.5f-h)/1.75f*.97f;
        return 1f;
    }
    public float dust(){return clamp((wind-.20f)*1.25f*(1f-soilWater)*(1f-rain));}
}
