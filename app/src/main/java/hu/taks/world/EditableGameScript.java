package hu.taks.world;

import android.content.Context;
import android.util.Log;
import org.json.JSONObject;
import org.json.JSONException;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

/**
 * Human-editable menu/settings presentation data. No gameplay logic or
 * discovery can be unlocked by editing a button title.
 *
 * Default offline source: assets/menu/menu_texts.json.
 * User override: app private files/menu_texts.json (survives restart).
 */
public final class EditableGameScript {
    public static final String ASSET_PATH="menu/menu_texts.json";
    private static final String OVERRIDE="menu_texts.json";
    private final Context context;
    private JSONObject data;
    private String lastError="";
    public EditableGameScript(Context source) {
        context=source.getApplicationContext();
        reload();
    }
    private static String read(InputStream in) throws java.io.IOException{
        try(InputStream stream=in;java.io.ByteArrayOutputStream out=new java.io.ByteArrayOutputStream()){
            byte[] b=new byte[8192];int n;
            while((n=stream.read(b))>=0)out.write(b,0,n);
            return new String(out.toByteArray(),StandardCharsets.UTF_8);
        }
    }
    private File override(){return new File(context.getFilesDir(),OVERRIDE);}
    private String builtin() throws java.io.IOException{
        return read(context.getAssets().open(ASSET_PATH));
    }
    private static JSONObject validate(String input) throws JSONException{
        if(input==null||input.length()<30||input.length()>65536)
            throw new JSONException("A JSON túl hosszú vagy üres.");
        JSONObject doc=new JSONObject(input);
        if(doc.optInt("version",-1)!=1)throw new JSONException("Ismeretlen JSON-verzió.");
        JSONObject menu=doc.getJSONObject("menu");
        for(String k:new String[]{"new_game","settings","creator","exit"}){
            String value=menu.getString(k);
            if(value.trim().isEmpty()||value.length()>50)
                throw new JSONException("Hiányzó vagy túl hosszú menüfelirat: "+k);
        }
        if(!doc.has("settings")||!doc.has("graphics")||!doc.has("environment"))
            throw new JSONException("Hiányzik a settings, graphics vagy environment szakasz.");
        for(String section:new String[]{"settings"}){
            JSONObject values=doc.getJSONObject(section);
            java.util.Iterator<String> keys=values.keys();
            while(keys.hasNext()){
                String k=keys.next();
                Object v=values.get(k);
                if(!(v instanceof String)||((String)v).length()>600)
                    throw new JSONException("Érvénytelen szöveg: "+k);
            }
        }
        // Numeric settings are data, never executable scripts.
        JSONObject gfx=doc.getJSONObject("graphics");
        for(String k:new String[]{"quick_width","balanced_width","detail_width"}){
            int width=gfx.getInt(k);
            if(width<320||width>2048)
                throw new JSONException("3D-felbontás 320 és 2048 közé essen: "+k);
        }
        double pitch=gfx.getDouble("isometric_pitch_degrees");
        if(!Double.isFinite(pitch)||pitch<20||pitch>80)
            throw new JSONException("A kamera dőlésszöge 20–80 fok lehet.");
        JSONObject env=doc.getJSONObject("environment");
        double fern=env.getDouble("fern_sway_degrees");
        double rain=env.getDouble("rain_visual_threshold");
        double water=env.getDouble("water_ripple_visibility");
        if(!Double.isFinite(fern)||fern<0||fern>15||
           !Double.isFinite(rain)||rain<0||rain>1||
           !Double.isFinite(water)||water<0||water>3)
            throw new JSONException("Érvénytelen időjárási megjelenítési beállítás.");
        return doc;
    }
    public synchronized boolean reload(){
        try{
            File user=override();
            data=validate(user.isFile()?read(new FileInputStream(user)):builtin());
            lastError="";
            return true;
        }catch(Exception e){
            Log.w("TAKS_SCRIPT","Saved text script unreadable; using verified built-in.",e);
            lastError=e.getMessage();
            try{data=validate(builtin());return false;}
            catch(Exception missing){data=new JSONObject();return false;}
        }
    }
    public synchronized String get(String section,String key,String fallback){
        JSONObject object=data.optJSONObject(section);
        String value=object!=null?object.optString(key,fallback):fallback;
        return value.trim().isEmpty()?fallback:value;
    }
    public synchronized int integer(String section,String key,int fallback){
        JSONObject object=data.optJSONObject(section);
        return object!=null?object.optInt(key,fallback):fallback;
    }
    public synchronized float decimal(String section,String key,float fallback){
        JSONObject object=data.optJSONObject(section);
        return object!=null?(float)object.optDouble(key,fallback):fallback;
    }
    public synchronized String rawJson(){return data.toString(2);}
    public synchronized String lastError(){return lastError;}
    public synchronized boolean save(String changed){
        try{
            JSONObject next=validate(changed);
            File target=override(),tmp=new File(context.getFilesDir(),OVERRIDE+".new");
            try(FileOutputStream stream=new FileOutputStream(tmp)){
                stream.write(next.toString(2).getBytes(StandardCharsets.UTF_8));
                stream.getFD().sync();
            }
            if(target.exists()&&!target.delete())throw new java.io.IOException("Régi fájl nem törölhető.");
            if(!tmp.renameTo(target))throw new java.io.IOException("Nem sikerült a JSON mentése.");
            data=next;lastError="";
            return true;
        }catch(Exception e){
            lastError=e.getMessage();Log.e("TAKS_SCRIPT","Cannot persist updated menu script",e);
            return false;
        }
    }
    public synchronized void restoreDefaults(){
        File f=override();
        if(f.exists()&&!f.delete())Log.w("TAKS_SCRIPT","Cannot delete user override");
        reload();
    }
}
