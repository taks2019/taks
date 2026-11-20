package hu.taks.world;

public final class TouchMovementTest {
    private static void check(boolean condition,String detail){
        if(!condition)throw new AssertionError(detail);
    }
    private static double distance(float[] movement){
        return Math.hypot(movement[0],movement[1]);
    }
    private static float[] distanceAfter(float dt,int frames,boolean sprint){
        TouchMovement m=new TouchMovement();
        double x=0,z=0;
        for(int i=0;i<frames;++i){
            float[] d=m.step(0,1,0f,dt,sprint);
            x+=d[0];z+=d[1];
        }
        return new float[]{(float)x,(float)z};
    }
    public static void main(String[] args){
        TouchMovement m=new TouchMovement();
        float[] forward=m.step(0,1,0,1f/30,false);
        check(forward[1]>0 && Math.abs(forward[0])<1e-5,"camera-forward axis");
        m.reset();
        float[] right=m.step(1,0,0,1f/30,false);
        check(right[0]<0 && Math.abs(right[1])<1e-5,"camera-right axis");
        m.reset();
        float[] rotated=m.step(0,1,(float)(Math.PI/2),1f/30,false);
        check(rotated[0]<0 && Math.abs(rotated[1])<1e-5,"camera orbit direction");
        check(distance(m.step(0,0,0,1f/30,false))==0,"release must stop without drift");
        float[] slow=distanceAfter(1f/30,90,false);
        float[] fastCadence=distanceAfter(1f/60,180,false);
        check(Math.abs(distance(slow)-distance(fastCadence))<.08,
              "movement must not double when frame rate doubles");
        float[] sprint=distanceAfter(1f/30,90,true);
        check(distance(sprint)>distance(slow)*1.60,"sprint exceeds walk speed");
        m.reset();
        float[] hugeFrame=m.step(0,1,0,100f,false);
        check(distance(hugeFrame)<.2,"lag spike cannot teleport character");
        check(distance(m.step(Float.NaN,1,0,.033f,false))==0,"bad touch input rejected");
        System.out.println("PASS: 30/60fps, diagonal, orbit, immediate stop, sprint and frame spike");
    }
}
