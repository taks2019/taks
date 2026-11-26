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
    private void clearWeb(){ if(web!=null){web.stopLoading();web.destroy();web=null;} }
    private void start(String p,String title){
        clearWeb(); page=p;
        root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setBackgroundColor(0xff101820);
        if(Build.VERSION.SDK_INT>=35) root.setOnApplyWindowInsetsListener((v,insets)->{
            android.graphics.Insets b=insets.getInsets(WindowInsets.Type.systemBars());v.setPadding(b.left,b.top,b.right,b.bottom);return insets;
        });
        setContentView(root);
        TextView h=new TextView(this);h.setText(title);h.setTextColor(0xffe3edf6);h.setTextSize(24);h.setPadding(dp(16),dp(14),dp(16),dp(10));root.addView(h);
        status=new TextView(this);status.setTextColor(0xff9bb9cf);status.setTextSize(14);status.setPadding(dp(16),dp(4),dp(16),dp(10));root.addView(status);
    }
    private void msg(String text){if(status!=null)status.setText(text);}
    private Button button(LinearLayout p,String text,Runnable action){
        Button b=new Button(this);b.setAllCaps(false);b.setText(text);b.setTextColor(0xffe5f0ff);b.setBackgroundTintList(android.content.res.ColorStateList.valueOf(0xff29455b));
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,dp(52));lp.setMargins(dp(12),dp(4),dp(12),dp(4));p.addView(b,lp);b.setOnClickListener(v->action.run());return b;
    }
    private void home(){
        start("home","TAKS 3D Studio");
        msg("Valódi GLB modellek • Android • TAKS World");
        button(root,"Felhős 3D-generálás · TRELLIS.2",()->new AlertDialog.Builder(this).setTitle("Felhős generálás").setMessage("A képet a Hugging Face szolgáltatása dolgozza fel. Internet, esetleg bejelentkezés és GPU-kvóta szükséges.").setNegativeButton("Mégse",null).setPositiveButton("Megnyitás",(d,w)->cloud()).show());
        button(root,"Modell importálása (GLB)",()->pick(IMPORT,"*/*"));
        button(root,"Modellkönyvtár",()->list());
        button(root,"Használati útmutató",()->new AlertDialog.Builder(this).setTitle("Munkafolyamat").setMessage("1. Felhős 3D-generálás: tölts fel képet, Generate, majd Extract GLB.\n2. Töltsd le a GLB-t.\n3. Importáld a Letöltések mappából.\n4. Forgasd, nagyítsd vagy exportáld.\n\nA 4K opció a felhőszolgáltatástól függ. Az UltraTex és az automatikus csontváz még nincs integrálva.").setPositiveButton("Rendben",null).show());
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
        button(root,"Vissza a könyvtárba",()->list());button(root,"GLB exportálása",()->{
            Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);i.setType("model/gltf-binary");i.addCategory(Intent.CATEGORY_OPENABLE);i.putExtra(Intent.EXTRA_TITLE,f.getName());startActivityForResult(i,EXPORT);
        });
        button(root,"Modell törlése",()->new AlertDialog.Builder(this).setMessage("Törlöd a helyi másolatot?").setNegativeButton("Mégse",null).setPositiveButton("Törlés",(d,w)->{if(f.delete())list();else msg("Nem sikerült törölni.");}).show());
        web=new WebView(this);root.addView(web,new LinearLayout.LayoutParams(-1,0,1));WebSettings s=web.getSettings();s.setJavaScriptEnabled(true);s.setAllowFileAccess(false);s.setAllowContentAccess(false);s.setBlockNetworkLoads(false);s.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
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
