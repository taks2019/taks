package hu.taks.world;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.SystemClock;
import android.view.View;
import java.io.InputStream;

/**
 * Only transparent light/water/plant motion OVER the accepted immutable menu art.
 * Time, rain, wind and moisture MUST come from World; no fake sun/weather.
 * Native Android view, no HTML canvas / no complete simulated world replacement.
 */
public final class LivingWonderwerkOverlay extends View {
    private final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
    private final Path water=new Path();
    private final Path sky=new Path();
    private Bitmap fern,canopy;
    private MenuClimate climate;
    private boolean running=true;
    private long startNanos=SystemClock.elapsedRealtimeNanos();
    private static final float W=1280f,H=720f;
    public LivingWonderwerkOverlay(Context context){
        super(context);
        setWillNotDraw(false);
        setClickable(false);
        setFocusable(false);
        setLayerType(View.LAYER_TYPE_HARDWARE,null);
        try(InputStream f=context.getAssets().open("menu/fern_live_layer.png")){
            fern=BitmapFactory.decodeStream(f);
        }catch(Exception missing){android.util.Log.i("TAKS_MENU","Fern sprite missing: ambient motion only");}
        try(InputStream f=context.getAssets().open("menu/canopy_live_layer.png")){
            canopy=BitmapFactory.decodeStream(f);
        }catch(Exception missing){android.util.Log.i("TAKS_MENU","Canopy sprite missing: no swaying canopy");}
        water.moveTo(238,498);water.lineTo(450,492);water.lineTo(509,537);
        water.lineTo(509,636);water.lineTo(432,719);water.lineTo(228,719);water.close();
        sky.moveTo(217,0);sky.lineTo(720,0);sky.lineTo(610,117);
        sky.lineTo(460,240);sky.lineTo(224,250);sky.close();
    }
    public void setClimate(MenuClimate next){
        climate=next;
        if(running)postInvalidateOnAnimation();
    }
    public void resume(){running=true;startNanos=SystemClock.elapsedRealtimeNanos();postInvalidateOnAnimation();}
    public void pause(){running=false;}
    private void drawPlant(Canvas canvas,Bitmap img,float x,float y,float pivotX,
                           float pivotY,double t,float magnitude,float phase){
        if(img==null||img.isRecycled())return;
        float drift=(float)Math.sin(t*.55+phase)*magnitude;
        canvas.save();
        canvas.rotate(drift,pivotX,pivotY);
        p.reset();p.setAntiAlias(true);p.setFilterBitmap(true);
        canvas.drawBitmap(img,x,y,p);
        canvas.restore();
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        MenuClimate c=climate;
        if(c==null||getWidth()<=0||getHeight()<=0||!running)return;
        canvas.save();
        canvas.scale(getWidth()/W,getHeight()/H);
        double t=(SystemClock.elapsedRealtimeNanos()-startNanos)*1e-9;
        float daylight=c.daylight();
        // A low-opacity light veil: real saved hour and real rain determine strength.
        float dim=(1f-daylight)*.40f+c.rain*.12f;
        if(dim>0.004f){
            p.reset();p.setColor(Color.argb(Math.min(116,Math.round(dim*220)),11,20,27));
            canvas.drawRect(0,0,W,H,p);
        }
        // Wet stone slowly darkens; this is not painted into the approved original.
        float wet=c.wetStoneDarkening();
        if(wet>.08f){
            p.reset();p.setColor(Color.argb(Math.round(wet*24f),18,21,15));
            canvas.drawRoundRect(new RectF(460,305,865,674),12,12,p);
        }
        // Sun rays only with real daylight and without intense rain, confined to the sky.
        float sun=daylight*(1f-c.rain);
        if(sun>.12f){
            canvas.save();canvas.clipPath(sky);
            p.reset();p.setStrokeWidth(13);p.setStrokeCap(Paint.Cap.ROUND);
            float strength=Math.min(24f,20f*sun);
            for(int i=0;i<5;i++){
                float sweep=(float)Math.sin(t*.10+i*.7f)*2f;
                p.setColor(Color.argb(Math.round(strength*(.75f+i*.04f)),255,228,167));
                canvas.drawLine(378,117,330+i*52+sweep,295,p);
            }
            canvas.restore();
        }
        // Roots stay fixed. Actual saved wind modulates branches/leaves.
        float sway=c.vegetationMotion();
        float direction=c.windX<0?-1f:1f;
        if(canopy!=null && sway>.005f){
            drawPlant(canvas,canopy,202,0,260,4,t,direction*sway*1.4f,1.1f);
        }
        // The approved bitmap ALREADY contains this plant; only animate it.
        // A species discovery flag must not manufacture or erase the master art.
        if(fern!=null && sway>.005f){
            drawPlant(canvas,806,481,892,616,t,direction*sway*3.1f,2.4f);
        }
        // Water is naturally present. Wind/rain magnify waves; a still day stays quiet.
        float ripple=c.waterMotion();
        if(ripple>.009f){
            canvas.save();canvas.clipPath(water);
            p.reset();p.setStyle(Paint.Style.STROKE);p.setStrokeCap(Paint.Cap.ROUND);
            p.setStrokeWidth(.7f+ripple*.8f);
            p.setColor(Color.argb(Math.round(9+26*ripple),224,240,217));
            for(int i=0;i<16;i++){
                float bx=238+(i*29)%245;
                float by=508+(i*41)%205;
                float wave=(float)Math.sin(t*(.6+ripple*.9)+i*1.9f);
                float length=7f+8f*ripple;
                canvas.drawLine(bx+wave*4,by,bx+length+wave*4,by+wave*.9f,p);
            }
            canvas.restore();
        }
        // Cave dust only when dry plus wind. No fire, glowing sparks or technologies.
        float dust=c.dust();
        if(dust>.025f){
            p.reset();p.setStyle(Paint.Style.FILL);
            p.setColor(Color.argb(Math.round(dust*33),218,192,147));
            canvas.save();canvas.clipRect(871,235,1040,458);
            for(int i=0;i<16;i++){
                float x=900+(i*31)%120;
                float y=256+(i*19)%175;
                float slide=(float)((t*.22+i*.17)%1);
                float dx=(c.windX<0?-1f:1f)*slide*31*dust;
                canvas.drawCircle(x+dx,y-(slide*6),.8f+i%3*.3f,p);
            }
            canvas.restore();
        }
        canvas.restore();
        if(isShown())postInvalidateDelayed(33); // <=30 FPS, automatically stops on pause.
    }
    public void release(){
        pause();
        if(fern!=null){fern.recycle();fern=null;}
        if(canopy!=null){canopy.recycle();canopy=null;}
    }
}
