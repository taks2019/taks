package hu.taks.world;

/** Runs as ordinary Java without an Android emulator or a fake UI. */
public final class MainMenuActionRouterTest {
    public static void main(String[] args) {
        final int[] calls=new int[4];
        MainMenuActionRouter menu=new MainMenuActionRouter(
            ()->calls[0]++,()->calls[1]++,()->calls[2]++,()->calls[3]++);
        String[] ids={"new_game","settings","creator","exit"};
        if(menu.commandCount()!=4)throw new AssertionError("Missing real menu actions");
        for(int i=0;i<4;i++){
            if(!menu.supports(ids[i]))throw new AssertionError("Missing action: "+ids[i]);
            menu.run(ids[i]);
        }
        for(int i=0;i<4;i++)
            if(calls[i]!=1)throw new AssertionError("Button "+ids[i]+" did not call callback");
        boolean unknownRejected=false;
        try{menu.run("no_action");}catch(IllegalArgumentException expected){unknownRejected=true;}
        if(!unknownRejected)throw new AssertionError("Unknown menu command swallowed");
        boolean nullRejected=false;
        try{new MainMenuActionRouter(()->{},null,()->{},()->{});}
        catch(NullPointerException expected){nullRejected=true;}
        if(!nullRejected)throw new AssertionError("Inert menu action accepted");
        System.out.println("PASS 4/4: Új Játék, Beállítások, Készítő, Kilépés");
    }
}
