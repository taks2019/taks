package hu.taks.world;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Reads the owner's UNCHANGED SuperImage.zip. Crops only runtime COPIES
 * of six existing 4x2 terrain panels. Grass and water remain separate
 * animated 3D/gameplay systems, not painted static ground textures.
 *
 * This does not claim a true 3D cave can be constructed from a PNG.
 * Surface geometry, collision and camera stay in the native engine.
 */
final class ApprovedTerrainPack {
    static final String ORIGINAL_SHEET="966856_x16_drawing.png";
    static final String STORE="approved_terrain_copies";
    static final int SIDE=512;
    static final String[] FILES={
        "dry.png","red.png","stone.png","sand.png","mud.png","transition.png"
    };
    // 4 columns x 2 rows: dry, red, [grass], rock, sand, mud, [water], transition.
    static final int[] COL={0,1,3,0,1,3};
    static final int[] ROW={0,0,0,1,1,1};
    private ApprovedTerrainPack(){}

    static Bitmap[] decode(InputStream source) throws IOException {
        if(source==null)throw new IOException("Hiányzó SuperImage forrás.");
        try(ZipInputStream zip=new ZipInputStream(source)){
            ZipEntry entry;
            while((entry=zip.getNextEntry())!=null){
                final String name=entry.getName();
                if(entry.isDirectory()||!name.substring(name.lastIndexOf('/')+1).equals(ORIGINAL_SHEET)){
                    zip.closeEntry();
                    continue;
                }
                // 6144x4096 original => decode at half size, ~12 MB in RGB565.
                BitmapFactory.Options options=new BitmapFactory.Options();
                options.inScaled=false;
                options.inSampleSize=2;
                options.inPreferredConfig=Bitmap.Config.RGB_565;
                Bitmap atlas=BitmapFactory.decodeStream(zip,null,options);
                if(atlas==null)throw new IOException("A talajatlasz nem olvasható.");
                try{
                    if(atlas.getWidth()!=3072||atlas.getHeight()!=2048)
                        throw new IOException("A talajatlasz mérete eltér az elfogadottól.");
                    Bitmap[] result=new Bitmap[FILES.length];
                    try{
                        // 768x1024 cells; crop inside photo (not white borders or
                        // caption plates near the bottom of each original panel).
                        for(int i=0;i<FILES.length;i++){
                            int x=COL[i]*768+74;
                            int y=ROW[i]*1024+132;
                            Bitmap region=Bitmap.createBitmap(atlas,x,y,620,620);
                            try{
                                result[i]=Bitmap.createScaledBitmap(region,SIDE,SIDE,true);
                            }finally{
                                region.recycle();
                            }
                        }
                        return result;
                    }catch(RuntimeException ex){
                        recycle(result);
                        throw ex;
                    }
                }finally{
                    atlas.recycle();
                }
            }
        }
        throw new IOException("A SuperImage.zip nem tartalmazza az elfogadott talajatlaszt.");
    }
    static void recycle(Bitmap[] frames){
        if(frames==null)return;
        for(Bitmap bitmap:frames)
            if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();
    }
    static File folder(Context context){return new File(context.getFilesDir(),STORE);}
    static boolean present(Context context){
        File dir=folder(context);
        for(String file:FILES)if(!new File(dir,file).isFile())return false;
        return true;
    }
    static void persist(Context context,Bitmap[] frames) throws IOException{
        if(frames==null||frames.length!=FILES.length)
            throw new IOException("Hiányzó talajmásolatok.");
        File root=context.getFilesDir(),stage=new File(root,STORE+"_incoming");
        if(!stage.exists()&&!stage.mkdirs())throw new IOException("Nincs hely a talajanyagok számára.");
        try{
            for(int i=0;i<FILES.length;i++){
                File dest=new File(stage,FILES[i]);
                try(FileOutputStream out=new FileOutputStream(dest)){
                    if(frames[i]==null||!frames[i].compress(Bitmap.CompressFormat.PNG,100,out))
                        throw new IOException("Sikertelen textúramásolat: "+FILES[i]);
                    out.getFD().sync();
                }
            }
            File output=folder(context);
            File backup=new File(root,STORE+"_previous");
            clear(backup);
            if(output.exists()&&!output.renameTo(backup))
                throw new IOException("A korábbi jóváhagyott talaj nem menthető.");
            if(!stage.renameTo(output)){
                if(backup.exists())backup.renameTo(output);
                throw new IOException("Nem telepíthető a teljes új talajkészlet.");
            }
            clear(backup);
        }finally{
            clear(stage);
        }
    }
    private static void clear(File path){
        if(path.isDirectory()){
            File[] children=path.listFiles();
            if(children!=null)for(File f:children)clear(f);
        }
        if(path.exists())path.delete();
    }
    }
    static Bitmap[] load(Context context) throws IOException {
        if(!present(context))throw new IOException("A játékbeli talajkészlet hiányzik.");
        Bitmap[] frames=new Bitmap[FILES.length];
        try{
            for(int i=0;i<FILES.length;i++){
                BitmapFactory.Options options=new BitmapFactory.Options();
                options.inScaled=false;
                frames[i]=BitmapFactory.decodeFile(new File(folder(context),FILES[i]).getPath(),options);
                if(frames[i]==null||frames[i].getWidth()!=SIDE||frames[i].getHeight()!=SIDE)
                    throw new IOException("Sérült textúramásolat: "+FILES[i]);
            }
            return frames;
        }catch(Exception e){
            recycle(frames);
            if(e instanceof IOException)throw (IOException)e;
            throw new IOException(e);
        }
    }
}
