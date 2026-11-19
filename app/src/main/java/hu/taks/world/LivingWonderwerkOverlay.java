package hu.taks.world;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.os.SystemClock;
import android.view.View;
import java.io.InputStream;

/** Real-time, world-state-driven layers over the untouched, user-approved
 *  Wonderwerk still. Neither rock geometry nor the sacred logo is animated.
 *  No prerendered water/rock MP4 and no fake time-of-day loops. */
public final class LivingWonderwerkOverlay extends View {
    private static final int W=1280,H=720;
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
    private final Path stream=new Path(),sky=new Path();
    private final byte[] waterMask=new byte[W*H];
    private Bitmap fern,canopy;
    private MenuClimate weather;
    private float fernDegrees=9.3f,rainThreshold=.045f,waterVisibility=1.f;
    private boolean running=true;
    private long startNanos=SystemClock.elapsedRealtimeNanos();

    public LivingWonderwerkOverlay(Context context) {
        super(context);
        setWillNotDraw(false);setClickable(false);setFocusable(false);
        setLayerType(View.LAYER_TYPE_HARDWARE,null);
        stream.moveTo(235,489);stream.lineTo(460,484);stream.lineTo(512,545);
        stream.lineTo(512,720);stream.lineTo(220,720);stream.close();
        sky.moveTo(220,0);sky.lineTo(720,0);sky.lineTo(623,128);
        sky.lineTo(455,248);sky.lineTo(220,251);sky.close();
        try(InputStream in=context.getAssets().open("menu/fern_live_layer.png")){
            fern=BitmapFactory.decodeStream(in);
        }catch(Exception absent){
            android.util.Log.i("TAKS_WORLD","Fern asset absent; extracting leaf pixels from approved art");
        }
        try(InputStream in=context.getAssets().open("menu/canopy_live_layer.png")){
            canopy=BitmapFactory.decodeStream(in);
        }catch(Exception absent){/* Derived foliage still works without the optional canopy. */}
        // The JPG itself stays immutable. Color-mask extraction happens only on
        // separate overlay textures. Brown rocks/soil are NEVER water pixels.
        try(InputStream in=context.getAssets().open("menu/wonderwerk_menu_poster.jpg")){
            Bitmap master=BitmapFactory.decodeStream(in);
            if(master!=null&&master.getWidth()==W&&master.getHeight()==H){
                for(int y=493;y<704;y++)for(int x=235;x<510;x++){
                    int color=master.getPixel(x,y);
                    int r=(color>>16)&255,g=(color>>8)&255,b=color&255;
                    if(b>r+5&&b>=g-4&&b>58)waterMask[y*W+x]=1;
                }
                if(fern==null||fern.getWidth()!=280||fern.getHeight()!=173){
                    fern=Bitmap.createBitmap(280,173,Bitmap.Config.ARGB_8888);
                    for(int y=0;y<173;y++)for(int x=0;x<280;x++){
                        int c=master.getPixel(806+x,481+y);
                        int r=(c>>16)&255,g=(c>>8)&255,b=c&255;
                        float green=Math.min(((float)g/Math.max(1,r)-.77f)*8f,(g-b*1.40f)/30f);
                        float leaf=Math.max(0f,Math.min(1f,green));
                        // Root remains fixed. Only the visible leafy ends swing.
                        float fade=Math.max(0f,Math.min(1f,(170f-y)/48f));
                        int alpha=Math.round(220f*leaf*fade);
                        fern.setPixel(x,y,(c&0x00ffffff)|(alpha<<24));
                    }
                }
                master.recycle();
            }
        }catch(Exception ex){android.util.Log.w("TAKS_WORLD","Approved poster masks unavailable",ex);}
    }
    /** Visual multipliers are editable; actual rain/wind still come ONLY from saved World. */
    public void setEditableScript(EditableGameScript config){
        if(config==null)return;
        fernDegrees=config.decimal("environment","fern_sway_degrees",9.3f);
        rainThreshold=config.decimal("environment","rain_visual_threshold",.045f);
        waterVisibility=config.decimal("environment","water_ripple_visibility",1.f);
    }
    public void setClimate(MenuClimate next) {
        weather=next;
        if(running)postInvalidateOnAnimation();
    }
    public void resume(){running=true;startNanos=SystemClock.elapsedRealtimeNanos();postInvalidateOnAnimation();}
    public void pause(){running=false;}
    private boolean waterAt(int x,int y){
        return x>=0&&x<W&&y>=0&&y<H&&waterMask[y*W+x]!=0;
    }
    private void drawWindLeaves(Canvas c,Bitmap sprite,float x,float y,float pivotX,
                                float pivotY,double t,float angle,float phase) {
        if(sprite==null||sprite.isRecycled())return;
        c.save();
        float gust=(float)(.78+.22*Math.sin(t*.23+phase*.4));
        c.rotate((float)Math.sin(t*1.19+phase)*angle*gust,pivotX,pivotY);
        paint.reset();paint.setAntiAlias(true);paint.setFilterBitmap(true);
        c.drawBitmap(sprite,x,y,paint);
        c.restore();
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        MenuClimate c=weather;
        if(!running||c==null||getWidth()==0||getHeight()==0)return;
        canvas.save();
        canvas.scale(getWidth()/(float)W,getHeight()/(float)H);
        double t=(SystemClock.elapsedRealtimeNanos()-startNanos)*1.e-9;
        float daylight=c.daylight();
        float dim=(1-daylight)*.40f+c.rain*.12f;
        if(dim>.004f){
            canvas.save();
            canvas.clipOutRect(520,0,870,315); // NEVER alter the accepted logotype
            paint.reset();
            paint.setColor(Color.argb(Math.min(116,Math.round(dim*220)),11,20,27));
            canvas.drawRect(0,0,W,H,paint);
            canvas.restore();
        }
        float sun=daylight*(1f-c.rain);
        if(sun>.12f){
            canvas.save();canvas.clipPath(sky);
            paint.reset();paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(11f);paint.setStrokeCap(Paint.Cap.ROUND);
            for(int i=0;i<5;i++){
                float drift=(float)Math.sin(t*.13+i)*2f;
                paint.setColor(Color.argb(Math.round(12f*sun),255,229,174));
                canvas.drawLine(373,116,330+i*50+drift,290,paint);
            }
            canvas.restore();
        }
        // Independent vegetation overlays. Their pivot/root stays in place.
        float sway=c.vegetationMotion();
        float sign=c.windX<0?-1f:1f;
        if(sway>.003f) {
            drawWindLeaves(canvas,canopy,202,0,260,4,t,sign*sway*2.4f,1.2f);
            drawWindLeaves(canvas,fern,806,481,892,616,t,sign*sway*fernDegrees,2.4f);
        }
        // ONLY reflecting blue water pixels react; fixed banks/rocks are never transformed.
        float ripple=c.waterMotion()*waterVisibility;
        if(ripple>.01f){
            canvas.save();canvas.clipPath(stream);
            paint.reset();paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeCap(Paint.Cap.ROUND);paint.setStrokeWidth(.75f+ripple*.7f);
            for(int i=0;i<68;i++){
                float px=250+(i*31)%249,py=506+(i*47)%198;
                float displacement=(float)Math.sin(t*(.65+ripple*.62f)+i*1.72f)*3.2f*ripple;
                int x=Math.round(px+displacement),y=Math.round(py);
                int length=Math.round(5f+8f*ripple);
                if(!waterAt(x,y)||!waterAt(x+length,y))continue;
                paint.setColor(Color.argb(Math.round(12+29*ripple),187,222,232));
                canvas.drawLine(x,y,x+length,y+.15f,paint);
                if(c.rain>.05f&&i%3==0){
                    float age=(float)((t*(.51+c.rain)+i*.177)%1.);
                    float radius=.75f+age*3.8f*c.rain;
                    paint.setColor(Color.argb(Math.round(31f*(1-age)*c.rain),184,215,228));
                    canvas.drawOval(new RectF(x-radius,y-radius*.4f,
                                               x+radius,y+radius*.4f),paint);
                }
            }
            canvas.restore();
        }
        // Rainfall is visible when the saved world actually reports rain.
        if(c.rain>rainThreshold){
            canvas.save();canvas.clipOutRect(520,0,870,315);
            paint.reset();paint.setStrokeCap(Paint.Cap.ROUND);
            paint.setStrokeWidth(.65f+c.rain*.65f);
            paint.setColor(Color.argb(Math.round(12+30*c.rain),191,215,225));
            for(int i=0;i<100;i++){
                float x=((i*117)+(float)(t*(38f+57f*c.rain)))%W;
                float y=((i*73)+(float)(t*(140f+122f*c.rain)))%H;
                float dx=c.windX*c.wind*6f;
                canvas.drawLine(x,y,x+dx,y+3f+c.rain*9f,paint);
            }
            canvas.restore();
        }
        // Dust appears only with dry ground, low rain and sufficiently strong wind.
        float dust=c.dust();
        if(dust>.02f){
            canvas.save();canvas.clipRect(862,244,1040,456);
            paint.reset();paint.setColor(Color.argb(Math.round(31*dust),219,190,143));
            for(int i=0;i<17;i++){
                float x=896+(i*29)%133+(float)Math.sin(t*.31+i)*14*dust;
                float y=248+(i*19)%178;
                canvas.drawCircle(x,y,.8f+i%3*.3f,paint);
            }
            canvas.restore();
        }
        canvas.restore();
        if(isShown())postInvalidateDelayed(33);
    }
    public void release(){
        pause();
        if(fern!=null){fern.recycle();fern=null;}
        if(canopy!=null){canopy.recycle();canopy=null;}
    }
}
