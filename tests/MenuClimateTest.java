import hu.taks.world.MenuClimate;

public final class MenuClimateTest {
    private static void check(boolean p,String msg){if(!p)throw new AssertionError(msg);}
    public static void main(String[] args){
        check(MenuClimate.fromNative(null)==null,"null cannot invent weather");
        check(MenuClimate.fromNative(new float[]{12,0,0,0,0,0})==null,"short array rejected");
        check(MenuClimate.fromNative(new float[]{12,0,0,Float.NaN,0,0,.5f,0})==null,"NaN rejected");
        check(MenuClimate.fromNative(new float[]{12,2,0,0,0,0,.5f,0})==null,"invalid rain rejected");
        MenuClimate calm=MenuClimate.fromNative(new float[]{12,0,.13f,0,0,0,.66f,0});
        MenuClimate windy=MenuClimate.fromNative(new float[]{12,.70f,.7f,.80f,.5f,-.4f,.75f,1});
        check(calm!=null&&windy!=null,"valid states");
        check(windy.waterMotion()>calm.waterMotion(),"wind/rain cause more ripples");
        check(windy.vegetationMotion()>calm.vegetationMotion(),"wind moves plants");
        check(windy.wetStoneDarkening()>calm.wetStoneDarkening(),"wet stones darken");
        MenuClimate dryWind=MenuClimate.fromNative(new float[]{12,0,.13f,.80f,.5f,-.4f,.75f,1});
        check(windy.dust()<dryWind.dust(),"rain prevents dust compared with equally strong dry wind");
        check(calm.daylight()>.95f,"midday brightness");
        check(MenuClimate.fromNative(new float[]{0,0,0,0,0,0,.5f,0}).daylight()<.05f,
              "night darkness");
        check(!calm.fern&&windy.fern,"fern tag from World only");
        MenuClimate night=MenuClimate.fromNative(new float[]{23.9f,0,.13f,0,0,0,.66f,0});
        MenuClimate dawn=MenuClimate.fromNative(new float[]{.1f,0,.13f,0,0,0,.66f,0});
        MenuClimate middle=MenuClimate.blend(night,dawn,.5f);
        check(middle!=null&&(middle.hour<.04f||middle.hour>23.96f),
              "Midnight must interpolate across wrap without jumping through noon");
        check(MenuClimate.blend(calm,windy,0).wind==calm.wind,
              "Zero blend must preserve the previous weather");
        check(MenuClimate.blend(calm,windy,1).wind==windy.wind,
              "Full blend reaches the actual new native weather");
        check(MenuClimate.blend(calm,windy,.5f).wind>calm.wind,
              "Cloud/wind changes blend continuously");
        System.out.println("PASS: validated World snapshot; smooth day/cloud/wind interpolation and river motion.");
    }
}
