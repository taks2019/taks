package hu.taks.world;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.graphics.Matrix;
import android.graphics.SurfaceTexture;
import android.media.MediaPlayer;
import android.view.Surface;
import android.view.TextureView;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

/** Silent, looping MP4 scenery BEHIND the unchanged main-menu controls.
 * TextureView permits correct Android view stacking and center-crop.
 * The old background image stays underneath until video actually renders.
 */
final class MenuMotionView extends TextureView implements TextureView.SurfaceTextureListener {
    private static final String ASSET = "menu/wonderwerk_loop.mp4";
    private MediaPlayer player;
    private Surface surface;
    private boolean prepared = false;
    private boolean playing = true;
    private int videoWidth = 960, videoHeight = 540;

    static boolean hasVideo(Context context) {
        try (InputStream stream = context.getAssets().open(ASSET)) {
            return stream.read() >= 0;
        } catch (Exception missing) {
            return false;
        }
    }

    MenuMotionView(Context context) {
        super(context);
        setOpaque(false);
        setAlpha(0f);
        setSurfaceTextureListener(this);
        setContentDescription("Animált, hangtalan Wonderwerk-táj a főmenü hátterében");
    }

    void setPlaying(boolean enabled) {
        playing = enabled;
        if (player == null || !prepared) return;
        try {
            if (playing) player.start(); else if (player.isPlaying()) player.pause();
        } catch (IllegalStateException badState) {
            android.util.Log.w("TAKS_MENU", "Cannot change video playback state", badState);
        }
    }

    private void configureSource(MediaPlayer mp) throws Exception {
        try (AssetFileDescriptor asset = getContext().getAssets().openFd(ASSET)) {
            mp.setDataSource(asset.getFileDescriptor(), asset.getStartOffset(), asset.getLength());
            return;
        } catch (java.io.IOException compressedAsset) {
            // Some APK packagers compress MP4 assets; copy once to private cache.
            File local = new File(getContext().getCacheDir(), "taks_wonderwerk_menu_motion.mp4");
            if (!local.isFile() || local.length() < 12000) {
                try (InputStream input = getContext().getAssets().open(ASSET);
                     FileOutputStream output = new FileOutputStream(local)) {
                    byte[] chunk = new byte[65536];
                    int n;
                    while ((n = input.read(chunk)) != -1) output.write(chunk, 0, n);
                }
            }
            mp.setDataSource(local.getAbsolutePath());
        }
    }

    private void applyCrop() {
        if (getWidth() <= 0 || getHeight() <= 0 || videoWidth <= 0 || videoHeight <= 0) return;
        float scale = Math.max((float) getWidth() / videoWidth,
                               (float) getHeight() / videoHeight);
        float sx = scale * videoWidth / getWidth();
        float sy = scale * videoHeight / getHeight();
        Matrix crop = new Matrix();
        crop.setScale(sx, sy, getWidth() * .5f, getHeight() * .5f);
        setTransform(crop);
    }

    @Override public void onSurfaceTextureAvailable(SurfaceTexture texture, int w, int h) {
        release();
        surface = new Surface(texture);
        try {
            final MediaPlayer mp = new MediaPlayer();
            player = mp;
            configureSource(mp);
            mp.setSurface(surface);
            mp.setVolume(0f, 0f);
            mp.setLooping(true);
            mp.setOnVideoSizeChangedListener((p, width, height) -> {
                videoWidth = width;
                videoHeight = height;
                applyCrop();
            });
            mp.setOnPreparedListener(p -> {
                prepared = true;
                applyCrop();
                if (playing) p.start();
            });
            mp.setOnInfoListener((p, what, extra) -> {
                if (what == MediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START) {
                    animate().alpha(1f).setDuration(320).start();
                }
                return false;
            });
            mp.setOnErrorListener((p, what, extra) -> {
                android.util.Log.e("TAKS_MENU", "Animated menu MP4 playback error: " + what + " / " + extra);
                setAlpha(0f); // Static approved menu image remains visible behind video.
                release();
                return true;
            });
            mp.prepareAsync();
        } catch (Exception error) {
            android.util.Log.w("TAKS_MENU", "Animated menu unavailable; using still fallback", error);
            setAlpha(0f);
            release();
        }
    }

    @Override public void onSurfaceTextureSizeChanged(SurfaceTexture texture, int w, int h) {
        applyCrop();
    }

    @Override public boolean onSurfaceTextureDestroyed(SurfaceTexture texture) {
        release();
        return true;
    }

    @Override public void onSurfaceTextureUpdated(SurfaceTexture texture) { }

    void release() {
        animate().cancel();
        prepared = false;
        if (player != null) {
            try { player.reset(); } catch (IllegalStateException ignored) { }
            player.release();
            player = null;
        }
        if (surface != null) { surface.release(); surface = null; }
    }

    @Override protected void onDetachedFromWindow() {
        release();
        super.onDetachedFromWindow();
    }
}
