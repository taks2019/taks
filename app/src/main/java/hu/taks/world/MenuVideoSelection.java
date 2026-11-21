package hu.taks.world;

import android.content.Context;
import java.io.InputStream;

/**
 * The menu evolves ONLY from actual saved World knowledge.
 * Until the user separately approves further discovery videos, the original
 * moving Wonderwerk main menu remains the sole visible background.
 * Future names are dormant slots, never procedural new flora or buildings.
 */
final class MenuVideoSelection {
    static final String BASE="menu/wonderwerk_loop.mp4";
    private MenuVideoSelection(){}
    static String select(Context context,float[] cues){
        if(cues==null||cues.length!=PrehistoricMenuTheme.CUE_COUNT)return BASE;
        // A new scene needs BOTH its video AND an explicit approval marker.
        // Descending priorities represent actual native World facts, not levels.
        final int[] indices={10,7,6,5};
        final String[] names={"rope","shelter","woodwork","stonework"};
        for(int i=0;i<indices.length;++i){
            if(!Float.isFinite(cues[indices[i]])||cues[indices[i]]<=.5f)continue;
            String prefix="menu/approved_discoveries/"+names[i];
            try(InputStream approval=context.getAssets().open(prefix+".approved");
                InputStream video=context.getAssets().open(prefix+".mp4")){
                if(approval.read()>=0&&video.read()>=0)return prefix+".mp4";
            }catch(Exception notYetApproved){
                // Important: NEVER invent or display an unapproved discovery.
            }
        }
        return BASE;
    }
}
