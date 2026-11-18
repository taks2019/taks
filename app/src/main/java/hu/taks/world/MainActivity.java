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
import android.view.ViewGroup;
import java.util.function.Consumer;
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
    private FrameLayout menuCover,modalOverlay;
    private Bitmap sharedStonePhoto;
    // Keep the original logo pixels untouched. Only its physical frame and
    // every other menu skin respond to actual native World causality.
    private final Handler skinHandler=new Handler(Looper.getMainLooper());
    private final Runnable skinLoop=new Runnable(){
        @Override public void run(){
            if(root!=null)refreshLivingSkin();
            skinHandler.postDelayed(this,650);
        }
    };
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
            refreshLivingSkin();
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
    private LivingUiSkin.State currentSkin(){
        return LivingUiSkin.fromCues(nativeMenuCues());
    }
    private GradientDrawable panelBackground(int color,int radius){
        GradientDrawable d=new GradientDrawable();
        d.setColor(color);
        d.setCornerRadius(dp(radius));
        d.setStroke(dp(1),0x886F6244);
        return d;
    }
    private Bitmap originalStonePhoto(){
        if(sharedStonePhoto!=null && !sharedStonePhoto.isRecycled())return sharedStonePhoto;
        // The same real rock photograph is already bundled offline for 3D.
        // Share one bitmap across every menu surface, no repeated decodes.
        try(InputStream source=getAssets().open("models/wall_cliff_side.png")){
            Bitmap image=BitmapFactory.decodeStream(source);
            if(image==null)return null;
            if(image.getWidth()>600){
                sharedStonePhoto=Bitmap.createScaledBitmap(image,512,512,true);
                image.recycle();
            }else sharedStonePhoto=image;
            return sharedStonePhoto;
        }catch(Exception error){
            android.util.Log.w("TAKS_UI","Scanned-rock GUI skin unavailable",error);
            return null;
        }
    }
    private static int opacity(int color,int alpha){
        return (color&0x00FFFFFF)|((alpha&0xFF)<<24);
    }
    private void setNaturalStone(View view,int upper,int lower,int border,int radius){
        if(view.getBackground() instanceof LivingStoneDrawable){
            ((LivingStoneDrawable)view.getBackground()).palette(upper,lower,border);
        }else{
            view.setBackground(new LivingStoneDrawable(originalStonePhoto(),
                         upper,lower,border,dp(radius),dp(1)));
        }
    }
    private GradientDrawable naturalGradient(int top,int bottom,int stroke,int radius){
        GradientDrawable d=new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM,
                                               new int[]{top,bottom});
        d.setCornerRadius(dp(radius));
        d.setStroke(dp(1),stroke);
        return d;
    }
    private void skin(View view,String type){
        view.setTag("TAKS_SKIN_"+type);
        applySkin(view,currentSkin(),android.os.SystemClock.uptimeMillis());
    }
    private void applySkin(View v,LivingUiSkin.State state,long now){
        if(v instanceof LogoEnvironmentView) {
            ((LogoEnvironmentView)v).setWorldState(state);
        }
        Object tag=v.getTag();
        if(tag instanceof String){
            String role=(String)tag;
            if("TAKS_SKIN_BUTTON".equals(role)){
                setNaturalStone(v,opacity(state.buttonTop,190),
                        opacity(state.buttonBottom,220),state.border,10);
                if(v instanceof TextView)((TextView)v).setTextColor(state.text);
            }else if("TAKS_SKIN_CARD".equals(role)){
                setNaturalStone(v,opacity(state.panelTop,208),
                        opacity(state.panelBottom,234),state.border,17);
            }else if("TAKS_SKIN_LOGO".equals(role)){
                int glow=state.animatedLogoGlow(now);
                setNaturalStone(v,opacity(glow,44),
                         opacity(state.panelBottom,180),glow,19);
            }else if("TAKS_SKIN_CAUSE".equals(role) && v instanceof TextView){
                ((TextView)v).setText(state.cause);
                ((TextView)v).setTextColor(state.mutedText);
            }else if("TAKS_SKIN_DISCOVERY".equals(role) && v instanceof TextView){
                ((TextView)v).setText(state.discovered);
                ((TextView)v).setTextColor(state.border);
            }
        }
        if(v instanceof ViewGroup){
            ViewGroup children=(ViewGroup)v;
            for(int i=0;i<children.getChildCount();i++){
                applySkin(children.getChildAt(i),state,now);
            }
        }
    }
    private void refreshLivingSkin(){
        if(root==null)return;
        LivingUiSkin.State state=currentSkin();
        if(menuCover!=null){
            menuCover.setBackground(naturalGradient(
                    (state.panelTop&0x00FFFFFF)|0x9A000000,
                    (state.panelBottom&0x00FFFFFF)|0xC2000000,
                    state.border,0));
        }
        applySkin(root,state,android.os.SystemClock.uptimeMillis());
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
        skin(b,"BUTTON");
        b.setOnClickListener(v->{action.run();refreshScene();refreshLivingSkin();});
        return b;
    }
    private void refreshScene(){if(surface!=null)surface.requestRender();}
    private void refreshNearby(){
        final String nearby=nativeNearby();
        if(proximity!=null)proximity.setText(nearby);
        if(pickupButton!=null)pickupButton.setVisibility(nearby.contains("FELVEHETŐ")?View.VISIBLE:View.GONE);
    }
    private void say(String msg){if(notice!=null){notice.setText(msg);notice.setVisibility(View.VISIBLE);}}
    private void act(String cmd){say(nativeDo(cmd));refreshNearby();refreshScene();if(activePanel!=0)showPanel(activePanel);refreshLivingSkin();}
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
        skin(sheet,"CARD");
        LinearLayout bar=new LinearLayout(this);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        TextView title=caption(which==1?"ÉPÍTÉS ÉS ANYAGOK":which==3?"KÖZÖSSÉG ÉS TÁBOR":"KÖRNYEZET ÉS ÁLLAPOT",15,0xFFE4D6B6);
        title.setPadding(0,0,0,dp(4));title.setLetterSpacing(.04f);
        bar.addView(title,new LinearLayout.LayoutParams(0,dp(38),1));
        Button config=button("⚙",this::showSettings,false);
        config.setContentDescription("Beállítások – minden almenüből");
        bar.addView(config,new LinearLayout.LayoutParams(dp(47),dp(36)));
        Button close=button("X",this::closePanel,false);
        close.setContentDescription("Almenü bezárása");
        LinearLayout.LayoutParams cp=new LinearLayout.LayoutParams(dp(47),dp(36));
        cp.leftMargin=dp(5);bar.addView(close,cp);
        sheet.addView(bar);
        TextView why=caption(currentSkin().cause,11,0xFFD2C5AA);
        why.setPadding(dp(4),dp(1),dp(4),dp(6));
        skin(why,"CAUSE");sheet.addView(why);
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
        addVertical(sheet,"Vissza a játékhoz",this::closePanel);
        FrameLayout.LayoutParams ps=new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM);
        ps.setMargins(dp(8),0,dp(8),bottomBarPx()+5);
        root.addView(sheet,ps);
        refreshLivingSkin();
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
        modal("3D FELBONTÁS",body->{
            addVertical(body,"Gyors – 640 képpont széles",()->{
                applyFramebufferSize(640);refreshScene();showSettings();
            });
            addVertical(body,"Kiegyensúlyozott – 840 képpont széles",()->{
                applyFramebufferSize(840);refreshScene();showSettings();
            });
            addVertical(body,"Részletes – 1200 képpont széles",()->{
                applyFramebufferSize(1200);refreshScene();showSettings();
            });
            body.addView(caption("A képarány megmarad. A valódi 3D világ töltődik be, nem előre rögzített kép.",
                    12,0xFFD2C5AA));
            addVertical(body,"Vissza a Beállításokhoz",this::showSettings);
        });
    }
    private void applyFramebufferSize(int desiredWidth){
        android.util.DisplayMetrics m=getResources().getDisplayMetrics();
        float scale=Math.min(1.0f,(float)desiredWidth/Math.max(m.widthPixels,m.heightPixels));
        int width=Math.max(320,Math.round(m.widthPixels*scale));
        int height=Math.max(180,Math.round(m.heightPixels*scale));
        surface.getHolder().setFixedSize(width,height);
    }
    private void openGameMenu(){
        modal("JÁTÉKMENÜ",body->{
            addVertical(body,"Folytatás",this::closeModal);
            addVertical(body,"Beállítások",this::showSettings);
            addVertical(body,"Mentés",this::save);
            addVertical(body,"Cél és környezet",()->{
                closeModal();showPanel(2);
            });
            addVertical(body,"Készítők",this::showCredits);
            addVertical(body,"Vissza a főmenübe",()->{
                saves.edit().putString("state",nativeSave()).commit();
                closeModal();showMainMenu();
            });
            addVertical(body,"Kilépés",this::confirmExit);
        });
    }
    private void setHudVisibility(boolean visible){
        for(View v:gameHud)v.setVisibility(visible?View.VISIBLE:View.GONE);
        if(visible)refreshNearby();
    }
    private void startPlaying(){
        closeModal();
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
        modal("ÚJ JÁTÉK",body->{
            addVertical(body,"Folytatás a mentett világból",this::startPlaying);
            addVertical(body,"Új világ – előbb biztonsági mentés",()->{
                modal("ÚJ VILÁG MEGERŐSÍTÉSE",confirm->{
                    confirm.addView(caption("A jelenlegi világ külön mentésbe kerül. "
                        +"Az új világban minden anyagot újra meg kell szerezni.",
                        13,0xFFE7D6BD));
                    addVertical(confirm,"Igen, új világ indítása",()->{
                        saves.edit().putString("backup_state",existing).commit();
                        nativeReset();
                        saves.edit().putString("state",nativeSave()).commit();
                        startPlaying();
                    });
                    addVertical(confirm,"Mégse",this::newWorldFromMenu);
                });
            });
        });
    }
    private void restoreOldWorld(){
        final String backup=saves.getString("backup_state",null);
        modal("KORÁBBI MENTÉS",body->{
            if(backup==null){
                body.addView(caption("Még nincs korábbi világ biztonsági másolata.",
                                  13,0xFFE7D6BD));
                addVertical(body,"Beállítások",this::showSettings);
                return;
            }
            body.addView(caption(
                "A jelenlegi világot külön biztonsági mentésbe tesszük. "
                +"A visszatöltés csak érvényes mentéssel történik.",
                13,0xFFE7D6BD));
            addVertical(body,"Visszaállítás",()->{
                String current=nativeSave();
                if(nativeLoad(backup)){
                    saves.edit().putString("state",nativeSave())
                        .putString("backup_state",current).commit();
                    refreshNearby();refreshScene();showSettings();
                }else{
                    modal("MENTÉSI HIBA",error->{
                        error.addView(caption("A mentés sérült vagy nem kompatibilis. "
                            +"Nem módosítottuk a világodat.",13,0xFFE7D6BD));
                        addVertical(error,"Vissza a Beállításokhoz",this::showSettings);
                    });
                }
            });
            addVertical(body,"Vissza a Beállításokhoz",this::showSettings);
        });
    }
    private void closeModal(){
        if(modalOverlay!=null && root!=null){
            root.removeView(modalOverlay);modalOverlay=null;
        }
    }
    private void modal(String title,Consumer<LinearLayout> populate){
        closeModal();
        modalOverlay=new FrameLayout(this);
        modalOverlay.setBackgroundColor(0xD8101411);
        modalOverlay.setOnClickListener(v->closeModal());
        LinearLayout card=new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(15),dp(10),dp(15),dp(12));
        card.setOnClickListener(v->{});
        skin(card,"CARD");
        LinearLayout header=new LinearLayout(this);
        header.setGravity(Gravity.CENTER_VERTICAL);
        FrameLayout logoFrame=new FrameLayout(this);
        logoFrame.setPadding(dp(4),dp(2),dp(4),dp(2));
        skin(logoFrame,"LOGO");
        logoFrame.addView(originalLogoOrTitle(),new FrameLayout.LayoutParams(-1,-1));
        logoFrame.addView(new LogoEnvironmentView(this),
                          new FrameLayout.LayoutParams(-1,-1));
        header.addView(logoFrame,new LinearLayout.LayoutParams(dp(104),dp(56)));
        TextView label=caption(title,16,0xFFF1E5CF);
        label.setGravity(Gravity.CENTER_VERTICAL);
        label.setPadding(dp(12),0,0,0);
        header.addView(label,new LinearLayout.LayoutParams(0,dp(48),1));
        if(!"BEÁLLÍTÁSOK".equals(title)){
            Button settings=button("⚙",this::showSettings,false);
            settings.setContentDescription("Beállítások ebben az almenüben");
            header.addView(settings,new LinearLayout.LayoutParams(dp(42),dp(39)));
        }
        Button close=button("X",this::closeModal,false);
        close.setContentDescription("Almenü bezárása");
        header.addView(close,new LinearLayout.LayoutParams(dp(42),dp(39)));
        card.addView(header);
        TextView cause=caption(currentSkin().cause,11,0xFFD2C5AA);
        cause.setPadding(dp(4),dp(7),dp(4),dp(9));skin(cause,"CAUSE");
        card.addView(cause);
        ScrollView scroll=new ScrollView(this);
        LinearLayout body=new LinearLayout(this);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setPadding(dp(3),dp(5),dp(3),dp(6));
        populate.accept(body);
        scroll.addView(body);
        card.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        Button back=button("Vissza",this::closeModal,false);
        card.addView(back,new LinearLayout.LayoutParams(-1,dp(42)));
        FrameLayout.LayoutParams size=new FrameLayout.LayoutParams(
                Math.min(dp(530),getResources().getDisplayMetrics().widthPixels-dp(24)),
                Math.min(dp(370),getResources().getDisplayMetrics().heightPixels-dp(20)),
                Gravity.CENTER);
        modalOverlay.addView(card,size);
        root.addView(modalOverlay,new FrameLayout.LayoutParams(-1,-1));
        refreshLivingSkin();
    }
    private void showSettings(){
        modal("BEÁLLÍTÁSOK",body->{
            addVertical(body,"3D felbontás – gyors / kiegyensúlyozott / részletes",
                    this::chooseGraphicsResolution);
            addVertical(body,"Izometrikus kamera",()->{
                orbit=.785398f;cameraPitch=.615480f;zoom=10f;
                nativeSetView(orbit,cameraPitch,zoom);refreshScene();
            });
            addVertical(body,"Kamera: felülnézet",()->{
                cameraPitch=1.37f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();
            });
            addVertical(body,"Kamera: alacsony nézet",()->{
                cameraPitch=.40f;nativeSetView(orbit,cameraPitch,zoom);refreshScene();
            });
            TextView explanation=caption(
                "IRÁNYÍTÁS: bal analóg = mozgás; FUTÁS = gyorsítás; jobb oldali húzás = kamera;"
                +" két ujj = nagyítás.\n"
                +"OK → FOLYAMAT → OKOZAT: az időjárás, a tűz, a nedvesség és a"
                +" tényleges felfedezések vezérlik az összes menü megjelenését.",12,0xFFE3D6BC);
            explanation.setPadding(dp(5),dp(10),dp(5),dp(10));
            body.addView(explanation);
            addVertical(body,"Korábbi mentés visszaállítása",this::restoreOldWorld);
            addVertical(body,"Mentés",this::save);
        });
    }
    private void showCredits(){
        modal("KÉSZÍTŐK",body->{
            TextView text=caption("TAKS WORLD · saját TAKS Engine\n"
                +"Natív Android, OpenGL ES 3, Wonderwerk-barlang.\n"
                +"Élő ok–okozati világ és kőzet/növény/embermodell teszt.\n\n"
                +"Az eredeti TAKS WORLD logó a projekt saját jelképe.\n"
                +"A realisztikus karakteranimáció és a történetileg ellenőrzött"
                +" élővilág fejlesztés alatt áll.",13,0xFFF1E5CF);
            text.setLineSpacing(dp(5),1f);body.addView(text);
        });
    }
    private void confirmExit(){
        modal("KILÉPÉS",body->{
            TextView why=caption("Kilépés előtt elmentjük a világ állapotát.\n"
                +"A játékból kilépés nem törli a korábbi mentést.",13,0xFFF1E5CF);
            why.setPadding(dp(5),dp(9),dp(5),dp(14));
            body.addView(why);
            addVertical(body,"Mentés és kilépés",()->{
                saves.edit().putString("state",nativeSave()).commit();
                closeModal();finish();
            });
            if(!inMainMenu)addVertical(body,"Mentés és vissza a főmenübe",()->{
                saves.edit().putString("state",nativeSave()).commit();
                closeModal();showMainMenu();
            });
        });
    }
    private View originalLogoOrTitle(){
        // This is an actual downscaled copy of the user's blue/silver TAKS
        // globe/tree logo, not a substitute image or invented design.
        for(String path:new String[]{"logo/TAKS_WORLD_LOGO.webp",
                                     "logo/TAKS_WORLD_LOGO.png"}){
            try(InputStream stream=getAssets().open(path)){
                Bitmap bitmap=BitmapFactory.decodeStream(stream);
                if(bitmap!=null){
                    ImageView logo=new ImageView(this);
                    logo.setImageBitmap(bitmap);
                    logo.setScaleType(ImageView.ScaleType.FIT_CENTER);
                    logo.setContentDescription("Az eredeti TAKS WORLD logó – élő környezeti fénykerettel");
                    return logo;
                }
            }catch(Exception missing){/* Try the other original format. */}
        }
        TextView title=caption("TAKS WORLD",25,0xFFE7D5B4);
        title.setGravity(Gravity.CENTER);title.setLetterSpacing(.12f);
        return title;
    }
    private void showMainMenu(){
        inMainMenu=true;
        worldHandler.removeCallbacks(worldLoop);
        padActive=false;moveIntent=false;sprinting=false;
        stopMovement();
        closeModal();closePanel();
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
        final int menuLogoHeight=Math.min(dp(134),Math.max(dp(65),usableHeight-4*menuButtonHeight-dp(78)));
        FrameLayout logoFrame=new FrameLayout(this);
        logoFrame.setPadding(dp(7),dp(4),dp(7),dp(4));
        skin(logoFrame,"LOGO");
        logoFrame.addView(originalLogoOrTitle(),new FrameLayout.LayoutParams(-1,-1));
        logoFrame.addView(new LogoEnvironmentView(this),
                          new FrameLayout.LayoutParams(-1,-1));
        LinearLayout.LayoutParams logoSize=new LinearLayout.LayoutParams(-1,menuLogoHeight);
        logoSize.bottomMargin=dp(5);
        stack.addView(logoFrame,logoSize);
        // Reflect only actual discoveries from the saved World, never a fake level.
        TextView discoveries=caption(natural.discoveryLine,11,natural.accent);
        discoveries.setGravity(Gravity.CENTER);
        discoveries.setPadding(dp(2),0,dp(2),dp(5));
        skin(discoveries,"DISCOVERY");
        stack.addView(discoveries,new LinearLayout.LayoutParams(-1,dp(22)));
        TextView causal=caption(currentSkin().cause,11,0xFFD2C5AA);
        causal.setGravity(Gravity.CENTER);
        causal.setMaxLines(2);
        causal.setPadding(dp(3),0,dp(3),dp(4));
        skin(causal,"CAUSE");
        stack.addView(causal,new LinearLayout.LayoutParams(-1,dp(33)));
        String[] labels={"ÚJ JÁTÉK","BEÁLLÍTÁSOK","KÉSZÍTŐK","KILÉPÉS"};
        Runnable[] actions={this::newWorldFromMenu,this::showSettings,this::showCredits,
                this::confirmExit};
        for(int i=0;i<labels.length;++i){
            Button menuButton=button(labels[i],actions[i],i==0);
            menuButton.setTextSize(16);
            menuButton.setLetterSpacing(.035f);
            // All four physical-looking buttons share the SAME live skin as
            // gameplay menus, settings, HUD and their "X" close controls.
            skin(menuButton,"BUTTON");
            LinearLayout.LayoutParams bp=new LinearLayout.LayoutParams(-1,menuButtonHeight);
            bp.bottomMargin=dp(5);
            stack.addView(menuButton,bp);
        }
        FrameLayout.LayoutParams p=new FrameLayout.LayoutParams(
            Math.min(dp(340),getResources().getDisplayMetrics().widthPixels-dp(32)),
            -2,Gravity.CENTER);
        menuCover.addView(stack,p);
        root.addView(menuCover,new FrameLayout.LayoutParams(-1,-1));
        refreshLivingSkin();
    }
    @Override public void onBackPressed(){
        if(modalOverlay!=null){closeModal();return;}
        if(sheet!=null){closePanel();return;}
        confirmExit();
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
        skin(header,"CARD");
        header.addView(caption("TAKS WORLD",22,0xFFE6D6B5));
        header.addView(caption("TAKS ENGINE · ÉLŐ VILÁG",10,0xFFC3D1C5));
        FrameLayout.LayoutParams hp=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.LEFT);
        hp.setMargins(dp(12),dp(12),0,0);root.addView(header,hp);gameHud.add(header);

        hint=caption("Bal: analóg mozgás · FUTÁS: nyomva tartva · Jobb: kamera · Két ujjal: zoom",12,0xFFECE9DC);
        hint.setPadding(dp(10),dp(7),dp(10),dp(7));
        skin(hint,"CARD");
        FrameLayout.LayoutParams hintParams=new FrameLayout.LayoutParams(-2,-2,Gravity.TOP|Gravity.CENTER_HORIZONTAL);
        hintParams.topMargin=dp(16);root.addView(hint,hintParams);gameHud.add(hint);
        hint.postDelayed(()->hint.setVisibility(View.GONE),6500);

        notice=caption(nativeGoal(),12,0xFFF1E3C9);
        notice.setPadding(dp(12),dp(9),dp(12),dp(9));
        skin(notice,"CARD");
        FrameLayout.LayoutParams np=new FrameLayout.LayoutParams(-1,-2,Gravity.BOTTOM);
        np.setMargins(dp(12),0,dp(12),bottomBarPx()+7);root.addView(notice,np);gameHud.add(notice);
        notice.postDelayed(()->{
            if(notice.getText().toString().startsWith("CÉL:")) notice.setVisibility(View.GONE);
        },9000);

        proximity=caption(nativeNearby(),12,0xFFE8E0CC);
        proximity.setPadding(dp(9),dp(6),dp(9),dp(6));
        skin(proximity,"CARD");
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
        skin(bottom,"CARD");
        Button build=button("ÉPÍTÉS",()->togglePanel(1),false);
        Button npc=button("TÁRSAK",()->togglePanel(3),false);
        Button info=button("TÁSKA",()->togglePanel(2),false);
        Button settings=button("⚙",this::showSettings,false);
        settings.setContentDescription("Beállítások: elérhető minden játékoldalon");
        Button menu=button("MENÜ",this::openGameMenu,false);
        for(Button b:new Button[]{build,npc,info,settings,menu}){
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
        skinHandler.removeCallbacks(skinLoop);
        skinHandler.post(skinLoop);
    }
    @Override protected void onResume(){
        super.onResume();
        if(surface!=null)surface.onResume();
        worldHandler.removeCallbacks(worldLoop);
        if(!inMainMenu)worldHandler.postDelayed(worldLoop,1800);
        skinHandler.removeCallbacks(skinLoop);
        skinHandler.post(skinLoop);
    }
    @Override protected void onPause(){
        padActive=false;moveIntent=false;sprinting=false;
        stopMovement();
        worldHandler.removeCallbacks(worldLoop);
        skinHandler.removeCallbacks(skinLoop);
        saves.edit().putString("state",nativeSave()).apply();
        if(surface!=null)surface.onPause();
        super.onPause();
    }
}
