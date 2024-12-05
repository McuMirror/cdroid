#include <attributeset.h>
#include <widget/linearlayout.h>
#include <core/windowmanager.h>
#include <color.h>
#include <string.h>
#include <vector>
#include <cdlog.h>

namespace cdroid{

static std::vector<std::string> split(const std::string & path) {
    std::vector<std::string> vec;
    size_t begin;
    begin = path.find_first_not_of("|");
    while (begin != std::string::npos) {
        size_t end = path.find_first_of("|", begin);
        vec.push_back(path.substr(begin, end-begin));
        begin = path.find_first_not_of("|", end);
    }
    return vec;
}

AttributeSet::AttributeSet():AttributeSet(nullptr,""){
}

AttributeSet::AttributeSet(Context*ctx,const std::string&package)
    :mContext(ctx),mPackage(package){
}

void AttributeSet::setContext(Context*ctx,const std::string&package){
    mContext = ctx;
    mPackage = package;
}

/*@android:+id/title ,?android:attr/windowContentOverlay*/
std::string AttributeSet::normalize(const std::string&pkg,const std::string&property){
    size_t pos;
    std::string value= property;
    const bool hasAT = value.size() && (property[0]=='@');
    const bool hasAsk= value.size() && (property[0]=='?');
    const bool hasSlash = value.find('/')!=std::string::npos;
    const bool isRes = (hasAT|hasAsk) && hasSlash;
    while(isRes && ((pos=value.find_first_of("@?")) != std::string::npos) ){
        value.erase(pos,1);
    }
    
    if( isRes && (value.find(':')==std::string::npos) && hasSlash ){
        value = std::string(pkg+":"+value);
        if(hasAsk)value = "?"+value;
    }/*else if(hasAsk&&(hasSlash==false)){
        value = std::string(pkg+":attr/"+value);
    }*/
    return value;
}

int AttributeSet::set(const char*atts[],int size){
    int rc = 0;
    for(int i = 0;atts[i]&&(size==0||i<size);i+=2,rc+=1){
        const char* key=strrchr(atts[i],' ');
        if(key)key++;else key=atts[i];
        mAttrs.insert({std::string(key),normalize(mPackage,std::string(atts[i+1]))});
    }
    return (int)mAttrs.size();
}

std::map<std::string,std::string>&AttributeSet::getEntries(){
    return mAttrs;
}

int AttributeSet::inherit(const AttributeSet&other){
    int inheritedCount = 0;
    for(auto it = other.mAttrs.begin(); it != other.mAttrs.end() ; it++){
        if(mAttrs.find(it->first)==mAttrs.end()){
           mAttrs.insert(std::pair<std::string,std::string>
              (it->first.c_str(),normalize(mPackage,it->second)));
           inheritedCount++;
        }
    }
    return inheritedCount;
}

bool AttributeSet::add(const std::string&key,const std::string&value){
    auto itr = mAttrs.find(key);
    std::string ks = key;
    size_t pos = ks.find(' ');
    if( pos != std::string::npos )ks = ks.substr(pos+1);
    if(itr == mAttrs.end())
        mAttrs.insert({(std::string)ks,normalize(mPackage,value)});
    else
        itr->second = value;
    return true;
}

bool AttributeSet::hasAttribute(const std::string&key)const{
    return mAttrs.find(key)!=mAttrs.end();
}

size_t AttributeSet::size()const{
    return mAttrs.size();
}

const std::string AttributeSet::getAttributeValue(const std::string&key)const{
    auto it = mAttrs.find(key);
    if( it != mAttrs.end() )
        return it->second;
    return std::string();
}

bool AttributeSet::getBoolean(const std::string&key,bool def)const{
    const std::string v = getAttributeValue(key);
    if(v.empty()) return def;
	return v.compare("true") == 0;
}

int AttributeSet::getInt(const std::string&key,int def)const{
    int base = 10;
    const std::string v = getAttributeValue(key);
    if(v.empty()||((v[0]>='a')&&(v[0]<='z'))){
        return def;
    }
    if(((v.length()>2)&&(v[1]=='x'||v[1]=='X'))||(v[0]=='#'))
        base = 16;
    return std::strtol(v.c_str(),nullptr,base);
}

int AttributeSet::getInt(const std::string&key,const std::map<const std::string,int>&kvs,int def)const{
    const std::string vstr = getAttributeValue(key);
    if( vstr.size() && (vstr.find('|') != std::string::npos) ){
        std::vector<std::string> gs = split(vstr);
        int result= 0;
        int count = 0;
        for(std::string s:gs){
            auto it = kvs.find(s);
            if(it != kvs.end()){
                result |= it->second;
                count++;
            }
        }
        return count ? result : def;
    }else{
        auto it = kvs.find(vstr);
        return it == kvs.end() ? def : it->second;
    }
}

int AttributeSet::getResourceId(const std::string&key,int def)const{
    const std::string str = getString(key);
    if(!str.empty()){
        int value = mContext->getId(str);
        return value == -1 ? def : value;
    }
    return def;
}

int AttributeSet::getArray(const std::string&key,std::vector<std::string>&array)const{
    const std::string str = getString(key);
    if(!str.empty()){
        int value = mContext->getArray(str,array);
        return value;
    }
    return 0;
}

int AttributeSet::getArray(const std::string&key,std::vector<int>&array)const{
    const std::string str = getString(key);
    if(!str.empty()){
        int value = mContext->getArray(str,array);
        return value;
    }
    return 0;
}

int AttributeSet::getColor(const std::string&key,int def)const{
    const std::string resid = getString(key);
    if(resid.empty()) return def;
    else if((resid[0]=='#')||(resid.find(':')==std::string::npos)) {
        return Color::parseColor(resid);
    }
    return mContext->getColor(resid);

}

float AttributeSet::getFloat(const std::string&key,float def)const{
    const std::string v = getAttributeValue(key);
    if(v.empty())return def;
    return std::strtof(v.c_str(),nullptr);
}

float AttributeSet::getFraction(const std::string&key,int base,int pbase,float def)const{
    const std::string v = getAttributeValue(key);
    if(v.empty()) return def;
    float ret = std::strtof(v.c_str(),nullptr);
    if( v.find('%') != std::string::npos )ret /= 100.f;
    return ret;
}

const std::string AttributeSet::getString(const std::string&key,const std::string&def)const{
    const std::string v = getAttributeValue(key);
    if(v.empty())return def;
    return v;
}

static std::map<const std::string,int>gravitykvs={
    {"none"  , Gravity::NO_GRAVITY},
    {"top"   , Gravity::TOP}   ,
    {"bottom", Gravity::BOTTOM},    
    {"left"  , Gravity::LEFT}  ,   
    {"right" , Gravity::RIGHT} ,
    {"center_vertical"  , Gravity::CENTER_VERTICAL},
    {"fill_vertical"    , Gravity::FILL_VERTICAL}  ,
    {"center_horizontal", Gravity::CENTER_HORIZONTAL},
    {"fill_horizontal"  , Gravity::FILL_HORIZONTAL}  ,
    {"center", Gravity::CENTER},
    {"fill"  , Gravity::FILL}  ,
    {"clip_vertical"  , Gravity::CLIP_VERTICAL},
    {"clip_horizontal", Gravity::CLIP_HORIZONTAL},
    {"start",Gravity::START},
    {"end",Gravity::END}
};

int AttributeSet::getGravity(const std::string&key,int defvalue)const{
    int gravity=0;
    const std::string prop=getString(key);
    std::vector<std::string>gs=split(prop);
    for(auto s:gs){
        auto it=gravitykvs.find(s);
        if(it!=gravitykvs.end()){
            gravity|=it->second;
        }
    }
    return gs.size()?gravity:defvalue;
}

int AttributeSet::getDimension(const std::string&key,int def)const{
    const char*p;
    const std::string v = getString(key);
    if( v.empty() ) return def;
    def = std::strtol(v.c_str(),nullptr,10);
    p   = strpbrk(v.c_str(),"sdp");
    return def;
}

int AttributeSet::getDimensionPixelSize(const std::string&key,int def)const{
    const char*p;
    const std::string v = getString(key);
    if( v.empty() ) return def;
    def = std::strtol(v.c_str(),nullptr,10);
    p   = strpbrk(v.c_str(),"sdp");
    if(p){
        const DisplayMetrics& dm=mContext->getDisplayMetrics();
        if(strncmp(p,"dp",2)==0||strncmp(p,"dip",3)==0)
            def = (dm.density * def /*+0.5f*/);
        if(strncmp(p,"sp",2)==0)
            def = int(dm.scaledDensity * def /*+0.5f*/);
    }
    return def;
}

int AttributeSet::getDimensionPixelOffset(const std::string&key,int def)const{
    return getDimensionPixelSize(key,def);
}

int AttributeSet::getLayoutDimension(const std::string&key,int def)const{
    const std::string v=getString(key);
    if(v.empty())return def;
    switch(v[0]){
    case 'f':
    case 'm':return -1;//MATCH_PARENT
    case 'w':return -2;//WRAP_CONTENT
    default :return std::strtol(v.c_str(),nullptr,10);
    }
}

ColorStateList*AttributeSet::getColorStateList(const std::string&key)const{
    const std::string resid = getString(key);
    return mContext->getColorStateList(resid);
}

Drawable* AttributeSet::getDrawable(const std::string&key)const{
    const std::string resid=getString(key);
    return mContext->getDrawable(resid);
}

void AttributeSet::dump()const{
    for(auto it=mAttrs.begin();it!=mAttrs.end();it++){
       LOGD("%s = %s",it->first.c_str(),it->second.c_str());
    }
}

}
