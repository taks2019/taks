package hu.taks.world;

/**
 * Frame-time based left-stick movement. No dependency on Android UI or frame
 * rate, so it can be verified by a plain Java unit test on CI.
 *
 * Screen right = camera right, screen up = camera-forward on the terrain.
 * A limited acceleration smooths abrupt touch changes without leaving a
 * character gliding when the player releases the stick.
 */
public final class TouchMovement {
    private float smoothX=0f, smoothY=0f;

    public void reset(){smoothX=0f;smoothY=0f;}

    public float[] step(float rawX,float rawY,float yaw,float deltaSeconds,boolean sprint){
        if(!Float.isFinite(rawX)||!Float.isFinite(rawY)||
           !Float.isFinite(yaw)||!Float.isFinite(deltaSeconds)) {
            reset();return new float[]{0f,0f};
        }
        // A screen stall can never teleport a character over a collider.
        final float dt=Math.max(0.001f,Math.min(0.060f,deltaSeconds));
        final float magnitude=(float)Math.hypot(rawX,rawY);
        float x=0f,y=0f;
        if(magnitude>.11f) {
            float analog=Math.min(1f,(magnitude-.11f)/.89f);
            x=rawX/magnitude*analog;
            y=rawY/magnitude*analog;
        }
        if(x==0f&&y==0f) {
            // Releasing the stick stops immediately: no sliding on dry terrain.
            reset();
            return new float[]{0f,0f};
        }
        // 13 Hz response, stable across variable frame cadence.
        float blend=1f-(float)Math.exp(-13.0*dt);
        smoothX+=(x-smoothX)*blend;
        smoothY+=(y-smoothY)*blend;
        final float speed=(sprint?3.05f:1.78f)*dt;
        float worldX=(float)(-Math.cos(yaw)*smoothX-Math.sin(yaw)*smoothY)*speed;
        float worldZ=(float)(-Math.sin(yaw)*smoothX+Math.cos(yaw)*smoothY)*speed;
        return new float[]{worldX,worldZ};
    }
}
