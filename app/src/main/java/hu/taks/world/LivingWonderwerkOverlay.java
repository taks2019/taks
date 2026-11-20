package hu.taks.world;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.RadialGradient;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.View;
import java.io.InputStream;

/**
 * The approved 1664x768 image is immutable. The visible natural motion is
 * rendered from individual TRANSPARENT photographic masks on the Android
 * canvas. There is no moving background, no shaking stone/button/logo, no rain
 * particles, no prerendered video and no canvas-based stand-in game engine.
 *
 * Time, solar intensity, ground wind and surface-water motion come exclusively
 * from the single persisted native World -> MenuClimate snapshot.
 */
public final class LivingWonderwerkOverlay extends View {
    private static final int WIDTH=1664,HEIGHT=768;
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
    private final Layer water,fern,grass,cloud,treeCanopy,cliffVines;
    private MenuClimate climate;
    private MenuClimate smoothClimate;
    private boolean playing=true;
    private long lastFrameNanos=0;
    private float fernDegrees=9.3f,waterVisibility=1.f;

    /** Transparent pre-masked cropped pixels, NOT the original fixed photo. */
    private static final class Layer {
        final Bitmap texture;
        final Rect bounds;
        Layer(Bitmap texture,Rect bounds){this.texture=texture;this.bounds=bounds;}
        boolean exists(){return texture!=null&&!texture.isRecycled();}
        void release(){if(texture!=null&&!texture.isRecycled())texture.recycle();}
    }
    private static Bitmap decode(Context app,String asset){
        try(InputStream in=app.getAssets().open(asset)){return BitmapFactory.decodeStream(in);}
        catch(Exception e){android.util.Log.e("TAKS_MENU","Missing approved moving layer: "+asset,e);return null;}
    }
    private static Layer cutLayer(Bitmap master,Bitmap mask,Rect region,String name){
        if(master==null||mask==null||master.getWidth()!=WIDTH||master.getHeight()!=HEIGHT
           ||mask.getWidth()!=WIDTH||mask.getHeight()!=HEIGHT){
            android.util.Log.e("TAKS_MENU","Cannot form full-width alpha layer "+name);
            return new Layer(null,region);
        }
        final int w=region.width(),h=region.height();
        int[] source=new int[w*h],coverage=new int[w*h],output=new int[w*h];
        master.getPixels(source,0,w,region.left,region.top,w,h);
        mask.getPixels(coverage,0,w,region.left,region.top,w,h);
        for(int y=0;y<h;y++)for(int x=0;x<w;x++){
            int globalX=region.left+x,globalY=region.top+y;
            // MASKS MAY NOT INCLUDE THE USER'S LOGO OR THE STONE BUTTONS.
            // The logo (sacred) has its own immutable photographic area.
            boolean protectLogo=globalX>=657&&globalX<=1080&&globalY<339;
            boolean protectStones=globalX>=653&&globalX<=1125&&globalY>=336;
            if(protectLogo||protectStones){output[y*w+x]=0;continue;}
            int intensity=coverage[y*w+x]&255; // source mask is grayscale PNG (L)
            if(intensity<12){output[y*w+x]=0;continue;}
            if("water".equals(name)){
                // The authored mask contains occasional shoreline-rock pixels.
                // Only WATER-looking pixels may receive moving highlights.
                final int c=source[y*w+x];
                final int r=(c>>>16)&255,g=(c>>>8)&255,b=c&255;
                if(intensity<79||b<r*.80f||g<r*.78f){
                    output[y*w+x]=0;continue;
                }
            }
            int a=(int)Math.min(238,intensity*.89);
            output[y*w+x]=(source[y*w+x]&0x00FFFFFF)|(a<<24);
        }
        Bitmap layer=Bitmap.createBitmap(w,h,Bitmap.Config.ARGB_8888);
        layer.setPixels(output,0,w,0,0,w,h);
        return new Layer(layer,region);
    }
    /**
     * Recover only the warm yellow/green leaf pixels from the already accepted
     * photograph. Sky, stone, logo, buttons and the dark trunk are excluded.
     * A small shifting leaf overlay adds real continuously redrawn foliage.
     * No external image or substituted tree art is introduced.
     */
    private static Layer makeCanopyLayer(Bitmap photo){
        final Rect r=new Rect(0,0,640,360);
        if(photo==null||photo.getWidth()!=WIDTH||photo.getHeight()!=HEIGHT)
            return new Layer(null,r);
        int w=r.width(),h=r.height();
        int[] original=new int[w*h],leaves=new int[w*h];
        photo.getPixels(original,0,w,r.left,r.top,w,h);
        for(int y=0;y<h;++y){
            for(int x=0;x<w;++x){
                final int color=original[y*w+x];
                final int red=(color>>>16)&255,green=(color>>>8)&255,blue=color&255;
                // Exclude blue sky, cloud, white sun, dark bark and far stones.
                final boolean leaf=green>=95&&red>=125&&blue<155&&
                    red>blue*1.43f&&green>blue*1.36f&&
                    red>green*.83f&&red<green*1.53f;
                if(!leaf)continue;
                // Fade toward the scene edge so the movement never looks like
                // a large sliding rectangular picture.
                final float xfade=Math.min(1f,(w-x)/110f);
                final float yfade=Math.min(1f,(h-y)/90f);
                final int alpha=(int)(165f*Math.min(xfade,yfade));
                leaves[y*w+x]=(color&0x00FFFFFF)|(alpha<<24);
            }
        }
        Bitmap mask=Bitmap.createBitmap(w,h,Bitmap.Config.ARGB_8888);
        mask.setPixels(leaves,0,w,0,0,w,h);
        return new Layer(mask,r);
    }
    private static Layer makeCliffVinesLayer(Bitmap photo){
        // Hanging greenery on the far-right cliff is its own living layer.
        // Never animate the rock, original TAKS emblem or stone buttons.
        final Rect r=new Rect(1170,0,1664,420);
        if(photo==null||photo.getWidth()!=WIDTH||photo.getHeight()!=HEIGHT)
            return new Layer(null,r);
        final int w=r.width(),h=r.height();
        int[] original=new int[w*h],leaves=new int[w*h];
        photo.getPixels(original,0,w,r.left,r.top,w,h);
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){
            final int c=original[y*w+x];
            final int red=(c>>>16)&255,green=(c>>>8)&255,blue=c&255;
            // Green/sage foliage differs from the yellow dolomite face.
            final boolean leaf=green>=65&&green>blue*1.35f&&
                    green>red*.91f&&red<green*1.20f&&blue<155;
            if(!leaf)continue;
            float edgeX=Math.min(1f,Math.min(x,w-x)/46f);
            float edgeY=Math.min(1f,Math.min(y,h-y)/34f);
            int alpha=(int)(195f*edgeX*edgeY);
            leaves[y*w+x]=(c&0x00FFFFFF)|(alpha<<24);
        }
        Bitmap layer=Bitmap.createBitmap(w,h,Bitmap.Config.ARGB_8888);
        layer.setPixels(leaves,0,w,0,0,w,h);
        return new Layer(layer,r);
    }
    private static Layer createLayer(Context c,Bitmap original,String maskAsset,Rect area,String label){
        Bitmap mask=decode(c,maskAsset);
        Layer layer=cutLayer(original,mask,area,label);
        if(mask!=null)mask.recycle();
        return layer;
    }
    public LivingWonderwerkOverlay(Context context){
        super(context);
        setWillNotDraw(false);
        setClickable(false);setFocusable(false);
        setLayerType(View.LAYER_TYPE_HARDWARE,null);
        Bitmap master=decode(context,"menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png");
        water=createLayer(context,master,"menu/WATER_MASK.png",new Rect(0,548,673,768),"water");
        fern=createLayer(context,master,"menu/FERN_MASK.png",new Rect(1109,429,1664,768),"fern");
        grass=createLayer(context,master,"menu/GRASS_MASK.png",new Rect(0,369,794,768),"grass");
        cloud=createLayer(context,master,"menu/CLOUD_MASK.png",new Rect(0,0,772,379),"clouds");
        treeCanopy=makeCanopyLayer(master);
        cliffVines=makeCliffVinesLayer(master);
        if(master!=null)master.recycle();
    }
    public void setEditableScript(EditableGameScript json){
        if(json==null)return;
        fernDegrees=json.decimal("environment","fern_sway_degrees",9.3f);
        waterVisibility=json.decimal("environment","water_ripple_visibility",1f);
    }
    public void setClimate(MenuClimate next){
        if(next==null)return;
        climate=next;
        if(smoothClimate==null)smoothClimate=next;
        if(playing)postInvalidateOnAnimation();
    }
    public void resume(){
        playing=true;
        lastFrameNanos=SystemClock.elapsedRealtimeNanos();
        postInvalidateOnAnimation();
    }
    public void pause(){playing=false;lastFrameNanos=0;}
    private void strips(Canvas c,Layer layer,double t,float amount,float phase,
                        float horizontalSpeed,boolean rooted,float alpha){
        if(!layer.exists()||amount<=.001f)return;
        final int w=layer.bounds.width(),h=layer.bounds.height();
        final int strip=5;
        paint.reset();paint.setFilterBitmap(true);paint.setAntiAlias(true);
        paint.setAlpha((int)Math.max(0,Math.min(255,alpha*255)));
        for(int y=0;y<h;y+=strip){
            int end=Math.min(h,y+strip);
            float frac=y/(float)h;
            float rootScale=rooted?(float)Math.pow(Math.max(0f,1f-frac),1.35):1f;
            float dx=(float)(Math.sin(t*horizontalSpeed+phase+y*.010)*amount*rootScale+
                       Math.sin(t*horizontalSpeed*.37+phase+y*.027)*amount*.28f*rootScale);
            int sy=layer.bounds.top+y;
            Rect from=new Rect(0,y,w,end);
            RectF to=new RectF(layer.bounds.left+dx,sy,layer.bounds.left+w+dx,
                               layer.bounds.top+end);
            c.drawBitmap(layer.texture,from,to,paint);
        }
    }
    private void swayHanging(Canvas canvas,Layer layer,double t,float motion){
        if(!layer.exists()||motion<=.003f)return;
        final int w=layer.bounds.width(),h=layer.bounds.height();
        paint.reset();paint.setFilterBitmap(true);paint.setAntiAlias(true);
        paint.setAlpha(215);
        for(int y=0;y<h;y+=5){
            int end=Math.min(h,y+5);
            float tip=(float)Math.pow(y/(float)h,1.24);
            float dx=(float)(Math.sin(t*.79+y*.017)*motion*tip+
                       Math.sin(t*.37+y*.023)*motion*.28f*tip);
            Rect src=new Rect(0,y,w,end);
            RectF target=new RectF(layer.bounds.left+dx,layer.bounds.top+y,
                                   layer.bounds.right+dx,layer.bounds.top+end);
            canvas.drawBitmap(layer.texture,src,target,paint);
        }
    }
    private void drawDust(Canvas canvas,double t,MenuClimate w){
        final float dust=Math.max(.12f*w.daylight(),w.dust());
        if(dust<.012f)return; // backlit cave particles + stronger gust-driven dust
        // Dust originates at the cave threshold and follows the true wind.
        // Slow translucent plumes plus individual grains, never static noise.
        // The protected emblem and stone button region is out of bounds.
        Paint plume=new Paint(Paint.ANTI_ALIAS_FLAG);
        plume.setStyle(Paint.Style.FILL);
        canvas.save();
        canvas.clipRect(1150,254,1485,585);
        final int count=66;
        for(int i=0;i<count;++i){
            double phase=(t*(.013+.002*(i%4))+i*.61803398875)%1.;
            float fade=(float)Math.pow(Math.sin(Math.PI*phase),2);
            float x=1195f+(float)phase*240f;
            float y=331f+(i*18.73f)%175f+
                    (float)Math.sin(t*.21+i*.7)*9f;
            float size=1.1f+(i%7)*.76f;
            plume.setColor(i%3==0?0xFFFFE6B6:0xFFE2C495);
            plume.setAlpha(Math.round(165f*dust*fade));
            canvas.drawCircle(x,y,size,plume);
        }
        canvas.restore();
    }
    private void drawStream(Canvas canvas,double t,float wave){
        if(!water.exists()||wave<=.005f)return;
        // IMPORTANT: NEVER translate photo pixels of the water or the shore.
        // Their original rock positions remain completely immutable.
        // Render evolving specular ripples separately, then intersect the
        // entire result with the conservatively filtered pixel water mask.
        RectF area=new RectF(water.bounds);
        int layer=canvas.saveLayer(area,null);
        Paint ripple=new Paint(Paint.ANTI_ALIAS_FLAG);
        ripple.setStyle(Paint.Style.STROKE);
        ripple.setStrokeCap(Paint.Cap.ROUND);
        ripple.setColor(0xFFF9E7BC);
        for(int i=0;i<38;++i){
            double phase=(t*(.035+.003*(i%5))+i*.61803398875)%1.;
            float x=10f+(float)phase*654f;
            float y=565f+(i*31.2f)%198f+(float)Math.sin(t*.53+i*1.3)*3.8f;
            float fade=(float)Math.pow(Math.sin(Math.PI*phase),2);
            ripple.setAlpha((int)(185f*fade*(.38f+.62f*wave)));
            ripple.setStrokeWidth(.7f+(i%3)*.55f);
            float length=4f+(i%6)*2.1f;
            canvas.drawLine(x,y,x+length,y+(float)Math.sin(t*.9+i)*.7f,ripple);
        }
        // Tight alpha intersection makes stones/grass on the riverbank static.
        Paint restrict=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
        restrict.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DST_IN));
        canvas.drawBitmap(water.texture,water.bounds.left,water.bounds.top,restrict);
        restrict.setXfermode(null);
        canvas.restoreToCount(layer);
    }
    private void drawFoliage(Canvas canvas,double t,MenuClimate w){
        // Even on a calm day the leaves experience subtle local turbulence,
        // while gust strength still comes from the real simulated wind.
        float gust=.20f+.80f*w.vegetationMotion();
        float sign=w.windX<-.1f?-1f:1f;
        float slowGust=.87f+.13f*(float)Math.sin(t*.22+1.7);
        float amplitude=Math.min(22f,Math.max(14f,fernDegrees*1.60f))*gust*slowGust*sign;
        strips(canvas,grass,t,amplitude*.59f,1.19f,.82f,true,.84f);
        strips(canvas,fern,t,amplitude,2.42f,.63f,true,.98f);
        // Tree and cliff plant groups are animated at different frequencies.
        strips(canvas,treeCanopy,t,Math.max(3.1f,Math.abs(amplitude)*.55f),.36f,
               .43f,false,.90f);
        swayHanging(canvas,cliffVines,t,Math.max(2.6f,Math.abs(amplitude)*.87f));
    }
    private void drawSunlight(Canvas canvas,double t,MenuClimate w){
        // Very subtle changing daylight only on the sunlit LEFT landscape.
        // The artist's logo, stone buttons and right-side cave remain untouched.
        if(w.daylight()<.12f)return;
        final float light=w.daylight();
        // One continuous cloud-shadow oscillator: no random frame changes,
        // no switching videos, no discontinuities at the bright sun.
        final float cloudCover=.5f+.5f*(float)Math.sin(t*.082+1.2);
        final float sunlight=light*(.28f+.72f*(1f-cloudCover));
        Paint glow=new Paint(Paint.ANTI_ALIAS_FLAG);
        float x=459f+(float)Math.sin(t*.045)*17f;
        float y=178f+(float)Math.sin(t*.027+.4)*8f;
        glow.setShader(new RadialGradient(x,y,375f,
            new int[]{0x30FFF3D2,0x12FFE7B5,0x00FFE7B5},
            new float[]{0f,.43f,1f},Shader.TileMode.CLAMP));
        glow.setAlpha(Math.round(220f*sunlight));
        canvas.save();
        canvas.clipRect(0,60,650,555);
        canvas.drawRect(0,60,650,555,glow);
        canvas.restore();
    }
    private void drawClouds(Canvas canvas,double t,MenuClimate w){
        if(!cloud.exists())return;
        // The whole cloud bank drifts gently rather than jumping between
        // screenshots; the sun/cloud occlusion follows its own slow cycle.
        float drift=.24f+w.wind*.76f;
        strips(canvas,cloud,t,drift*11.5f,.77f,.092f,false,.43f);
    }
    private void drawBirds(Canvas canvas,double t,MenuClimate weather){
        // Distant aerial silhouettes only: the main five human characters
        // remain the real native GLES3 skinned models in the gameplay world.
        Paint wings=new Paint(Paint.ANTI_ALIAS_FLAG);
        wings.setStyle(Paint.Style.STROKE);
        wings.setStrokeWidth(2.3f);
        wings.setStrokeCap(Paint.Cap.ROUND);
        wings.setColor(0xFF493826);
        Path flight=new Path();
        for(int i=0;i<4;++i){
            final double phase=(t*(.016+.003*i)+i*.31)%1.0;
            final float x=54f+(float)phase*560f;
            final float y=197f+i*21f+(float)Math.sin(t*.13+i*2)*11f;
            final float opacity=(float)Math.pow(Math.sin(Math.PI*phase),2);
            wings.setAlpha((int)(205f*opacity*weather.daylight()));
            if(wings.getAlpha()<3)continue;
            final float bend=(float)Math.sin(t*(4.2+i*.4)+i*2.1)*4.5f;
            final float size=11f+i*2.4f;
            flight.reset();
            flight.moveTo(x-size,y-1f+bend);
            flight.quadTo(x-size*.45f,y-2f-bend*.55f,x,y);
            flight.quadTo(x+size*.45f,y-2f-bend*.55f,x+size,y-1f+bend);
            canvas.drawPath(flight,wings);
        }
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        if(!playing||climate==null||getWidth()<=0||getHeight()<=0)return;
        final long now=SystemClock.elapsedRealtimeNanos();
        final float dt=lastFrameNanos==0?.033f:
                         Math.max(0f,Math.min(.2f,(now-lastFrameNanos)*1.e-9f));
        lastFrameNanos=now;
        final float blend=(float)(1.0-Math.exp(-dt/2.4));
        smoothClimate=MenuClimate.blend(smoothClimate,climate,blend);
        final MenuClimate weather=smoothClimate;
        canvas.save();
        canvas.scale(getWidth()/(float)WIDTH,getHeight()/(float)HEIGHT);
        // Absolute monotonic time has no 8s seam and survives pause/resume.
        double t=now*1e-9;
        drawClouds(canvas,t,weather);
        drawStream(canvas,t,weather.waterMotion()*waterVisibility);
        drawFoliage(canvas,t,weather);
        drawSunlight(canvas,t,weather);
        drawBirds(canvas,t,weather);
        drawDust(canvas,t,weather);
        canvas.restore();
        if(isShown())postInvalidateDelayed(33); // render only while visible
    }
    public void release(){
        pause();
        water.release();fern.release();grass.release();cloud.release();
        treeCanopy.release();cliffVines.release();
    }
}
