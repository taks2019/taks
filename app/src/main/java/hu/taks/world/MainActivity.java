package hu.taks.world;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public final class MainActivity extends Activity {
    static { System.loadLibrary("taks_engine"); }
    private native String nativeDo(String command);
    private native void nativeAdvance();
    private native String nativeState();
    private native String nativeSave();
    private native boolean nativeLoad(String data);
    private native void nativeReset();
    private native void nativeOnSurfaceCreated();
    private native void nativeRender();

    private GLSurfaceView gpuSurface;
    private TextView status;
    private TextView log;
    private SharedPreferences preferences;

    private Button button(String label, Runnable action){
        Button b=new Button(this);
        b.setText(label);
        b.setAllCaps(false);
        b.setOnClickListener(v->{action.run();updateStatus();});
        return b;
    }
    private void updateStatus(){ if(status!=null)status.setText(nativeState()); }
    private void perform(String command){
        log.setText(nativeDo(command));
        updateStatus();
    }
    @Override public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        preferences=getSharedPreferences("taks_native_save_v1",MODE_PRIVATE);
        String saved=preferences.getString("state",null);
        if(saved!=null && !nativeLoad(saved))nativeReset();
        gpuSurface=new GLSurfaceView(this);
        gpuSurface.setEGLContextClientVersion(3);
        gpuSurface.setRenderer(new GLSurfaceView.Renderer(){
            @Override public void onSurfaceCreated(javax.microedition.khronos.opengles.GL10 gl,javax.microedition.khronos.egl.EGLConfig config){nativeOnSurfaceCreated();}
            @Override public void onSurfaceChanged(javax.microedition.khronos.opengles.GL10 gl,int width,int height){android.opengl.GLES30.glViewport(0,0,width,height);}
            @Override public void onDrawFrame(javax.microedition.khronos.opengles.GL10 gl){nativeRender();}
        });
        gpuSurface.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        FrameLayout root=new FrameLayout(this);
        root.addView(gpuSurface,new FrameLayout.LayoutParams(-1,-1));
        ScrollView scroller=new ScrollView(this);
        FrameLayout.LayoutParams params=new FrameLayout.LayoutParams(-1,-1);
        params.setMargins(30,18,30,18);
        scroller.setFillViewport(true);
        LinearLayout panel=new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(12,4,12,8);
        TextView title=new TextView(this);title.setText("TAKS WORLD  ·  NATÍV MOTOR");title.setTextSize(21);title.setTextColor(Color.rgb(228,218,191));
        panel.addView(title);
        TextView warning=new TextView(this);
        warning.setText("FEJLESZTŐI MÉRFÖLDKŐ · Ez még nem a kész 3D játék. A natív oksági motort és az APK-fordítást ellenőrzi.");
        warning.setTextColor(Color.rgb(184,208,199));warning.setPadding(0,7,0,14);panel.addView(warning);
        status=new TextView(this);status.setTextColor(Color.WHITE);status.setTextSize(15);panel.addView(status);
        log=new TextView(this);log.setTextColor(Color.rgb(248,201,113));log.setPadding(0,14,0,12);log.setText("Minden művelet valós készletváltozással jár. A világállapot mentéskor megmarad.");panel.addView(log);
        LinearLayout actions=new LinearLayout(this);actions.setOrientation(LinearLayout.VERTICAL);
        actions.addView(button("Faág gyűjtése",()->perform("wood")));
        actions.addView(button("Kő gyűjtése",()->perform("stone")));
        actions.addView(button("Gyújtós gyűjtése",()->perform("tinder")));
        actions.addView(button("Tűzhely építése (3 fa + 2 kő)",()->perform("hearth")));
        actions.addView(button("Tűzgyújtás (2 fa + gyújtós)",()->perform("ignite")));
        actions.addView(button("Pihenés",()->perform("rest")));
        actions.addView(button("1 játékóra szimulálása",()->{nativeAdvance();log.setText("Eltelt 1 játékóra: a környezet és a tűz állapota megváltozott.");}));
        actions.addView(button("Mentés",()->{preferences.edit().putString("state",nativeSave()).apply();log.setText("Helyi mentés elkészült.");}));
        actions.addView(button("Új tesztvilág",()->{nativeReset();log.setText("Új tesztvilág indult. A korábbi mentést még nem írtam felül.");}));
        panel.addView(actions);
        scroller.addView(panel);
        root.addView(scroller,params);
        setContentView(root);
        updateStatus();
    }
    @Override protected void onResume(){super.onResume();if(gpuSurface!=null)gpuSurface.onResume();}
    @Override protected void onPause(){preferences.edit().putString("state",nativeSave()).apply();if(gpuSurface!=null)gpuSurface.onPause();super.onPause();}
}
