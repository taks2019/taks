package hu.taks.world;

import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;

/**
 * Native Android GUI material: authentic offline CC0 rock photographic surface
 * beneath a state-driven dark stone/earth translucent coating.
 * Not a fabricated game model, browser renderer, HTML or web canvas.
 * The photograph is loaded ONCE and shared by all cards/buttons.
 */
public final class LivingStoneDrawable extends Drawable {
    private final BitmapShader stone;
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
    private final RectF bounds=new RectF();
    private final Path clip=new Path();
    private final float radiusPx;
    private int top,bottom,stroke;
    private final float borderWidthPx;

    public LivingStoneDrawable(Bitmap authenticStone,int top,int bottom,
                               int stroke,float radiusPx,float borderWidthPx){
        this.top=top;this.bottom=bottom;this.stroke=stroke;
        this.radiusPx=Math.max(0,radiusPx);
        this.borderWidthPx=Math.max(1,borderWidthPx);
        if(authenticStone!=null && !authenticStone.isRecycled()){
            stone=new BitmapShader(authenticStone,Shader.TileMode.REPEAT,Shader.TileMode.REPEAT);
            Matrix matrix=new Matrix();
            matrix.setScale(.32f,.32f);
            stone.setLocalMatrix(matrix);
        } else stone=null;
    }
    public void palette(int a,int b,int outline){
        if(top==a && bottom==b && stroke==outline)return;
        top=a;bottom=b;stroke=outline;invalidateSelf();
    }
    @Override public void draw(Canvas canvas){
        Rect r=getBounds();
        if(r.width()<=0||r.height()<=0)return;
        bounds.set(r.left,r.top,r.right,r.bottom);
        clip.reset();
        clip.addRoundRect(bounds,radiusPx,radiusPx,Path.Direction.CW);
        canvas.save();
        canvas.clipPath(clip);
        paint.reset();
        paint.setAntiAlias(true);
        paint.setFilterBitmap(true);
        if(stone!=null){
            paint.setShader(stone);
            paint.setAlpha(242);
            canvas.drawRect(bounds,paint);
        }
        paint.setShader(new LinearGradient(r.left,r.top,r.left,r.bottom,
            top,bottom,Shader.TileMode.CLAMP));
        canvas.drawRect(bounds,paint);
        paint.setShader(null);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(borderWidthPx);
        paint.setColor(stroke);
        RectF inner=new RectF(bounds);
        inner.inset(borderWidthPx*.5f,borderWidthPx*.5f);
        canvas.drawRoundRect(inner,radiusPx,radiusPx,paint);
        canvas.restore();
    }
    @Override public void setAlpha(int alpha){
        paint.setAlpha(alpha);invalidateSelf();
    }
    @Override public void setColorFilter(ColorFilter filter){
        paint.setColorFilter(filter);invalidateSelf();
    }
    @Override public int getOpacity(){return PixelFormat.TRANSLUCENT;}
}
