package hu.taks.world;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
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
    private final Layer water,fern,grass,cloud,treeCanopy;
    private MenuClimate climate;
    private boolean playing=true;
    private long begunNanos=SystemClock.elapsedRealtimeNanos();
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
            int a=(int)Math.min(232,intensity*.80);
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
                final boolean leaf=green>=54&&red>=72&&blue<155&&
                    red>blue*1.27f&&green>blue*1.28f&&
                    red>green*.77f&&red<green*1.77f;
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
        if(master!=null)master.recycle();
    }
    public void setEditableScript(EditableGameScript json){
        if(json==null)return;
        fernDegrees=json.decimal("environment","fern_sway_degrees",9.3f);
        waterVisibility=json.decimal("environment","water_ripple_visibility",1f);
    }
    public void setClimate(MenuClimate next){
        climate=next;
        if(playing)postInvalidateOnAnimation();
    }
    public void resume(){playing=true;begunNanos=SystemClock.elapsedRealtimeNanos();postInvalidateOnAnimation();}
    public void pause(){playing=false;}
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
    private void drawStream(Canvas canvas,double t,float wave){
        if(!water.exists())return;
        // Every rendered pixel in this layer was already selected as WATER by
        // the authoring mask; moving it NEVER moves rock/shore pixels.
        strips(canvas,water,t,1.0f+wave*4.0f,1.3f,.82f,false,.60f+.30f*wave);
    }
    private void drawFoliage(Canvas canvas,double t,MenuClimate w){
        float gust=w.vegetationMotion();
        if(gust<.003f)return; // no invented wind in still weather
        float sign=w.windX<0?-1f:1f;
        // 9.3° visually translates to an approx 2-9px flexible tip shift.
        float amplitude=Math.min(12f,fernDegrees)*gust*sign;
        strips(canvas,grass,t,amplitude*.62f,1.19f,1.24f,true,.86f);
        strips(canvas,fern,t,amplitude,2.42f,1.12f,true,.92f);
        // Independent overlapping canopy movements, distinct from the ferns.
        strips(canvas,treeCanopy,t,Math.max(1.5f,amplitude*.55f),.36f,
               .97f,false,.88f);
    }
    private void drawSunlight(Canvas canvas,double t,MenuClimate w){
        // Very subtle changing daylight only on the sunlit LEFT landscape.
        // The artist's logo, stone buttons and right-side cave remain untouched.
        if(w.daylight()<.12f)return;
        final float light=w.daylight();
        final float drift=(float)Math.sin(t*.23)*33f;
        Paint glow=new Paint(Paint.ANTI_ALIAS_FLAG);
        float x=286f+drift,y=225f+(float)Math.cos(t*.19)*16f;
        glow.setShader(new RadialGradient(x,y,272f,
            new int[]{0x11FFF0C6,0x08FFF0C6,0x00FFF0C6},
            new float[]{0f,.45f,1f},Shader.TileMode.CLAMP));
        glow.setAlpha(Math.round(190f*light));
        canvas.save();
        canvas.clipRect(0,110,650,540);
        canvas.drawRect(0,110,650,540,glow);
        canvas.restore();
    }
    private void drawClouds(Canvas canvas,double t,MenuClimate w){
        float drift=Math.max(0f,w.wind*.68f);
        if(drift<.003f||!cloud.exists())return;
        strips(canvas,cloud,t,drift*5.5f,.77f,.26f,false,.27f);
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        if(!playing||climate==null||getWidth()<=0||getHeight()<=0)return;
        canvas.save();
        canvas.scale(getWidth()/(float)WIDTH,getHeight()/(float)HEIGHT);
        double t=(SystemClock.elapsedRealtimeNanos()-begunNanos)*1e-9;
        // No rain streaks or rain video. The menu uses the accepted fair-weather
        // scene; water/wind evolve according to actual native simulation values.
        drawClouds(canvas,t,climate);
        drawStream(canvas,t,climate.waterMotion()*waterVisibility);
        drawFoliage(canvas,t,climate);
        drawSunlight(canvas,t,climate);
        canvas.restore();
        if(isShown())postInvalidateDelayed(33); // render only while visible
    }
    public void release(){
        pause();
        water.release();fern.release();grass.release();cloud.release();
        treeCanopy.release();
    }
}
