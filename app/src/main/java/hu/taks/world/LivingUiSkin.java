package hu.taks.world;

/**
 * Causal UI skin derived exclusively from the saved native World snapshot.
 * No fake flora, fire, climate or discovery can be created by a menu.
 * Pure Java: testable on CI without Android and used by EVERY screen.
 */
public final class LivingUiSkin {
    private LivingUiSkin(){}

    public static final class State {
        public final boolean night, raining, wet, burning, grass, fern, vine, rope;
        public final int panelTop, panelBottom, border, text, mutedText, buttonTop, buttonBottom;
        public final int logoGlow;
        public final String cause, discovered;
        State(boolean night, boolean raining, boolean wet, boolean burning,
              boolean grass, boolean fern, boolean vine, boolean rope,
              int panelTop, int panelBottom, int border, int text, int mutedText,
              int buttonTop, int buttonBottom, int logoGlow,
              String cause, String discovered) {
            this.night=night;this.raining=raining;this.wet=wet;this.burning=burning;
            this.grass=grass;this.fern=fern;this.vine=vine;this.rope=rope;
            this.panelTop=panelTop;this.panelBottom=panelBottom;this.border=border;
            this.text=text;this.mutedText=mutedText;this.buttonTop=buttonTop;
            this.buttonBottom=buttonBottom;this.logoGlow=logoGlow;
            this.cause=cause;this.discovered=discovered;
        }
        // Small warm flicker only if the REAL fire is burning.
        // This changes the frame light, NEVER repaints the original logo pixels.
        public int animatedLogoGlow(long elapsedMs) {
            if(!burning)return logoGlow;
            double wave=Math.sin(elapsedMs*.006)*.5+.5;
            int alpha=0x86+(int)(wave*44);
            return (alpha<<24)|(logoGlow&0x00FFFFFF);
        }
    }
    private static float get(float[] c,int i,float fallback) {
        return c!=null && c.length==PrehistoricMenuTheme.CUE_COUNT &&
               Float.isFinite(c[i])?c[i]:fallback;
    }
    public static State fromCues(float[] cues) {
        PrehistoricMenuTheme.Look source=PrehistoricMenuTheme.fromCues(cues);
        float hour=(get(cues,0,6f)%24f+24f)%24f;
        float rain=get(cues,1,0f);
        float moisture=get(cues,2,.13f);
        boolean night=hour<5.5f || hour>19.5f;
        boolean raining=rain>.14f;
        boolean wet=raining || moisture>.34f;
        boolean burning=get(cues,3,0f)>.5f;
        boolean grass=get(cues,4,0f)>.5f;
        String cause;
        if(raining)cause="OK: eső → FOLYAMAT: nedvesedés → HATÁS: sötétebb kő és föld";
        else if(wet)cause="OK: nedves talaj → FOLYAMAT: lassú száradás → HATÁS: mélyebb földszínek";
        else if(burning)cause="OK: égő tűz → FOLYAMAT: fény és hő → HATÁS: meleg tűzfény";
        else if(night)cause="OK: napnyugta → FOLYAMAT: fogyó fény → HATÁS: sötétedő környezet";
        else cause="OK: napsütés → FOLYAMAT: száradás → HATÁS: világosabb talaj";
        // Fire adds local warm color even during wet weather; never marks wetness dry.
        int glow=burning?0xA7E6A054:(wet?0xAA657D72:night?0x94685C48:0xAEB68D53);
        int top=night?0xF2161715:(wet?0xF52A3029:0xF5362A20);
        int bottom=night?0xF5090B0A:(wet?0xF5131D1A:0xF5181511);
        int stroke=burning?0xFFE4B678:(wet?0xFF9AAFA0:0xFFD0B18A);
        int upper=wet?0xE74D5043:0xE753402E;
        int lower=night?0xE71B211F:0xE72C2923;
        return new State(night,raining,wet,burning,grass,source.fern,source.vine,
                source.rope,top,bottom,stroke,0xFFF1E5CF,0xFFD2C5AA,
                upper,lower,glow,cause,source.discoveryLine);
    }
}
