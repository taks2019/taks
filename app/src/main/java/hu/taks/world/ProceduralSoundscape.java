package hu.taks.world;

/** Original offline synthesis, no recordings or repeating sample buffers. */
public final class ProceduralSoundscape {
    public static final int RATE=24000;
    private final java.util.Random random;
    private volatile float[] target=new float[13];
    private final double[] gains=new double[9],echo=new double[RATE/2];
    private int echoAt,material;
    private double low,waterLow,phase,callPhase,callEnvelope,drip,step,stepTone;
    private double lastTravel=Double.NaN,stepDistance;
    public ProceduralSoundscape(long seed){random=new java.util.Random(seed);}
    public void update(float[] snapshot){
        if(snapshot==null||snapshot.length!=13)return;
        for(float v:snapshot)if(!Float.isFinite(v))return;
        float[] copy=snapshot.clone();
        for(int i=0;i<9;i++)copy[i]=Math.max(0,Math.min(1,copy[i]));
        copy[9]=Math.max(0,Math.min(5,copy[9]));
        copy[11]=Math.max(-1,Math.min(1,copy[11]));
        copy[12]=Math.max(-1,Math.min(1,copy[12]));
        target=copy;
    }
    private double noise(){return random.nextDouble()*2-1;}
    public void render(short[] stereo){
        float[] t=target;
        double travel=t[10];
        if(travel<0){lastTravel=Double.NaN;stepDistance=0;}
        else {
            if(Double.isFinite(lastTravel)){
                double delta=travel-lastTravel;
                if(delta>=0&&delta<1)stepDistance+=delta;
                else stepDistance=0; // reset/load is never a footstep
            }
            lastTravel=travel;
            if(stepDistance>=.58){stepDistance%=.58;step=1;stepTone=0;material=(int)t[9];}
        }
        for(int i=0;i+1<stereo.length;i+=2){
            for(int g=0;g<9;g++)gains[g]+=(t[g]-gains[g])*.00022;
            double n=noise();low+=(n-low)*.007;waterLow+=(noise()-waterLow)*.09;
            phase+=1.0/RATE;
            if(random.nextDouble()<(2.5*gains[3])/RATE)drip=1;
            drip*=.9985;
            double drop=drip*Math.sin(phase*2*Math.PI*(2100+drip*900));
            if(callEnvelope<.001&&random.nextDouble()<(gains[6]*.7+gains[7]*1.3)/RATE){
                callEnvelope=1;callPhase=0;
            }
            callPhase+=1.0/RATE;callEnvelope*=.99978;
            double chirp=Math.sin(2*Math.PI*(1800*callPhase+850*callPhase*callPhase));
            double animal=Math.sin(2*Math.PI*180*callPhase+3*Math.sin(callPhase*22));
            double call=callEnvelope*(chirp*gains[7]+animal*gains[6])*.10;
            double rain=n*gains[0]*.16,wind=low*gains[1]*.65;
            double water=waterLow*gains[2]*.32,rustle=(n-low)*gains[5]*.055;
            double crack=random.nextDouble()<18.0/RATE?noise()*.7:0;
            double fire=(n*.035+crack)*gains[4];
            step*=material==1?.9983:.9968;stepTone+=1.0/RATE;
            double foot;
            switch(material){
                case 1: foot=(waterLow*.5+n*.12)*step;break;
                case 2: foot=(Math.sin(stepTone*2300)*.3+n*.16)*step;break;
                case 3: foot=(n-low)*step*.25;break;
                case 4: foot=(waterLow*.9+n*.1)*step;break;
                case 5: foot=(n-low)*step*.38;break;
                default: foot=(n*.25+Math.sin(stepTone*800)*.14)*step;
            }
            double center=rain+wind+rustle+drop*.20+call+foot*.3;
            double reflected=echo[echoAt];
            echo[echoAt]=(center+water+fire)*gains[8]*.30+reflected*.38;
            echoAt=(echoAt+1)%echo.length;
            double l=center+water*(1-t[11]*.65)+fire*(1-t[12]*.65)+reflected;
            double r=center+water*(1+t[11]*.65)+fire*(1+t[12]*.65)+reflected;
            stereo[i]=(short)(Math.tanh(l*.8)*24000);
            stereo[i+1]=(short)(Math.tanh(r*.8)*24000);
        }
    }
}
