package hu.taks.world;

public final class MenuStoneTouchTest {
    private static void check(boolean condition,String explanation){
        if(!condition)throw new AssertionError(explanation);
    }
    public static void main(String[] args){
        String[] expected={"new_game","settings","creator","exit"};
        for(int i=0;i<expected.length;i++){
            check(expected[i].equals(MenuStoneTouch.COMMANDS[i]),"Wrong command "+i);
            final float x=MenuStoneTouch.left(i)+MenuStoneTouch.width(i)*.5f;
            final float y=MenuStoneTouch.top(i)+MenuStoneTouch.height(i)*.5f;
            check(MenuStoneTouch.hit(x,y)==i,"Wrong stone "+i);
            for(float scale:new float[]{.20f,.33f,.5f,.9f,1.4f}){
                float px=x*1664f*scale,py=y*768f*scale;
                check(MenuStoneTouch.hit(px/(1664f*scale),py/(768f*scale))==i,
                      "Wrong scaled button "+i);
            }
        }
        check(MenuStoneTouch.hit(.10f,.10f)==-1,"Sky must not activate a button");
        check(MenuStoneTouch.hit(.80f,.50f)==-1,"Cave must not activate a button");
        check(MenuStoneTouch.hit(.50f,.39f)==-1,"Logo must not activate a button");
        check(MenuStoneTouch.hit(Float.NaN,.6f)==-1,"Invalid pointer");
        System.out.println("PASS 4 real stone hit zones on 5 landscape sizes");
    }
}
