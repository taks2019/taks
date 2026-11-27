package hu.taks.studio;

import android.app.*;
import android.content.*;
import android.net.Uri;
import android.os.*;
import android.view.*;
import android.widget.*;
import org.json.*;
import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.*;
import java.util.zip.*;

public class StudioActivity extends Activity {
    private static final int OPEN_PROMPT=51, OPEN_IMAGE=52, EXPORT=53;
    private static final long MAX_MODEL=256L*1024*1024;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private LinearLayout root, panel;
    private TextView status;
    private File project, models;
    private JSONObject data, prompt;
    private byte[] inputImage;
    private String inputImageName="input.png", page="home", selected="", lastPrompt="";
    private EditText endpoint, token, imageNode;
    private JSONArray remoteFiles=new JSONArray();
    private interface Work { void run() throws Exception; }
    private interface Pick { void use(String v); }
    private int dp(int x){return Math.round(x*getResources().getDisplayMetrics().density);}
    @Override public void onCreate(Bundle b){
        super.onCreate(b);project=new File(getFilesDir(),"studio-project.json");models=new File(getFilesDir(),"models");models.mkdirs();
        try{data=new JSONObject(read(new FileInputStream(project),4*1024*1024));}catch(Exception e){data=new JSONObject();}
        try{data.put("schema","taks.asset-project/1");for(String k:new String[]{"assets","recipes","rules","placements"})if(!data.has(k))data.put(k,new JSONArray());}catch(Exception e){throw new IllegalStateException(e);}
        selected=getIntent().getStringExtra("model");
        if(selected!=null&&new File(models,selected).isFile()){asset(selected);return;}
        String route=getIntent().getStringExtra("route");home();
        if("asset".equals(route))pickModel(f->asset(f));
        else if("material".equals(route))pickModel(f->material(f));
        else if("recipes".equals(route))recipes();
        else if("rules".equals(route))rules();
        else if("placements".equals(route))startActivity(new Intent(this,SceneActivity.class));
        else if("remote".equals(route))remote();
        else if("export".equals(route)){Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/zip").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_TITLE,"TAKS_Project.zip");startActivityForResult(i,EXPORT);}

    }
    private static String read(InputStream in,int limit)throws IOException{try(InputStream x=in){return new String(bytes(x,limit),StandardCharsets.UTF_8);}}
    private static byte[] bytes(InputStream in,int limit)throws IOException{ByteArrayOutputStream b=new ByteArrayOutputStream();transfer(in,b,limit);return b.toByteArray();}
    private static void transfer(InputStream in,OutputStream out,long max)throws IOException{byte[] b=new byte[65536];int n;long sum=0;while((n=in.read(b))!=-1){sum+=n;if(sum>max)throw new IOException("Méretkorlát túllépve");out.write(b,0,n);}}
    @Override protected void onResume(){super.onResume();if(project!=null&&project.isFile())try{data=new JSONObject(read(new FileInputStream(project),4*1024*1024));}catch(Exception ignored){}}
    private void screen(String key,String title,String desc){
        page=key;root=new LinearLayout(this);root.setOrientation(1);root.setBackgroundColor(0xff101820);
        if(Build.VERSION.SDK_INT>=35)root.setOnApplyWindowInsetsListener((v,i)->{android.graphics.Insets a=i.getInsets(WindowInsets.Type.systemBars());v.setPadding(a.left,a.top,a.right,a.bottom);return i;});
        setContentView(root);label(root,title,24,0xffd9edff);status=label(root,desc,14,0xff92bad4);
        ScrollView sc=new ScrollView(this);root.addView(sc,new LinearLayout.LayoutParams(-1,0,1));panel=new LinearLayout(this);panel.setOrientation(1);panel.setPadding(dp(12),0,dp(12),dp(20));sc.addView(panel);
        if(!key.equals("home"))btn("Vissza a műhelybe",()->home());
    }
    private TextView label(LinearLayout p,String s,int size,int color){TextView t=new TextView(this);t.setText(s);t.setTextSize(size);t.setTextColor(color);t.setPadding(dp(8),dp(8),dp(8),dp(8));p.addView(t);return t;}
    private void info(String s){label(panel,s,14,0xffa3bed1);}
    private void msg(String s){runOnUiThread(()->{if(status!=null)status.setText(s);});}
    private void btn(String title,Runnable r){Button b=new Button(this);b.setAllCaps(false);b.setText(title);b.setTextColor(0xffe8f2ff);android.graphics.drawable.GradientDrawable bg=new android.graphics.drawable.GradientDrawable();bg.setColor(0xff21394b);bg.setCornerRadius(dp(12));bg.setStroke(dp(1),0xff39566d);b.setBackground(bg);LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,dp(52));lp.setMargins(0,dp(5),0,dp(5));panel.addView(b,lp);b.setOnClickListener(v->r.run());}
    private EditText field(String title,String value){info(title);EditText e=new EditText(this);e.setTextColor(0xffe8f2ff);e.setText(value);e.setSingleLine(false);e.setPadding(dp(12),dp(10),dp(12),dp(10));android.graphics.drawable.GradientDrawable bg=new android.graphics.drawable.GradientDrawable();bg.setColor(0xff1a2834);bg.setCornerRadius(dp(10));bg.setStroke(dp(1),0xff32495c);e.setBackground(bg);panel.addView(e,new LinearLayout.LayoutParams(-1,-2));return e;}
    private Spinner choices(String title,String[] values,String current){info(title);Spinner s=new Spinner(this);ArrayAdapter<String> a=new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,values);s.setAdapter(a);for(int i=0;i<values.length;i++)if(values[i].equals(current))s.setSelection(i);panel.addView(s);return s;}
    private void run(Work w){worker.execute(()->{try{w.run();}catch(Exception e){msg("Hiba: "+e.getMessage());}});}
    private void save()throws Exception{android.util.AtomicFile a=new android.util.AtomicFile(project);FileOutputStream o=a.startWrite();try{o.write(data.toString(2).getBytes(StandardCharsets.UTF_8));a.finishWrite(o);}catch(Exception e){a.failWrite(o);throw e;}}
    private JSONArray array(String k){return data.optJSONArray(k);}
    private void add(String k,JSONObject o){array(k).put(o);try{save();msg("Mentve");}catch(Exception e){msg(e.toString());}}
    private void home(){screen("home","TAKS műhely","Helyi szerkesztők és távoli GPU");
        btn("Modelladatok · karakter, állat, növény, gép",()->pickModel(f->asset(f)));
        btn("PBR anyag · valódi GLB szerkesztés",()->pickModel(f->material(f)));
        btn("Építés és gyártás · alkatrészreceptek",()->recipes());
        btn("Ok és okozat · szabályok és próba",()->rules());
        btn("Világterv · valódi 3D jelenet",()->startActivity(new Intent(this,SceneActivity.class)));
        btn("Objektumok XYZ adatai",()->placements());
        btn("PC / ComfyUI · API kapcsolat",()->remote());
        btn("Projekt exportálása · GLB + JSON ZIP",()->{Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/zip").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_TITLE,"TAKS_Project.zip");startActivityForResult(i,EXPORT);});
        btn("Vissza a Studio főmenübe",()->finish());
        info("A csontváz és animáció generálása még nem része ennek a kiadásnak. A szabálypróba számszerű modell, nem a játék fizikai motorja.");
    }
    private String[] modelNames(){String[] a=models.list((d,n)->n.endsWith(".glb"));if(a==null)return new String[0];Arrays.sort(a);return a;}
    private void pickModel(Pick p){String[] a=modelNames();if(a.length==0){msg("Először importálj egy GLB-t a főmenüben.");return;}new AlertDialog.Builder(this).setTitle("Modell kiválasztása").setItems(a,(d,n)->p.use(a[n])).show();}
    private JSONObject assetData(String f){JSONArray a=array("assets");for(int i=0;i<a.length();i++){JSONObject o=a.optJSONObject(i);if(o!=null&&f.equals(o.optString("file")))return o;}return new JSONObject();}
    private static double num(EditText e){double x=Double.parseDouble(e.getText().toString().trim().replace(',','.'));if(!Double.isFinite(x))throw new IllegalArgumentException("Véges szám szükséges");return x;}
    private static double nonnegative(EditText e){double n=num(e);if(n<0)throw new IllegalArgumentException("Nem lehet negatív");return n;}
    private static double unit(EditText e){double n=num(e);if(n<0||n>1)throw new IllegalArgumentException("0 és 1 közötti érték szükséges");return n;}
    private static String val(EditText e){return e.getText().toString().trim();}
    private void asset(String f){JSONObject o=assetData(f);screen("asset","Modell tulajdonságai",f);
        Spinner category=choices("Típus",new String[]{"karakter","állat","növény","talaj","épület","alkatrész","gép","jármű","repülőgép","egyéb"},o.optString("category","egyéb"));
        EditText name=field("Megnevezés",o.optString("name",f)), mass=field("Tömeg (kg)",o.optString("massKg","1")), material=field("Anyag",o.optString("material","kő")), hp=field("Állapot (0–1)",o.optString("health","1")), moisture=field("Nedvesség (0–1)",o.optString("moisture","0")), age=field("Kor (nap)",o.optString("ageDays","0")), collision=field("Ütközési forma (pl. mesh, convex, capsule)",o.optString("collision","mesh")), sockets=field("Csatlakozási pontok nevei, vesszővel",o.optString("sockets","")), anim=field("Meglévő animációk / sérülési állapotok nevei",o.optString("animationTags",""));
        btn("Tulajdonságok mentése",()->{try{
            JSONObject n=new JSONObject().put("file",f).put("name",val(name)).put("category",category.getSelectedItem().toString()).put("massKg",nonnegative(mass)).put("material",val(material)).put("health",unit(hp)).put("moisture",unit(moisture)).put("ageDays",nonnegative(age)).put("collision",val(collision)).put("sockets",val(sockets)).put("animationTags",val(anim));
            JSONArray ar=array("assets");for(int i=ar.length()-1;i>=0;i--)if(f.equals(ar.optJSONObject(i).optString("file")))ar.remove(i);add("assets",n);
        }catch(Exception e){msg(e.getMessage());}});
        btn("GLB anyag módosítása",()->material(f));info("Ezek játékadatok. A csatlakozási pontok és animációnevek nem hoznak létre csontvázat vagy animációt.");
    }
    private static int le(RandomAccessFile r)throws IOException{return Integer.reverseBytes(r.readInt());}
    private static void le(DataOutputStream d,int n)throws IOException{d.writeInt(Integer.reverseBytes(n));}
    private static JSONObject glbJson(RandomAccessFile r)throws Exception{
        if(le(r)!=0x46546c67||le(r)!=2||Integer.toUnsignedLong(le(r))!=r.length())throw new IOException("Érvénytelen GLB");
        int n=le(r),type=le(r);if(type!=0x4e4f534a||n<2||n>32*1024*1024||20L+n>r.length())throw new IOException("Érvénytelen GLB JSON");byte[] b=new byte[n];r.readFully(b);return new JSONObject(new String(b,StandardCharsets.UTF_8).trim());
    }
    private void material(String f){screen("material","PBR anyag módosítása",f);
        info("Új GLB-másolatot készít; az eredeti és a textúrák érintetlenek maradnak. A kiválasztott anyag szín-, érdesség- és fémességfaktorát módosítja.");
        try(RandomAccessFile rr=new RandomAccessFile(new File(models,f),"r")){
            JSONObject j=glbJson(rr);JSONArray mats=j.optJSONArray("materials");if(mats==null||mats.length()==0){msg("Nincs szerkeszthető anyag a GLB-ben.");return;}
            String[] names=new String[mats.length()];for(int i=0;i<names.length;i++)names[i]=i+" · "+mats.getJSONObject(i).optString("name","Anyag");
            Spinner sel=choices("Anyag",names,names[0]);EditText color=field("Szín (R,G,B,A; 0–1)","1, 1, 1, 1"),rough=field("Érdesség (0–1)","0.8"),metal=field("Fémesség (0–1)","0");
            Runnable fill=()->{JSONObject p=mats.optJSONObject(sel.getSelectedItemPosition()).optJSONObject("pbrMetallicRoughness");if(p==null)p=new JSONObject();JSONArray c=p.optJSONArray("baseColorFactor");color.setText(c==null?"1, 1, 1, 1":c.optDouble(0)+", "+c.optDouble(1)+", "+c.optDouble(2)+", "+c.optDouble(3));rough.setText(""+p.optDouble("roughnessFactor",1));metal.setText(""+p.optDouble("metallicFactor",1));};
            sel.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){public void onItemSelected(AdapterView<?> p,View v,int pos,long id){fill.run();}public void onNothingSelected(AdapterView<?> p){}});fill.run();
            btn("Új GLB mentése a könyvtárba",()->{try{String[] c=val(color).split(",");if(c.length!=4)throw new IllegalArgumentException("Négy színérték szükséges");JSONArray rgba=new JSONArray();for(String v:c){double n=Double.parseDouble(v.trim());if(!Double.isFinite(n)||n<0||n>1)throw new IllegalArgumentException("Színérték: 0–1");rgba.put(n);}int idx=sel.getSelectedItemPosition();double ro=unit(rough),me=unit(metal);run(()->{File dst=editMaterial(new File(models,f),idx,rgba,ro,me);msg("Elmentve: "+dst.getName());});}catch(Exception e){msg(e.getMessage());}});
        }catch(Exception e){msg("GLB hiba: "+e.getMessage());}
    }
    private File editMaterial(File src,int idx,JSONArray rgba,double rough,double metal)throws Exception{
        File out=File.createTempFile("PBR_",".glb",models);boolean success=false;
        try(RandomAccessFile r=new RandomAccessFile(src,"r");DataOutputStream d=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(out)))){
            JSONObject j=glbJson(r);JSONObject m=j.getJSONArray("materials").getJSONObject(idx),p=m.optJSONObject("pbrMetallicRoughness");if(p==null){p=new JSONObject();m.put("pbrMetallicRoughness",p);}p.put("baseColorFactor",rgba).put("roughnessFactor",rough).put("metallicFactor",metal);
            byte[] b=j.toString().getBytes(StandardCharsets.UTF_8);int padded=(b.length+3)&~3;long total=20L+padded+(r.length()-r.getFilePointer());if(total>MAX_MODEL)throw new IOException("Túl nagy GLB");le(d,0x46546c67);le(d,2);le(d,(int)total);le(d,padded);le(d,0x4e4f534a);d.write(b);for(int k=b.length;k<padded;k++)d.write(32);byte[] buf=new byte[65536];int n;while((n=r.read(buf))!=-1)d.write(buf,0,n);d.flush();success=true;
        }finally{if(!success)out.delete();}return out;
    }
    private void records(String k){JSONArray a=array(k);for(int i=0;i<a.length();i++){final int at=i;JSONObject o=a.optJSONObject(i);if(o==null)continue;info(o.toString());btn("Törlés · "+o.optString("name",o.optString("file","elem")),()->new AlertDialog.Builder(this).setMessage("Törlöd ezt a rekordot?").setNegativeButton("Mégse",null).setPositiveButton("Törlés",(d,w)->{a.remove(at);try{save();}catch(Exception e){msg(e.getMessage());}if(k.equals("recipes"))recipes();else if(k.equals("rules"))rules();else placements();}).show());}}
    private void recipes(){screen("recipes","Építés és gyártás","Menthető alkatrészrecept, idő és munka");
        EditText name=field("Építési lépés / termék","Alapozás"), inputs=field("Bemenetek (soronként név=mennyiség)","kő=10\nfa=2"), output=field("Eredmény","alap"), duration=field("Munkaidő (másodperc)","60"), prereq=field("Előfeltétel / előző lépés","");
        btn("Recept mentése",()->{try{JSONObject req=new JSONObject();for(String line:val(inputs).split("\\r?\\n")){if(line.trim().isEmpty())continue;String[] p=line.split("=",2);if(p.length!=2||p[0].trim().isEmpty())throw new IllegalArgumentException("név=mennyiség formátum");double n=Double.parseDouble(p[1].trim());if(!Double.isFinite(n)||n<=0)throw new IllegalArgumentException("Pozitív mennyiség szükséges");req.put(p[0].trim(),n);}if(val(name).isEmpty()||val(output).isEmpty())throw new IllegalArgumentException("Név és eredmény kötelező");add("recipes",new JSONObject().put("name",val(name)).put("inputs",req).put("output",val(output)).put("durationSec",nonnegative(duration)).put("prerequisite",val(prereq)));recipes();}catch(Exception e){msg(e.getMessage());}});
        records("recipes");
    }
    private void rules(){screen("rules","Ok és okozat","Számszerű feltételek és következmények");
        EditText name=field("Szabály neve","Eső → nedvesség"), source=field("Bemeneti állapot neve","rain"), threshold=field("Küszöb","0.5");
        Spinner op=choices("Feltétel",new String[]{">=","<=",">","<","==", "!="},">=");
        EditText target=field("Módosított állapot neve","soil_moisture"), delta=field("Változás","0.1");
        btn("Szabály mentése",()->{try{if(val(source).isEmpty()||val(target).isEmpty())throw new IllegalArgumentException("Állapot neve kötelező");add("rules",new JSONObject().put("name",val(name)).put("source",val(source)).put("op",op.getSelectedItem().toString()).put("threshold",num(threshold)).put("target",val(target)).put("delta",num(delta)));rules();}catch(Exception e){msg(e.getMessage());}});
        EditText init=field("Próba kiinduló állapotai (JSON)","{\"rain\":1,\"soil_moisture\":0}");
        btn("Egy szimulációs lépés",()->{try{JSONObject state=new JSONObject(val(init));JSONArray a=array("rules");int hits=0;for(int i=0;i<a.length();i++){JSONObject rule=a.getJSONObject(i);double x=state.optDouble(rule.getString("source"),0),y=rule.getDouble("threshold");String opv=rule.getString("op");boolean pass=opv.equals(">=")?x>=y:opv.equals("<=")?x<=y:opv.equals(">")?x>y:opv.equals("<")?x<y:opv.equals("==")?x==y:x!=y;if(pass){String k=rule.getString("target");state.put(k,state.optDouble(k,0)+rule.getDouble("delta"));hits++;}}init.setText(state.toString(2));msg(hits+" szabály aktiválódott (sorrendben).");}catch(Exception e){msg(e.getMessage());}});
        records("rules");
    }
    private void placements(){screen("placements","Világterv","Modell elhelyezési adatok; nem valós idejű 3D jelenetszerkesztő");
        btn("GLB elhelyezése",()->pickModel(f->{screen("placement","Objektum helyzete",f);EditText x=field("X (m)","0"),y=field("Y (m)","0"),z=field("Z (m)","0"),angle=field("Forgatás Y tengely körül (fok)","0"),scale=field("Méretarány","1");btn("Elhelyezés mentése",()->{try{double s=num(scale);if(s<=0)throw new IllegalArgumentException("Pozitív méretarány szükséges");add("placements",new JSONObject().put("file",f).put("x",num(x)).put("y",num(y)).put("z",num(z)).put("rotationY",num(angle)).put("scale",s));placements();}catch(Exception e){msg(e.getMessage());}});}));
        records("placements");
    }
    private void remote(){screen("remote","PC / ComfyUI","HTTPS-en elérhető, működő saját ComfyUI szerver szükséges.");
        endpoint=field("HTTPS szervercím",getPreferences(0).getString("endpoint",""));endpoint.setSingleLine(true);token=field("Bearer token (opcionális, csak munkamenet)", "");token.setInputType(129);
        btn("Kapcsolat tesztelése",()->{String ep=val(endpoint),auth=val(token);run(()->{JSONObject j=new JSONObject(request(ep,auth,"GET","/system_stats",null,null));getPreferences(0).edit().putString("endpoint",ep).apply();msg("Kapcsolat OK: "+j.optJSONObject("system"));});});
        btn("ComfyUI API-formátumú JSON betöltése",()->open(OPEN_PROMPT,"*/*"));
        info(prompt==null?"Nincs API workflow":prompt.length()+" node betöltve");btn("PNG / JPEG kiválasztása",()->open(OPEN_IMAGE,"image/*"));info(inputImage==null?"Nincs új kép":inputImageName+" betöltve");
        imageNode=field("LoadImage node azonosítója (új kép esetén)","3");
        btn("Generálás indítása",()->{try{if(prompt==null)throw new IOException("API-formátumú JSON szükséges");String ep=val(endpoint),auth=val(token),id=val(imageNode);JSONObject p=new JSONObject(prompt.toString());byte[] img=inputImage;String fn=inputImageName;run(()->{
            if(img!=null){JSONObject node=p.optJSONObject(id);if(node==null||!"LoadImage".equals(node.optString("class_type")))throw new IOException("A megadott node nem LoadImage");String boundary="TAKS"+UUID.randomUUID();ByteArrayOutputStream b=new ByteArrayOutputStream();b.write(("--"+boundary+"\r\nContent-Disposition: form-data; name=\"image\"; filename=\""+fn+"\"\r\nContent-Type: application/octet-stream\r\n\r\n").getBytes(StandardCharsets.UTF_8));b.write(img);b.write(("\r\n--"+boundary+"\r\nContent-Disposition: form-data; name=\"type\"\r\n\r\ninput\r\n--"+boundary+"--\r\n").getBytes(StandardCharsets.UTF_8));JSONObject uploaded=new JSONObject(request(ep,auth,"POST","/upload/image",b.toByteArray(),"multipart/form-data; boundary="+boundary));String sub=uploaded.optString("subfolder","");node.getJSONObject("inputs").put("image",(sub.isEmpty()?"":sub+"/")+uploaded.getString("name"));}
            JSONObject res=new JSONObject(request(ep,auth,"POST","/prompt",new JSONObject().put("prompt",p).put("client_id",UUID.randomUUID().toString()).toString().getBytes(StandardCharsets.UTF_8),"application/json"));lastPrompt=res.getString("prompt_id");msg("Sorba állítva: "+lastPrompt);});}catch(Exception e){msg(e.getMessage());}});
        btn("Állapot / GLB kimenetek lekérése",()->{String ep=val(endpoint),auth=val(token),id=lastPrompt;run(()->{
            if(id.isEmpty())throw new IOException("Még nincs elküldött feladat");JSONObject j=new JSONObject(request(ep,auth,"GET","/history/"+id,null,null));JSONObject h=j.optJSONObject(id);if(h==null){msg("A feladat még nem szerepel az előzményekben.");return;}JSONArray found=new JSONArray();collect(h.optJSONObject("outputs"),found);remoteFiles=found;msg("Állapot: "+h.optJSONObject("status")+" · GLB: "+found.length());runOnUiThread(()->{if(page.equals("remote")){for(int i=0;i<found.length();i++){JSONObject file=found.optJSONObject(i);btn("Letöltés · "+file.optString("filename"),()->run(()->download(ep,auth,file)));}}});});});
        info("A ComfyUI-ban API-formátumban exportált JSON kell, nem a szokásos szerkesztői workflow. A PC-nek futnia kell. Ne tedd nyilvánosan elérhetővé a védelem nélküli ComfyUI portot; használj hitelesített HTTPS hozzáférést.");
    }
    private static void collect(Object obj,JSONArray out){if(obj instanceof JSONObject){JSONObject o=(JSONObject)obj;if(o.optString("filename").toLowerCase(Locale.ROOT).endsWith(".glb")){out.put(o);return;}Iterator<String> keys=o.keys();while(keys.hasNext())collect(o.opt(keys.next()),out);}else if(obj instanceof JSONArray){JSONArray a=(JSONArray)obj;for(int i=0;i<a.length();i++)collect(a.opt(i),out);}}
    private HttpURLConnection connection(String ep,String auth,String method,String route)throws Exception{URL base=new URL(ep);if(!"https".equals(base.getProtocol())||base.getUserInfo()!=null||base.getQuery()!=null||base.getRef()!=null)throw new IOException("Érvényes HTTPS szervercím szükséges");URL u=new URL(ep.replaceAll("/+$","")+route);HttpURLConnection c=(HttpURLConnection)u.openConnection();c.setInstanceFollowRedirects(false);c.setConnectTimeout(15000);c.setReadTimeout(120000);c.setRequestMethod(method);if(!auth.isEmpty())c.setRequestProperty("Authorization","Bearer "+auth);return c;}
    private String request(String ep,String auth,String method,String route,byte[] body,String mime)throws Exception{HttpURLConnection c=connection(ep,auth,method,route);try{if(body!=null){c.setDoOutput(true);c.setRequestProperty("Content-Type",mime);c.setFixedLengthStreamingMode(body.length);try(OutputStream out=c.getOutputStream()){out.write(body);}}int code=c.getResponseCode();if(code<200||code>=300){InputStream err=c.getErrorStream();throw new IOException("HTTP "+code+(err==null?"":": "+read(err,256*1024)));}return read(c.getInputStream(),16*1024*1024);}finally{c.disconnect();}}
    private static String enc(String s)throws Exception{return URLEncoder.encode(s,"UTF-8");}
    private void download(String ep,String auth,JSONObject f)throws Exception{String name=f.getString("filename").replaceAll("[^a-zA-Z0-9._-]","_");if(!name.toLowerCase(Locale.ROOT).endsWith(".glb"))throw new IOException("Nem GLB");String route="/view?filename="+enc(f.getString("filename"))+"&subfolder="+enc(f.optString("subfolder"))+"&type="+enc(f.optString("type","output"));HttpURLConnection c=connection(ep,auth,"GET",route);File tmp=File.createTempFile("remote_",".tmp",models);boolean ok=false;try{if(c.getResponseCode()!=200)throw new IOException("HTTP "+c.getResponseCode());try(InputStream in=c.getInputStream();OutputStream out=new FileOutputStream(tmp)){transfer(in,out,MAX_MODEL);}try(RandomAccessFile r=new RandomAccessFile(tmp,"r")){glbJson(r);}File dest=new File(models,System.currentTimeMillis()+"_"+name);if(!tmp.renameTo(dest))throw new IOException("Mentési hiba");ok=true;msg("GLB a helyi könyvtárban: "+dest.getName());}finally{c.disconnect();if(!ok)tmp.delete();}}
    private void open(int code,String mime){Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType(mime).addCategory(Intent.CATEGORY_OPENABLE);startActivityForResult(i,code);}
    @Override protected void onActivityResult(int code,int result,Intent i){super.onActivityResult(code,result,i);if(result!=RESULT_OK||i==null||i.getData()==null)return;Uri u=i.getData();
        if(code==OPEN_PROMPT){try{JSONObject p=new JSONObject(read(getContentResolver().openInputStream(u),4*1024*1024));if(p.has("prompt"))p=p.getJSONObject("prompt");Iterator<String> keys=p.keys();int n=0;while(keys.hasNext()){JSONObject node=p.optJSONObject(keys.next());if(node==null||!node.has("class_type")||!node.has("inputs"))throw new IOException("Nem API-formátumú workflow");n++;}if(n==0)throw new IOException("Üres workflow");prompt=p;msg(n+" node betöltve");}catch(Exception e){msg(e.getMessage());}}
        if(code==OPEN_IMAGE){try(InputStream in=getContentResolver().openInputStream(u)){inputImage=bytes(in,24*1024*1024);String mime=getContentResolver().getType(u);inputImageName="TAKS_"+System.currentTimeMillis()+("image/jpeg".equals(mime)?".jpg":".png");msg(inputImageName+" betöltve");}catch(Exception e){msg(e.getMessage());}}
        if(code==EXPORT){try{String snapshot=data.toString(2);run(()->{try(OutputStream o=getContentResolver().openOutputStream(u);ZipOutputStream z=new ZipOutputStream(o)){z.putNextEntry(new ZipEntry("project.json"));z.write(snapshot.getBytes(StandardCharsets.UTF_8));z.closeEntry();for(String n:modelNames()){z.putNextEntry(new ZipEntry("models/"+n));try(InputStream in=new FileInputStream(new File(models,n))){transfer(in,z,MAX_MODEL);}z.closeEntry();}}msg("Projekt ZIP exportálva.");});}catch(Exception e){msg(e.getMessage());}}
    }
    @Override public void onBackPressed(){if(page.equals("home"))finish();else home();}
    @Override protected void onDestroy(){worker.shutdownNow();super.onDestroy();}
}
