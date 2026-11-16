#version 300 es
precision highp float;
out vec4 fragColor;
uniform vec2 u_resolution;
uniform float u_orbit;
uniform float u_zoom;
uniform float u_clock;
uniform float u_rain;
uniform float u_moisture;
uniform float u_hearth;
uniform float u_shelterStage;
uniform float u_fire;
uniform float u_quality;
uniform vec2 u_player;
uniform float u_walkPhase;
uniform vec4 u_nodes[12];

float hash(vec3 p){p=fract(p*.1031);p+=dot(p,p.yzx+33.33);return fract((p.x+p.y)*p.z);}
float n3(vec3 x){vec3 i=floor(x),f=fract(x);f=f*f*(3.-2.*f);return mix(mix(mix(hash(i),hash(i+vec3(1,0,0)),f.x),mix(hash(i+vec3(0,1,0)),hash(i+vec3(1,1,0)),f.x),f.y),mix(mix(hash(i+vec3(0,0,1)),hash(i+vec3(1,0,1)),f.x),mix(hash(i+vec3(0,1,1)),hash(i+vec3(1,1,1)),f.x),f.y),f.z);}
float fb(vec3 p){float n=0.,a=.5;for(int i=0;i<4;i++){n+=a*n3(p);p*=2.03;a*=.5;}return n;}
float terrain(vec2 x){float r=fb(vec3(x.x*.17,1.,x.y*.17));float v=n3(vec3(x.x*.8,7.,x.y*.8));return -.45+.6*r +.11*v + .015*x.y;}
float sdBox(vec3 p,vec3 b){vec3 q=abs(p)-b;return length(max(q,0.))+min(max(q.x,max(q.y,q.z)),0.);}
float sdOcta(vec3 p,float s){p=abs(p);return (p.x+p.y+p.z-s)*.57735;}
float sdSegment(vec3 p,vec3 a,vec3 b,float ra){vec3 d=b-a;float t=clamp(dot(p-a,d)/dot(d,d),0.,1.);return length(p-a-d*t)-ra;}
vec2 geo(vec3 p){
 float g=p.y-terrain(p.xz);
 vec3 q=p-vec3(0.,0.,6.5);
 float top=4.8+1.2*sin(.29*p.x+0.4)+.65*sin(.67*p.x)+.42*(n3(vec3(p.x*.34,0.,p.z*.3))-.5);
 float edge=9.0+.7*sin(p.z*.41+p.x*.12);
 float front=3.35+.3*sin(p.x*.45)+.21*n3(vec3(p.x*.7,2.,p.y*.3));
 float wall=max(max(p.y-top,terrain(p.xz)-p.y),max(abs(p.x)-edge,max(front-p.z,p.z-15.5)));
 float noiseWall=(fb(p*vec3(1.9,2.3,1.8))-.49)*.44;
 wall+=noiseWall;
 float innerRadiusX=1.65+.12*sin(p.z*.9)+.13*n3(vec3(p.z*.8,4.,1.));
 float innerRadiusY=1.7+.10*sin(p.z*1.3);
 float arch=length(vec2(p.x/innerRadiusX,(p.y-1.62)/innerRadiusY))-1.0;
 float tunnel=arch*1.55+.09*(n3(p*2.6)-.5);
 wall=max(wall,-tunnel);
 float d=wall;float mat=2.;
 if(g<d){d=g;mat=1.;}
 for(int i=0;i<7;i++){
  float k=float(i);vec2 xy=vec2(-5.5+mod(k*3.71,10.5),-3.8+mod(k*2.19,5.7));
  vec3 v=p-vec3(xy.x,terrain(xy)+.19,xy.y);
  v.xz*=mat2(cos(k*1.1),-sin(k*1.1),sin(k*1.1),cos(k*1.1));
  float stone=sdOcta(v*vec3(1.05,1.6,1.2),.54+.15*hash(vec3(k)));stone+=.035*(n3(p*5.)-.5);
  if(stone<d){d=stone;mat=3.;}
 }

 // A térbeli készletek NEM HUD-ikonok: látható, véges helyi lerakódások.
 if(p.y>-1.2 && p.y<1.85 && p.z<3.4){
  for(int i=0;i<12;i++){
   vec4 node=u_nodes[i];if(node.w<0.5)continue;
   vec2 diff=p.xz-node.xy;
   if(dot(diff,diff)>1.35)continue;
   float h=terrain(node.xy);
   vec3 local=p-vec3(node.x,h,node.y);
   float resource=10.0;
   if(node.z<.5){
     // Két egymásnak támaszkodó, vékony földre helyezett ág.
     float a=sdSegment(local,vec3(-.38,.09,-.12),vec3(.40,.15,.16),.065);
     float b=sdSegment(local,vec3(-.22,.10,.20),vec3(.28,.13,-.24),.054);
     resource=min(a,b);
   }else if(node.z<1.5){
     resource=sdOcta((local-vec3(0.,.18,0.))*vec3(1.,1.30,1.),.38);
   }else if(node.z<2.5){
     resource=min(sdSegment(local,vec3(-.18,.08,0.),vec3(.17,.22,.12),.065),
                  sdSegment(local,vec3(.11,.08,-.12),vec3(-.18,.22,.12),.05));
   }else{
     resource=sdBox(local-vec3(0.,.10,0.),vec3(.34,.06,.26));
     resource+=.04*(n3(p*8.)-.5);
   }
   if(resource<d){d=resource;mat=node.z<.5?5.:node.z<1.5?3.:node.z<2.5?9.:6.;}
  }
 }
 // Világkoordinátában mozgó, emberi méretarányú munkakarakter-váz.
 // Ideiglenes ellenőrző geometria, nem végleges hiteles embermodell.
 if(abs(p.x-u_player.x)<.8 && abs(p.z-u_player.y)<.8){
  float ground=terrain(u_player);
  vec3 human=p-vec3(u_player.x,ground,u_player.y);
  float walk=sin(u_walkPhase)*.15;
  float torso=sdSegment(human,vec3(0.,.80,0.),vec3(0.,1.37,0.),.18);
  float head=length((human-vec3(0.,1.56,0.))*vec3(1.,1.08,1.))-0.125;
  float leftLeg=sdSegment(human,vec3(-.10,.80,0.),vec3(-.12,.10,walk),.075);
  float rightLeg=sdSegment(human,vec3(.10,.80,0.),vec3(.12,.10,-walk),.075);
  float leftArm=sdSegment(human,vec3(-.20,1.28,0.),vec3(-.26,.85,-walk*.5),.06);
  float rightArm=sdSegment(human,vec3(.20,1.28,0.),vec3(.26,.85,walk*.5),.06);
  float humanShape=min(torso,min(head,min(leftLeg,min(rightLeg,min(leftArm,rightArm)))));
  if(humanShape<d){d=humanShape;mat=head<humanShape+.01?7.:8.;}
  // Földhöz rögzített kiválasztó jel, nem lebegő ikon.
  float ring=abs(length(human.xz)-.32)-.016;
  ring=max(ring,abs(human.y-.018)-.022);
  if(ring<d){d=ring;mat=9.;}
 }

 if(u_hearth>.5){
  float h=terrain(vec2(-1.15,-1.8));
  for(int j=0;j<7;j++){
   float k=float(j),a=k*6.283185/7.;
   vec3 v=p-vec3(-1.15+.55*cos(a),h+.17,-1.8+.45*sin(a));
   float ring=sdOcta(v*vec3(1.,1.5,1.1),.28);
   if(ring<d){d=ring;mat=4.;}
  }
  float wood=sdSegment(p,vec3(-1.6,h+.15,-2.),vec3(-.8,h+.20,-1.5),.085);
  if(wood<d){d=wood;mat=5.;}
 }
 // A készültségi fázisokat nem egy készre kapcsolt objektum, hanem szerkezeti részek rajzolják.
 // 0: üres; 1: két cölöp + kőlábazat; 2: gerenda és szarufák; 3: átlapolt növényi tető.
 if(u_shelterStage>0.5 && p.x>0.5 && p.x<5.7 && p.z>-3.4 && p.z<2.3 && p.y<3.5){
  float zFront=-1.55;
  float h=terrain(vec2(3.1,zFront));
  for(int k=0;k<2;k++){
   float x=2.03+float(k)*2.12;
   float fy=terrain(vec2(x,zFront));
   float pole=sdSegment(p,vec3(x,fy+.07,zFront),vec3(x,h+2.34,zFront),.105);
   pole+=.014*(n3(p*14.)-.5);
   if(pole<d){d=pole;mat=5.;}
   // A cölöpök tövében két ténylegesen felhasznált rögzítőkő.
   float footing=sdOcta((p-vec3(x,fy+.13,zFront)) *vec3(1.,1.2,1.),.34);
   if(footing<d){d=footing;mat=4.;}
  }
  if(u_shelterStage>1.5){
   float beam=sdSegment(p,vec3(1.98,h+2.30,zFront),vec3(4.20,h+2.30,zFront),.13);
   if(beam<d){d=beam;mat=5.;}
   // A harmadik, negyedik és ötödik cölöp gerenda és két ferde tetőtartó.
   for(int k=0;k<2;k++){
    float x=2.04+2.10*float(k);
    vec3 a=vec3(x,h+2.32,zFront);
    vec3 b=vec3(x,terrain(vec2(x,.95))+.42,.95);
    float rafter=sdSegment(p,a,b,.10);
    if(rafter<d){d=rafter;mat=5.;}
   }
  }
  if(u_shelterStage>2.5){
   // Hat, egymásra fektetett levélköteg: megmarad a tető ferde rétegszerkezete.
   for(int k=0;k<6;k++){
    float zz=-1.33+float(k)*.37;
    float yy=h+2.26-(zz-zFront)*.69;
    vec3 v=p-vec3(3.09,yy,zz);
    v.y+=v.z*.69;
    float leafSheet=sdBox(v,vec3(1.12,.038,.31));
    leafSheet+=.008*(n3(p*18.)-.5);
    if(leafSheet<d){d=leafSheet;mat=6.;}
   }
  }
 }
 return vec2(d,mat);
}
vec3 normalAt(vec3 p){vec2 e=vec2(.007,0.);return normalize(vec3(geo(p+e.xyy).x-geo(p-e.xyy).x,geo(p+e.yxy).x-geo(p-e.yxy).x,geo(p+e.yyx).x-geo(p-e.yyx).x));}
float shadow(vec3 p,vec3 sun){float shade=1.,t=.13;for(int i=0;i<15;i++){float h=geo(p+sun*t).x;shade=min(shade,14.*h/t);t+=clamp(h,.08,.7);if(h<.01||t>14.)break;}return clamp(shade,.16,1.);}
vec3 material(vec3 p,vec3 norm,float id){
 float grain=fb(p*3.8),micro=n3(p*23.);
 float strata=.5+.5*sin(p.y*7.8+fb(vec3(p.x*.37,p.y*.26,p.z*.35))*2.9);
 float fractures=pow(1.-abs(sin(p.y*13.0 + n3(p*.28)*2.)),15.);
 vec3 rock=mix(vec3(.32,.235,.165),vec3(.65,.48,.31),strata*.64+.2);
 rock=mix(rock,rock*.45,fractures*.28);
 rock*=.67+.53*grain;
 vec3 dirt=mix(vec3(.52,.38,.23),vec3(.76,.60,.37),fb(vec3(p.x*.5,7.8,p.z*.5)));
 dirt*=.73+.32*micro;
 float rough=clamp((1.-norm.y)*.4,0.,.5);
 dirt=mix(dirt,rock,rough);
 float wet=clamp(u_moisture*.85+u_rain*.15,0.,.8);
 if(id<1.5)return dirt*(1.-wet*.55);
 if(id<3.5)return rock*(1.-wet*.35);
 if(id<4.5)return vec3(.37,.31,.25)*(1.-wet*.45);
 if(id<5.5)return mix(vec3(.19,.105,.052),vec3(.38,.24,.13),fb(p*vec3(3.,7.,3.)))*(1.-wet*.22);
 if(id<6.5)return mix(vec3(.22,.27,.10),vec3(.46,.40,.15),fb(p*4.5))*(1.-wet*.3);
 if(id<7.5)return vec3(.45,.29,.19);
 if(id<8.5)return vec3(.24,.17,.115);
 return vec3(.8,.64,.28);
}
vec3 sky(vec3 d,vec3 sun){float h=clamp(d.y*.6+.45,0.,1.);vec3 c=mix(vec3(.81,.76,.65),vec3(.29,.50,.67),h);float solar=pow(max(dot(d,sun),0.),330.);c+=vec3(1.,.70,.35)*solar*1.9;float haze=pow(1.-max(d.y,0.),6.);c+=vec3(.22,.16,.09)*haze;return c;}
void main(){
 vec2 uv=(gl_FragCoord.xy-.5*u_resolution)/u_resolution.y;
 float angle=u_orbit;
 float dist=max(7.,u_zoom);
 vec3 eye=vec3(sin(angle)*dist*.8,dist*.43,-cos(angle)*dist*.86)+vec3(u_player.x,1.5,u_player.y+2.1);
 vec3 target=vec3(u_player.x,1.20,u_player.y+2.1);
 vec3 forward=normalize(target-eye),right=normalize(cross(forward,vec3(0.,1.,0.))),up=cross(right,forward);
 vec3 rd=normalize(forward+uv.x*right*1.12+uv.y*up*1.12);
 vec3 sun=normalize(vec3(-.55,1.1,-.6));
 float timeOfDay=mod(u_clock,24.);
 float warmth=clamp(1.-abs(timeOfDay-13.)/12.,.1,1.);
 vec3 skyColor=sky(rd,sun)*mix(vec3(.22,.31,.45),vec3(1.),warmth);
 vec3 p=eye;float t=0.;vec2 result=vec2(0.,0.);bool hit=false;
 for(int i=0;i<92;i++){
  p=eye+rd*t;result=geo(p);
  if(result.x<.012+(.0007*t)){hit=true;break;}
  t+=clamp(result.x*.78,.045,1.3);
  if(t>65.)break;
 }
 vec3 c=skyColor;
 if(hit){
  vec3 n=normalAt(p);
  float coarse=n3(p*13.);
  n=normalize(n+(.08*(coarse-.5))*vec3(.5,.9,.7));
  float diff=max(dot(n,sun),0.);
  float sh=shadow(p+n*.06,sun);
  vec3 base=material(p,n,result.y);
  vec3 ambient=vec3(.27,.35,.44)*(.6+.4*max(n.y,0.));
  vec3 lighting=ambient+vec3(1.35,1.11,.87)*diff*sh*warmth;
  c=base*lighting;
  float occ=clamp(geo(p+n*.25).x*1.9,.28,1.);
  c*=occ;
  float fog=1.-exp(-.00083*t*t);
  c=mix(c,skyColor,fog);
  if(u_fire>.5){
   vec3 fp=vec3(-1.15,terrain(vec2(-1.15,-1.8))+.35,-1.8);
   float dst=length(p-fp);
   c+=vec3(1.1,.43,.12)*exp(-dst*2.3)*.38;
  }
 }
 if(u_fire>.5){
   vec3 flame=vec3(-1.15,terrain(vec2(-1.15,-1.8))+.34,-1.8);
   vec3 v=flame-eye;
   float proj=dot(v,rd);vec3 separation=v-rd*proj;
   float ember=length(separation);
   if(proj>0. && ember<.31 && (!hit||proj<t)){
    float flicker=.88+.12*sin(u_clock*6.+gl_FragCoord.x*.04);
    c=mix(c,vec3(2.2,.85,.24),smoothstep(.31,.045,ember)*flicker);
   }
 }
 c=c/(1.+c);
 c=pow(max(c,vec3(0.)),vec3(.92));
 fragColor=vec4(c,1.);
}