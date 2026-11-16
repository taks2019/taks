package hu.taks.world;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

public final class MainActivity extends Activity {
    static { System.loadLibrary("taks_engine"); }
    private native String nativeDo(String action);
    private native void nativeAdvance();
    private native String nativeState();
    private native String nativeSave();
    private native boolean nativeLoad(String saved);
    private native void nativeReset();
    private native void nativeOnSurfaceCreated(String fragmentSource);
    private native void nativeRender();
    private native void nativeSetView(float angle,float zoom);
    private native String nativeRendererError();

    private GLSurfaceView surface;
    private SharedPreferences saves;
    private FrameLayout root;
    private LinearLayout sheet;
    private TextView notice;
    private TextView hint;
    private String shaderSource;
    private float orbit=0f,zoom=14f,lastX=0,lastY=0,lastSpan=0;
    private int activePanel=0;

    private int dp(float n){return Math.round(getResources().getDisplayMetrics().density*n);}
    private GradientDrawable panelBackground(int color,int radius){
        GradientDrawable d=new GradientDrawable();
        d.setColor(color);
        d.setCornerRadius(dp(radius));
        d.setStroke(dp(1),0x668E998B);
        return d;
    }
    private TextView caption(String content,int size,int color){
        TextView v=new TextView(this);
        v.setText(content);v.setTextColor(color);v.setTextSize(size);
        return v;
    }
    private Button button(String text,Runnable action,boolean bright){
        Button b=new Button(this);
        b.setText(text);b.setAllCaps(false);
        b.setTextColor(bright?0xFF25291F:0xFFECE8DF);
        b.setTextSize(13);b.setPadding(dp(10),0,dp(10),0);
        b.setBackground(panelBackground(bright?0xFFE5CE9C:0xDA202D29,12));
        b.setOnClickListener(v->{action.run();refreshScene();});
        return b;
    }
    private void refreshScene(){if(surface!=null)surface.requestRender();}
    private void say(String msg){notice.setText(msg);notice.setVisibility(View.VISIBLE);}
    private void act(String cmd){say(nativeDo(cmd));refreshScene();if(activePanel!=0)showPanel(activePanel);}
    private void save(){saves.edit().putString("state",nativeSave()).apply();say("Világállapot mentve az eszközön.");}
    private void addVertical(LinearLayout box,String text,Runnable a){
        Button b=button(text,a,false);
        LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,dp(43));p.bottomMargin=dp(7);
        box.addView(b,p);
    }
    private void closePanel(){if(sheet!=null){root.removeView(sheet);sheet=null;}activePanel=0;}
    private void showPanel(int which){
        closePanel();activePanel=which;
        sheet=new LinearLayout(this);sheet.setOrientation(LinearLayout.VERTICAL);
        sheet.setPadding(dp(16),dp(12),dp(16),dp(11));
        sheet.setBackground(panelBackground(0xED101B19,18));
        TextView title=caption(which==1?"ANYAGOK ÉS ÉPÍTÉS":"VILÁGÁLLAPOT",16,0xFFE4D6B6);
        title.setPadding(0,0,0,dp(8));sheet.addView(title);
        if(which==1){
            TextView information=caption("A tárgy csak szükséges anyagból, munkával készül. A 3D terepen látszik az eredmény.",12,0xFFB9C3B6);
            sheet.addView(information);
            addVertical(sheet,"Faág gyűjtése",()->act("wood"));
            addVertical(sheet,"Kő gyűjtése",()->act("stone"));
            addVertical(sheet,"Gyújtós gyűjtése",()->act("tinder"));
            addVertical(sheet,"Tűzhely megépítése · 3 faág + 2 kő",()->act("hearth"));
            addVertical(sheet,"Tűzgyújtás · 2 faág + 1 gyújtós",()->act("ignite"));
        }else{
            TextView summary=caption(nativeState(),13,0xFFEBE7DC);
            summary.setLineSpacing(dp(3),1f);
            sheet.addView(summary);
            addVertical(sheet,"1 játékóra előreléptetése",()->{nativeAdvance();say("Egy óra eltelt: nedvesség és tüzelőanyag változott.");});
            addVertical(sheet,"Pihenés",()->act("rest"));
            addVertical(sheet,"Mentés",this::save);
            addVertical(sheet,"Új tesztvilág – nem írja felül a mentést",()->{nativeReset();say("Új tesztvilág indult. A korábbi mentés megmaradt.");});
        }
        addVertical(sheet,"Bezárás",this::closePanel);
        FrameLayout.LayoutParams ps=new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM);
        ps.setMargins(dp(12),dp(10),dp(12),dp(75));
        root.addView(sheet,ps);
    }
    private void togglePanel(int which){if(activePanel==which)closePanel();else showPanel(which);}
    private float dist(MotionEvent e){
        if(e.getPointerCount()<2)return 0f;
        float x=e.getX(1)-e.getX(0),y=e.getY(1)-e.getY(0);
        return (float)Math.sqrt(x*x+y*y);
    }
    private boolean touch(MotionEvent e){
        int action=e.getActionMasked();
        if(action==MotionEvent.ACTION_DOWN){lastX=e.getX();lastY=e.getY();lastSpan=0;return true;}
        if(action==MotionEvent.ACTION_POINTER_DOWN){lastSpan=dist(e);return true;}
        if(action==MotionEvent.ACTION_MOVE){
            if(e.getPointerCount()>=2){
                float s=dist(e);
                if(lastSpan>4){zoom=Math.min(26,Math.max(7,zoom*lastSpan/s));}
                lastSpan=s;
            }else{
                float dx=e.getX()-lastX;
                orbit+=dx*.008f;
                lastX=e.getX();lastY=e.getY();
            }
            nativeSetView(orbit,zoom);refreshScene();return true;
        }
        if(action==MotionEvent.ACTION_POINTER_UP){lastSpan=0;return true;}
        return true;
    }
    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        getWindow().setStatusBarColor(Color.rgb(15,25,22));
        getWindow().setNavigationBarColor(Color.rgb(15,25,22));
        saves=getSharedPreferences("taks_native_save_v1",MODE_PRIVATE);
        String previous=saves.getString("state",null);
        if(previous!=null&&!nativeLoad(previous))nativeReset();
        try(InputStream in=getAssets().open("scene.frag")){
            shaderSource=new String(in.readAllBytes(),StandardCharsets.UTF_8);
        }catch(Exception error){shaderSource="";}

        root=new FrameLayout(this);
        surface=new GLSurfaceView(this);
        surface.setEGLContextClientVersion(3);
        surface.setPreserveEGLContextOnPause(true);
        surface.setRenderer(new GLSurfaceView.Renderer(){
            @Override public void onSurfaceCreated(javax.microedition.khronos.opengles.GL10 gl,javax.microedition.khronos.egl.EGLConfig config){
                nativeOnSurfaceCreated(shaderSource);
                runOnUiThread(()->{
                    String err=nativeRendererError();
                    if(err!=null&&!err.isEmpty())say("3D motor hiba: "+err);
                });
            }
            @Override public void onSurfaceChanged(javax.microedition.khronos.opengles.GL10 gl,int w,int h){GLES30.glViewport(0,0,w,h);}
            @Override public void onDrawFrame(javax.microedition.khronos.opengles.GL10 gl){nativeRender();}
        });
        surface.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        surface.setOnTouchListener((v,e)->touch(e));
        root.addView(surface,new FrameLayout.LayoutParams(-1,-1));
        nativeSetView(orbit,zoom);

        LinearLayout header=new LinearLayout(this);header.setOrientation(LinearLayout.VERTICAL);
        header.setPadding(dp(15),dp(10),dp(10),dp(10));
        header.setBackground(panelBackground(0xB8101A1A,13));
        header.addView(caption("TAKS WORLD",22,0xFFE6D6B5));
        header.addView(caption("NATÍV 3D MOTOR · SZIKLÁS TESZTTERÜLET",10,0xFFC3D1C5));
        FrameLayout.LayoutParams hp=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.LEFT);
        hp.setMargins(dp(12),dp(12),0,0);root.addView(header,hp);

        hint=caption("Húzd: kamera forgatása    •    Két ujjal: nagyítás",12,0xFFECE9DC);
        hint.setPadding(dp(10),dp(7),dp(10),dp(7));
        hint.setBackground(panelBackground(0xA5101A18,10));
        FrameLayout.LayoutParams hintParams=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.CENTER_HORIZONTAL);
        hintParams.topMargin=dp(16);root.addView(hint,hintParams);

        notice=caption("3D próbaszektor · A barlang még nem hiteles Wonderwerk-rekonstrukció.",13,0xFFF1E3C9);
        notice.setPadding(dp(12),dp(9),dp(12),dp(9));
        notice.setBackground(panelBackground(0xE0192420,12));
        FrameLayout.LayoutParams np=new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM);
        np.setMargins(dp(12),0,dp(12),dp(78));root.addView(notice,np);

        LinearLayout bottom=new LinearLayout(this);bottom.setGravity(Gravity.CENTER);
        bottom.setPadding(dp(8),dp(7),dp(8),dp(7));
        bottom.setBackground(panelBackground(0xC7101B19,15));
        Button build=button("ÉPÍTÉS",()->togglePanel(1),true);
        Button info=button("ÁLLAPOT",()->togglePanel(2),false);
        Button time=button("IDŐ +1 óra",()->{nativeAdvance();say("Egy játékóra eltelt: ellenőrizd a talaj és a tűz állapotát.");},false);
        Button save=button("MENTÉS",this::save,false);
        for(Button b:new Button[]{build,info,time,save}){
            LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,dp(47),1);
            lp.setMargins(dp(3),0,dp(3),0);bottom.addView(b,lp);
        }
        FrameLayout.LayoutParams bp=new FrameLayout.LayoutParams(-1,dp(65),Gravity.BOTTOM);
        bp.setMargins(dp(6),0,dp(6),dp(6));root.addView(bottom,bp);
        setContentView(root);
    }
    @Override protected void onResume(){super.onResume();if(surface!=null)surface.onResume();}
    @Override protected void onPause(){
        saves.edit().putString("state",nativeSave()).apply();
        if(surface!=null)surface.onPause();
        super.onPause();
    }
}
