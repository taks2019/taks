package hu.taks.world;

import android.app.Activity;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.RippleDrawable;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * One continuously living menu surface. A single immutable 1664x768 user-
 * approved photograph backs a weather-driven overlay, never a video or a
 * timed still/video swap. Four native Android buttons receive the input.
 *
 * Their captions are GUARANTEED visible on top through a separate final
 * non-interactive Canvas text layer: no codec, SurfaceView, elevation or
 * background can cover the Hungarian captions.
 */
public final class AcceptedWonderwerkMenu extends FrameLayout {
    public static final String TEXT_FREE="menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png";
    private static final int DESIGN_W=1664,DESIGN_H=768;
    private final FrameLayout content;
    private final LivingWonderwerkOverlay environment;
    private final MenuCaptionView captions;
    private final List<Hotspot> hotspots=new ArrayList<>();
    private final EditableGameScript labels;
    private final MainMenuActionRouter actions;
    private boolean released=false;

    private static final String[] KEYS={"new_game","settings","creator","exit"};
    private static final String[] DEFAULTS={"Új Játék","Beállítások","Készítő","Kilépés"};
    // Real stone coordinates in the approved source. Never change the master.
    private static final float[] CAPTION_Y={387f,479f,573f,668f};
    private static final float CAPTION_X=928f;
    // Icon ends at ~805; the natural stone ends at ~1065.
    // 205 design pixels (+ outline) never protrude beyond either edge.
    private static final float CAPTION_MAX_WIDTH=205f;

    private static final class Hotspot {
        final Button control;
        final float x,y,w,h;
        Hotspot(Button c,float px,float py,float width,float height){
            control=c;x=px;y=py;w=width;h=height;
        }
    }

    public static AcceptedWonderwerkMenu tryCreate(Activity host,
            EditableGameScript script,Runnable onNew,Runnable onSettings,
            Runnable onCreator,Runnable onExit){
        try(InputStream photo=host.getAssets().open(TEXT_FREE)){
            if(photo.read()==-1)return null;
            return new AcceptedWonderwerkMenu(host,script,onNew,onSettings,
                                              onCreator,onExit);
        }catch(IOException e){
            android.util.Log.e("TAKS_MENU","Accepted text-free menu photo missing",e);
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
            throw new IOException("Accepted original full-screen menu photo is incomplete");

        content=new FrameLayout(host);
        ImageView still=new ImageView(host);
        still.setImageBitmap(backdrop);
        still.setScaleType(ImageView.ScaleType.FIT_XY);
        content.addView(still,new FrameLayout.LayoutParams(-1,-1));

        environment=new LivingWonderwerkOverlay(host);
        environment.setEditableScript(labels);
        content.addView(environment,new FrameLayout.LayoutParams(-1,-1));

        // Native Buttons own all click targets and accessibility semantics.
        addButton(host,"new_game",.390f,.442f,.280f,.116f);
        addButton(host,"settings",.390f,.558f,.280f,.119f);
        addButton(host,"creator",.390f,.685f,.280f,.115f);
        addButton(host,"exit",.390f,.806f,.280f,.117f);

        // Added LAST to the Android drawing order, but deliberately untouchable.
        // The four button rectangles stay clickable beneath the text.
        captions=new MenuCaptionView(host);
        captions.setClickable(false);
        captions.setFocusable(false);
        captions.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        content.addView(captions,new FrameLayout.LayoutParams(-1,-1));
        addView(content,new FrameLayout.LayoutParams(-1,-1,Gravity.CENTER));
    }

    private String titleFor(String key){
        for(int i=0;i<KEYS.length;++i){
            if(KEYS[i].equals(key)){
                String text=labels.get("menu",key,DEFAULTS[i]);
                return text==null||text.trim().isEmpty()?DEFAULTS[i]:text;
            }
        }
        return key;
    }
    private void addButton(Activity host,String key,float x,float y,float w,float h){
        if(!actions.supports(key))throw new IllegalStateException("Missing menu function "+key);
        Button button=new Button(host);
        button.setTag(key);
        button.setText(titleFor(key));
        button.setAllCaps(false);
        button.setSingleLine(true);
        // The single visible caption is drawn on the guaranteed top overlay.
        // Keep the button's text for Android's accessibility system.
        button.setTextColor(Color.TRANSPARENT);
        button.setBackground(new RippleDrawable(ColorStateList.valueOf(0x66D8B16C),
                             new ColorDrawable(Color.TRANSPARENT),null));
        button.setContentDescription(button.getText().toString());
        button.setOnClickListener(v->{if(!released)actions.run(key);});
        button.setSoundEffectsEnabled(true);
        hotspots.add(new Hotspot(button,x,y,w,h));
        content.addView(button,new FrameLayout.LayoutParams(1,1));
    }

    /**
     * Visuals from exactly the same source as the accessible buttons.
     * Opaque light lettering + a stone-dark outer contour; visible in sun,
     * cloud and shadow. Nothing is baked into the user's original emblem.
     */
    private final class MenuCaptionView extends View {
        private final Paint type=new Paint(Paint.ANTI_ALIAS_FLAG|
                                            Paint.SUBPIXEL_TEXT_FLAG);
        MenuCaptionView(Activity host){super(host);setWillNotDraw(false);}
        @Override protected void onDraw(Canvas c){
            super.onDraw(c);
            if(released||getWidth()==0||getHeight()==0)return;
            c.save();
            c.scale(getWidth()/(float)DESIGN_W,getHeight()/(float)DESIGN_H);
            type.setTypeface(Typeface.create(Typeface.SERIF,Typeface.BOLD));
            type.setTextAlign(Paint.Align.CENTER);
            for(int i=0;i<KEYS.length;++i){
                String title=titleFor(KEYS[i]).toUpperCase(java.util.Locale.forLanguageTag("hu-HU"));
                type.setTextSize(35f);
                float width=type.measureText(title);
                if(width>CAPTION_MAX_WIDTH)
                    type.setTextSize(35f*CAPTION_MAX_WIDTH/width);
                // Truly long user-edited captions are still bounded inside
                // the stone; never enforce a minimum size that overflows.
                width=type.measureText(title);
                if(width>CAPTION_MAX_WIDTH+0.5f)
                    type.setTextSize(type.getTextSize()*CAPTION_MAX_WIDTH/width);
                type.clearShadowLayer();
                type.setStyle(Paint.Style.STROKE);
                type.setStrokeWidth(6.5f);
                type.setStrokeJoin(Paint.Join.ROUND);
                type.setColor(0xFF38240F);
                c.drawText(title,CAPTION_X,CAPTION_Y[i],type);
                type.setStyle(Paint.Style.FILL);
                type.setColor(0xFFFFF1CD);
                type.setShadowLayer(1.3f,0f,1.5f,0xFF3A2712);
                c.drawText(title,CAPTION_X,CAPTION_Y[i],type);
            }
            c.restore();
        }
    }

    @Override protected void onSizeChanged(int w,int h,int oldw,int oldh){
        super.onSizeChanged(w,h,oldw,oldh);
        if(w<=0||h<=0)return;
        float s=Math.min(w/(float)DESIGN_W,h/(float)DESIGN_H);
        int cw=Math.max(1,Math.round(DESIGN_W*s));
        int ch=Math.max(1,Math.round(DESIGN_H*s));
        content.setLayoutParams(new FrameLayout.LayoutParams(cw,ch,Gravity.CENTER));
        for(Hotspot item:hotspots){
            FrameLayout.LayoutParams params=new FrameLayout.LayoutParams(
                Math.round(cw*item.w),Math.round(ch*item.h),
                Gravity.TOP|Gravity.LEFT);
            params.leftMargin=Math.round(cw*item.x);
            params.topMargin=Math.round(ch*item.y);
            item.control.setLayoutParams(params);
            item.control.setTextSize(TypedValue.COMPLEX_UNIT_SP,12);
        }
        captions.invalidate();
    }

    public void refreshText(){
        environment.setEditableScript(labels);
        for(Hotspot h:hotspots){
            String updated=titleFor((String)h.control.getTag());
            h.control.setText(updated);
            h.control.setContentDescription(updated);
        }
        captions.invalidate();
    }
    public void setClimate(MenuClimate next){if(!released)environment.setClimate(next);}
    public void play(){if(!released)environment.resume();}
    public void pause(){if(!released)environment.pause();}
    public void release(){
        if(released)return;
        released=true;
        environment.release();
    }
    @Override protected void onDetachedFromWindow(){
        release();super.onDetachedFromWindow();
    }
}
