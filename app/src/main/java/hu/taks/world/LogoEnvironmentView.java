package hu.taks.world;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.View;

/**
 * Genuine Android overlay around/on the existing user-approved logo bitmap.
 * This is a tiny UI layer, NOT a web Canvas renderer or fake game world.
 * Weather/flicker is never invented: all effects use LivingUiSkin.State.
 */
public final class LogoEnvironmentView extends View {
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private LivingUiSkin.State world=LivingUiSkin.fromCues(null);

    public LogoEnvironmentView(Context context){
        super(context);
        setClickable(false);
        setFocusable(false);
        setLayerType(View.LAYER_TYPE_SOFTWARE,null);
    }
    public void setWorldState(LivingUiSkin.State state){
        if(state==null)return;
        boolean changed=world.night!=state.night || world.wet!=state.wet ||
            world.raining!=state.raining || world.burning!=state.burning ||
            world.grass!=state.grass;
        world=state;
        if(changed)invalidate();
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        final int w=getWidth(),h=getHeight();
        if(w<1||h<1)return;
        // Only dim the surroundings in actual nighttime. Underlying PNG/WebP
        // remains untouched and keeps the exact approved TA​KS emblem.
        if(world.night){
            paint.reset();
            paint.setColor(0x260B161B);
            canvas.drawRoundRect(1,1,w-1,h-1,15,15,paint);
        }
        if(world.wet){
            paint.reset();
            paint.setColor(0x126B9B99);
            canvas.drawRoundRect(1,h*.75f,w-1,h-1,12,12,paint);
        }
        final long now=SystemClock.uptimeMillis();
        if(world.burning){
            // Actual lit hearth: warm, low-alpha source at the lower roots.
            paint.reset();
            paint.setShader(new RadialGradient(w*.5f,h*.89f,w*.42f,
                new int[]{0x3DF6B66B,0x16E59D48,0x00F4B16B},
                new float[]{0f,.55f,1f},Shader.TileMode.CLAMP));
            canvas.drawRect(0,0,w,h,paint);
            paint.setShader(null);
        }
        if(world.raining){
            // Droplets fall over the emblem only while the game reports rain.
            // Deterministic streaks, no particles allocated per frame.
            final float drift=(now%2400L)/2400f;
            paint.reset();
            paint.setColor(0x688AB9B7);
            paint.setStrokeWidth(Math.max(.75f,w/280f));
            paint.setStrokeCap(Paint.Cap.ROUND);
            for(int i=0;i<19;++i){
                float nx=(i*.618034f+drift*.27f)%1f;
                float ny=(i*.382f+drift*1.65f)%1f;
                float x=nx*w,y=ny*h;
                float len=Math.max(4.f,h*.048f);
                canvas.drawLine(x,y,x-w*.009f,y+len,paint);
            }
        }
        if((world.raining||world.burning)&&isShown()){
            postInvalidateDelayed(90);
        }
    }
}
