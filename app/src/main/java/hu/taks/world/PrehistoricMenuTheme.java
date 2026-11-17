package hu.taks.world;

/**
 * TAKS WORLD / MEGALODON: prehistoric menu appearance driven by World cues.
 * No LEDs, neon, modern symbols or resources before they exist in the world.
 * This rule set contains no Android dependencies.
 */
public final class PrehistoricMenuTheme {
    private PrehistoricMenuTheme() {}
    // Native World supplies: hour, rain, soil moisture, fire, nearby vegetation,
    // stonework evidence, woodwork evidence, shelter, verified fern, vine, rope.
    public static final int CUE_COUNT = 11;
    public static final class Look {
        public final int veilTop, veilBottom, buttonTop, buttonBottom, text, accent;
        public final boolean moss, fern, vine, rope, ember;
        public final String discoveryLine;
        Look(int a,int b,int c,int d,int e,int f,
             boolean moss,boolean fern,boolean vine,boolean rope,boolean ember,String label){
            this.veilTop=a;this.veilBottom=b;this.buttonTop=c;this.buttonBottom=d;
            this.text=e;this.accent=f;this.moss=moss;this.fern=fern;
            this.vine=vine;this.rope=rope;this.ember=ember;this.discoveryLine=label;
        }
    }
    private static float get(float[] s,int i,float fallback){
        return s!=null && s.length==CUE_COUNT && Float.isFinite(s[i])?s[i]:fallback;
    }
    private static boolean present(float[] s,int i){return get(s,i,0)>.5f;}
    public static Look fromCues(float[] cues){
        float hour=(get(cues,0,6f)%24f+24f)%24f;
        boolean night=hour<5.5f||hour>19.5f;
        boolean wet=get(cues,1,0f)>.30f||get(cues,2,.13f)>.38f;
        boolean liveFire=present(cues,3);
        boolean moss=wet&&present(cues,4);
        // A fern/vine and rope NEVER appear merely from moisture or leaf stock.
        boolean fern=present(cues,8),vine=present(cues,9),rope=present(cues,10);
        int top=night?0xCC130F0B:(wet?0xAF231D16:0xA8291E13);
        int bottom=night?0xE10E0D0A:0xD018120C;
        int buttonTop=wet?0xE3453A28:0xE356412C;
        int buttonBottom=wet?0xF02A2419:0xF0302419;
        int accent=liveFire?0xFFE5A457:(moss?0xFF9EA071:0xFFD4AA70);
        StringBuilder discovered=new StringBuilder("MEGISMERT: ");
        if(present(cues,5))discovered.append("kőmegmunkálás · ");
        if(present(cues,6))discovered.append("famegmunkálás · ");
        if(present(cues,7))discovered.append("menedék · ");
        if(liveFire)discovered.append("égő tűz · ");
        if(rope)discovered.append("kötél · ");
        if(discovered.length()=="MEGISMERT: ".length())discovered.append("felfedezésre vár");
        else discovered.setLength(discovered.length()-3);
        return new Look(top,bottom,buttonTop,buttonBottom,0xFFF4E5C7,accent,
                        moss,fern,vine,rope,liveFire,discovered.toString());
    }
}
