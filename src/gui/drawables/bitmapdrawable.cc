#include <drawables/bitmapdrawable.h>
#include <gravity.h>
#include <fstream>
#include <app.h>
#include <cdlog.h>
extern "C" int  _cairo_image_analyze_transparency (void*image);
namespace cdroid{

BitmapDrawable::BitmapState::BitmapState(){
    mGravity = Gravity::FILL;
    mBaseAlpha=1.0f;
    mAlpha=255;
    mTint=nullptr;
    mTintMode=DEFAULT_TINT_MODE;
    mAutoMirrored=false;
    mSrcDensityOverride=0;
    mTargetDensity=160;
    mChangingConfigurations=0;
}

BitmapDrawable::BitmapState::BitmapState(RefPtr<ImageSurface>bitmap)
    :BitmapState(){
    mBitmap=bitmap;
}

BitmapDrawable::BitmapState::BitmapState(const BitmapState&bitmapState){
    mBitmap = bitmapState.mBitmap;
    mTint = bitmapState.mTint;
    mTintMode = bitmapState.mTintMode;
    mThemeAttrs = bitmapState.mThemeAttrs;
    mChangingConfigurations = bitmapState.mChangingConfigurations;
    mGravity = bitmapState.mGravity;
    //mTileModeX = bitmapState.mTileModeX;
    //mTileModeY = bitmapState.mTileModeY;
    mSrcDensityOverride = bitmapState.mSrcDensityOverride;
    mTargetDensity = bitmapState.mTargetDensity;
    mBaseAlpha = bitmapState.mBaseAlpha;
    mAlpha=bitmapState.mAlpha;
    //mPaint = new Paint(bitmapState.mPaint);
    //mRebuildShader = bitmapState.mRebuildShader;
    mAutoMirrored = bitmapState.mAutoMirrored;
}

Drawable* BitmapDrawable::BitmapState::newDrawable(){
    return new BitmapDrawable(shared_from_this());    
}

int BitmapDrawable::BitmapState::getChangingConfigurations()const{
    return mChangingConfigurations |(mTint ? mTint->getChangingConfigurations() : 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

BitmapDrawable::BitmapDrawable(RefPtr<ImageSurface>img){
    mBitmapState=std::make_shared<BitmapState>(img);
    mDstRectAndInsetsDirty=true;
    computeBitmapSize();
    mMutated=false;
    mTintFilter=nullptr;
}

BitmapDrawable::BitmapDrawable(std::shared_ptr<BitmapState>state){
    mBitmapState=state;
    mDstRectAndInsetsDirty=true;
    mMutated=false;
    mTintFilter=nullptr;
    computeBitmapSize();
}

BitmapDrawable::BitmapDrawable(std::istream&is){
    mTintFilter=nullptr;
    mBitmapState=std::make_shared<BitmapState>();
    RefPtr<ImageSurface>b=ImageSurface::create_from_stream(is);
    setBitmap(b);
}

BitmapDrawable::BitmapDrawable(const std::string&resname){
    mBitmapState=std::make_shared<BitmapState>();
    std::ifstream fs(resname);
    RefPtr<ImageSurface>b;
    mTintFilter=nullptr;
    LOGD("%s",resname.c_str());
    if(fs.good())
        b=ImageSurface::create_from_stream(fs);
    else {
        b=App::getInstance().getImage(resname);
    }
    setBitmap(b);
}

RefPtr<ImageSurface> BitmapDrawable::getBitmap()const{
    return mBitmapState->mBitmap;
}

void BitmapDrawable::setBitmap(RefPtr<ImageSurface>bmp){
    mBitmapState->mBitmap=bmp;
    LOGD("setbitmap %p",bmp.get());
    mDstRectAndInsetsDirty=true;
    computeBitmapSize();
    invalidateSelf();
}

int BitmapDrawable::getAlpha()const{
    return mBitmapState->mAlpha;
}

void BitmapDrawable::setAlpha(int alpha){
    mBitmapState->mAlpha=alpha&0xFF;
}

int BitmapDrawable::getGravity()const{
    return mBitmapState->mGravity;
}

int BitmapDrawable::getIntrinsicWidth()const{
    return mBitmapWidth;
}

int BitmapDrawable::getIntrinsicHeight()const{
    return mBitmapHeight;
}

int ComputeTransparency(RefPtr<ImageSurface>bmp){
    if((bmp->get_content()&Cairo::Content::CONTENT_ALPHA)==0)
        return Drawable::OPAQUE;
    if(bmp->get_width()==0||bmp->get_height()==0)
        return Drawable::TRANSPARENT;

    if(bmp->get_content()&CONTENT_COLOR==0){
        switch(bmp->get_format()){
        case Surface::Format::A1: return Drawable::TRANSPARENT;//CAIRO_IMAGE_HAS_BILEVEL_ALPHA;
        case Surface::Format::A8:
            for(int y=0;y<bmp->get_height();y++){
                uint8_t*alpha=bmp->get_data()+bmp->get_stride()*y;
                for(int x=0;x<bmp->get_width();x++,alpha++)
                    if(*alpha > 0 && *alpha < 255)
                        return Drawable::TRANSLUCENT;//CAIRO_IMAGE_HAS_ALPHA;
            }
            return Drawable::TRANSPARENT;//CAIRO_IMAGE_HAS_BILEVEL_ALPHA;
        default:return Drawable::TRANSLUCENT; 
        }
    }
    if(bmp->get_format()==Surface::Format::RGB16_565||bmp->get_format()==Surface::Format::RGB24)
        return Drawable::OPAQUE;
    if(bmp->get_format()!=Surface::Format::ARGB32)
        return Drawable::TRANSLUCENT;
    for(int y=0;y<bmp->get_height();y++){
        uint32_t*pixel=(uint32_t*)(bmp->get_data()+bmp->get_stride()*y);
        for (int x = 0; x < bmp->get_width(); x++, pixel++){
            int a = (*pixel & 0xff000000) >> 24;
            if (a > 0 && a < 255)return Drawable::TRANSLUCENT;//CAIRO_IMAGE_HAS_ALPHA;
            else if(a==0)return Drawable::TRANSPARENT;//CAIRO_IMAGE_HAS_BILEVEL_ALPHA
        }
    }
    return  Drawable::OPAQUE;
}

int BitmapDrawable::getOpacity(){
    if(mBitmapState->mGravity != Gravity::FILL)
        return TRANSLUCENT;
    if(mBitmapState->mBitmap==nullptr)
        return TRANSPARENT;

    int t=0;//_cairo_image_analyze_transparency(mBitmapState->mBitmap->cobj());
    switch(t){
    case 0:/*CAIRO_IMAGE_IS_OPAQUE,       */ return OPAQUE;
    case 1:/*CAIRO_IMAGE_HAS_BILEVEL_ALPHA*/ return TRANSPARENT;
    case 2:/*CAIRO_IMAGE_HAS_ALPHA        */ return TRANSLUCENT;
    case 3:/*CAIRO_IMAGE_UNKNOWN          */ return UNKNOWN;
    default:return OPAQUE;
    }
}

void BitmapDrawable::setTintList(ColorStateList*tint){
    if (mBitmapState->mTint != tint) {
        mBitmapState->mTint = tint;
        mTintFilter = updateTintFilter(mTintFilter, tint, mBitmapState->mTintMode);
        invalidateSelf();
    }
}

void BitmapDrawable::setTintMode(int tintMode) {
    if (mBitmapState->mTintMode != tintMode) {
        mBitmapState->mTintMode = tintMode;
        mTintFilter = updateTintFilter(mTintFilter, mBitmapState->mTint, tintMode);
        invalidateSelf();
    }
}
std::shared_ptr<Drawable::ConstantState>BitmapDrawable::getConstantState(){
    return mBitmapState;
}

void BitmapDrawable::setGravity(int gravity){
    if(mBitmapState->mGravity!=gravity){
        mBitmapState->mGravity=gravity;
        mDstRectAndInsetsDirty=true;
        invalidateSelf();
    }
}

void BitmapDrawable::computeBitmapSize() {
    if (mBitmapState->mBitmap != nullptr) {
        mBitmapWidth = mBitmapState->mBitmap->get_width();//getScaledWidth(mTargetDensity);
        mBitmapHeight= mBitmapState->mBitmap->get_height();//getScaledHeight(mTargetDensity);
    } else {
        mBitmapWidth = mBitmapHeight = -1;
    }
}

void BitmapDrawable::updateDstRectAndInsetsIfDirty(){
    if (mDstRectAndInsetsDirty) {
        if (0/*mBitmapState.mTileModeX == null && mBitmapState.mTileModeY == null*/) {
            const int layoutDir = getLayoutDirection();
            mDstRect.set(0,0,0,0);
            Gravity::apply(mBitmapState->mGravity,mBitmapWidth,mBitmapHeight,mBounds, mDstRect, layoutDir);

            const int left  = mDstRect.x - mBounds.x;
            const int top   = mDstRect.y - mBounds.y;
            const int right = mBounds.right() - mDstRect.right();
            const int bottom= mBounds.bottom()- mDstRect.bottom();
            mOpticalInsets.set(left, top, right, bottom);
        } else {
            mDstRect=getBounds();
            mOpticalInsets.set(0,0,0,0);// = Insets.NONE;
        }
    }
    mDstRectAndInsetsDirty = false;
}

void BitmapDrawable::onBoundsChange(const RECT&r){
    mDstRectAndInsetsDirty = true;
}

bool BitmapDrawable::onStateChange(const std::vector<int>&){
    if (mBitmapState->mTint  /*&& mBitmapState->mTintMode != nullptr*/) {
        mTintFilter = updateTintFilter(mTintFilter, mBitmapState->mTint, mBitmapState->mTintMode);
        return true;
    }
    return false;    
}

Drawable*BitmapDrawable::mutate(){
    if (!mMutated && Drawable::mutate() == this) {
        mBitmapState=std::make_shared<BitmapState>(*mBitmapState);
        mMutated = true;
    }
    return this;
}

void BitmapDrawable::clearMutated() {
    Drawable::clearMutated();
    mMutated = false;
}

void BitmapDrawable::draw(Canvas&canvas){
    if(mBitmapState->mBitmap==nullptr) return;
    updateDstRectAndInsetsIfDirty();
    LOGV("BitmapSize=%dx%d bounds=%d,%d-%d,%d dst=%d,%d-%d,%d alpha=%d mColorFilter=%p",mBitmapWidth,mBitmapHeight,
            mBounds.x,mBounds.y,mBounds.width,mBounds.height, mDstRect.x,mDstRect.y,
	    mDstRect.width,mDstRect.height,mBitmapState->mAlpha,mTintFilter);
    updateDstRectAndInsetsIfDirty();

    const float sw=mBitmapWidth, sh=mBitmapHeight;
    float dx =mBounds.x     , dy = mBounds.y;
    float dw =mBounds.width , dh = mBounds.height;
    float fx = dw / sw  , fy = dh / sh;
    const float alpha=mBitmapState->mBaseAlpha*mBitmapState->mAlpha/255;

    //canvas.save();
    canvas.rectangle(dx,dy,dw,dh);
    canvas.clip();
    if (mBounds.width !=mBitmapWidth  || mBounds.height != mBitmapHeight) {
       canvas.scale(fx,fy);
       dx /= fx;       dy /= fy;
       dw /= fx;       dh /= fy;
    }

    canvas.set_source(mBitmapState->mBitmap, dx, dy);
    cairo_pattern_set_filter(cairo_get_source(canvas.cobj()), CAIRO_FILTER_BEST);
    cairo_pattern_set_extend(cairo_get_source(canvas.cobj()), CAIRO_EXTEND_NONE);
    canvas.paint_with_alpha(alpha);
    canvas.scale(1./fx,1./fy);
    //canvas.restore();
    if(mTintFilter)mTintFilter->apply(canvas,mBounds);
}

Drawable*BitmapDrawable::inflate(Context*ctx,const AttributeSet&atts){
    const std::string src=atts.getString("src");
    bool antialias=atts.getBoolean("antialias",true);
    bool dither=atts.getBoolean("dither",true);
    bool filter=atts.getBoolean("filter",true);
    bool mipMap=atts.getBoolean("mipMap",true);
    int gravity=atts.getGravity("gravity",Gravity::CENTER);
    int tileMode=0;//"disabled" | "clamp" | "repeat" | "mirror";
    std::string path=src;
    if(ctx==nullptr)path=atts.getAbsolutePath(src);

    LOGD("src=%s",src.c_str());
    if(src.empty())  return nullptr;
    BitmapDrawable*d=new BitmapDrawable(path);
    LOGD("bitmap=%p",d);
    d->setGravity(gravity);
    return d;
}

}

