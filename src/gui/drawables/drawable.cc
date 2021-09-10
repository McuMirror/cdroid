#include <color.h>
#include <drawables.h>
#include <expat.h>
#include <gravity.h>
#include <fstream>
#include <cdlog.h>
#include <string.h>
#include <textutils.h>


namespace cdroid{

Drawable::Drawable(){
    mLevel=0;
    mChangingConfigurations=0;
    mVisible=true;
    mLayoutDirection=LayoutDirection::LTR;
    mCallback=nullptr;
    mBounds.set(0,0,0,0);
    mColorFilter=nullptr;
}

Drawable::~Drawable(){
    delete mColorFilter;
}

void Drawable::setBounds(const Rect&r){
    setBounds(r.left,r.top,r.width,r.height); 
}

void Drawable::setBounds(int x,int y,int w,int h){
    if((mBounds.left!=x)||(mBounds.top!=y)||(mBounds.width!=w)||(mBounds.height!=h)){
        mBounds.set(x,y,w,h);
        onBoundsChange(mBounds);
    }
}

bool Drawable::getPadding(Rect&padding){
    padding.set(0,0,0,0);
    return false;
}

const Rect&Drawable::getBounds()const{
    return mBounds;
}

Rect Drawable::getDirtyBounds(){
    return mBounds;
}

Drawable*Drawable::mutate(){
    return this;
}

void Drawable::clearMutated(){
}

int Drawable::getOpacity(){
    return UNKNOWN;
}

std::shared_ptr<Drawable::ConstantState>Drawable::getConstantState(){
    return nullptr;
}

void Drawable::setAutoMirrored(bool mirrored){
}

bool Drawable::isAutoMirrored(){
    return false;
}
void Drawable::setColorFilter(ColorFilter*cf){
    delete mColorFilter;
    mColorFilter=cf;
    invalidateSelf();
    LOGD("setColorFilter %p:%p",this,cf);
}

void Drawable::setColorFilter(int color,PorterDuffMode mode){
    setColorFilter(new PorterDuffColorFilter(color,mode));
}
void Drawable::setTint(int color){
    setTintList(ColorStateList::valueOf(color));
}

PorterDuffColorFilter *Drawable::updateTintFilter(PorterDuffColorFilter* tintFilter,ColorStateList* tint,int tintMode){
    if (tint == nullptr/*|| tintMode == nullptr*/) {
        return nullptr;
     }

    int color = tint->getColorForState(getState(), Color::TRANSPARENT);
    if (tintFilter == nullptr) {
        return new PorterDuffColorFilter(color, tintMode);
    }

    tintFilter->setColor(color);
    tintFilter->setMode(tintMode);
    return tintFilter;
}

void Drawable::setTintList(ColorStateList* tint){
}

void Drawable::setTintMode(int mode){
}

bool Drawable::isStateful()const {
    return false;
}

bool Drawable::hasFocusStateSpecified()const{
    return false;
}

bool Drawable::setState(const std::vector<int>&states){
    mStateSet=states;
    return onStateChange(states);
}

const std::vector<int>& Drawable::getState()const{
    return mStateSet;
}

bool Drawable::setLevel(int level){
    if(mLevel!=level){
        mLevel=level;
        return onLevelChange(level);
    }
    return false;
}

int Drawable::getMinimumWidth()const{
    const int intrinsicWidth = getIntrinsicWidth();
    return intrinsicWidth > 0 ? intrinsicWidth : 0;
}

int Drawable::getMinimumHeight()const{
    const int intrinsicHeight = getIntrinsicHeight();
    return intrinsicHeight > 0 ? intrinsicHeight : 0;
}

bool Drawable::setLayoutDirection (int dir){ 
    if (mLayoutDirection != dir) {
        mLayoutDirection = dir;
        return onLayoutDirectionChanged(dir);
    }
    return false;
}

int Drawable::getLayoutDirection()const{
    return mLayoutDirection;
}

int Drawable::getIntrinsicWidth()const{
    return -1;
}

int Drawable::getIntrinsicHeight()const{
    return -1;
}

bool Drawable::isVisible()const{
    return mVisible;
}

bool Drawable::setVisible(bool visible, bool restart){
    const bool changed = mVisible != visible;
    if (changed) {
        mVisible = visible;
        invalidateSelf();
    }
    return changed;
}

int Drawable::getChangingConfigurations()const{
    return mChangingConfigurations;
}

void Drawable::setChangingConfigurations(int configs){
    mChangingConfigurations =configs;
}

void Drawable::setCallback(Drawable::Callback*cbk){
    mCallback=cbk;
}

Drawable::Callback* Drawable::getCallback()const{
    return mCallback;
}

void Drawable::scheduleSelf(Runnable what, long when){
    if(mCallback)mCallback->scheduleDrawable(*this, what, when);
}

void Drawable::unscheduleSelf(Runnable what){
    if(mCallback)mCallback->unscheduleDrawable(*this, what);
}

void Drawable::invalidateSelf(){
    if(mCallback)mCallback->invalidateDrawable(*this);
}

void Drawable::jumpToCurrentState(){
}

Drawable*Drawable::getCurrent(){
    return this;
}

float Drawable::scaleFromDensity(float pixels, int sourceDensity, int targetDensity) {
    return pixels * targetDensity / sourceDensity;
}

int Drawable::scaleFromDensity(int pixels, int sourceDensity, int targetDensity, bool isSize){
    if (pixels == 0 || sourceDensity == targetDensity)
        return pixels;

    const float result = pixels * targetDensity / (float) sourceDensity;
    if (!isSize)  return (int) result;

    const int rounded = round(result);
    if (rounded != 0)     return rounded;
    else if (pixels > 0) return 1;
    else return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Drawable inflate//////////////////////////////////////////////

typedef std::function<Drawable*(Context*ctx, const AttributeSet&attrs)>DrawableParser;

class ParseItem{
public:
    Drawable*drawable;
    AttributeSet props;
    std::string name;
    int mType;//shapetype
    int line;//parser line
    ParseItem(){
        drawable=nullptr;
        mType=0;
        line=0;
    }
};

class ParseData{
public:
    std::vector<ParseItem>items;
    Drawable*drawable;
    XML_Parser parser;
    Context*ctx;
    std::string resourceFile;
    std::string basePath;//only for file parse(ctx=nullptr)
    ParseData(){
        drawable=nullptr;
        parser=nullptr;
    }
    Drawable*getTopDrawable()const{
        return items.size()?items.back().drawable:nullptr;
    }
    bool isTopShape()const{
        return items.size()&&(items.back().mType>0);
    }
};


Drawable*Drawable::createItemDrawable(Context*ctx,const AttributeSet&atts){
    RefPtr<ImageSurface>img;
    std::string resname=atts.getString("color");
    if(!resname.empty()){
        int color=Color::parseColor(resname);
        return new ColorDrawable(color);
    }
    resname=atts.getString("drawable");
    if(resname.empty())
        return nullptr;
    std::vector<NinePatchBlock>horz,vert;
    
    if(resname.find("xml")!=std::string::npos){
        if(ctx==nullptr){
            std::string path=atts.getAbsolutePath(resname);
            return Drawable::inflate(ctx,path);
        }else
            return Drawable::inflate(ctx,resname);
    }
    if(ctx==nullptr){
        std::string path=atts.getAbsolutePath(resname);
        std::ifstream fs(path);
        img=ImageSurface::create_from_stream(fs);
    }else{
        std::unique_ptr<std::istream>is=ctx->getInputStream(resname);
        if(is!=nullptr&&is->good())
            img=ImageSurface::create_from_stream(*is);
    }
    if(img==nullptr)
        return nullptr;
    else if(TextUtils::endWith(resname,"9.png"))
        return new NinePatchDrawable(img);
    else
        return new BitmapDrawable(img);
}

static std::map<const std::string,DrawableParser>drawableParsers={
    {"color"      , ColorDrawable::inflate} ,
    {"shape"      , ShapeDrawable::inflate} ,
    {"bitmap"     , BitmapDrawable::inflate} ,
    {"nine-patch" , NinePatchDrawable::inflate},
    {"inset"      , InsetDrawable::inflate} ,
    {"scale"      , ScaleDrawable::inflate},
    {"rotate"     , RotateDrawable::inflate} ,
    {"clip"       , ClipDrawable::inflate},
    {"transition" , TransitionDrawable::inflate},
    {"layer-list" , LayerDrawable::inflate},
    {"level-list" , LevelListDrawable::inflate} ,
    {"selector"   , StateListDrawable::inflate},
    {"item"       , Drawable::createItemDrawable }     ,
    {"animated-rotate",AnimatedRotateDrawable::inflate}
};

static int parseColor(const std::string&value){
    int color=Color::parseColor(value);
    return color;
}

static void parseShapeGradient(Shape*shape,const AttributeSet&atts){
    std::vector<int>cls;

    cls.push_back(atts.getColor("startColor"));
    if(atts.hasAttribute("centerColor"))
        cls.push_back(atts.getColor("centerColor"));
    cls.push_back(atts.getColor("endColor"));
    shape->setGradientColors(cls);

    shape->setGradientCenterX(atts.getFloat("centerX"));
    shape->setGradientCenterY(atts.getFloat("centerY"));
    shape->setGradientAngle(90);//atts.getFloat("angle",.0));
    if(atts.hasAttribute("gradientRadius"))
        shape->setGradientRadius(atts.getDimensionPixelSize("gradientRadius"));

    std::string type=atts.getString("type");
    if(!type.empty()){
        switch(type[0]){
        case 'l': shape->setGradientType(1); break; //linear gradient
        case 'r': shape->setGradientType(2); break; //radial gradient
        case 's': shape->setGradientType(3); break; //sweep  gradient(not support)
        }
    }
}

static void parseCorners(Shape*shape,const AttributeSet&atts){
    int radius=atts.getDimensionPixelSize("radius",-1);
    
}

static void startElement(void *userData, const XML_Char *name, const XML_Char **satts){
    Drawable*d=nullptr;
    ParseData*pd=(ParseData*)userData;
    ParseItem item;
    auto it=drawableParsers.find(name);
    if(it!=drawableParsers.end()){
        DrawableParser parser=it->second;
        item.props.set(satts);
        if(pd->ctx==nullptr)
            item.props.setBasePath(pd->basePath);
        item.name=name;
        item.line=XML_GetCurrentLineNumber(pd->parser);
        item.drawable=parser(pd->ctx,item.props);
        LOGV("created drawable %s:%p props:%d",name,item.drawable,item.props.size());
    }
    if(pd->isTopShape()){//if current drawable is shapedrawable
        const AttributeSet atts(satts);
        const ParseItem &item=pd->items.back();
        ShapeDrawable*sd=(ShapeDrawable*)item.drawable;
        Shape*shape=sd->getShape();
        LOGV("drawable %p parse shape %p's props %s",item.drawable,shape,name);
        if(strcmp(name,"size")==0){
            int w=atts.getDimensionPixelSize("width");
            int h=atts.getDimensionPixelSize("height");
            shape->resize(w,h);
            sd->setIntrinsicWidth(w);
            sd->setIntrinsicHeight(h);
            LOGV("size %dx%d",w,h);
        }
        if(strcmp(name,"stroke")==0){
            shape->setStrokeColor(atts.getColor("color"));
            shape->setStrokeSize(atts.getDimensionPixelSize("width",1));
            shape->setStrokeDash(atts.getInt("dashWidth"),atts.getInt("dashGap"));
        }
        if(strcmp(name,"solid")==0)   shape->setSolidColor(atts.getColor("color"));
        else if(strcmp(name,"gradient")==0) parseShapeGradient(shape,atts);

        if(strcmp(name,"corners")==0) parseCorners(shape,atts);
        return ;
    }
    if(strcmp(name,"shape")==0){
        const std::string& type=item.props.getString("shape");
        LOGE_IF(type.empty(),"Shape syntax error at %s: line:%d Shape must use attributed shapetype",
                       pd->resourceFile.c_str(),XML_GetCurrentLineNumber(pd->parser));
        item.mType=type[0];
    }
    pd->items.push_back(item);
    LOGV("<%s> props.size=%d stack.size=%d drawable=%p topshape=%d",
            name,item.props.size(),pd->items.size(),item.drawable,pd->isTopShape());
}

static void endElement(void *userData, const XML_Char *name){
    ParseData*pd=(ParseData*)userData;
    Drawable*topchild=pd->items.back().drawable;
    if(strcmp(name,"item")==0){
        const AttributeSet atts=pd->items.back().props;
        pd->items.pop_back();//popup item
        ParseItem&pitem=pd->items.back();
        Drawable*parent=pitem.drawable;
        if(dynamic_cast<StateListDrawable*>(parent)){
            std::vector<int>state;
            StateSet::parseState(state,atts);
            ((StateListDrawable*)parent)->addState(state,topchild);
            LOGV("add drawable %p to StateListDrawable %p",topchild,parent);
        }else if(dynamic_cast<LevelListDrawable*>(parent)){
            int minLevel= atts.getInt("minLevel");//get child level info
            int maxLevel= atts.getInt("maxLevel");
            ((LevelListDrawable*)parent)->addLevel(minLevel,maxLevel,topchild);
            LOGV("add drawable %p to LevelListDrawable %p level=(%d,%d)",topchild,parent,minLevel,maxLevel);
        }else if(dynamic_cast<LayerDrawable*>(parent)){
            LayerDrawable*ld=dynamic_cast<LayerDrawable*>(parent);
            int idx=ld->addLayer(topchild);
            ld->setLayerInset(idx,atts.getDimensionPixelOffset("left"),atts.getDimensionPixelOffset("top"),
                  atts.getDimensionPixelOffset("right"),atts.getDimensionPixelOffset("bottom"));
            ld->setLayerGravity(idx,atts.getGravity("gravity",0));
            int id=atts.getInt("id",-1);
            const std::string src=atts.getString("drawable");
            if(id!=-1)ld->setId(idx,id);
            LOGV("add drawable %pi[%s] to LayerDrawable %p index=%d id=%d",topchild,src.c_str(),parent,idx,id);
        }
    }else if(drawableParsers.find(name)!=drawableParsers.end()){//process shape element
        ParseItem citem=pd->items.back();//child item
        Drawable*cd=citem.drawable;//childdrawable
        pd->drawable=cd;
        pd->items.pop_back();
        if(pd->items.size()){
            ParseItem&pitem=pd->items.back();
            Drawable*parent=pitem.drawable;
            if(dynamic_cast<DrawableWrapper*>(parent)){
                LOGV("%s drawable %p setChild %p",pitem.name.c_str(),parent,topchild);
                ((DrawableWrapper*)parent)->setDrawable(topchild);
            }
            if(pitem.name.compare("item")==0){
                if(pitem.drawable==nullptr){
                   pitem.drawable=topchild;
                   LOGV("parent %p(%s)'s drawable is null ,set to poped drawable %p stacksize=%d",parent,pitem.name.c_str(),topchild,pd->items.size());
                }
           }
       }
    }
}

Drawable*Drawable::fromStream(Context*ctx,std::istream&stream,const std::string& resname){
    ParseData pd;
    int rdlen;
    char buf[256];
    XML_Parser parser=XML_ParserCreate(nullptr);

    std::string basePath=resname.substr(0,resname.find_last_of("/"));
    basePath=basePath.substr(0,basePath.find_last_of("/"));

    pd.parser=parser;
    pd.ctx=ctx;
    pd.resourceFile=resname;
    pd.basePath=basePath;
    XML_SetUserData(parser,&pd);
    pd.items.clear();
    XML_SetElementHandler(parser, startElement, endElement);
    do {
        stream.read(buf,sizeof(buf));
        rdlen=stream.gcount();
        if (XML_Parse(parser, buf,rdlen,!rdlen) == XML_STATUS_ERROR) {
            const char*es=XML_ErrorString(XML_GetErrorCode(parser));
            LOGE("%s at %s:line %ld",es, resname.c_str(),XML_GetCurrentLineNumber(parser));
            XML_ParserFree(parser);
            return nullptr;
        }
    } while(rdlen);
    XML_ParserFree(parser);

    LOGD("parsed drawable [%p] from %s",pd.drawable,resname.c_str());
    return pd.drawable;
}

Drawable*Drawable::inflate(Context*ctx,const std::string& resname){
    Drawable*d=nullptr;
    if(ctx==nullptr){
        std::ifstream fs(resname);
        if(fs.good())d=fromStream(ctx,fs,resname);
    }else if(!resname.empty()){
        std::unique_ptr<std::istream>is=ctx->getInputStream(resname);
        d=fromStream(ctx,*is,resname);
    }
    return d;
}

}
