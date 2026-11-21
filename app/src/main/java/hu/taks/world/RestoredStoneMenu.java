package hu.taks.world;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.RectF;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;
import java.io.InputStream;

/**
 * The EXACT approved, full-screen 1664x768 stone menu.
 *
 * Regression fix: the previous release placed the only ImageView inside a
 * FrameLayout initially sized 1x1 pixel; on some Android compositors the
 * container stayed 1x1, so only its brown fallback background showed.
 *
 * The approved art and all four captions are now drawn directly to the
 * actual MATCH_PARENT menu view in onDraw(). There are no dependent 1x1
 * artwork containers and no MediaPlayer or invented fallback image.
 *
 * Native Android transparent hit targets preserve touch/accessibility.
 * The original approved PNG bytes are never altered.
 */
final class RestoredStoneMenu extends FrameLayout {
    static final String ASSET = "menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png";
    private static final int DESIGN_W = 1664, DESIGN_H = 768;
    private static final float[] TOP = {.443f,.566f,.687f,.809f};
    private static final float[] HEIGHT = {.120f,.120f,.115f,.120f};
    private static final String[] LABELS = {"ÚJ JÁTÉK","BEÁLLÍTÁSOK","KÉSZÍTŐ","KILÉPÉS"};
    private static final float[] CAPTION_Y = {387f,479f,573f,668f};
    private static final float CAPTION_X = 928f;
    private static final float CAPTION_MAX_WIDTH = 176f;
    private final Bitmap accepted;
    private final Paint imagePaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final Paint typePaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.SUBPIXEL_TEXT_FLAG);
    private final View[] touch = new View[4];
    private float scale = 1f, offsetX = 0f, offsetY = 0f;
    private boolean firstFrameLogged = false;

    static RestoredStoneMenu load(Context context, Runnable newGame,
                                  Runnable settings, Runnable creator, Runnable exit) {
        try(InputStream input = context.getAssets().open(ASSET)) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inScaled = false;
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            Bitmap image = BitmapFactory.decodeStream(input, null, options);
            if(image == null || image.getWidth() != DESIGN_W ||
               image.getHeight() != DESIGN_H){
                android.util.Log.e("TAKS_MENU","Approved original menu dimensions are invalid");
                if(image != null) image.recycle();
                return null;
            }
            return new RestoredStoneMenu(context,image,
                      new Runnable[]{newGame,settings,creator,exit});
        }catch(Exception error){
            android.util.Log.e("TAKS_MENU","Approved exact menu artwork not accessible",error);
            return null;
        }
    }

    private RestoredStoneMenu(Context context, Bitmap image, Runnable[] actions){
        super(context);
        accepted=image;
        setBackgroundColor(0xFF21180F);
        setWillNotDraw(false); // Mandatory for direct full-screen image rendering.
        setClipChildren(false);
        setContentDescription("TAKS WORLD: az eredeti kőlogós főmenü");
        typePaint.setTypeface(Typeface.create(Typeface.SERIF,Typeface.BOLD));
        typePaint.setTextAlign(Paint.Align.CENTER);
        for(int i=0;i<4;++i){
            final int index=i;
            View target=new View(context);
            target.setContentDescription(LABELS[i]);
            target.setClickable(true);
            target.setFocusable(true);
            target.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
            target.setBackgroundColor(Color.TRANSPARENT);
            target.setElevation(0f);
            target.setOnClickListener(v->{
                android.util.Log.i("TAKS_MENU_TOUCH",LABELS[index]+" pressed on approved original image");
                actions[index].run();
            });
            touch[i]=target;
            addView(target,new FrameLayout.LayoutParams(1,1,Gravity.TOP|Gravity.LEFT));
        }
    }

    private void geometry(int width, int height){
        if(width<=0||height<=0) return;
        scale=Math.min(width/(float)DESIGN_W,height/(float)DESIGN_H);
        offsetX=(width-DESIGN_W*scale)*.5f;
        offsetY=(height-DESIGN_H*scale)*.5f;
    }

    private int measuredTargetWidth=-1,measuredTargetHeight=-1;

    // The original implementation resized the touch targets in
    // onSizeChanged(), AFTER FrameLayout had measured the initial 1x1
    // children. On a real 2400x1080 Android emulator the descendants
    // therefore stayed ONE PIXEL wide, despite the correct menu artwork.
    // Install the exact same four hitboxes BEFORE FrameLayout measures.
    @Override protected void onMeasure(int widthMeasureSpec,int heightMeasureSpec){
        int width=View.MeasureSpec.getSize(widthMeasureSpec);
        int height=View.MeasureSpec.getSize(heightMeasureSpec);
        if(width>0&&height>0 &&
           (width!=measuredTargetWidth||height!=measuredTargetHeight)){
            geometry(width,height);
            for(int i=0;i<4;++i){
                FrameLayout.LayoutParams area=new FrameLayout.LayoutParams(
                    Math.max(1,Math.round(414f*scale)),
                    Math.max(1,Math.round(DESIGN_H*HEIGHT[i]*scale)),
                    Gravity.LEFT|Gravity.TOP);
                area.leftMargin=Math.round(offsetX+DESIGN_W*.396f*scale);
                area.topMargin=Math.round(offsetY+DESIGN_H*TOP[i]*scale);
                touch[i].setLayoutParams(area);
                android.util.Log.i("TAKS_MENU_TOUCH","Measured hitbox "+LABELS[i]+
                    " x="+area.leftMargin+" y="+area.topMargin+
                    " w="+area.width+" h="+area.height);
            }
            measuredTargetWidth=width;
            measuredTargetHeight=height;
        }
        super.onMeasure(widthMeasureSpec,heightMeasureSpec);
    }

    @Override protected void onSizeChanged(int width,int height,int oldWidth,int oldHeight){
        super.onSizeChanged(width,height,oldWidth,oldHeight);
        geometry(width,height);
        invalidate();
    }

    @Override protected void onLayout(boolean changed,int l,int t,int r,int b){
        super.onLayout(changed,l,t,r,b);
        for(int i=0;i<touch.length;++i){
            if(touch[i].getWidth()<30||touch[i].getHeight()<30){
                android.util.Log.e("TAKS_MENU_TOUCH","INVALID real Android hitbox "+
                    LABELS[i]+" "+touch[i].getWidth()+"x"+touch[i].getHeight());
            }
        }
    }

    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        if(accepted==null||accepted.isRecycled()) return;
        if(getWidth()<=0||getHeight()<=0) return;
        geometry(getWidth(),getHeight());
        // Pixel-identical original art; it is rendered even when any child
        // touch target is mismeasured or an Android hardware layer is stale.
        canvas.save();
        canvas.translate(offsetX,offsetY);
        canvas.scale(scale,scale);
        canvas.drawBitmap(accepted,0f,0f,imagePaint);

        for(int i=0;i<LABELS.length;++i){
            float size=28f;
            typePaint.setTextSize(size);
            float measured=typePaint.measureText(LABELS[i]);
            if(measured>CAPTION_MAX_WIDTH) size*=CAPTION_MAX_WIDTH/measured;
            typePaint.setTextSize(size);
            canvas.save();
            canvas.clipRect(CAPTION_X-95f,CAPTION_Y[i]-34f,
                            CAPTION_X+95f,CAPTION_Y[i]+12f);
            typePaint.setStyle(Paint.Style.STROKE);
            typePaint.setStrokeWidth(3.2f);
            typePaint.setStrokeJoin(Paint.Join.ROUND);
            typePaint.clearShadowLayer();
            typePaint.setColor(0xFF33210F);
            canvas.drawText(LABELS[i],CAPTION_X,CAPTION_Y[i],typePaint);
            typePaint.setStyle(Paint.Style.FILL);
            typePaint.setColor(0xFFFFF1CF);
            typePaint.setShadowLayer(1.2f,0,1,0xFF34220D);
            canvas.drawText(LABELS[i],CAPTION_X,CAPTION_Y[i],typePaint);
            canvas.restore();
        }
        canvas.restore();
        if(!firstFrameLogged){
            firstFrameLogged=true;
            android.util.Log.i("TAKS_MENU","Original approved 1664x768 menu actually DRAWN at "
                +getWidth()+"x"+getHeight()+" scale="+scale);
        }
    }

    @Override protected void onDetachedFromWindow(){
        super.onDetachedFromWindow();
        if(accepted!=null && !accepted.isRecycled()) accepted.recycle();
    }
}
