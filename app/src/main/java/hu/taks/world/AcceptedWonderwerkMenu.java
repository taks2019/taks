package hu.taks.world;

import android.app.Activity;
import android.content.res.AssetManager;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.RippleDrawable;
import android.net.Uri;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.VideoView;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Approved 2026-11-19 image presented as smooth silent MP4.
 * Button glyphs are pixels in the immutable artwork, while the
 * invisible Android hit zones are fully functional native controls.
 * If the two binary assets are absent, MainActivity uses its original menu.
 */
public final class AcceptedWonderwerkMenu extends FrameLayout {
    private static final String VIDEO="menu/wonderwerk_menu_30fps.mp4";
    private static final String POSTER="menu/wonderwerk_menu_poster.jpg";
    private static final int DESIGN_W=1280, DESIGN_H=720;
    private final FrameLayout content;
    private final VideoView video;
    private final List<Hotspot> hotspots=new ArrayList<>();
    private boolean released=false;

    private static final class Hotspot {
        final Button view;
        final float x,y,w,h;
        Hotspot(Button v,float x,float y,float w,float h){
            view=v;this.x=x;this.y=y;this.w=w;this.h=h;
        }
    }
    public static AcceptedWonderwerkMenu tryCreate(Activity host,
          Runnable onNew,Runnable onSettings,Runnable onCreator,Runnable onExit){
        try(InputStream v=host.getAssets().open(VIDEO);
            InputStream p=host.getAssets().open(POSTER)){
            if(v.read()==-1||p.read()==-1)return null;
            return new AcceptedWonderwerkMenu(host,onNew,onSettings,onCreator,onExit);
        }catch(IOException e){
            android.util.Log.w("TAKS_MENU","Accepted media absent; previous menu remains available",e);
            return null;
        }
    }
    private AcceptedWonderwerkMenu(Activity host,
            Runnable onNew,Runnable onSettings,Runnable onCreator,Runnable onExit)
            throws IOException {
        super(host);
        setBackgroundColor(0xFF21180F);
        ImageView edges=new ImageView(host);
        Bitmap backdrop;
        try(InputStream in=host.getAssets().open(POSTER)){
            backdrop=BitmapFactory.decodeStream(in);
        }
        if(backdrop==null)throw new IOException("Missing approved poster");
        edges.setImageBitmap(backdrop);
        edges.setScaleType(ImageView.ScaleType.CENTER_CROP);
        edges.setColorFilter(0x99201612,PorterDuff.Mode.MULTIPLY);
        addView(edges,new FrameLayout.LayoutParams(-1,-1));
        content=new FrameLayout(host);
        ImageView still=new ImageView(host);
        still.setImageBitmap(backdrop);
        still.setScaleType(ImageView.ScaleType.FIT_XY);
        content.addView(still,new FrameLayout.LayoutParams(-1,-1));
        video=new VideoView(host);
        video.setBackgroundColor(Color.TRANSPARENT);
        video.setOnPreparedListener(mp->{
            mp.setLooping(true);
            mp.setVolume(0f,0f);
            if(!released)video.start();
        });
        video.setOnErrorListener((mp,what,extra)->{
            android.util.Log.w("TAKS_MENU","Video unavailable; keep approved static poster");
            video.setVisibility(View.GONE);return true;
        });
        content.addView(video,new FrameLayout.LayoutParams(-1,-1));
        addHit(host,"Új Játék",.390f,.442f,.280f,.116f,onNew);
        addHit(host,"Beállítások",.390f,.558f,.280f,.119f,onSettings);
        addHit(host,"Készítő",.390f,.685f,.280f,.115f,onCreator);
        addHit(host,"Kilépés",.390f,.806f,.280f,.117f,onExit);
        addView(content,new FrameLayout.LayoutParams(-1,-1,Gravity.CENTER));
        File mp4=copyToCache(host.getAssets(),VIDEO,
            new File(host.getCacheDir(),"TAKS_MENU_30FPS_APPROVED.mp4"));
        video.setVideoURI(Uri.fromFile(mp4));
    }
    private void addHit(Activity host,String label,float x,float y,float w,float h,
                        Runnable action){
        Button b=new Button(host);
        b.setText("");
        b.setContentDescription(label);
        b.setAllCaps(false);
        b.setBackground(new RippleDrawable(ColorStateList.valueOf(0x77F4C979),
                           new ColorDrawable(Color.TRANSPARENT),null));
        b.setOnClickListener(v->{if(!released)action.run();});
        b.setSoundEffectsEnabled(true);
        hotspots.add(new Hotspot(b,x,y,w,h));
        content.addView(b,new FrameLayout.LayoutParams(1,1));
    }
    private static File copyToCache(AssetManager asset,String path,File target)
            throws IOException{
        long expected=0;
        try(InputStream scan=asset.open(path)){
            byte[] block=new byte[65536];int n;
            while((n=scan.read(block))!=-1)expected+=n;
        }
        if(expected<1000)throw new IOException("Empty approved video");
        if(target.isFile()&&target.length()==expected)return target;
        File tmp=new File(target.getPath()+".tmp");
        try(InputStream in=asset.open(path);
            FileOutputStream out=new FileOutputStream(tmp)){
            byte[] block=new byte[32768];int n;
            while((n=in.read(block))!=-1)out.write(block,0,n);
            out.getFD().sync();
        }
        if(target.exists()&&!target.delete())throw new IOException("Cannot replace old menu cache");
        if(!tmp.renameTo(target))throw new IOException("Cannot install approved menu cache");
        return target;
    }
    @Override protected void onSizeChanged(int w,int h,int ow,int oh){
        super.onSizeChanged(w,h,ow,oh);
        if(w<=0||h<=0)return;
        float s=Math.min(w/(float)DESIGN_W,h/(float)DESIGN_H);
        int cw=Math.round(DESIGN_W*s),ch=Math.round(DESIGN_H*s);
        content.setLayoutParams(new FrameLayout.LayoutParams(cw,ch,Gravity.CENTER));
        for(Hotspot p:hotspots){
            FrameLayout.LayoutParams params=new FrameLayout.LayoutParams(
                Math.round(cw*p.w),Math.round(ch*p.h),Gravity.TOP|Gravity.LEFT);
            params.leftMargin=Math.round(cw*p.x);
            params.topMargin=Math.round(ch*p.y);
            p.view.setLayoutParams(params);
        }
    }
    public void play(){if(!released&&video.getVisibility()==View.VISIBLE)video.start();}
    public void pause(){if(!released&&video.isPlaying())video.pause();}
    public void release(){
        if(released)return;
        released=true;
        video.stopPlayback();
    }
    @Override protected void onDetachedFromWindow(){
        release();
        super.onDetachedFromWindow();
    }
}
