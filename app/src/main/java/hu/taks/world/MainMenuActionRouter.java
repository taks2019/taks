package hu.taks.world;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Objects;

/**
 * Four real TAKS World menu commands. Their names are stable IDs, independent
 * of editable Hungarian labels. A script may change text, not what a button does.
 */
public final class MainMenuActionRouter {
    private final Map<String,Runnable> actions;
    public MainMenuActionRouter(Runnable newGame,Runnable settings,
                                Runnable creator,Runnable exit){
        Map<String,Runnable> wired=new LinkedHashMap<>();
        wired.put("new_game",Objects.requireNonNull(newGame,"new_game"));
        wired.put("settings",Objects.requireNonNull(settings,"settings"));
        wired.put("creator",Objects.requireNonNull(creator,"creator"));
        wired.put("exit",Objects.requireNonNull(exit,"exit"));
        actions=Collections.unmodifiableMap(wired);
    }
    public boolean supports(String key){return actions.containsKey(key);}
    public int commandCount(){return actions.size();}
    public void run(String key){
        Runnable action=actions.get(key);
        if(action==null)throw new IllegalArgumentException("Unwired TAKS menu action: "+key);
        action.run();
    }
}
