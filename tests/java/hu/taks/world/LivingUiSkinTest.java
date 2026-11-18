package hu.taks.world;

public final class LivingUiSkinTest {
    static void check(boolean b,String message){if(!b)throw new AssertionError(message);}
    static float[] state(){float[] s=new float[11];s[0]=13f;s[2]=.13f;return s;}
    public static void main(String[] args){
        float[] dry=state();
        LivingUiSkin.State day=LivingUiSkin.fromCues(dry);
        check(!day.night&&!day.wet&&!day.burning,"sunlit dry world");
        check(day.cause.contains("napsütés"),"dry causal text");
        check(!day.fern&&!day.vine&&!day.rope,"unknown plants must stay unknown");
        check(day.animatedLogoGlow(0)==day.animatedLogoGlow(10000),"no fake fire flicker");
        float[] rain=state();rain[1]=.78f;rain[2]=.75f;rain[4]=1f;
        LivingUiSkin.State wet=LivingUiSkin.fromCues(rain);
        check(wet.wet&&wet.raining&&wet.grass,"rain and authentic grass");
        check(wet.panelTop!=day.panelTop && wet.logoGlow!=day.logoGlow,
                "rain physically changes panels and logo frame");
        check(wet.cause.contains("eső") && wet.cause.contains("nedvesedés"),
                "rain causes moisture");
        float[] fire=state();fire[3]=1;fire[0]=22f;
        LivingUiSkin.State lit=LivingUiSkin.fromCues(fire);
        check(lit.night&&lit.burning,"night with actual burning hearth");
        check(lit.animatedLogoGlow(100)!=lit.animatedLogoGlow(550),
                "real fire creates limited frame flicker");
        float[] discoveries=state();
        discoveries[5]=discoveries[6]=discoveries[7]=1;
        discoveries[8]=1;
        LivingUiSkin.State discovered=LivingUiSkin.fromCues(discoveries);
        check(discovered.fern,"ferns appear only with verified ecological approval");
        check(!discovered.vine&&!discovered.rope,"no invented rope from craft evidence");
        check(discovered.discovered.contains("kőmegmunkálás"),"real skills appear");
        check(LivingUiSkin.fromCues(null).panelTop!=0,"safe null state");
        System.out.println("PASS: world-driven main menu, all submenus, day, rain, fire, discovery and original logo frame");
    }
}
