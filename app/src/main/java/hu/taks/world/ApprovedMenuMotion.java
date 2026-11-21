package hu.taks.world;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.SurfaceTexture;
import android.media.MediaPlayer;
import android.view.Gravity;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.FrameLayout;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

/**
 * Real looping MP4 layered ABOVE the unmodified, approved RestoredStoneMenu.
 *
 * The original 1664x768 scene, logo, stone button shapes and hit targets
 * are unchanged. A native TextureView streams the animated scene. This
 * overlay is NON-CLICKABLE, so the four original controls underneath receive
 * all input; text on the four buttons is redrawn at exactly the old coordinates.
 *
 * If the MP4 is missing or MediaPlayer fails, alpha=0 and the previously
 * approved, fully functional static menu is exposed without any substitute.
 */
final class ApprovedMenuMotion extends FrameLayout
        implements TextureView.SurfaceTextureListener {
    private static final String VIDEO="menu/wonderwerk_loop.mp4";
    private final TextureView video;
    private final View captions;
    private MediaPlayer player;
    private Surface surface;
    private boolean paused;
    private boolean prepared;
    private boolean renderedFirstFrame;

    static ApprovedMenuMotion create(Context context){
        try(InputStream input=context.getAssets().open(VIDEO)){
            if(input.read()<0)return null;
            return new ApprovedMenuMotion(context);
        }catch(Exception noMedia){
            android.util.Log.w("TAKS_MENU_MOTION","Approved menu animation not bundled; original image remains");
            return null;
        }
    }

    private ApprovedMenuMotion(Context context){
        super(context);
        setClickable(false);
        setFocusable(false);
        setWillNotDraw(true);
        setClipChildren(false);
        setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        setAlpha(0f);
        video=new TextureView(context);
        video.setOpaque(false);
        video.setClickable(false);
        video.setFocusable(false);
        video.setSurfaceTextureListener(this);
        video.setContentDescription(null);
        addView(video,new LayoutParams(-1,-1,Gravity.CENTER));

        captions=new View(context){
            final Paint pen=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.SUBPIXEL_TEXT_FLAG);
            final String[] names={"ÚJ JÁTÉK","BEÁLLÍTÁSOK","KÉSZÍTŐ","KILÉPÉS"};
            final float[] rows={387f,479f,573f,668f};
            {
                pen.setTypeface(Typeface.create(Typeface.SERIF,Typeface.BOLD));
                pen.setTextAlign(Paint.Align.CENTER);
                setClickable(false);
                setFocusable(false);
                setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            }
            @Override protected void onDraw(Canvas canvas){
                super.onDraw(canvas);
                if(getWidth()==0||getHeight()==0)return;
                float scale=Math.min(getWidth()/1664f,getHeight()/768f);
                float x=(getWidth()-1664f*scale)*.5f;
                float y=(getHeight()-768f*scale)*.5f;
                canvas.save();
                canvas.translate(x,y);
                canvas.scale(scale,scale);
                for(int i=0;i<4;i++){
                    float size=28f;
                    pen.setTextSize(size);
                    float measured=pen.measureText(names[i]);
                    if(measured>176f)size*=176f/measured;
                    pen.setTextSize(size);
                    canvas.save();
                    canvas.clipRect(833f,rows[i]-34f,1023f,rows[i]+12f);
                    pen.setStyle(Paint.Style.STROKE);
                    pen.setStrokeWidth(3.2f);
                    pen.setStrokeJoin(Paint.Join.ROUND);
                    pen.clearShadowLayer();
                    pen.setColor(0xFF33210F);
                    canvas.drawText(names[i],928f,rows[i],pen);
                    pen.setStyle(Paint.Style.FILL);
                    pen.setColor(0xFFFFF1CF);
                    pen.setShadowLayer(1.2f,0,1,0xFF34220D);
                    canvas.drawText(names[i],928f,rows[i],pen);
                    canvas.restore();
                }
                canvas.restore();
            }
        };
        addView(captions,new LayoutParams(-1,-1,Gravity.CENTER));
    }

    @Override protected void onSizeChanged(int w,int h,int ow,int oh){
        super.onSizeChanged(w,h,ow,oh);
        if(w<=0||h<=0)return;
        final float ratio=Math.min(w/1664f,h/768f);
        LayoutParams v=new LayoutParams(Math.max(2,Math.round(1664f*ratio)),
                Math.max(2,Math.round(768f*ratio)),Gravity.CENTER);
        video.setLayoutParams(v);
        captions.invalidate();
    }

    void setRunning(boolean enabled){
        paused=!enabled;
        if(player!=null && prepared){
            try{
                if(enabled&&!player.isPlaying())player.start();
                else if(!enabled&&player.isPlaying())player.pause();
            }catch(IllegalStateException error){
                android.util.Log.w("TAKS_MENU_MOTION","Pause/resume failed",error);
            }
        }
    }

    private void prepareSource(MediaPlayer mp)throws Exception {
        try(AssetFileDescriptor descriptor=getContext().getAssets().openFd(VIDEO)){
            mp.setDataSource(descriptor.getFileDescriptor(),descriptor.getStartOffset(),
                    descriptor.getLength());
        }catch(java.io.IOException compressed){
            File cached=new File(getContext().getCacheDir(),"taks_menu_video_update.mp4");
            if(!cached.exists()||cached.length()<50_000){
                try(InputStream input=getContext().getAssets().open(VIDEO);
                    FileOutputStream output=new FileOutputStream(cached)){
                    byte[] bytes=new byte[65536];
                    int n;while((n=input.read(bytes))!=-1)output.write(bytes,0,n);
                }
            }
            mp.setDataSource(cached.getAbsolutePath());
        }
    }

    @Override public void onSurfaceTextureAvailable(SurfaceTexture texture,int w,int h){
        releaseVideo();
        surface=new Surface(texture);
        try{
            final MediaPlayer mp=new MediaPlayer();
            player=mp;
            prepareSource(mp);
            mp.setSurface(surface);
            mp.setVolume(0,0);
            mp.setLooping(true);
            mp.setOnPreparedListener(ready->{
                prepared=true;
                if(!paused)ready.start();
            });
            mp.setOnInfoListener((ready,what,extra)->{
                if(what==MediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START&&!renderedFirstFrame){
                    renderedFirstFrame=true;
                    // Fade from original static image to same pixels now moving.
                    animate().alpha(1f).setDuration(220).start();
                }
                return false;
            });
            mp.setOnErrorListener((ready,what,extra)->{
                android.util.Log.e("TAKS_MENU_MOTION","MP4 decoding failed "+what+"/"+extra);
                setAlpha(0f);releaseVideo();
                return true;
            });
            mp.prepareAsync();
        }catch(Exception error){
            android.util.Log.e("TAKS_MENU_MOTION","Unable to load moving menu",error);
            setAlpha(0f);releaseVideo();
        }
    }
    @Override public void onSurfaceTextureSizeChanged(SurfaceTexture texture,int w,int h){}
    @Override public boolean onSurfaceTextureDestroyed(SurfaceTexture texture){
        releaseVideo();
        return true;
    }
    @Override public void onSurfaceTextureUpdated(SurfaceTexture texture){}

    void releaseVideo(){
        prepared=false;
        if(player!=null){
            try{player.reset();}catch(IllegalStateException ignored){}
            player.release();
            player=null;
        }
        if(surface!=null){surface.release();surface=null;}
    }
    @Override protected void onDetachedFromWindow(){
        releaseVideo();
        super.onDetachedFromWindow();
    }
}
