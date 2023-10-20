#include <drawables/animatedimagedrawable.h>
#include <systemclock.h>
#include <cdlog.h>
#include <gui/gui_features.h>

namespace cdroid{

#ifdef ENABLE_GIF

#include <gif_lib.h>
#define  argb(a, r, g, b) ( ((a) & 0xff) << 24 ) | ( ((r) & 0xff) << 16 ) | ( ((g) & 0xff) << 8 ) | ((b) & 0xff)

#define  dispose(ext) (((ext)->Bytes[0] & 0x1c) >> 2)
#define  trans_index(ext) ((ext)->Bytes[3])
#define  transparency(ext) ((ext)->Bytes[0] & 1)
#define  delay(ext) (10*((ext)->Bytes[2] << 8 | (ext)->Bytes[1]))
static int gifDrawFrame(GifFileType*gif,int&current_frame,size_t pxstride, uint8_t*pixels,bool force_dispose_1);
#endif

AnimatedImageDrawable::State::State(){
    mAutoMirrored= false;
    mCurrentFrame= 0;
    mFrameCount  = 0;
    mHandler = nullptr;
    mRepeatCount = REPEAT_UNDEFINED;
}

AnimatedImageDrawable::State::~State(){
}

AnimatedImageDrawable::AnimatedImageDrawable():Drawable(){
    mHandler = nullptr;
    mStarting = false;
    mIntrinsicWidth = 0;
    mIntrinsicHeight= 0;
}

AnimatedImageDrawable::AnimatedImageDrawable(cdroid::Context*ctx,const std::string&res)
   :AnimatedImageDrawable(){
    auto stm = ctx->getInputStream(res);
    if(stm){
	 loadGIF(*stm);
	 start();
    }
}

AnimatedImageDrawable::~AnimatedImageDrawable(){
#ifdef ENABLE_GIF
    GifFileType*gif = (GifFileType*)mState.mHandler;
    DGifCloseFile(gif,nullptr);
#endif
}

Handler* AnimatedImageDrawable::getHandler() {
    if (mHandler == nullptr) {
        mHandler = new Handler();//Looper.getMainLooper());
    }
    return mHandler;
}

void AnimatedImageDrawable::setRepeatCount(int repeatCount){
    if (repeatCount < REPEAT_INFINITE) {
         LOGE("invalid value passed to setRepeatCount %d",repeatCount);
    }
    if (mState.mRepeatCount != repeatCount) {
        mState.mRepeatCount = repeatCount;
    }
}

int AnimatedImageDrawable::getRepeatCount()const{
    return mState.mRepeatCount;
}

int AnimatedImageDrawable::getIntrinsicWidth()const{
    return mIntrinsicWidth;
}

int AnimatedImageDrawable::getIntrinsicHeight()const{
    return mIntrinsicHeight;
}

void AnimatedImageDrawable::setAlpha(int alpha){
    
}

int AnimatedImageDrawable::getAlpha()const{
    return 255;
}
#ifdef ENABLE_GIF
static int GIFRead(GifFileType *gifFile, GifByteType *buff, int rdlen){
    std::istream*is=(std::istream*)gifFile->UserData;
    is->read((char*)buff,rdlen);
    return is->gcount();
}
#endif
int AnimatedImageDrawable::loadGIF(std::istream&is){
    int err;
#ifdef ENABLE_GIF
    GifFileType*gifFileType = DGifOpen(&is,GIFRead,&err);
    LOGE_IF(gifFileType==nullptr,"git load failed");
    if(gifFileType==nullptr)return 0;
    DGifSlurp(gifFileType);
    ExtensionBlock *ext;

    mState.mImage = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,gifFileType->SWidth,gifFileType->SHeight);
    mState.mFrameCount = gifFileType->ImageCount;
    mState.mCurrentFrame = 0;
    LOGD("gif %d frames loaded size(%dx%d)",mState.mFrameCount,gifFileType->SWidth,gifFileType->SHeight);
    mState.mHandler = gifFileType;
    mIntrinsicWidth = gifFileType->SWidth;
    mIntrinsicHeight= gifFileType->SHeight;
    return gifFileType->ImageCount; 
#else
    return 0;
#endif
}

constexpr int FINISHED=-1;
void AnimatedImageDrawable::draw(Canvas& canvas){
    if (mStarting) {
        mStarting = false;
        postOnAnimationStart();
    }
#ifdef ENABLE_GIF
    canvas.save();
    const long nextUpdate = gifDrawFrame((GifFileType*)mState.mHandler,mState.mCurrentFrame,
             mState.mImage->get_stride(),mState.mImage->get_data(),false);
    // a value <= 0 indicates that the drawable is stopped or that renderThread
    // will manage the animation
    LOGV("draw Frame %d/%d nextUpdate=%d",mState.mCurrentFrame,mState.mFrameCount,nextUpdate);
    if (nextUpdate > 0) {
        if (mRunnable == nullptr) {
            mRunnable = std::bind(&AnimatedImageDrawable::invalidateSelf,this);
        }
        scheduleSelf(mRunnable, nextUpdate + SystemClock::uptimeMillis());
    } else if (nextUpdate == FINISHED) {
        // This means the animation was drawn in software mode and ended.
        postOnAnimationEnd();
    }
    mState.mImage->mark_dirty();
    canvas.set_source(mState.mImage,mBounds.left,mBounds.top);
    canvas.rectangle(mBounds.left,mBounds.top,mBounds.width,mBounds.height);
    canvas.fill();
    canvas.restore();
#endif
}

bool AnimatedImageDrawable::isRunning(){
    return true;    
}

void AnimatedImageDrawable::start(){
    /*if (mState.mNativePtr == 0) {
        throw "called start on empty AnimatedImageDrawable";
    }

    if (nStart(mState.mNativePtr))*/{
        mStarting = true;
        invalidateSelf();
    }
}

void AnimatedImageDrawable::stop(){
    /*if (mState.mNativePtr == 0) {
        throw "called stop on empty AnimatedImageDrawable";
    }
    if (nStop(mState.mNativePtr))*/ {
        postOnAnimationEnd();
    }
}

void AnimatedImageDrawable::registerAnimationCallback(Animatable2::AnimationCallback callback){
    mAnimationCallbacks.push_back(callback);
}

static bool operator==(const Animatable2::AnimationCallback&a,const Animatable2::AnimationCallback&b){
    return (a.onAnimationStart==b.onAnimationStart) && (a.onAnimationEnd==b.onAnimationEnd);
}

bool AnimatedImageDrawable::unregisterAnimationCallback(Animatable2::AnimationCallback callback){
    auto it=std::find(mAnimationCallbacks.begin(),mAnimationCallbacks.end(),callback);
    bool rc=(it!=mAnimationCallbacks.end());
    if(rc)
        mAnimationCallbacks.erase(it);
    return rc;
}

void AnimatedImageDrawable::postOnAnimationStart(){
    if (mAnimationCallbacks.size() == 0) {
        return;
    }
    Runnable r([this](){
        for (Animatable2::AnimationCallback callback : mAnimationCallbacks) {
            callback.onAnimationStart(*this);
        }
    });
    getHandler()->post(r);
}

void AnimatedImageDrawable::postOnAnimationEnd(){
    if (mAnimationCallbacks.size()==0) {
        return;
    }
    Runnable r([this]{
        for (Animatable2::AnimationCallback callback : mAnimationCallbacks) {
            callback.onAnimationEnd(*this);
        }
    });
    getHandler()->post(r);
}

void AnimatedImageDrawable::clearAnimationCallbacks(){
    mAnimationCallbacks.clear();
}

#ifdef ENABLE_GIF
static int gifDrawFrame(GifFileType*gif,int&current_frame,size_t pxstride,uint8_t *pixels,bool force_dispose_1) {
    GifColorType *bg;
    GifColorType *color;
    SavedImage *frame;
    ExtensionBlock *ext = nullptr;
    GifImageDesc *frameInfo;
    ColorMapObject *colorMap;
    uint32_t *line;
    int width, height, x, y, j;
    uint8_t *px;
    width = gif->SWidth;
    height = gif->SHeight;
    frame = &(gif->SavedImages[current_frame]);
    frameInfo = &(frame->ImageDesc);
    if (frameInfo->ColorMap) {
        colorMap = frameInfo->ColorMap;
    } else {
        colorMap = gif->SColorMap;
    }

    bg = &colorMap->Colors[gif->SBackGroundColor];
    for (j = 0; j < frame->ExtensionBlockCount; j++) {
        if (frame->ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE) {
            ext = &(frame->ExtensionBlocks[j]);
            break;
        }
    }
    // For dispose = 1, we assume its been drawn
    px=(uint8_t*)pixels;
    if (ext && dispose(ext) == 1 && force_dispose_1 && current_frame > 0) {
        current_frame = current_frame - 1;
        gifDrawFrame(gif,current_frame,pxstride, pixels, true);
    } else if (ext && dispose(ext) == 2 && bg) {
        for (y = 0; y < height; y++) {
            line = (uint32_t *) px;
            for (x = 0; x < width; x++) {
                line[x] = argb(255, bg->Red, bg->Green, bg->Blue);
            }
            px = (px + pxstride);
        }
        current_frame=(current_frame+1)%gif->ImageCount;
    } else if (ext && dispose(ext) == 3 && current_frame > 1) {
        current_frame = current_frame - 2;
        gifDrawFrame(gif,current_frame,pxstride, pixels, true);
    }else current_frame=(current_frame+1)%gif->ImageCount;
    px = (uint8_t*)pixels;
    if (frameInfo->Interlace) {
        int n = 0, inc = 8, p = 0,loc=0;
        px = (px + pxstride * frameInfo->Top);
        for (y = frameInfo->Top; y < frameInfo->Top + frameInfo->Height; y++) {
            for (x = frameInfo->Left; x < frameInfo->Left + frameInfo->Width; x++) {
                loc = (y - frameInfo->Top) * frameInfo->Width + (x - frameInfo->Left);
                if (ext && frame->RasterBits[loc] == trans_index(ext) && transparency(ext)) {
                    continue;
                }
                color = (ext && frame->RasterBits[loc] == trans_index(ext)) ? bg
                        : &colorMap->Colors[frame->RasterBits[loc]];
                if (color)
                    line[x] = argb(255, color->Red, color->Green, color->Blue);
            }
            px = (px + pxstride * inc);
            n += inc;
            if (n >= frameInfo->Height) {
                n = 0;
                switch (p) {
                case 0:
                    px = (pixels + pxstride * (4 + frameInfo->Top));
                    inc = 8;     p++;     break;
                case 1:
                    px = (pixels + pxstride * (2 + frameInfo->Top));
                    inc = 4;     p++;     break;
                case 2:
                    px = (pixels + pxstride * (1 + frameInfo->Top));
                    inc = 2;     p++;     break;
                }
            }
        }
    } else {
        px = ( px + pxstride * frameInfo->Top);
        for (y = frameInfo->Top; y < frameInfo->Top + frameInfo->Height; y++) {
            line = (uint32_t *) px;
            for (x = frameInfo->Left; x < frameInfo->Left + frameInfo->Width; x++) {
                int loc = (y - frameInfo->Top) * frameInfo->Width + (x - frameInfo->Left);
                if (ext && frame->RasterBits[loc] == trans_index(ext) && transparency(ext)) {
                    continue;
                }
                color = (ext && frame->RasterBits[loc] == trans_index(ext)) ? bg
                        : &colorMap->Colors[frame->RasterBits[loc]];
                if (color)
                    line[x] = argb(255, color->Red, color->Green, color->Blue);
            }
            px +=pxstride;
        }
    }
    return delay(ext);
}
#endif

Drawable*AnimatedImageDrawable::inflate(Context*ctx,const AttributeSet&atts){
    const std::string res = atts.getString("src");
    AnimatedImageDrawable*d = new AnimatedImageDrawable(ctx,res);
    const bool autoStart = atts.getBoolean("autoStart");
    const int repeatCount =atts.getInt("repeatCount",REPEAT_UNDEFINED);
    if(autoStart)d->start();
    if(repeatCount!=REPEAT_UNDEFINED)
	d->setRepeatCount(repeatCount);
    return d;
}

}
