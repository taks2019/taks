package hu.taks.studio;

import android.app.*;
import android.content.*;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.*;
import android.provider.OpenableColumns;
import android.view.*;
import android.webkit.*;
import android.widget.*;
import java.io.*;
import java.nio.*;
import java.util.*;
import java.util.concurrent.*;

public class MainActivity extends Activity {
    private static final String SPACE = "https://huggingface.co/spaces/microsoft/TRELLIS.2";
    private static final String EMBED = "https://microsoft-trellis-2.hf.space/";
    private static final String LOCAL = "https://studio.invalid";
    private static final long LIMIT = 256L * 1024 * 1024;
    private static final int IMPORT=10, EXPORT=11, UPLOAD=12;
    private final ExecutorService io = Executors.newSingleThreadExecutor();
    private LinearLayout root;
    private TextView status;
    private File library, current;
    private WebView web;
    private ValueCallback<Uri[]> fileCallback;
    private String page="home";
    private int dp(float n){return (int)(n*getResources().getDisplayMetrics().density+0.5f);}
    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        library=new File(getFilesDir(),"models"); library.mkdirs();
        getWindow().setStatusBarColor(0xff101820); getWindow().setNavigationBarColor(0xff101820);
        home();
    }
    private void clearWeb(){ if(web!=null){web.stopLoading();if(web.getParent() instanceof android.view.ViewGroup)((android.view.ViewGroup)web.getParent()).removeView(web);web.destroy();web=null;} }
    private void start(String p,String title){
        clearWeb(); page=p;
        root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setBackgroundColor(0xff101820);
        if(Build.VERSION.SDK_INT>=35) root.setOnApplyWindowInsetsListener((v,insets)->{
            android.graphics.Insets b=insets.getInsets(WindowInsets.Type.systemBars());v.setPadding(b.left,b.top,b.right,b.bottom);return insets;
        });
        setContentView(root);
        TextView h=new TextView(this);h.setText(title);h.setTextColor(0xffe3edf6);h.setTextSize(26);h.setTypeface(android.graphics.Typeface.DEFAULT,android.graphics.Typeface.BOLD);h.setPadding(dp(16),dp(14),dp(16),dp(10));root.addView(h);
        status=new TextView(this);status.setTextColor(0xff9bb9cf);status.setTextSize(14);status.setPadding(dp(16),dp(4),dp(16),dp(10));root.addView(status);
    }
    private void msg(String text){if(status!=null)status.setText(text);}
    private Button button(LinearLayout p,String text,Runnable action){
        Button b=new Button(this);b.setAllCaps(false);b.setText(text);b.setTextColor(0xffe5f0ff);b.setBackground(surface(0xff25445b,0xff3e6680,13));
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,dp(52));lp.setMargins(dp(12),dp(4),dp(12),dp(4));p.addView(b,lp);b.setOnClickListener(v->action.run());return b;
    }
    private android.graphics.drawable.GradientDrawable surface(int color,int stroke,int radius){
        android.graphics.drawable.GradientDrawable d=new android.graphics.drawable.GradientDrawable();d.setColor(color);d.setCornerRadius(dp(radius));d.setStroke(dp(1),stroke);return d;
    }
    private TextView text(LinearLayout parent,String value,int size,int color,boolean bold){
        TextView t=new TextView(this);t.setText(value);t.setTextSize(size);t.setTextColor(color);if(bold)t.setTypeface(android.graphics.Typeface.DEFAULT,android.graphics.Typeface.BOLD);parent.addView(t);return t;
    }
    private LinearLayout section(LinearLayout parent,String title){
        TextView t=text(parent,title,12,0xff8fa6bb,true);t.setLetterSpacing(.12f);t.setPadding(dp(4),dp(16),0,dp(8));
        LinearLayout v=new LinearLayout(this);v.setOrientation(1);parent.addView(v);return v;
    }
    private void tiles(LinearLayout parent,String[][] items,Runnable[] actions){
        int cols=getResources().getConfiguration().screenWidthDp>=580?3:2;
        for(int i=0;i<items.length;i+=cols){
            LinearLayout row=new LinearLayout(this);row.setOrientation(0);parent.addView(row);
            for(int j=0;j<cols;j++){
                final int k=i+j;LinearLayout card=new LinearLayout(this);card.setOrientation(1);
                LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,dp(113),1);lp.setMargins(dp(4),dp(4),dp(4),dp(4));row.addView(card,lp);
                if(k>=items.length)continue;
                card.setPadding(dp(12),dp(10),dp(10),dp(8));card.setBackground(surface(0xff182735,0xff2b4356,15));card.setClickable(true);card.setFocusable(true);card.setContentDescription(items[k][1]);
                TextView icon=text(card,items[k][0],21,0xff75c4ef,true);icon.setIncludeFontPadding(false);
                TextView title=text(card,items[k][1],15,0xffeff7ff,true);title.setPadding(0,dp(5),0,dp(2));title.setMaxLines(2);
                TextView desc=text(card,items[k][2],12,0xffa9bdce,false);desc.setMaxLines(2);card.setOnClickListener(v->actions[k].run());
            }
        }
    }
    private void studio(String route){Intent i=new Intent(this,StudioActivity.class);i.putExtra("route",route);startActivity(i);}
    private void askCloud(){new AlertDialog.Builder(this).setTitle("Felhős generálás").setMessage("A képet a Hugging Face szolgáltatása dolgozza fel. Internet, esetleg bejelentkezés és GPU-kvóta szükséges.").setNegativeButton("Mégse",null).setPositiveButton("Megnyitás",(d,w)->cloud()).show();}
    private File latest(){File[] entries=library.listFiles((d,n)->n.endsWith(".glb"));if(entries==null||entries.length==0)return null;Arrays.sort(entries,Comparator.comparingLong(File::lastModified).reversed());return entries[0];}
    private void home(){
        start("home","TAKS 3D Studio");msg("MODELLGYÁRTÓ MŰHELY  /  ANDROID + PC");
        ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);root.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        LinearLayout body=new LinearLayout(this);body.setOrientation(1);body.setPadding(dp(12),0,dp(12),dp(24));scroll.addView(body);
        LinearLayout hero=new LinearLayout(this);hero.setOrientation(1);hero.setPadding(dp(12),dp(12),dp(12),dp(8));hero.setBackground(surface(0xff162530,0xff39576a,18));body.addView(hero);
        text(hero,"3D ELŐNÉZET",12,0xff79c5ec,true);
        File model=latest();
        if(model==null){
            TextView empty=text(hero,"＋",44,0xff7abfe5,false);empty.setGravity(Gravity.CENTER);empty.setHeight(dp(90));
            TextView caption=text(hero,"Importáld az első valódi 3D modelledet",15,0xffe2eef8,true);caption.setGravity(Gravity.CENTER);
            TextView detail=text(hero,"GLB 2.0 • forgatás • nagyítás",12,0xff9bb5c7,false);detail.setGravity(Gravity.CENTER);
            button(hero,"GLB megnyitása",()->pick(IMPORT,"*/*"));
        }else{
            TextView filename=text(hero,model.getName(),13,0xffd8e8f4,true);filename.setMaxLines(2);
            attachViewer(hero,new LinearLayout.LayoutParams(-1,dp(215)),model);
            button(hero,"Teljes képernyős 3D nézet",()->view(model));
        }
        LinearLayout gen=section(body,"01  /  MODELLEK ÉS ANYAGOK");
        tiles(gen,new String[][]{{"✦","3D generálás","PNG → GLB • felhő"},{"＋","Importálás","GLB fájl megnyitása"},{"▤","Modellkönyvtár","Helyi modellek"},{"◈","PBR anyagok","Szín • fémesség • érdesség"}},new Runnable[]{()->askCloud(),()->pick(IMPORT,"*/*"),()->list(),()->studio("material")});
        LinearLayout world=section(body,"02  /  VILÁG ÉS SZIMULÁCIÓ");
        tiles(world,new String[][]{{"♙","Karakterek és tárgyak","Tömeg • kor • állapot"},{"▧","Építés és ipar","Receptek • alkatrészek"},{"↳","Ok és okozat","Szabályok • próba"},{"⌖","3D világszerkesztő","Térbeli elhelyezés"}},new Runnable[]{()->studio("asset"),()->studio("recipes"),()->studio("rules"),()->studio("placements")});
        LinearLayout pc=section(body,"03  /  PC ÉS PROJEKT");
        tiles(pc,new String[][]{{"▣","PC / ComfyUI","Távoli GPU • API"},{"⇩","Projekt export","GLB + JSON ZIP"},{"⚙","Műhely","Összes szerkesztő"},{"?","Útmutató","Használat és korlátok"}},new Runnable[]{()->studio("remote"),()->studio("export"),()->studio("home"),()->new AlertDialog.Builder(this).setTitle("TAKS 3D Studio").setMessage("Importálj GLB-t vagy használd a felhős generálást. A PBR szerkesztő külön GLB-t készít. A PC kapcsolat HTTPS és API workflow használatát igényli. A csontváz- és animációgenerálás, UltraTex és a valódi 3D világszerkesztő még nincs integrálva.").setPositiveButton("Rendben",null).show()});
        TextView footer=text(body,"TAKS ENGINE  •  JÁTÉKASSET ELŐKÉSZÍTÉS",11,0xff71899c,true);footer.setPadding(dp(4),dp(18),0,0);
    }
    private void pick(int code,String mime){
        Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.addCategory(Intent.CATEGORY_OPENABLE);i.setType(mime);
        try{startActivityForResult(i,code);}catch(Exception e){msg("Nem nyitható meg a fájlválasztó.");}
    }
    private void browser(){try{startActivity(new Intent(Intent.ACTION_VIEW,Uri.parse(SPACE)));}catch(Exception e){msg("Nem található böngésző.");}}
    private void cloud(){
        start("cloud","Felhős 3D-generálás");msg("PNG → Generate → Extract GLB → letöltés");
        LinearLayout bar=new LinearLayout(this);bar.setOrientation(LinearLayout.HORIZONTAL);root.addView(bar);
        Button back=new Button(this);back.setText("Vissza");bar.addView(back,new LinearLayout.LayoutParams(0,-2,1));back.setOnClickListener(v->home());
        Button external=new Button(this);external.setText("Böngésző");bar.addView(external,new LinearLayout.LayoutParams(0,-2,1));external.setOnClickListener(v->browser());
        web=new WebView(this);root.addView(web,new LinearLayout.LayoutParams(-1,0,1));WebSettings s=web.getSettings();s.setJavaScriptEnabled(true);s.setDomStorageEnabled(true);s.setAllowFileAccess(false);s.setAllowContentAccess(true);s.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
        CookieManager.getInstance().setAcceptCookie(true);CookieManager.getInstance().setAcceptThirdPartyCookies(web,true);
        web.setWebViewClient(new WebViewClient(){@Override public boolean shouldOverrideUrlLoading(WebView v,WebResourceRequest r){return !"https".equals(r.getUrl().getScheme());}});
        web.setWebChromeClient(new WebChromeClient(){@Override public boolean onShowFileChooser(WebView v,ValueCallback<Uri[]> cb,FileChooserParams params){
            if(fileCallback!=null)fileCallback.onReceiveValue(null);fileCallback=cb;pick(UPLOAD,"image/*");return true;
        }});
        web.setDownloadListener((url,ua,cd,mime,len)->{
            try{
                Uri u=Uri.parse(url);if(!"https".equals(u.getScheme())){msg("A letöltéshez használd a Böngésző gombot.");return;}
                String name=URLUtil.guessFileName(url,cd,mime).replaceAll("[^a-zA-Z0-9._-]","_");
                DownloadManager.Request req=new DownloadManager.Request(u);req.setTitle(name);req.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);req.setDestinationInExternalPublicDir(Environment.DIRECTORY_DOWNLOADS,name);
                String cookie=CookieManager.getInstance().getCookie(url);if(cookie!=null)req.addRequestHeader("Cookie",cookie);if(ua!=null)req.addRequestHeader("User-Agent",ua);
                ((DownloadManager)getSystemService(DOWNLOAD_SERVICE)).enqueue(req);msg("Letöltés elindítva. Importáld a GLB-t a Letöltésekből.");
            }catch(Exception e){msg("Letöltési hiba. Próbáld a böngészőt.");}
        });
        web.loadUrl(EMBED);
    }
    private String name(Uri u){
        try(Cursor c=getContentResolver().query(u,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){if(c!=null&&c.moveToFirst())return c.getString(0);}catch(Exception ignored){}
        return "modell.glb";
    }
    private void importModel(Uri u){
        msg("GLB ellenőrzése és másolása…");
        io.execute(()->{
            File tmp=null;
            try{
                tmp=File.createTempFile("import-",".tmp",library);
                try(InputStream in=getContentResolver().openInputStream(u);OutputStream out=new FileOutputStream(tmp)){if(in==null)throw new IOException("Nem olvasható fájl");copy(in,out,LIMIT);}
                validate(tmp);
                String n=name(u).replaceAll("[^\\p{L}\\p{N}._-]","_");if(n.length()>100)n=n.substring(0,100);if(!n.toLowerCase(Locale.ROOT).endsWith(".glb"))n+=".glb";
                File dst=new File(library,System.currentTimeMillis()+"_"+n);
                if(!tmp.renameTo(dst))throw new IOException("Mentési hiba");
                runOnUiThread(()->view(dst));
            }catch(Exception e){if(tmp!=null)tmp.delete();runOnUiThread(()->msg("Importálási hiba: "+e.getMessage()));}
        });
    }
    private static void copy(InputStream in,OutputStream out,long max)throws IOException{byte[] buf=new byte[65536];long total=0;int n;while((n=in.read(buf))!=-1){total+=n;if(total>max)throw new IOException("A fájl meghaladja a 256 MB korlátot");out.write(buf,0,n);}}
    private static void validate(File f)throws IOException{
        try(RandomAccessFile r=new RandomAccessFile(f,"r")){
            if(r.length()<20)throw new IOException("Túl rövid GLB");
            byte[] b=new byte[12];r.readFully(b);ByteBuffer h=ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN);
            if(h.getInt()!=0x46546c67||h.getInt()!=2||Integer.toUnsignedLong(h.getInt())!=r.length())throw new IOException("Érvénytelen GLB 2.0 fejléc");
        }
    }
    private void list(){
        start("list","Modellkönyvtár");button(root,"Vissza",()->home());button(root,"Új GLB importálása",()->pick(IMPORT,"*/*"));
        File[] entries=library.listFiles((d,n)->n.endsWith(".glb"));msg(entries==null?"0 modell":entries.length+" modell • helyi tárhely");
        ScrollView sv=new ScrollView(this);root.addView(sv,new LinearLayout.LayoutParams(-1,0,1));LinearLayout l=new LinearLayout(this);l.setOrientation(1);sv.addView(l);
        if(entries!=null){Arrays.sort(entries,Comparator.comparingLong(File::lastModified).reversed());for(File f:entries)button(l,f.getName()+" · "+(f.length()/1024)+" KB",()->view(f));}
    }
    private void view(File f){
        current=f;start("viewer","3D modell");msg(f.getName());
        button(root,"Vissza a könyvtárba",()->list());button(root,"Modell tulajdonságai / anyagai",()->{Intent i=new Intent(this,StudioActivity.class);i.putExtra("model",f.getName());startActivity(i);});
        button(root,"GLB exportálása",()->{
            Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);i.setType("model/gltf-binary");i.addCategory(Intent.CATEGORY_OPENABLE);i.putExtra(Intent.EXTRA_TITLE,f.getName());startActivityForResult(i,EXPORT);
        });
        button(root,"Modell törlése",()->new AlertDialog.Builder(this).setMessage("Törlöd a helyi másolatot?").setNegativeButton("Mégse",null).setPositiveButton("Törlés",(d,w)->{if(f.delete())list();else msg("Nem sikerült törölni.");}).show());
        attachViewer(root,new LinearLayout.LayoutParams(-1,0,1),f);
    }
    private void attachViewer(LinearLayout parent,LinearLayout.LayoutParams params,File f){
        web=new WebView(this);parent.addView(web,params);WebSettings s=web.getSettings();s.setJavaScriptEnabled(true);s.setAllowFileAccess(false);s.setAllowContentAccess(false);s.setBlockNetworkLoads(false);s.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
        web.setWebViewClient(new WebViewClient(){
            @Override public boolean shouldOverrideUrlLoading(WebView v,WebResourceRequest req){return true;}
            @Override public WebResourceResponse shouldInterceptRequest(WebView v,WebResourceRequest req){
                Uri u=req.getUrl();try{
                    if("https".equals(u.getScheme())&&"studio.invalid".equals(u.getHost())){
                        if("/model.glb".equals(u.getPath()))return new WebResourceResponse("model/gltf-binary",null,new FileInputStream(f));
                        if("/assets/viewer.html".equals(u.getPath()))return new WebResourceResponse("text/html","UTF-8",getAssets().open("viewer.html"));
                        if("/assets/model-viewer.min.js".equals(u.getPath()))return new WebResourceResponse("text/javascript","UTF-8",getAssets().open("model-viewer.min.js"));
                    }
                }catch(IOException ignored){}
                return new WebResourceResponse("text/plain","UTF-8",404,"Not Found",Collections.emptyMap(),new ByteArrayInputStream(new byte[0]));
            }
        });web.loadUrl(LOCAL+"/assets/viewer.html");
    }
    @Override protected void onActivityResult(int code,int result,Intent data){
        super.onActivityResult(code,result,data);
        Uri u=result==RESULT_OK&&data!=null?data.getData():null;
        if(code==UPLOAD){if(fileCallback!=null){fileCallback.onReceiveValue(u==null?null:new Uri[]{u});fileCallback=null;}return;}
        if(u==null)return;
        if(code==IMPORT)importModel(u);
        if(code==EXPORT&&current!=null){File src=current;io.execute(()->{try(InputStream in=new FileInputStream(src);OutputStream out=getContentResolver().openOutputStream(u)){if(out==null)throw new IOException("Nem írható");copy(in,out,LIMIT);runOnUiThread(()->msg("GLB exportálva."));}catch(Exception e){runOnUiThread(()->msg("Exportálási hiba: "+e.getMessage()));}});}
    }
    @Override public void onBackPressed(){if("home".equals(page))super.onBackPressed();else if("viewer".equals(page))list();else home();}
    @Override protected void onDestroy(){clearWeb();if(fileCallback!=null)fileCallback.onReceiveValue(null);io.shutdown();super.onDestroy();}
}
