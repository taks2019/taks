package hu.taks.world;

import android.app.Activity;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.RippleDrawable;
import android.view.Gravity;
import android.view.View;
import android.util.TypedValue;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Real interactive TEXT over a TEXT-FREE approved Wonderwerk background.
 * The sacred logo remains raster and absolutely unchanged. Menu captions
 * are real, selectable/accessible Android TextView glyphs, sourced from a
 * UTF-8 JSON script. Nothing written into the menu photograph.
 */
public final class AcceptedWonderwerkMenu extends FrameLayout {
    public static final String TEXT_FREE="menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png";
    private static final int DESIGN_W=1664,DESIGN_H=768;
    private final FrameLayout content;
    private final LivingWonderwerkOverlay environment;
    private final List<Hotspot> hotspots=new ArrayList<>();
    private final EditableGameScript labels;
    private final MainMenuActionRouter actions;
    private boolean released=false;

    private static final class Hotspot {
        final Button control;
        final float x,y,w,h;
        Hotspot(Button c,float px,float py,float width,float height){
            control=c;x=px;y=py;w=width;h=height;
        }
    }
    public static AcceptedWonderwerkMenu tryCreate(Activity host,
        EditableGameScript labels,Runnable onNew,Runnable onSettings,
        Runnable onCreator,Runnable onExit){
        // NEVER fall back to baked-text art as though it were editable.
        try(InputStream p=host.getAssets().open(TEXT_FREE)){
            if(p.read()==-1)return null;
            return new AcceptedWonderwerkMenu(host,labels,onNew,onSettings,onCreator,onExit);
        }catch(IOException e){
            android.util.Log.e("TAKS_MENU","Text-free accepted background missing; not displaying false editable labels",e);
            return null;
        }
    }
    private AcceptedWonderwerkMenu(Activity host,EditableGameScript script,
            Runnable onNew,Runnable onSettings,Runnable onCreator,Runnable onExit)
            throws IOException{
        super(host);
        labels=script;
        actions=new MainMenuActionRouter(onNew,onSettings,onCreator,onExit);
        setBackgroundColor(0xFF21180F);
        Bitmap backdrop;
        try(InputStream in=host.getAssets().open(TEXT_FREE)){
            backdrop=BitmapFactory.decodeStream(in);
        }
        if(backdrop==null||backdrop.getWidth()!=DESIGN_W||backdrop.getHeight()!=DESIGN_H)
            throw new IOException("Approved 1664x768 wide text-free background missing");
        // Use the whole available Android landscape screen, not a centered
        // 16:9 plate with black/blurred side gutters. The approved source
        // bitmap itself stays untouched; FIT_XY is presentation only.
        content=new FrameLayout(host);
        ImageView still=new ImageView(host);
        still.setImageBitmap(backdrop);
        still.setScaleType(ImageView.ScaleType.FIT_XY);
        content.addView(still,new FrameLayout.LayoutParams(-1,-1));
        environment=new LivingWonderwerkOverlay(host);
        environment.setEditableScript(labels);
        content.addView(environment,new FrameLayout.LayoutParams(-1,-1));
        addButton(host,"new_game",.390f,.442f,.280f,.116f);
        addButton(host,"settings",.390f,.558f,.280f,.119f);
        addButton(host,"creator",.390f,.685f,.280f,.115f);
        addButton(host,"exit",.390f,.806f,.280f,.117f);
        addView(content,new FrameLayout.LayoutParams(-1,-1,Gravity.CENTER));
    }
    private void addButton(Activity host,String key,float x,float y,float w,float h){
        if(!actions.supports(key))throw new IllegalStateException("Missing functional button: "+key);
        Button button=new Button(host);
        button.setTag(key);
        button.setText(labels.get("menu",key,key));
        button.setAllCaps(false);
        button.setSingleLine(true);
        button.setTypeface(Typeface.create("serif",Typeface.BOLD));
        button.setTextColor(0xFFF9E3B6);
        button.setIncludeFontPadding(false);
        button.setGravity(Gravity.CENTER);
        button.setBackground(new RippleDrawable(ColorStateList.valueOf(0x46D8B16C),
                     new ColorDrawable(Color.TRANSPARENT),null));
        button.setContentDescription(button.getText().toString());
        button.setOnClickListener(v->{if(!released)actions.run(key);});
        button.setSoundEffectsEnabled(true);
        hotspots.add(new Hotspot(button,x,y,w,h));
        content.addView(button,new FrameLayout.LayoutParams(1,1));
    }
    @Override protected void onSizeChanged(int w,int h,int oldw,int oldh){
        super.onSizeChanged(w,h,oldw,oldh);
        if(w<=0||h<=0)return;
        float s=Math.min(w/(float)DESIGN_W,h/(float)DESIGN_H);
        int cw=w,ch=h;
        content.setLayoutParams(new FrameLayout.LayoutParams(cw,ch,Gravity.CENTER));
        for(Hotspot item:hotspots){
            Button b=item.control;
            FrameLayout.LayoutParams params=new FrameLayout.LayoutParams(
                Math.round(cw*item.w),Math.round(ch*item.h),Gravity.TOP|Gravity.LEFT);
            params.leftMargin=Math.round(cw*item.x);
            params.topMargin=Math.round(ch*item.y);
            b.setLayoutParams(params);
            // The original stone carving occupied the right of the icon.
            b.setPadding(Math.round(87*s),0,Math.round(4*s),0);
            float maxFont=Math.max(15f,50f*s);
            b.setTextSize(TypedValue.COMPLEX_UNIT_PX,maxFont);
            b.setShadowLayer(Math.max(1,2f*s),1*s,2*s,0xFF2E170C);
            b.setAutoSizeTextTypeUniformWithConfiguration(
                Math.max(13,Math.round(23*s)),Math.max(17,Math.round(50*s)),1,
                TypedValue.COMPLEX_UNIT_PX);
        }
    }
    public void refreshText(){
        environment.setEditableScript(labels);
        for(Hotspot h:hotspots){
            String text=labels.get("menu",(String)h.control.getTag(),"");
            h.control.setText(text);
            h.control.setContentDescription(text);
        }
    }
    public void setClimate(MenuClimate next){if(!released)environment.setClimate(next);}
    public void play(){if(!released)environment.resume();}
    public void pause(){if(!released)environment.pause();}
    public void release(){
        if(released)return;
        released=true;
        environment.release();
    }
    @Override protected void onDetachedFromWindow(){release();super.onDetachedFromWindow();}
}
