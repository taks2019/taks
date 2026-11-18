#include "Structure.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace taks {
namespace {
constexpr double kGravity=9.81;
constexpr double kLashCapacity=900.0;      // N, kötéssel rögzített csatlakozás
constexpr double kSupportCapacity=20000.0; // N, nyomott támasz
bool finite(double v){return std::isfinite(v);}
// Függőleges kiterjedés (a talpponttól felfelé) és vízszintes alapterület-átmérő alkatrészenként.
double extentOf(const Part& p){ return p.kind==PART_POLE||p.kind==PART_STONE?p.length:(p.kind==PART_BEAM?0.25:0.1); }
double footprintOf(const Part& p){ return p.kind==PART_POLE?0.25:(p.kind==PART_STONE?p.length:p.length); }
bool setWhy(std::string* why,const char* text){ if(why)*why=text; return false; }
}
Part Structure::makeDefault(int kind){
    Part p;p.kind=kind;
    switch(kind){
        case PART_POLE:  p.length=2.2;p.mass=6.0; p.strength=6000.0;break;
        case PART_BEAM:  p.length=3.0;p.mass=9.0; p.strength=2500.0;break;
        case PART_STONE: p.length=0.4;p.mass=18.0;p.strength=30000.0;break;
        default:         p.length=1.6;p.mass=4.0; p.strength=1200.0;break; // deszka
    }
    return p;
}
const Part* Structure::find(unsigned id) const{
    for(const auto& p:parts)if(p.id==id)return &p;
    return nullptr;
}
void Structure::checkpoint(){
    undoStack_.push_back({parts,joints,nextId});
    if(static_cast<int>(undoStack_.size())>maxUndo)undoStack_.erase(undoStack_.begin());
    redoStack_.clear();
}
unsigned Structure::addPart(int kind,double x,double y,double z,double yaw,std::string* why){
    if(kind<0||kind>PART_PLANK){setWhy(why,"Ismeretlen alkatrésztípus.");return 0;}
    if(!finite(x)||!finite(y)||!finite(z)||!finite(yaw)){setWhy(why,"Érvénytelen pozíció.");return 0;}
    if(std::fabs(x)>200||std::fabs(z)>200||y<0||y>50){setWhy(why,"A hely a megengedett építési területen kívül van.");return 0;}
    if(static_cast<int>(parts.size())>=maxParts){setWhy(why,"Elérted az alkatrészek számának határát.");return 0;}
    for(const auto& q:parts){
        const double dx=q.x-x,dy=q.y-y,dz=q.z-z;
        if(dx*dx+dy*dy+dz*dz<0.05*0.05){setWhy(why,"Ott már van alkatrész.");return 0;}
    }
    checkpoint();
    Part p=makeDefault(kind);
    p.id=nextId++;p.x=x;p.y=y;p.z=z;p.yaw=yaw;
    parts.push_back(p);
    return p.id;
}
bool Structure::addJoint(unsigned a,unsigned b,int type,std::string* why){
    if(a==b)return setWhy(why,"Egy alkatrész önmagához nem csatlakozhat.");
    if(type!=0&&type!=1)return setWhy(why,"Ismeretlen kötéstípus.");
    const Part* pa=find(a);const Part* pb=find(b);
    if(!pa||!pb)return setWhy(why,"Hiányzó alkatrész.");
    for(const auto& j:joints)
        if((j.a==a&&j.b==b)||(j.a==b&&j.b==a))return setWhy(why,"Ezek már össze vannak kötve.");
    const double dx=pa->x-pb->x,dy=pa->y-pb->y,dz=pa->z-pb->z;
    const double horizontal=std::sqrt(dx*dx+dz*dz);
    if(horizontal>(footprintOf(*pa)+footprintOf(*pb))*0.5+0.15)return setWhy(why,"Az alkatrészek túl messze vannak egymástól.");
    const Part& lower=(pa->y<=pb->y)?*pa:*pb;
    const Part& upper=(pa->y<=pb->y)?*pb:*pa;
    if(std::fabs(upper.y-(lower.y+extentOf(lower)))>0.2&&std::fabs(dy)>0.2)return setWhy(why,"Az alkatrészek nem érnek össze függőlegesen.");
    if(type==1&&std::fabs(dy)<=0.02)return setWhy(why,"Támaszhoz az egyik alkatrésznek lejjebb kell lennie.");
    checkpoint();
    joints.push_back({a,b,type});
    return true;
}
bool Structure::removePart(unsigned id){
    if(!find(id))return false;
    checkpoint();
    parts.erase(std::remove_if(parts.begin(),parts.end(),[id](const Part& p){return p.id==id;}),parts.end());
    joints.erase(std::remove_if(joints.begin(),joints.end(),[id](const Joint& j){return j.a==id||j.b==id;}),joints.end());
    return true;
}
bool Structure::rotatePart(unsigned id,double yaw,std::string* why){
    if(!finite(yaw))return setWhy(why,"Érvénytelen szög.");
    for(auto& p:parts)if(p.id==id){
        checkpoint();
        for(auto& q:parts)if(q.id==id){q.yaw=std::remainder(yaw,2.0*3.14159265358979323846);break;}
        return true;
    }
    return setWhy(why,"Hiányzó alkatrész.");
}
unsigned Structure::cutPart(unsigned id,double fraction,std::string* why){
    const Part* src=find(id);
    if(!src){setWhy(why,"Hiányzó alkatrész.");return 0;}
    if(!finite(fraction)||fraction<0.2||fraction>0.8){setWhy(why,"A vágás helye a hossz 20–80%-a között lehet.");return 0;}
    if(src->kind==PART_STONE){setWhy(why,"A követ nem lehet elvágni.");return 0;}
    if(static_cast<int>(parts.size())>=maxParts){setWhy(why,"Elérted az alkatrészek számának határát.");return 0;}
    const Part o=*src;
    Part a=o,b=o;
    a.length=o.length*fraction;b.length=o.length-a.length;
    a.mass=o.mass*fraction;b.mass=o.mass-a.mass; // tömegmegmaradás
    if(o.kind==PART_POLE){ // függőleges: y a talppont
        b.y=o.y+a.length;
    }else{                 // vízszintes: középpont, a yaw irányában
        const double dx=std::cos(o.yaw),dz=std::sin(o.yaw);
        const double ca=-o.length*0.5+a.length*0.5, cb=o.length*0.5-b.length*0.5;
        a.x=o.x+dx*ca;a.z=o.z+dz*ca;b.x=o.x+dx*cb;b.z=o.z+dz*cb;
    }
    checkpoint();
    const unsigned newId=nextId++;
    b.id=newId;
    // Az eredeti kötések a hozzájuk közelebbi darabra kerülnek; ami egyikhez sem ér el, megszűnik.
    std::vector<Joint> nj;
    for(const auto& j:joints){
        if(j.a!=id&&j.b!=id){nj.push_back(j);continue;}
        const unsigned otherId=(j.a==id)?j.b:j.a;const Part* op=find(otherId);
        if(!op)continue;
        auto d2=[&](const Part& q){const double dx=q.x-op->x,dy=(q.y+(q.kind==PART_POLE?q.length*0.5:0))-(op->y+(op->kind==PART_POLE?op->length*0.5:0)),dz=q.z-op->z;return dx*dx+dy*dy+dz*dz;};
        const unsigned keep=(d2(a)<=d2(b))?id:newId;
        Joint m=j;(m.a==id)?m.a=keep:m.b=keep;nj.push_back(m);
    }
    for(auto& p:parts)if(p.id==id){p=a;break;}
    parts.push_back(b);joints=nj;
    return newId;
}
void Structure::wear(double amount){
    if(!finite(amount)||amount<=0)return;
    for(auto& p:parts)p.condition=std::max(0.0,p.condition-amount);
}
StructureReport Structure::analyze() const{
    StructureReport r;
    const size_t n=parts.size();
    std::vector<size_t> order(n);
    for(size_t i=0;i<n;++i)order[i]=i;
    std::sort(order.begin(),order.end(),[this](size_t l,size_t r2){
        if(parts[l].y!=parts[r2].y)return parts[l].y>parts[r2].y;
        return parts[l].id<parts[r2].id;});
    auto indexOf=[this](unsigned id){for(size_t i=0;i<parts.size();++i)if(parts[i].id==id)return static_cast<int>(i);return -1;};
    std::vector<double> load(n);
    for(size_t i=0;i<n;++i)load[i]=parts[i].mass*kGravity;
    std::vector<char> failed(n,0);
    std::vector<char> jointFailed(joints.size(),0);
    for(size_t oi=0;oi<n;++oi){
        const size_t i=order[oi];const Part& p=parts[i];
        const bool grounded=p.y<=0.05;
        const double cap=std::max(1e-9,p.strength*p.condition/100.0);
        r.maxLoadRatio=std::max(r.maxLoadRatio,load[i]/cap);
        if(load[i]>cap)failed[i]=1;
        if(grounded)continue; // a talaj tartja
        std::vector<size_t> sup;
        for(size_t k=0;k<joints.size();++k){
            const Joint& j=joints[k];
            const int ia=indexOf(j.a),ib=indexOf(j.b);
            if(ia<0||ib<0)continue;
            int other=-1;
            if(static_cast<size_t>(ia)==i)other=ib;else if(static_cast<size_t>(ib)==i)other=ia;
            if(other<0)continue;
            if(parts[other].y<p.y-0.02)sup.push_back(k); // csak lejjebb lévő társ tarthat
        }
        if(sup.empty()){failed[i]=1;continue;} // lebeg: semmi sem tartja
        const double share=load[i]/static_cast<double>(sup.size());
        for(size_t k:sup){
            const Joint& j=joints[k];
            const int ia=indexOf(j.a),ib=indexOf(j.b);
            const int other=(static_cast<size_t>(ia)==i)?ib:ia;
            const double jc=(j.type==0?kLashCapacity:kSupportCapacity)*std::max(0.0,std::min(parts[i].condition,parts[other].condition))/100.0;
            r.maxLoadRatio=std::max(r.maxLoadRatio,share/std::max(1e-9,jc));
            if(share>jc)jointFailed[k]=1;
            load[other]+=share;
        }
    }
    for(size_t i=0;i<n;++i)if(failed[i])r.failedParts.push_back(parts[i].id);
    for(size_t k=0;k<joints.size();++k)if(jointFailed[k])r.failedJoints.push_back(static_cast<unsigned>(k));
    r.stable=r.failedParts.empty()&&r.failedJoints.empty();
    return r;
}
bool Structure::undo(){
    if(undoStack_.empty())return false;
    redoStack_.push_back({parts,joints,nextId});
    Snapshot s=undoStack_.back();undoStack_.pop_back();
    parts=s.parts;joints=s.joints;nextId=std::max(nextId,s.nextId); // azonosító nem használódik újra
    return true;
}
bool Structure::redo(){
    if(redoStack_.empty())return false;
    undoStack_.push_back({parts,joints,nextId});
    Snapshot s=redoStack_.back();redoStack_.pop_back();
    parts=s.parts;joints=s.joints;nextId=std::max(nextId,s.nextId);
    return true;
}
std::string Structure::save() const{
    std::ostringstream o;o<<std::setprecision(17);
    o<<"STRUCT2 "<<mode<<' '<<nextId<<' '<<parts.size();
    for(const auto& p:parts)
        o<<' '<<p.id<<' '<<p.kind<<' '<<p.x<<' '<<p.y<<' '<<p.z<<' '<<p.yaw<<' '<<p.length<<' '<<p.mass<<' '<<p.strength<<' '<<p.condition;
    o<<' '<<joints.size();
    for(const auto& j:joints)o<<' '<<j.a<<' '<<j.b<<' '<<j.type;
    return o.str();
}
bool Structure::load(const std::string& data){
    std::istringstream is(data);std::string tag;
    unsigned next=0;size_t np=0,nj=0;
    int md=0;
    if(!(is>>tag))return false;
    if(tag=="STRUCT2"){if(!(is>>md)||(md!=0&&md!=1))return false;}
    else if(tag!="STRUCT1")return false; // régi mentés: túlélő mód
    if(!(is>>next>>np)||np>static_cast<size_t>(maxParts)||next==0)return false;
    std::vector<Part> ps;std::unordered_set<unsigned> ids;
    for(size_t i=0;i<np;++i){
        Part p;
        if(!(is>>p.id>>p.kind>>p.x>>p.y>>p.z>>p.yaw>>p.length>>p.mass>>p.strength>>p.condition))return false;
        if(p.id==0||p.id>=next||!ids.insert(p.id).second||p.kind<0||p.kind>PART_PLANK)return false;
        if(!finite(p.x)||!finite(p.y)||!finite(p.z)||!finite(p.yaw)||!finite(p.length)||!finite(p.mass)||!finite(p.strength)||!finite(p.condition))return false;
        if(p.length<=0||p.mass<=0||p.strength<=0||p.condition<0||p.condition>100||p.y<0||p.y>50||std::fabs(p.x)>200||std::fabs(p.z)>200)return false;
        ps.push_back(p);
    }
    if(!(is>>nj)||nj>static_cast<size_t>(maxParts)*8)return false;
    std::vector<Joint> js;
    for(size_t i=0;i<nj;++i){
        Joint j;
        if(!(is>>j.a>>j.b>>j.type))return false;
        if(j.a==j.b||!ids.count(j.a)||!ids.count(j.b)||(j.type!=0&&j.type!=1))return false;
        for(const auto& e:js)if((e.a==j.a&&e.b==j.b)||(e.a==j.b&&e.b==j.a))return false;
        js.push_back(j);
    }
    std::string extra;if(is>>extra)return false;
    parts=ps;joints=js;nextId=next;mode=md;undoStack_.clear();redoStack_.clear();
    return true;
}
}
