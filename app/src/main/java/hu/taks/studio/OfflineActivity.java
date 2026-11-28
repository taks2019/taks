package hu.taks.studio;

import android.app.*;
import android.os.*;
import android.content.*;
import android.net.Uri;
import android.widget.*;
import android.view.*;
import java.io.*;
import java.net.*;
import java.security.MessageDigest;
import java.util.*;
import java.util.concurrent.*;
import ai.onnxruntime.*;

public class OfflineActivity extends Activity {
    private static final String BASE="https://huggingface.co/yapaymizeka/image-to-3d/resolve/main/";
    private static final String ENCODER="image_to_triplanes.onnx", DECODER="mlp.onnx";
    private static final long MAX=2_200_000_000L;
    private static final int IMPORT_ENCODER=71, IMPORT_DECODER=72;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private volatile boolean busy=false, stopped=false;
    private File dir;
    private TextView report, summary;
    private ProgressBar progress;
    private Button[] actions;
    private int dp(int n){return Math.round(n*getResources().getDisplayMetrics().density);}
    @Override public void onCreate(Bundle b){
        super.onCreate(b);
        dir=new File(getFilesDir(),"offline-triposr");dir.mkdirs();
        ScrollView sc=new ScrollView(this);LinearLayout p=new LinearLayout(this);p.setOrientation(1);p.setPadding(dp(16),dp(24),dp(16),dp(24));p.setBackgroundColor(0xff101820);sc.addView(p);setContentView(sc);
        if(Build.VERSION.SDK_INT>=35)sc.setOnApplyWindowInsetsListener((v,i)->{android.graphics.Insets a=i.getInsets(WindowInsets.Type.systemBars());v.setPadding(a.left,a.top,a.right,a.bottom);return i;});
        label(p,"Offline 3D motor",26);
        label(p,"TripoSR / ONNX • kísérleti modul. Jelenleg a modellek telepítése és futtatókörnyezeti ellenőrzése működésre tervezett; a kép → GLB generálás még nincs kész.",15);
        summary=label(p,"",14);
        actions=new Button[]{button(p,"AI modellek letöltése (kb. 1,6 GB)",()->new AlertDialog.Builder(this).setTitle("Egyszeri letöltés").setMessage("A Hugging Face tárhelyéről letöltjük a két nyilvános ONNX-modellt. Ez jelentős adatforgalom és tárhely. Nem töltünk fel képet.").setNegativeButton("Mégse",null).setPositiveButton("Letöltés",(d,w)->run(()->{download(ENCODER);download(DECODER);show("A két fájl helyben elérhető. Indítsd el az ellenőrzést.");})).show()),button(p,"ONNX kódoló importálása",()->pick(IMPORT_ENCODER)),button(p,"ONNX dekóder importálása",()->pick(IMPORT_DECODER)),button(p,"Helyi ONNX motor ellenőrzése",()->run(this::check))};
        progress=new ProgressBar(this,null,android.R.attr.progressBarStyleHorizontal);progress.setMax(100);p.addView(progress,new LinearLayout.LayoutParams(-1,dp(12)));
        report=label(p,"",14);report.setTextIsSelectable(true);
        button(p,"Vissza",()->{if(!busy)finish();else new AlertDialog.Builder(this).setMessage("A művelet megszakad. Visszalépsz?").setNegativeButton("Maradok",null).setPositiveButton("Vissza",(d,w)->finish()).show();});
        refresh();
    }
    private TextView label(LinearLayout p,String s,int size){TextView t=new TextView(this);t.setText(s);t.setTextSize(size);t.setTextColor(0xffdce8f4);t.setPadding(0,dp(7),0,dp(7));p.addView(t);return t;}
    private Button button(LinearLayout p,String s,Runnable r){Button b=new Button(this);b.setAllCaps(false);b.setText(s);p.addView(b);b.setOnClickListener(v->r.run());return b;}
    private void refresh(){summary.setText("Kódoló: "+state(ENCODER)+"\nDekóder: "+state(DECODER)+"\nSzabad tárhely: "+(dir.getUsableSpace()/1048576)+" MiB");}
    private String state(String n){File f=new File(dir,n);return f.isFile()?(f.length()/1024)+" KiB":"nincs telepítve";}
    private void show(String s){runOnUiThread(()->{if(!stopped)report.setText(s);});}
    private void tick(long n,long total){if(total>0){int value=(int)Math.min(100,n*100/total);runOnUiThread(()->{if(!stopped){progress.setIndeterminate(false);progress.setProgress(value);}});}}
    private interface Task{void go()throws Exception;}
    private void run(Task t){if(busy)return;busy=true;for(Button b:actions)b.setEnabled(false);progress.setIndeterminate(true);worker.execute(()->{try{t.go();}catch(Exception e){show("Hiba: "+e.getMessage());}catch(OutOfMemoryError e){show("Nincs elegendő memória az ONNX-modellhez.");}finally{busy=false;runOnUiThread(()->{if(!stopped){for(Button b:actions)b.setEnabled(true);progress.setIndeterminate(false);refresh();}});}});}
    private void pick(int code){startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("application/octet-stream").addCategory(Intent.CATEGORY_OPENABLE),code);}
    @Override protected void onActivityResult(int request,int result,Intent data){super.onActivityResult(request,result,data);if(result!=RESULT_OK||data==null||data.getData()==null)return;String name=request==IMPORT_ENCODER?ENCODER:request==IMPORT_DECODER?DECODER:null;if(name==null)return;Uri u=data.getData();run(()->{try(InputStream in=getContentResolver().openInputStream(u)){if(in==null)throw new IOException("Nem olvasható fájl");store(name,in,-1);}show("Helyi modell importálva: "+name);});}
    private void download(String name)throws Exception{
        if(new File(dir,name).isFile()){show("Már telepítve: "+name);return;}
        show("Letöltés: "+name);
        HttpURLConnection c=(HttpURLConnection)new URL(BASE+name).openConnection();
        c.setConnectTimeout(30000);c.setReadTimeout(60000);c.setInstanceFollowRedirects(true);
        try{if(c.getResponseCode()!=200||!"https".equals(c.getURL().getProtocol()))throw new IOException("Nem sikerült letölteni: HTTP "+c.getResponseCode());long size=c.getContentLengthLong();try(InputStream in=c.getInputStream()){store(name,in,size);}}finally{c.disconnect();}
    }
    private void store(String name,InputStream in,long expected)throws Exception{
        File dest=new File(dir,name);
        if(dest.exists())throw new IOException("A modell már telepítve van; nem írom felül.");
        if(expected>MAX||expected==0)throw new IOException("Hibás fájlméret");
        if(expected>0&&dir.getUsableSpace()<expected+128*1024*1024L)throw new IOException("Kevés szabad tárhely");
        File tmp=File.createTempFile("model-",".part",dir);boolean ok=false;
        try{
            byte[] b=new byte[65536];int n;long total=0;MessageDigest sha=MessageDigest.getInstance("SHA-256");
            try(FileOutputStream out=new FileOutputStream(tmp)){while((n=in.read(b))!=-1){if(stopped||Thread.currentThread().isInterrupted())throw new InterruptedIOException("Megszakítva");total+=n;if(total>MAX)throw new IOException("Méretkorlát");out.write(b,0,n);sha.update(b,0,n);tick(total,expected);}out.getFD().sync();}
            if(total<1000||(expected>0&&total!=expected))throw new IOException("Hiányos modellfájl");
            if(dest.exists()||!tmp.renameTo(dest))throw new IOException("Mentési hiba");
            ok=true;StringBuilder h=new StringBuilder();for(byte x:sha.digest())h.append(String.format(Locale.ROOT,"%02x",x&255));show("Mentve: "+name+"\nSHA-256 (helyi lenyomat, nem hitelesített gyári hash): "+h);
        }finally{if(!ok)tmp.delete();}
    }
    private void check()throws Exception{
        if(!new File(dir,ENCODER).isFile()||!new File(dir,DECODER).isFile())throw new IOException("Először telepítsd mindkét ONNX-modellt.");
        OrtEnvironment env=OrtEnvironment.getEnvironment();StringBuilder log=new StringBuilder("ONNX helyi betöltési ellenőrzés\n");
        for(String name:new String[]{ENCODER,DECODER}){
            if(stopped)throw new InterruptedIOException("Megszakítva");show(log+"\nBetöltés: "+name);
            try(OrtSession.SessionOptions opts=new OrtSession.SessionOptions()){
                opts.setIntraOpNumThreads(2);opts.setInterOpNumThreads(1);
                try(OrtSession session=env.createSession(new File(dir,name).getAbsolutePath(),opts)){
                    log.append(name).append("\nBemenet: ").append(session.getInputInfo()).append("\nKimenet: ").append(session.getOutputInfo()).append("\n");
                }
            }
        }
        log.append("Mindkét ONNX hálózat megnyitható. Ez még nem inference- vagy GLB-teszt.");show(log.toString());
    }
    @Override protected void onDestroy(){stopped=true;worker.shutdownNow();super.onDestroy();}
}
