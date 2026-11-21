package hu.taks.world;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;
import java.io.InputStream;

/**
 * Review-only PNG landscape layer over the existing native GLES3 world.
 *
 * The underlying simulation, actors, savegame, weather and collision remain
 * native. The eight camera-facing external cave images are 2.5D sprites,
 * NOT a geological 3D scan and NOT accepted final art.
 *
 * If the review image assets are unavailable, gameplay keeps the original
 * renderer unchanged. No asset is ever implicitly approved.
 */
public final class IsometricCaveOverlay extends View {
    private static final String[] DEGREES = {
            "000", "045", "090", "135", "180", "225", "270", "315"
    };
    private final Bitmap[] views = new Bitmap[8];
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private boolean enabled = true;
    private boolean loaded = false;
    private float yaw = -.785398f, pitch = .615480f, zoom = 19f;
    private float playerX = 0f, playerZ = -5.2f;

    public IsometricCaveOverlay(Context context) {
        super(context);
        setClickable(false);
        setFocusable(false);
        setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        boolean ready = true;
        for (int i=0; i<8; ++i) {
            try (InputStream stream=context.getAssets().open("wonderwerk_png/" + DEGREES[i] + ".png")) {
                BitmapFactory.Options opts = new BitmapFactory.Options();
                opts.inScaled = false;
                views[i] = BitmapFactory.decodeStream(stream, null, opts);
                ready &= views[i] != null && views[i].getWidth() >= 256;
            } catch (Exception missing) {
                ready = false;
            }
        }
        if (!ready) {
            for (int i=0; i<8; ++i) {
                if (views[i] != null) {
                    views[i].recycle();
                    views[i] = null;
                }
            }
        }
        loaded = ready;
        paint.setDither(true);
        setContentDescription("Nyolcirányú, jóváhagyásra váró Wonderwerk PNG barlang-előnézet");
    }

    public boolean hasImages() { return loaded; }
    public boolean isPreviewEnabled() { return enabled && loaded; }
    public void setPreviewEnabled(boolean value) { enabled = value; invalidate(); }
    public void setCamera(float yaw, float pitch, float zoom, float playerX, float playerZ) {
        this.yaw = yaw;
        this.pitch = pitch;
        this.zoom = zoom;
        this.playerX = playerX;
        this.playerZ = playerZ;
        invalidate();
    }

    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (!enabled || !loaded || getWidth() <= 0 || getHeight() <= 0) return;
        // Native RoofCutaway: invisible at close zoom 8, opaque by zoom 14.
        float alpha = Math.max(0f, Math.min(1f, (zoom - 8f) / 6f));
        if (alpha < .005f) return;

        // A fixed point ON THE CAVE is used for all 8 views. Previous
        // versions changed the apparent sprite centre when switching angle;
        // the cave jumped across the screen while the camera was rotated.
        final float anchorX = 0f;
        final float anchorZ = 7.4f;
        float[] camera = projectWorld(anchorX, surfaceHeight(anchorX,anchorZ), anchorZ);
        float screenScale = getHeight() / Math.max(zoom * .99f, .01f);
        float renderedWidth = 21.5f * screenScale;
        float renderedHeight = renderedWidth;

        // All eight supplied RGBA images are 512x512. Their image-space
        // ground anchor is x=256,y=428, regardless of silhouette width.
        // Keep one identical rectangle for all views, no auto-cropping.
        final RectF dst = new RectF(camera[0] - renderedWidth*.5f,
                                   camera[1] - renderedHeight*(428f/512f),
                                   camera[0] + renderedWidth*.5f,
                                   camera[1] + renderedHeight*(84f/512f));

        // Two neighbouring 45° frames are blended continuously rather than
        // snapping to a new static frame at an angle boundary. This does not
        // claim to be true 3D: it only makes this review sprite turn smoothly.
        final double direction = (double)yaw / (Math.PI/4.0);
        final double base = Math.floor(direction);
        final int first = Math.floorMod((int)base,8);
        final int second = (first+1)%8;
        final float blend = (float)(direction-base);
        paint.setAlpha(Math.round(alpha*(1f-blend)*255));
        if(paint.getAlpha()>0)canvas.drawBitmap(views[first],null,dst,paint);
        if(blend>.001f){
            paint.setAlpha(Math.round(alpha*blend*255));
            if(paint.getAlpha()>0)canvas.drawBitmap(views[second],null,dst,paint);
        }
    }

    private static float saturate(float f){return Math.max(0f,Math.min(1f,f));}
    private static float surfaceHeight(float x,float z){
        // Identical height expression to the native FastScene renderer:
        // mismatched Java/C++ camera origins caused parallax and shifting.
        float edge = saturate((11f-Math.abs(x))/2.4f) *
                     saturate((z+11f)/2.4f) * saturate((17f-z)/2.4f);
        float relief = -.27f+.17f*(float)Math.sin(x*.21f)*(float)Math.cos(z*.22f)
                +.09f*(float)Math.sin(x*.87f+z*.57f)
                +.065f*(float)Math.cos(z*.73f-x*.42f);
        return -.48f+(relief+.48f)*edge;
    }

    private float[] projectWorld(float x, float y, float z) {
        float fy = surfaceHeight(playerX,playerZ)+.90f;
        float fz = playerZ + 1.65f + Math.max(0f, Math.min(3f, (zoom - 10f)*.29f));
        float dx=x-playerX, dy=y-fy, dz=z-fz;
        // lookAt() uses cross(forward, UP), not the opposite direction.
        float rightX=-(float)Math.cos(yaw), rightZ=-(float)Math.sin(yaw);
        float upX=-(float)Math.sin(yaw)*(float)Math.sin(pitch);
        float upY=(float)Math.cos(pitch);
        float upZ=(float)Math.cos(yaw)*(float)Math.sin(pitch);
        float scale=getHeight() / Math.max(zoom * .99f, .01f);
        return new float[]{
            getWidth()*.5f + (dx*rightX+dz*rightZ)*scale,
            getHeight()*.5f - (dx*upX+dy*upY+dz*upZ)*scale
        };
    }

    public void release() {
        for (int i=0; i<8; ++i) {
            if (views[i] != null && !views[i].isRecycled()) views[i].recycle();
            views[i] = null;
        }
        loaded = false;
    }
}
