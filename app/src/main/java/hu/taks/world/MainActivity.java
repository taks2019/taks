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
    private native float[] nativeMenuCues();
    private native boolean nativeUploadRealModel(int slot,byte[] model,int[] argb,int width,int height);
    private native boolean nativeUploadTerrainTexture(int slot,int[] pixels,int width,int height);

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
    private float orbit=.785398f,cameraPitch=.615480f,zoom=10f,lastX=0,lastY=0,lastSpan=0;
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
    private boolean moving=false,moveIntent=false,padActive=false,sprinting=false;
    private float anchorX,anchorY,axisX=0,axisY=0;
    private long lastMovementNs=0L;
    private final TouchMovement touchMovement=new TouchMovement();
    private int movementTicks=0;
    private final Handler movementHandler=new Handler(Looper.getMainLooper());
    private final Runnable movementLoop=new Runnable(){
        @Override public void run(){
            if(!moving||inMainMenu)return;
            long now=System.nanoTime();
            float dt=lastMovementNs==0L?.025f:
                    Math.max(.001f,Math.min(.06f,(now-lastMovementNs)*1.e-9f));
            lastMovementNs=now;
            float[] delta=touchMovement.step(axisX,axisY,orbit,dt,sprinting);
            if(Math.abs(delta[0])+Math.abs(delta[1])>.00005f){
                String result=nativeMove(delta[0],delta[1]);
                if((result.contains("kimerült")||result.contains("elállja")||
                    result.contains("határához")||result.contains("ütközöl"))&&movementTicks%12==0){
                    say(result);
                }
                refreshScene();
                if(proximity!=null&&movementTicks%4==0)refreshNearby();
                movementTicks++;
            }
            movementHandler.postDelayed(this,25);
        }
    };
    private void startMovement(){
        moving=true;
        movementTicks=0;
        lastMovementNs=0L;
        touchMovement.reset();
        movementHandler.removeCallbacks(movementLoop);
        movementHandler.post(movementLoop);
    }
    private void stopMovement(){
        moving=false;
        axisX=axisY=0;
        touchMovement.reset();
        lastMovementNs=0L;
        movementHandler.removeCallbacks(movementLoop);
    }


    private boolean loadRealPbrSurface(int slot,String name){
        try(InputStream data=getAssets().open("models/"+name+".png")){
            Bitmap bitmap=BitmapFactory.decodeStream(data);
            if(bitmap==null)throw new java.io.IOException("PBR image decoding failed: "+name);
            int width=bitmap.getWidth(),height=bitmap.getHeight();
            int[] pixels=new int[width*height];
            bitmap.getPixels(pixels,0,width,0,0,width,height);
            bitmap.recycle();
            boolean ok=nativeUploadTerrainTexture(slot,pixels,width,height);
            if(!ok)throw new java.io.IOException("GPU refused photographic surface: "+name);
            return true;
        }catch(Exception e){
            android.util.Log.e("TAKS_SURFACE","Missing real PBR material: "+name,e);
            return false;
        }
    }
    private boolean loadAuthenticRockAsset(int slot,String name){
        // The compressed PNG and mesh are bundled into the APK by the CI import job.
        try(InputStream meshStream=getAssets().open("models/"+name+".tkm");
            java.io.ByteArrayOutputStream bytes=new java.io.ByteArrayOutputStream()){
            byte[] buffer=new byte[8192];int count;
            while((count=meshStream.read(buffer))!=-1)bytes.write(buffer,0,count);
            Bitmap image;
            try(InputStream imageStream=getAssets().open("models/"+name+".png")){
                image=BitmapFactory.decodeStream(imageStream);
            }
            if(image==null)throw new java.io.IOException("Photographic texture decode failed: "+name);
            int width=image.getWidth(),height=image.getHeight();
            int[] pixels=new int[width*height];
            image.getPixels(pixels,0,width,0,0,width,height);
            image.recycle();
            boolean loaded=nativeUploadRealModel(slot,bytes.toByteArray(),pixels,width,height);
            if(!loaded)throw new java.io.IOException("The native GLES 3D model importer rejected "+name);
            return true;
        }catch(Exception exception){
            android.util.Log.e("TAKS_MODEL","Missing or invalid CC0 scanned model: "+name,exception);
            return false;
        }
    }

    private int dp(float n){return Math.round(getResources().getDisplayMetrics().density*n);}
    private GradientDrawable panelBackground(int color,int radius){
        GradientDrawable d=new GradientDrawable();
        d.setColor(color);
        d.setCornerRadius(dp(radius));
        d.setStroke(dp(1),0x886F6244);
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
        b.setTextColor(bright?0xFF2D2115:0xFFF2E4CB);
        b.setTextSize(13);b.setPadding(dp(10),0,dp(10),0);
        b.setBackground(panelBackground(bright?0xFFE4C38A:0xDA30251B,12));
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
    private void setMovementAxes(float dx,float dy,float radius){
        float length=(float)Math.hypot(dx,dy);
        if(length<dp(5)){axisX=axisY=0f;return;}
        float factor=Math.min(1f,length/radius)/length;
        axisX=dx*factor;
        axisY=-dy*factor;
    }
    private boolean touch(MotionEvent e,boolean fromMovementPad){
        final int action=e.getActionMasked();
        // The left physical control and right-hand camera can be held by
        // different fingers simultaneously. Surface gestures never reset pad.
        if(fromMovementPad){
            if(action==MotionEvent.ACTION_DOWN){
                padActive=true;moveIntent=false;
                setMovementAxes(e.getX()-dp(48),e.getY()-dp(48),dp(38));
                startMovement();
            }else if(action==MotionEvent.ACTION_MOVE && padActive){
                setMovementAxes(e.getX()-dp(48),e.getY()-dp(48),dp(38));
            }else if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL){
                padActive=false;
                stopMovement();
                refreshNearby();refreshScene();
            }
            return true;
        }
        if(action==MotionEvent.ACTION_DOWN){
            lastX=e.getX();lastY=e.getY();lastSpan=0f;
            moveIntent=!padActive &&
                (e.getX()<surface.getWidth()*.38f && e.getY()>surface.getHeight()*.30f);
            if(moveIntent){
                anchorX=e.getX();anchorY=e.getY();
                axisX=axisY=0f;startMovement();
            }
            return true;
        }
        if(action==MotionEvent.ACTION_POINTER_DOWN){
            if(moveIntent&&!padActive)stopMovement();
            moveIntent=false;
            lastSpan=dist(e);return true;
        }
        if(action==MotionEvent.ACTION_MOVE){
            if(e.getPointerCount()>=2){
                float span=dist(e);
                if(lastSpan>4&&span>4){
                    zoom=Math.min(26,Math.max(6,zoom*lastSpan/span));
                }
                lastSpan=span;
                nativeSetView(orbit,cameraPitch,zoom);
                refreshScene();return true;
            }
            if(moveIntent&&!padActive){
                setMovementAxes(e.getX()-anchorX,e.getY()-anchorY,dp(62));
            }else{
                float dx=e.getX()-lastX,dy=e.getY()-lastY;
                orbit+=dx*.006f;
                cameraPitch=Math.min(1.396263f,Math.max(.349066f,cameraPitch-dy*.0045f));
                lastX=e.getX();lastY=e.getY();
                nativeSetView(orbit,cameraPitch,zoom);
                refreshScene();
            }
            return true;
        }
        if(action==MotionEvent.ACTION_POINTER_UP){
            lastSpan=0f;
            // Carry on with the remaining camera finger without a sudden jump.
            int remain=e.getActionIndex()==0?1:0;
            if(remain<e.getPointerCount()){
                lastX=e.getX(remain);lastY=e.getY(remain);
            }
            return true;
        }
        if(action==MotionEvent.ACTION_CANCEL||action==MotionEvent.ACTION_UP){
            if(moveIntent&&!padActive)stopMovement();
            moveIntent=false;
            lastSpan=0f;
            refreshNearby();refreshScene();return true;
        }
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
                    case 4: orbit=.785398f;cameraPitch=.615480f;zoom=10f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();break;
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
                    case 1:orbit=.785398f;cameraPitch=.615480f;zoom=10f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();break;
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
        padActive=false;moveIntent=false;sprinting=false;
        stopMovement();
        closePanel();
        setHudVisibility(false);
        if(menuCover!=null)root.removeView(menuCover);

        menuCover=new FrameLayout(this);
        final PrehistoricMenuTheme.Look natural=PrehistoricMenuTheme.fromCues(nativeMenuCues());
        // A background is a translucent veil over the ACTUAL game world.
        // Only earth, bark, soot and ochre; world daylight/rain drive its tones.
        GradientDrawable earthenVeil=new GradientDrawable(
            GradientDrawable.Orientation.TOP_BOTTOM,
            new int[]{natural.veilTop,natural.veilBottom});
        menuCover.setBackground(earthenVeil);
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
        // Reflect only actual discoveries from the saved World, never a fake level.
        TextView discoveries=caption(natural.discoveryLine,11,natural.accent);
        discoveries.setGravity(Gravity.CENTER);
        discoveries.setPadding(dp(2),0,dp(2),dp(5));
        stack.addView(discoveries,new LinearLayout.LayoutParams(-1,dp(22)));
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
            // Four distinct hit targets, shared prehistoric visual materials.
            GradientDrawable physicalButton=new GradientDrawable(
                GradientDrawable.Orientation.LEFT_RIGHT,
                new int[]{natural.buttonTop,natural.buttonBottom});
            physicalButton.setCornerRadius(dp(10));
            physicalButton.setStroke(dp(1),natural.accent);
            menuButton.setBackground(physicalButton);
            menuButton.setTextColor(natural.text);
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
        // Landscape gameplay fills the phone screen, not the Android gesture
        // rail; a deliberate edge swipe still exposes system navigation.
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
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
                // A kép nem távoli webes tartalom: a valódi fotogrammetriai modellek
                // és saját fénykép-textúráik közvetlenül az Android APK assetjeiből töltődnek.
                boolean rockOk=loadAuthenticRockAsset(0,"rock_07");
                boolean bouldersOk=loadAuthenticRockAsset(1,"namaqualand_boulders_01");
                boolean cliffModelOk=loadAuthenticRockAsset(2,"namaqualand_cliff_01");
                // Real Poly Haven authored botanical meshes and RGBA atlases.
                // None is replaced with placeholder tubes, boxes or billboard graphics.
                boolean grassOk=loadAuthenticRockAsset(3,"grass_bermuda_01");
                boolean fernOk=loadAuthenticRockAsset(4,"fern_02");
                boolean humanOk=loadAuthenticRockAsset(6,"makehuman_cc0");
                // The 2M-triangle tree awaits a genuine UV-preserving mobile LOD.
                // No fake tree model is substituted.
                boolean groundOk=loadRealPbrSurface(0,"ground_sandy_gravel");
                boolean cliffOk=loadRealPbrSurface(1,"wall_cliff_side");
                runOnUiThread(()->{
                    String err=nativeRendererError();
                    if(!rockOk||!bouldersOk||!cliffModelOk||!grassOk||!fernOk||!humanOk||!groundOk||!cliffOk||err!=null&&!err.isEmpty()){
                        say("Valódi 3D modellhiba: "+(err==null?"":err)+
                            " · Az APK modellcsomagja sérült vagy hiányzik.");
                    }
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
        header.setBackground(panelBackground(0xB8251C14,13));
        header.addView(caption("TAKS WORLD",22,0xFFE6D6B5));
        header.addView(caption("TAKS ENGINE 0.7 · FŐMENÜ",10,0xFFC3D1C5));
        FrameLayout.LayoutParams hp=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.LEFT);
        hp.setMargins(dp(12),dp(12),0,0);root.addView(header,hp);gameHud.add(header);

        hint=caption("Bal: analóg mozgás · FUTÁS: nyomva tartva · Jobb: kamera · Két ujjal: zoom",12,0xFFECE9DC);
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
        movePad.setBackground(panelBackground(0x9713211D,48));
        movePad.setOnTouchListener((v,e)->touch(e,true));
        FrameLayout.LayoutParams moveParams=new FrameLayout.LayoutParams(dp(96),dp(96),Gravity.LEFT|Gravity.BOTTOM);
        moveParams.setMargins(dp(11),0,0,dp(18));root.addView(movePad,moveParams);gameHud.add(movePad);
        TextView sprint=caption("FUTÁS",12,0xFFF4E8D2);
        sprint.setGravity(Gravity.CENTER);
        sprint.setBackground(panelBackground(0xC02B3B2D,16));
        sprint.setOnTouchListener((v,e)->{
            int action=e.getActionMasked();
            if(action==MotionEvent.ACTION_DOWN)sprinting=true;
            if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL)sprinting=false;
            return true;
        });
        FrameLayout.LayoutParams runParams=new FrameLayout.LayoutParams(dp(67),dp(48),Gravity.LEFT|Gravity.BOTTOM);
        runParams.setMargins(dp(116),0,0,dp(37));root.addView(sprint,runParams);gameHud.add(sprint);

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
        padActive=false;moveIntent=false;sprinting=false;
        stopMovement();
        worldHandler.removeCallbacks(worldLoop);
        saves.edit().putString("state",nativeSave()).apply();
        if(surface!=null)surface.onPause();
        super.onPause();
    }
}
