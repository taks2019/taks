package hu.taks.world;
/** Run with javac; needs no Android SDK. */
public final class PrehistoricMenuThemeTest {
    private static float[] cues(){float[] c=new float[PrehistoricMenuTheme.CUE_COUNT];c[0]=12f;return c;}
    private static void check(boolean b,String message){if(!b)throw new AssertionError(message);}
    private static void noNeon(int c){
        int r=(c>>16)&255,g=(c>>8)&255,b=c&255;
        check(!(b>r+15&&b>g+15),"Unexpected blue-neon component: "+Integer.toHexString(c));
    }
    public static void main(String[] args){
        float[] c=cues();
        PrehistoricMenuTheme.Look initial=PrehistoricMenuTheme.fromCues(c);
        check(initial.discoveryLine.endsWith("felfedezésre vár"),"Unknown craft shown prematurely");
        check(!initial.moss&&!initial.fern&&!initial.vine&&!initial.rope&&!initial.ember,
              "World features cannot be invented");
        c[1]=.7f;c[2]=.6f;c[4]=1f;c[5]=1f;c[6]=1f;
        PrehistoricMenuTheme.Look rainy=PrehistoricMenuTheme.fromCues(c);
        check(rainy.moss,"Damp vegetation should influence material");
        check(!rainy.fern&&!rainy.vine&&!rainy.rope,"Leaves must not unlock fern/vine/rope");
        check(rainy.discoveryLine.contains("kőmegmunkálás"),"Real stone discovery absent");
        check(rainy.discoveryLine.contains("famegmunkálás"),"Real wood discovery absent");
        c[8]=1f;c[9]=1f;c[10]=1f;c[3]=1f;c[7]=1f;
        PrehistoricMenuTheme.Look all=PrehistoricMenuTheme.fromCues(c);
        check(all.fern&&all.vine&&all.rope&&all.ember,"Verified knowledge not reflected");
        check(all.discoveryLine.contains("kötél"),"Rope should appear after discovery");
        c[0]=23f;
        PrehistoricMenuTheme.Look night=PrehistoricMenuTheme.fromCues(c);
        check(night.veilTop!=rainy.veilTop,"Day/night must change visual palette");
        for(PrehistoricMenuTheme.Look s:new PrehistoricMenuTheme.Look[]{initial,rainy,all,night}){
            noNeon(s.veilTop);noNeon(s.veilBottom);noNeon(s.buttonTop);
            noNeon(s.buttonBottom);noNeon(s.text);noNeon(s.accent);
        }
        System.out.println("PASS: natural palette + causality + gated discoveries");
    }
}
