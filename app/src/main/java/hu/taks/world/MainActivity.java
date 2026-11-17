package hu.taks.world;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap;
import android.widget.ImageView;
import java.util.ArrayList;
import android.graphics.drawable.GradientDrawable;
import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ScrollView;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

public final class MainActivity extends Activity {
    static { System.loadLibrary("taks_engine"); }
    private native String nativeDo(String action);
    private native void nativeAdvance();
    private native String nativeState();
    private native String nativeBuildState();
    private native int nativeSiteStage();
    private native String nativeMove(float dx,float dz);
    private native String nativeNearby();
    private native String nativeGoal();
    private native String nativeNpcStatus();
    private native String nativeCampStatus();
    private native void nativeTick();
    private native String nativeSave();
    private native boolean nativeLoad(String saved);
    private native void nativeReset();
    private native void nativeOnSurfaceCreated(String fragmentSource);
    private native void nativeRender();
    private native void nativeSetView(float angle,float pitch,float zoom);
    private native String nativeRendererError();

    private GLSurfaceView surface;
    private SharedPreferences saves;
    private FrameLayout root;
    private LinearLayout sheet;
    private TextView notice;
    private TextView hint;
    private String shaderSource;
    private FrameLayout menuCover;
    private final ArrayList<View> gameHud=new ArrayList<>();
    private boolean inMainMenu=true;
    private Button pickupButton;
    private float orbit=.785398f,cameraPitch=.615480f,zoom=14f,lastX=0,lastY=0,lastSpan=0;
    private int activePanel=0;
    private final Handler worldHandler=new Handler(Looper.getMainLooper());
    private int worldTickNumber=0;
    private final Runnable worldLoop=new Runnable(){
        @Override public void run(){
            if(inMainMenu)return;
            nativeTick();
            if(surface!=null)surface.requestRender();
            if(activePanel==3 && ++worldTickNumber%7==0)showPanel(3);
            worldHandler.postDelayed(this,1800);
        }
    };
    private TextView proximity;
    private boolean moving=false,moveIntent=false;
    private float anchorX,anchorY,axisX=0,axisY=0;
    private int movementTicks=0;
    private final Handler movementHandler=new Handler(Looper.getMainLooper());
    private final Runnable movementLoop=new Runnable(){
        @Override public void run(){
            if(!moving)return;
            if(Math.abs(axisX)+Math.abs(axisY)>.12f){
                float wx=(float)(-Math.cos(orbit)*axisX-Math.sin(orbit)*axisY);
                float wz=(float)(-Math.sin(orbit)*axisX+Math.cos(orbit)*axisY);
                String result=nativeMove(wx*.065f,wz*.065f);
                if(result.contains("kimerült")||result.contains("elállja")||result.contains("határához")){
                    if(movementTicks%6==0)say(result);
                }
                refreshScene();
                if(proximity!=null && movementTicks%2==0)refreshNearby();
                movementTicks++;
            }
            movementHandler.postDelayed(this,33);
        }
    };


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
    private void refreshNearby(){
        final String nearby=nativeNearby();
        if(proximity!=null)proximity.setText(nearby);
        if(pickupButton!=null)pickupButton.setVisibility(nearby.contains("FELVEHETŐ")?View.VISIBLE:View.GONE);
    }
    private void say(String msg){if(notice!=null){notice.setText(msg);notice.setVisibility(View.VISIBLE);}}
    private void act(String cmd){say(nativeDo(cmd));refreshNearby();refreshScene();if(activePanel!=0)showPanel(activePanel);}
    private void save(){saves.edit().putString("state",nativeSave()).apply();say("Világállapot mentve az eszközön.");}
    private int bottomBarPx(){
        int h=getResources().getDisplayMetrics().heightPixels;
        return Math.min(dp(65),Math.max(62,h/5));
    }
    private void addVertical(LinearLayout box,String text,Runnable a){
        Button b=button(text,a,false);
        int h=getResources().getDisplayMetrics().heightPixels;
        LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,Math.min(dp(42),Math.max(44,h/8)));
        p.bottomMargin=4;
        box.addView(b,p);
    }
    private void closePanel(){if(sheet!=null){root.removeView(sheet);sheet=null;}activePanel=0;}
    private void showPanel(int which){
        closePanel();activePanel=which;
        sheet=new LinearLayout(this);sheet.setOrientation(LinearLayout.VERTICAL);
        sheet.setPadding(dp(12),dp(9),dp(12),dp(7));
        sheet.setBackground(panelBackground(0xF1101B19,16));
        TextView title=caption(which==1?"ÉPÍTÉS ÉS ANYAGOK":which==3?"KÖZÖSSÉG ÉS TÁBOR":"KÖRNYEZET ÉS ÁLLAPOT",15,0xFFE4D6B6);
        title.setPadding(0,0,0,4);sheet.addView(title);
        ScrollView scroll=new ScrollView(this);
        scroll.setVerticalScrollBarEnabled(false);
        LinearLayout content=new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(0,0,0,6);
        if(which==1){
            content.addView(caption(nativeGoal(),12,0xFFF4D59B));
            TextView status=caption(nativeBuildState(),12,0xFFE2D9BD);
            status.setPadding(0,2,0,8);content.addView(status);
            TextView materialTitle=caption("1 · A KÖRNYEZETBEN TALÁLHATÓ NYERSANYAG",12,0xFF9EC9B9);
            content.addView(materialTitle);
            content.addView(caption(nativeNearby(),12,0xFFDDCEB2));
            addVertical(content,"Közeli nyersanyag felvétele",()->act("gather"));
            content.addView(caption("2 · MEGMUNKÁLÁS",12,0xFF9EC9B9));
            addVertical(content,"Kőél készítése · 2 kő",()->act("knap"));
            addVertical(content,"Cölöp megmunkálása · 1 faág + kopó kőél",()->act("pole"));
            content.addView(caption("3 · SAJÁT ÉPÍTÉSI HELY",12,0xFF9EC9B9));
            addVertical(content,"Új építési hely kijelölése · 1 kő",()->act("site"));
            int stage=nativeSiteStage();
            if(stage<0){
                content.addView(caption("Nincs közel kijelölt hely. Menj egy alkalmas pontra, és jelöld ki, vagy lépj egy korábbi menedékhez.",12,0xFFDDCDAF));
            }else if(stage==0){
                addVertical(content,"Oszlopok állítása · 2 cölöp + 2 kő",()->act("base"));
            }else if(stage==1){
                addVertical(content,"Tetőváz építése · 3 cölöp",()->act("frame"));
            }else if(stage==2){
                addVertical(content,"Tetőfedés · 6 leveles ág",()->act("roof"));
            }else{
                content.addView(caption("Ez a menedék fedett. Az anyaga időjárásban kopik; ellenőrizd az ép szerkezetet.",12,0xFFDDCDAF));
                addVertical(content,"Szerkezet javítása · 1 cölöp + 2 leveles ág",()->act("repair"));
            }
            content.addView(caption("4 · TŰZ ÉS MELEG",12,0xFF9EC9B9));
            addVertical(content,"Tűzhely építése · 3 faág + 2 kő",()->act("hearth"));
            addVertical(content,"Tűzgyújtás · 2 faág + gyújtós",()->act("ignite"));
        }else if(which==3){
            content.addView(caption("A társak véges lelőhelyekről, önállóan szállítanak a táborba.",12,0xFFDDCBAC));
            TextView camp=caption(nativeCampStatus(),12,0xFFE2D9BD);
            camp.setPadding(0,dp(7),0,dp(8));content.addView(camp);
            addVertical(content,"Tábori faág átvétele",()->act("camp_wood"));
            addVertical(content,"Tábori kő átvétele",()->act("camp_stone"));
            addVertical(content,"Tábori levél átvétele",()->act("camp_leaves"));
            addVertical(content,"Tábori gyújtós átvétele",()->act("camp_tinder"));
            addVertical(content,"Beszélgetés a közeli társaddal",()->act("npc_talk"));
            content.addView(caption("NÉGY KÜLÖN DOLGOZÓ NPC",12,0xFF9EC9B9));
            TextView npc=caption(nativeNpcStatus(),12,0xFFECE4D5);
            npc.setLineSpacing(dp(4),1f);content.addView(npc);
        }else{
            content.addView(caption(nativeGoal(),12,0xFFF4D59B));
            TextView status=caption(nativeState(),12,0xFFEEE7DB);
            status.setLineSpacing(4,1f);content.addView(status);
            addVertical(content,"Behúzódás a menedék alá",()->act("shelter_enter"));
            addVertical(content,"Menedék elhagyása",()->act("shelter_exit"));
            addVertical(content,"Pihenés · 1 játékóra",()->act("rest"));
            addVertical(content,"1 játékóra előreléptetése",()->{
                nativeAdvance();say("Egy játékóra eltelt: nedvesség és tűzállapot újraszámítva.");
                if(activePanel!=0)showPanel(activePanel);
            });
            addVertical(content,"Mentés",this::save);
            addVertical(content,"Új tesztvilág · régi mentés megtartva",()->{
                nativeReset();say("Új tesztvilág: a korábbi mentés változatlanul megmaradt.");
                if(activePanel!=0)showPanel(activePanel);
            });
        }
        scroll.addView(content);
        int screenHeight=getResources().getDisplayMetrics().heightPixels;
        int maxScroll=Math.max(95,Math.min(screenHeight/2,dp(250)));
        sheet.addView(scroll,new LinearLayout.LayoutParams(-1,maxScroll));
        addVertical(sheet,"Bezárás",this::closePanel);
        FrameLayout.LayoutParams ps=new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM);
        ps.setMargins(dp(8),0,dp(8),bottomBarPx()+5);
        root.addView(sheet,ps);
    }
    private void togglePanel(int which){if(activePanel==which)closePanel();else showPanel(which);}
    private float dist(MotionEvent e){
        if(e.getPointerCount()<2)return 0f;
        float x=e.getX(1)-e.getX(0),y=e.getY(1)-e.getY(0);
        return (float)Math.sqrt(x*x+y*y);
    }
    private boolean touch(MotionEvent e,boolean fromMovementPad){
        final int action=e.getActionMasked();
        if(action==MotionEvent.ACTION_DOWN){
            lastX=e.getX();lastY=e.getY();lastSpan=0;
            moveIntent=fromMovementPad ||
                (e.getX()<surface.getWidth()*.38f && e.getY()>surface.getHeight()*.30f);
            if(moveIntent){
                moving=true;anchorX=e.getX();anchorY=e.getY();
                axisX=axisY=0f;movementTicks=0;
                movementHandler.removeCallbacks(movementLoop);
                movementHandler.post(movementLoop);
            }
            return true;
        }
        if(action==MotionEvent.ACTION_POINTER_DOWN){
            moving=false;moveIntent=false;
            movementHandler.removeCallbacks(movementLoop);
            lastSpan=dist(e);return true;
        }
        if(action==MotionEvent.ACTION_MOVE){
            if(e.getPointerCount()>=2){
                float span=dist(e);
                if(lastSpan>4)zoom=Math.min(26,Math.max(6,zoom*lastSpan/span));
                lastSpan=span;
                nativeSetView(orbit,cameraPitch,zoom);
                refreshScene();return true;
            }
            if(moveIntent){
                float dx=e.getX()-anchorX;
                float dy=e.getY()-anchorY;
                float length=(float)Math.sqrt(dx*dx+dy*dy);
                float factor=length<8?0:Math.min(1f,length/66f)/length;
                axisX=dx*factor;
                axisY=-dy*factor;
            }else{
                float dx=e.getX()-lastX;
                float dy=e.getY()-lastY;
                orbit+=dx*.0075f;
                cameraPitch=Math.min(1.396263f,Math.max(.349066f,cameraPitch-dy*.006f));
                lastX=e.getX();lastY=e.getY();
                nativeSetView(orbit,cameraPitch,zoom);refreshScene();
            }
            return true;
        }
        if(action==MotionEvent.ACTION_CANCEL||action==MotionEvent.ACTION_UP){
            moving=false;moveIntent=false;axisX=axisY=0;
            movementHandler.removeCallbacks(movementLoop);
            refreshNearby();
            refreshScene();return true;
        }
        if(action==MotionEvent.ACTION_POINTER_UP){lastSpan=0;return true;}
        return true;
    }
    private void chooseGraphicsResolution(){
        final String[] presets={"Gyors – 640 képpont széles","Kiegyensúlyozott – 840 képpont széles","Részletesebb – 1200 képpont széles"};
        new AlertDialog.Builder(this).setTitle("3D grafikai teljesítmény")
            .setItems(presets,(dialog,which)->{
                int targetWidth=which==0?640:which==1?840:1200;
                applyFramebufferSize(targetWidth);
                refreshScene();
                say("Renderfelbontás: "+targetWidth+" px. A kamera valódi ortografikus 3D.");
            }).setNegativeButton("Mégse",null).show();
    }
    private void applyFramebufferSize(int desiredWidth){
        android.util.DisplayMetrics m=getResources().getDisplayMetrics();
        float scale=Math.min(1.0f,(float)desiredWidth/Math.max(m.widthPixels,m.heightPixels));
        int width=Math.max(320,Math.round(m.widthPixels*scale));
        int height=Math.max(180,Math.round(m.heightPixels*scale));
        surface.getHolder().setFixedSize(width,height);
    }
    private void openGameMenu(){
        showMainMenu();
    }
    private void openGameTools(){
        closePanel();
        String[] commands={"Folytatás","Mi a következő cél?","Mentés","Új világ","Izometrikus kamera visszaállítása","Kamera: felülnézet","Kamera: alacsony nézet","Grafikai felbontás","Készítők","Kilépés"};
        new AlertDialog.Builder(this).setTitle("TAKS WORLD · FŐMENÜ")
            .setItems(commands,(dialog,index)->{
                switch(index){
                    case 0: break;
                    case 1: new AlertDialog.Builder(this)
                            .setTitle("JÁTÉKCÉL")
                            .setMessage(nativeGoal()+"\n\n"+nativeNearby()+"\n\nMozgás: bal oldalon húzd az ujjad; kamera: jobb oldalon húzd; kétujjas zoom. Ha az anyag közel van, válaszd a FELVESZ gombot.")
                            .setPositiveButton("Rendben",null).show();break;
                    case 2: save(); break;
                    case 3: new AlertDialog.Builder(this)
                         .setTitle("Új világ indítása")
                         .setMessage("Biztosan új világot szeretnél? A régi mentés megmarad, amíg nem mentesz ismét.")
                         .setPositiveButton("Új világ",(d,w)->{
                             nativeReset();refreshScene();
                             if(proximity!=null)proximity.setText(nativeNearby());
                             say(nativeGoal());
                         })
                         .setNegativeButton("Mégse",null).show();break;
                    case 4: orbit=.785398f;cameraPitch=.615480f;zoom=14f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();break;
                    case 5: cameraPitch=1.37f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();break;
                    case 6: cameraPitch=.40f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();break;
                    case 7: chooseGraphicsResolution();break;
                    case 8: new AlertDialog.Builder(this).setTitle("Készítők")
                              .setMessage("TAKS WORLD – saját fejlesztésű natív játékmotor. A grafika és az oksági szimuláció fejlesztés alatt.")
                              .setPositiveButton("OK",null).show();break;
                    case 9: save();finish();break;
                }
            }).setNegativeButton("Vissza a játékba",null).show();
    }
    private void setHudVisibility(boolean visible){
        for(View v:gameHud)v.setVisibility(visible?View.VISIBLE:View.GONE);
        if(visible)refreshNearby();
    }
    private void startPlaying(){
        inMainMenu=false;
        if(menuCover!=null){root.removeView(menuCover);menuCover=null;}
        setHudVisibility(true);
        refreshScene();
        worldHandler.removeCallbacks(worldLoop);
        worldHandler.postDelayed(worldLoop,1800);
    }
    private void newWorldFromMenu(){
        final String existing=saves.getString("state",null);
        if(existing==null){
            nativeReset();
            saves.edit().putString("state",nativeSave()).apply();
            startPlaying();return;
        }
        final String[] choices={"Folytatás a mentett világból","Új világ indítása","Mégse"};
        new AlertDialog.Builder(this).setTitle("TAKS WORLD · ÚJ JÁTÉK")
            .setItems(choices,(dialog,choice)->{
                if(choice==0){startPlaying();return;}
                if(choice!=1)return;
                new AlertDialog.Builder(this).setTitle("Új világ indítása")
                    .setMessage("A meglévő világot külön biztonsági másolatba mentjük. Az új világban minden anyagot újra meg kell szerezni.")
                    .setPositiveButton("Új világ",(d,which)->{
                        saves.edit().putString("backup_state",existing).apply();
                        nativeReset();
                        saves.edit().putString("state",nativeSave()).apply();
                        startPlaying();
                    }).setNegativeButton("Vissza",null).show();
            }).show();
    }
    private void restoreOldWorld(){
        final String backup=saves.getString("backup_state",null);
        if(backup==null){
            new AlertDialog.Builder(this).setMessage("Még nincs korábbi világ biztonsági másolata.")
                .setPositiveButton("Rendben",null).show();return;
        }
        new AlertDialog.Builder(this).setTitle("Korábbi világ visszaállítása")
            .setMessage("A jelenlegi világot előbb biztonsági másolatba mentjük, majd a régi világot visszatöltjük.")
            .setPositiveButton("Visszaállítás",(dialog,which)->{
                String current=nativeSave();
                if(nativeLoad(backup)){
                    saves.edit().putString("state",nativeSave())
                        .putString("backup_state",current).apply();
                    refreshNearby();refreshScene();
                }else new AlertDialog.Builder(this).setMessage("A mentés sérült vagy nem kompatibilis; semmit sem módosítottunk.")
                    .setPositiveButton("Rendben",null).show();
            }).setNegativeButton("Mégse",null).show();
    }
    private void showSettings(){
        final String[] items={"Grafikai felbontás","Izometrikus kamera","Kamera felülnézetből",
                              "Kamera alacsonyabb nézetből","Vezérlés és ok-okozati szabályok","Korábbi mentés visszaállítása"};
        new AlertDialog.Builder(this).setTitle("BEÁLLÍTÁSOK")
            .setItems(items,(dialog,which)->{
                switch(which){
                    case 0:chooseGraphicsResolution();break;
                    case 1:orbit=.785398f;cameraPitch=.615480f;zoom=14f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();break;
                    case 2:cameraPitch=1.37f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();break;
                    case 3:cameraPitch=.40f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();break;
                    case 4:new AlertDialog.Builder(this).setTitle("IRÁNYÍTÁS")
                         .setMessage("Bal oldalon: a karakter mozgatása.\nJobb oldalon: 3D kamera forgatása és döntése.\nKét ujjal: izometrikus nagyítás.\n\nAz építéshez valódi anyag, szerszám, idő és munkavégzés kell. Sikertelenségnél a játék kiírja az okot.")
                         .setPositiveButton("Rendben",null).show();break;
                    case 5:restoreOldWorld();break;
                }
            }).setNegativeButton("Vissza",null).show();
    }
    private void showCredits(){
        new AlertDialog.Builder(this).setTitle("TAKS WORLD")
            .setMessage("Saját TAKS Engine · Wonderwerk\n\nNatív C++ játékmotor, Android 3D grafika és ok-okozati szimuláció.\n\nFejlesztői változat. A valódi geológiai és embermodellek, valamint az eredeti jóváhagyott logó külön forrásfájlokhoz kötöttek; a jelenlegi APK-ban még nincsenek kész.")
            .setPositiveButton("Vissza",null).show();
    }
    private View originalLogoOrTitle(){
        // A Biblia név szerint ezt az EREDETI assetet rögzíti. Nem generálunk helyette másik logót.
        try(InputStream stream=getAssets().open("logo/TAKS_WORLD_LOGO.png")){
            Bitmap bitmap=BitmapFactory.decodeStream(stream);
            if(bitmap!=null){
                ImageView logo=new ImageView(this);
                logo.setImageBitmap(bitmap);
                logo.setScaleType(ImageView.ScaleType.FIT_CENTER);
                return logo;
            }
        }catch(Exception missing){}
        TextView title=caption("TAKS WORLD",29,0xFFE7D5B4);
        title.setGravity(Gravity.CENTER);
        title.setLetterSpacing(.13f);
        return title;
    }
    private void showMainMenu(){
        inMainMenu=true;
        worldHandler.removeCallbacks(worldLoop);
        moving=false;moveIntent=false;axisX=axisY=0;
        movementHandler.removeCallbacks(movementLoop);
        closePanel();
        setHudVisibility(false);
        if(menuCover!=null)root.removeView(menuCover);

        menuCover=new FrameLayout(this);
        menuCover.setBackgroundColor(0xD5091411);
        LinearLayout stack=new LinearLayout(this);
        stack.setOrientation(LinearLayout.VERTICAL);
        stack.setGravity(Gravity.CENTER);
        stack.setPadding(dp(12),dp(8),dp(12),dp(8));
        final int usableHeight=getResources().getDisplayMetrics().heightPixels;
        final int menuButtonHeight=Math.min(dp(43),Math.max(dp(33),(usableHeight-dp(90))/5));
        final int menuLogoHeight=Math.min(dp(73),Math.max(dp(44),usableHeight-4*menuButtonHeight-dp(65)));
        View logo=originalLogoOrTitle();
        LinearLayout.LayoutParams logoSize=new LinearLayout.LayoutParams(-1,menuLogoHeight);
        logoSize.bottomMargin=dp(5);
        stack.addView(logo,logoSize);
        String[] labels={"ÚJ JÁTÉK","BEÁLLÍTÁSOK","KÉSZÍTŐK","KILÉPÉS"};
        Runnable[] actions={this::newWorldFromMenu,this::showSettings,this::showCredits,
                ()->new AlertDialog.Builder(this).setTitle("Kilépés")
                    .setMessage("Biztosan kilépsz? A világállapot mentésre kerül.")
                    .setPositiveButton("Kilépés",(dialog,choice)->{save();finish();})
                    .setNegativeButton("Vissza",null).show()};
        for(int i=0;i<labels.length;++i){
            Button menuButton=button(labels[i],actions[i],i==0);
            menuButton.setTextSize(16);
            menuButton.setLetterSpacing(.035f);
            LinearLayout.LayoutParams bp=new LinearLayout.LayoutParams(-1,menuButtonHeight);
            bp.bottomMargin=dp(5);
            stack.addView(menuButton,bp);
        }
        FrameLayout.LayoutParams p=new FrameLayout.LayoutParams(
            Math.min(dp(340),getResources().getDisplayMetrics().widthPixels-dp(32)),
            -2,Gravity.CENTER);
        menuCover.addView(stack,p);
        root.addView(menuCover,new FrameLayout.LayoutParams(-1,-1));
    }
    @Override public void onBackPressed(){
        if(!inMainMenu){showMainMenu();return;}
        new AlertDialog.Builder(this).setTitle("TAKS WORLD").setMessage("Kilépsz a játékból?")
            .setPositiveButton("Kilépés",(d,w)->{save();finish();})
            .setNegativeButton("Vissza",null).show();
    }
    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        getWindow().setStatusBarColor(Color.rgb(15,25,22));
        getWindow().setNavigationBarColor(Color.rgb(15,25,22));
        saves=getSharedPreferences("taks_native_save_v1",MODE_PRIVATE);
        String previous=saves.getString("state",null);
        if(previous!=null&&!nativeLoad(previous))nativeReset();
        // Java 9 readAllBytes helyett Android 26-tól kompatibilis betöltés.
        try(InputStream in=getAssets().open("scene.frag");
            java.io.ByteArrayOutputStream bytes=new java.io.ByteArrayOutputStream()){
            byte[] block=new byte[8192];int n;
            while((n=in.read(block))!=-1)bytes.write(block,0,n);
            shaderSource=new String(bytes.toByteArray(),StandardCharsets.UTF_8);
        }catch(Exception error){shaderSource="";}

        root=new FrameLayout(this);
        surface=new GLSurfaceView(this);
        surface.setEGLContextClientVersion(3);
        // Isometric GPU mesh renderer: dynamic resolution menu, aspect ratio always preserved.
        applyFramebufferSize(840);
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
        surface.setOnTouchListener((v,e)->touch(e,false));
        root.addView(surface,new FrameLayout.LayoutParams(-1,-1));
        nativeSetView(orbit,cameraPitch,zoom);

        LinearLayout header=new LinearLayout(this);header.setOrientation(LinearLayout.VERTICAL);
        header.setPadding(dp(15),dp(10),dp(10),dp(10));
        header.setBackground(panelBackground(0xB8101A1A,13));
        header.addView(caption("TAKS WORLD",22,0xFFE6D6B5));
        header.addView(caption("TAKS ENGINE 0.7 · FŐMENÜ",10,0xFFC3D1C5));
        FrameLayout.LayoutParams hp=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.LEFT);
        hp.setMargins(dp(12),dp(12),0,0);root.addView(header,hp);gameHud.add(header);

        hint=caption("Bal: mozgás · Jobb: kamera forgatás és döntés · Két ujjal: zoom",12,0xFFECE9DC);
        hint.setPadding(dp(10),dp(7),dp(10),dp(7));
        hint.setBackground(panelBackground(0xA5101A18,10));
        FrameLayout.LayoutParams hintParams=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.CENTER_HORIZONTAL);
        hintParams.topMargin=dp(16);root.addView(hint,hintParams);gameHud.add(hint);
        hint.postDelayed(()->hint.setVisibility(View.GONE),6500);

        notice=caption(nativeGoal(),12,0xFFF1E3C9);
        notice.setPadding(dp(12),dp(9),dp(12),dp(9));
        notice.setBackground(panelBackground(0xE0192420,12));
        FrameLayout.LayoutParams np=new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM);
        np.setMargins(dp(12),0,dp(12),bottomBarPx()+7);root.addView(notice,np);gameHud.add(notice);
        notice.postDelayed(()->{
            if(notice.getText().toString().startsWith("CÉL:")) notice.setVisibility(View.GONE);
        },9000);

        proximity=caption(nativeNearby(),12,0xFFE8E0CC);
        proximity.setPadding(dp(9),dp(6),dp(9),dp(6));
        proximity.setBackground(panelBackground(0xBE162821,10));
        FrameLayout.LayoutParams proxParams=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.RIGHT);
        proxParams.topMargin=dp(49);proxParams.rightMargin=dp(12);root.addView(proximity,proxParams);gameHud.add(proximity);

        TextView movePad=caption("MOZGÁS\n  ◉",13,0xFFF4E8D2);
        movePad.setGravity(Gravity.CENTER);
        movePad.setBackground(panelBackground(0x9713211D,40));
        movePad.setOnTouchListener((v,e)->touch(e,true));
        FrameLayout.LayoutParams moveParams=new FrameLayout.LayoutParams(dp(82),dp(82),Gravity.LEFT|Gravity.BOTTOM);
        moveParams.setMargins(dp(11),0,0,dp(18));root.addView(movePad,moveParams);gameHud.add(movePad);

        // Kontextuális FELVESZ – csak akkor van a képernyőn, ha tényleg van elérhető tárgy.
        pickupButton=button("FELVESZ",()->act("gather"),true);
        FrameLayout.LayoutParams collectLayout=new FrameLayout.LayoutParams(dp(98),dp(43),Gravity.BOTTOM|Gravity.RIGHT);
        collectLayout.setMargins(0,0,dp(16),dp(72));
        root.addView(pickupButton,collectLayout);
        gameHud.add(pickupButton);

        // Kompakt, jobb alsó kezelősáv. A világ képernyőjét nem takarja 5 óriásgomb.
        LinearLayout bottom=new LinearLayout(this);bottom.setGravity(Gravity.CENTER);
        bottom.setPadding(dp(2),dp(2),dp(2),dp(2));
        bottom.setBackground(panelBackground(0xC9101B19,11));
        Button build=button("ÉPÍTÉS",()->togglePanel(1),false);
        Button npc=button("TÁRSAK",()->togglePanel(3),false);
        Button info=button("TÁSKA",()->togglePanel(2),false);
        Button menu=button("MENÜ",this::openGameMenu,false);
        for(Button b:new Button[]{build,npc,info,menu}){
            b.setTextSize(11);
            LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,dp(43),1);
            lp.setMargins(dp(2),0,dp(2),0);bottom.addView(b,lp);
        }
        FrameLayout.LayoutParams bp=new FrameLayout.LayoutParams(
            Math.min(dp(395),getResources().getDisplayMetrics().widthPixels-dp(124)),
            dp(48),Gravity.BOTTOM|Gravity.RIGHT);
        bp.setMargins(0,0,dp(8),dp(8));root.addView(bottom,bp);
        gameHud.add(bottom);
        setContentView(root);
        showMainMenu();
    }
    @Override protected void onResume(){
        super.onResume();
        if(surface!=null)surface.onResume();
        worldHandler.removeCallbacks(worldLoop);
        if(!inMainMenu)worldHandler.postDelayed(worldLoop,1800);
    }
    @Override protected void onPause(){
        moving=false;movementHandler.removeCallbacks(movementLoop);
        worldHandler.removeCallbacks(worldLoop);
        saves.edit().putString("state",nativeSave()).apply();
        if(surface!=null)surface.onPause();
        super.onPause();
    }
}
