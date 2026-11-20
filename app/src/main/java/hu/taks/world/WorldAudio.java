package hu.taks.world;
import android.content.Context;
import android.media.*;
import android.util.Log;

/** Activity-scoped PCM stream; respects system volume, focus and app lifecycle. */
public final class WorldAudio {
    private final AudioManager manager;
    private final ProceduralSoundscape synth=new ProceduralSoundscape(System.nanoTime());
    private AudioTrack track;
    private Thread worker;
    private volatile boolean running;
    private AudioFocusRequest request;
    private boolean desired;
    private final AudioManager.OnAudioFocusChangeListener focus=this::focusChanged;
    private synchronized void focusChanged(int change){
        Log.i("TAKSAudio","Focus "+change);
        if(change==AudioManager.AUDIOFOCUS_GAIN&&desired)openOutput();
        else if(change<0)closeOutput();
    }
    public WorldAudio(Context context){manager=(AudioManager)context.getSystemService(Context.AUDIO_SERVICE);}
    public void update(float[] state){synth.update(state);}
    public synchronized void start(){
        desired=true;
        if(running)return;
        AudioAttributes attributes=new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
        if(request==null)request=new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                .setAudioAttributes(attributes).setOnAudioFocusChangeListener(focus).build();
        if(manager.requestAudioFocus(request)!=AudioManager.AUDIOFOCUS_REQUEST_GRANTED){
            Log.i("TAKSAudio","Focus not granted yet; wait for foreground window");return;
        }
        openOutput();
    }
    private synchronized void openOutput(){
        if(running||!desired)return;
        closeOutput();
        AudioAttributes attributes=new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
        try {
            int min=AudioTrack.getMinBufferSize(ProceduralSoundscape.RATE,
                    AudioFormat.CHANNEL_OUT_STEREO,AudioFormat.ENCODING_PCM_16BIT);
            track=new AudioTrack.Builder().setAudioAttributes(attributes)
                    .setAudioFormat(new AudioFormat.Builder().setSampleRate(ProceduralSoundscape.RATE)
                        .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO)
                        .setEncoding(AudioFormat.ENCODING_PCM_16BIT).build())
                    .setBufferSizeInBytes(Math.max(4096,min)).setTransferMode(AudioTrack.MODE_STREAM).build();
            track.play();running=true;
            final AudioTrack output=track;
            worker=new Thread(()->{
                android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);
                short[] pcm=new short[960];
                while(running){
                    synth.render(pcm);
                    int offset=0;
                    while(running&&offset<pcm.length){
                        int n=output.write(pcm,offset,pcm.length-offset,AudioTrack.WRITE_BLOCKING);
                        if(n<=0){running=false;break;}
                        offset+=n;
                    }
                }
            },"TAKS-procedural-audio");worker.start();
            Log.i("TAKSAudio","PCM stream started 24000 Hz stereo");
        }catch(RuntimeException error){Log.e("TAKSAudio","Audio output unavailable",error);stop();}
    }
    public synchronized void stop(){
        desired=false;
        closeOutput();
        if(request!=null){manager.abandonAudioFocusRequest(request);request=null;}
    }
    private synchronized void closeOutput(){
        running=false;
        if(track!=null){try{track.pause();track.flush();}catch(IllegalStateException ignored){}}
        if(worker!=null){try{worker.join(500);}catch(InterruptedException e){Thread.currentThread().interrupt();}}
        if(track!=null){track.release();track=null;}
        worker=null;
        Log.i("TAKSAudio","PCM stream stopped");
    }
}
