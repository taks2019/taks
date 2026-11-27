package hu.taks.studio;
import android.app.*;
import android.os.*;
import android.webkit.*;
import android.view.*;
import android.widget.*;
import android.net.Uri;
import org.json.*;
import java.io.*;
import java.util.*;
import java.nio.charset.StandardCharsets;
public class SceneActivity extends Activity {
    private WebView web;
    private File project, models;
    private static final String ORIGIN="https://scene.taks.invalid";
    private static final long LIMIT=256L*1024*1024;
    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        project=new File(getFilesDir(),"studio-project.json");models=new File(getFilesDir(),"models");
        LinearLayout l=new LinearLayout(this);l.setOrientation(1);l.setBackgroundColor(0xff101820);
        if(Build.VERSION.SDK_INT>=35)l.setOnApplyWindowInsetsListener((v,i)->{android.graphics.Insets b=i.getInsets(WindowInsets.Type.systemBars());v.setPadding(b.left,b.top,b.right,b.bottom);return i;});
        Button back=new Button(this);back.setText("Vissza a műhelybe");back.setAllCaps(false);back.setOnClickListener(v->finish());l.addView(back);web=new WebView(this);l.addView(web,new LinearLayout.LayoutParams(-1,0,1));setContentView(l);
        WebSettings s=web.getSettings();s.setJavaScriptEnabled(true);s.setAllowFileAccess(false);s.setAllowContentAccess(false);s.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
        web.addJavascriptInterface(new Bridge(),"TAKS");
        web.setWebViewClient(new WebViewClient(){
            @Override public boolean shouldOverrideUrlLoading(WebView v,WebResourceRequest req){return true;}
            @Override public WebResourceResponse shouldInterceptRequest(WebView v,WebResourceRequest req){
                Uri u=req.getUrl();try{if("https".equals(u.getScheme())&&"scene.taks.invalid".equals(u.getHost())){
                    String p=u.getPath();if("/scene.html".equals(p))return response("text/html",getAssets().open("scene.html"));
                    if("/scene.bundle.js".equals(p))return response("text/javascript",getAssets().open("scene.bundle.js"));
                    if("/project".equals(p)){JSONObject j=readProject();String[] names=models.list((d,n)->n.endsWith(".glb"));j.put("library",new JSONArray(names==null?new String[0]:names));return response("application/json",new ByteArrayInputStream(j.toString().getBytes(StandardCharsets.UTF_8)));}
                    if("/model".equals(p)){File f=file(u.getQueryParameter("name"));if(f.isFile()&&f.length()<=LIMIT)return response("model/gltf-binary",new FileInputStream(f));}
                }}catch(Exception ignored){}return new WebResourceResponse("text/plain","UTF-8",404,"Not Found",Collections.emptyMap(),new ByteArrayInputStream(new byte[0]));
            }
        });web.loadUrl(ORIGIN+"/scene.html");
    }
    private WebResourceResponse response(String mime,InputStream in){return new WebResourceResponse(mime,mime.startsWith("model/")?null:"UTF-8",in);}
    private File file(String name)throws IOException{if(name==null||name.contains("/")||name.contains("\\")||!name.endsWith(".glb"))throw new IOException("Érvénytelen modell");File f=new File(models,name);if(!f.getCanonicalFile().getParentFile().equals(models.getCanonicalFile()))throw new IOException("Érvénytelen útvonal");return f;}
    private synchronized JSONObject readProject()throws Exception{if(!project.exists())return new JSONObject().put("schema","taks.asset-project/1").put("assets",new JSONArray()).put("recipes",new JSONArray()).put("rules",new JSONArray()).put("placements",new JSONArray());if(project.length()>4*1024*1024)throw new IOException("Túl nagy projekt");try(InputStream in=new FileInputStream(project);ByteArrayOutputStream out=new ByteArrayOutputStream()){byte[] b=new byte[8192];int n;while((n=in.read(b))!=-1)out.write(b,0,n);return new JSONObject(new String(out.toByteArray(),StandardCharsets.UTF_8));}}
    private class Bridge{
        @JavascriptInterface public String save(String value){try{
            if(value.length()>512*1024)throw new IOException("Túl nagy jelenet");JSONArray a=new JSONArray(value);if(a.length()>64)throw new IOException("Legfeljebb 64 objektum");
            JSONArray clean=new JSONArray();for(int i=0;i<a.length();i++){JSONObject x=a.getJSONObject(i);String n=x.getString("file");if(!file(n).isFile())throw new IOException("Hiányzó GLB");JSONObject y=new JSONObject().put("file",n);for(String k:new String[]{"x","y","z","rotationY","scale"}){double v=x.getDouble(k);if(!Double.isFinite(v)||Math.abs(v)>100000||("scale".equals(k)&&v<=0))throw new IOException("Érvénytelen koordináta");y.put(k,v);}clean.put(y);}
            synchronized(SceneActivity.this){JSONObject j=readProject();j.put("placements",clean);android.util.AtomicFile af=new android.util.AtomicFile(project);FileOutputStream out=af.startWrite();try{out.write(j.toString(2).getBytes(StandardCharsets.UTF_8));af.finishWrite(out);}catch(Exception e){af.failWrite(out);throw e;}}return "OK";
        }catch(Exception e){return "Hiba: "+e.getMessage();}}
    }
    @Override protected void onDestroy(){if(web!=null){web.removeJavascriptInterface("TAKS");web.destroy();}super.onDestroy();}
}
