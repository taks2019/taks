package hu.taks.world;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Typeface;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;
import java.io.InputStream;

/**
 * Restores the actual previously accepted 1664x768 Stone Age menu image.
 * The menu is NOT synthesized from the Wonderwerk cave and never uses MP4.
 * Only four Android touch targets and their captions are drawn above the
 * original unmodified image. All game callbacks remain native.
 */
final class RestoredStoneMenu extends FrameLayout {
    static final String ASSET = "menu/WONDERWERK_APPROVED_WIDE_NO_TEXT.png";
    private static final int DESIGN_W = 1664, DESIGN_H = 768;
    private static final float[] TOP = {.443f,.566f,.687f,.809f};
    private static final float[] HEIGHT = {.120f,.120f,.115f,.120f};
    private static final String[] LABELS = {"ÚJ JÁTÉK","BEÁLLÍTÁSOK","KÉSZÍTŐ","KILÉPÉS"};

    private final FrameLayout artwork;
    private final View[] touch = new View[4];
    private final TextView[] caption = new TextView[4];

    static RestoredStoneMenu load(Context context, Runnable newGame,
                                  Runnable settings, Runnable creator, Runnable exit) {
        try (InputStream stream = context.getAssets().open(ASSET)) {
            Bitmap image = BitmapFactory.decodeStream(stream);
            if (image == null || image.getWidth() != DESIGN_W ||
                image.getHeight() != DESIGN_H) return null;
            return new RestoredStoneMenu(context, image,
                                         new Runnable[]{newGame,settings,creator,exit});
        } catch (Exception error) {
            android.util.Log.e("TAKS_MENU", "Eredeti jóváhagyott menükép hiányzik", error);
            return null;
        }
    }

    private RestoredStoneMenu(Context context, Bitmap image, Runnable[] actions) {
        super(context);
        setBackgroundColor(0xFF21180F);
        artwork = new FrameLayout(context);
        ImageView photo = new ImageView(context);
        photo.setImageBitmap(image);
        photo.setScaleType(ImageView.ScaleType.FIT_XY);
        artwork.addView(photo, new FrameLayout.LayoutParams(-1,-1));
        for (int i=0;i<4;i++) {
            final int buttonIndex = i;
            View target = new View(context);
            target.setContentDescription(LABELS[i]);
            target.setClickable(true);
            target.setFocusable(true);
            target.setOnClickListener(v -> actions[buttonIndex].run());
            touch[i] = target;
            artwork.addView(target, new FrameLayout.LayoutParams(1,1));

            TextView text = new TextView(context);
            text.setText(LABELS[i]);
            text.setTypeface(Typeface.SERIF,Typeface.BOLD);
            text.setTextColor(0xFFFFF0CB);
            text.setShadowLayer(2f,1f,2f,0xFF38240F);
            text.setGravity(Gravity.CENTER);
            text.setTextSize(19f);
            text.setSingleLine(true);
            text.setClickable(false);
            text.setFocusable(false);
            caption[i]=text;
            artwork.addView(text,new FrameLayout.LayoutParams(1,1));
        }
        addView(artwork,new FrameLayout.LayoutParams(1,1,Gravity.CENTER));
    }

    @Override protected void onSizeChanged(int w,int h,int oldw,int oldh) {
        super.onSizeChanged(w,h,oldw,oldh);
        if (w<=0||h<=0) return;
        float scale=Math.min(w/(float)DESIGN_W,h/(float)DESIGN_H);
        int cw=Math.max(1,Math.round(DESIGN_W*scale));
        int ch=Math.max(1,Math.round(DESIGN_H*scale));
        artwork.setLayoutParams(new FrameLayout.LayoutParams(cw,ch,Gravity.CENTER));
        for (int i=0;i<4;i++) {
            FrameLayout.LayoutParams hit=new FrameLayout.LayoutParams(
                    Math.round(cw*.249f),Math.round(ch*HEIGHT[i]));
            hit.leftMargin=Math.round(cw*.396f);
            hit.topMargin=Math.round(ch*TOP[i]);
            touch[i].setLayoutParams(hit);
            FrameLayout.LayoutParams title=new FrameLayout.LayoutParams(
                    Math.round(cw*.145f),Math.round(ch*HEIGHT[i]));
            title.leftMargin=Math.round(cw*.486f);
            title.topMargin=Math.round(ch*TOP[i]);
            caption[i].setLayoutParams(title);
            caption[i].setTextSize(android.util.TypedValue.COMPLEX_UNIT_PX,
                                   28f*scale);
        }
    }
}
