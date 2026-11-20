package hu.taks.world;

/** The click targets match the exact unchanged 1664x768 stone plaques. */
public final class MenuStoneTouch {
    public static final String[] COMMANDS={"new_game","settings","creator","exit"};
    private static final float LEFT=.396f, WIDTH=.249f;
    private static final float[] TOP={.443f,.566f,.687f,.809f};
    private static final float[] HEIGHT={.120f,.120f,.115f,.120f};
    private MenuStoneTouch(){}
    public static float left(int index){check(index);return LEFT;}
    public static float top(int index){check(index);return TOP[index];}
    public static float width(int index){check(index);return WIDTH;}
    public static float height(int index){check(index);return HEIGHT[index];}
    public static int hit(float nx,float ny){
        if(!Float.isFinite(nx)||!Float.isFinite(ny))return -1;
        for(int i=0;i<COMMANDS.length;i++)
            if(nx>=LEFT&&nx<=LEFT+WIDTH&&ny>=TOP[i]&&ny<=TOP[i]+HEIGHT[i])
                return i;
        return -1;
    }
    private static void check(int index){
        if(index<0||index>=COMMANDS.length)throw new IndexOutOfBoundsException();
    }
}
