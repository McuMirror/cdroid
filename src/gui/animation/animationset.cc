#include <animation/animationset.h>
#include <limits.h>
namespace cdroid{

static constexpr int PROPERTY_FILL_AFTER_MASK         = 0x1;
static constexpr int PROPERTY_FILL_BEFORE_MASK        = 0x2;
static constexpr int PROPERTY_REPEAT_MODE_MASK        = 0x4;
static constexpr int PROPERTY_START_OFFSET_MASK       = 0x8;
static constexpr int PROPERTY_SHARE_INTERPOLATOR_MASK = 0x10;
static constexpr int PROPERTY_DURATION_MASK           = 0x20;
static constexpr int PROPERTY_MORPH_MATRIX_MASK       = 0x40;
static constexpr int PROPERTY_CHANGE_BOUNDS_MASK      = 0x80;

AnimationSet::AnimationSet(const AnimationSet&o)
    :Animation(o){
    init();
    mFlags = o.mFlags;
    mHasAlpha = o.mHasAlpha;
}

AnimationSet::AnimationSet(Context* context,const AttributeSet& attrs)
 :Animation(context, attrs){

    setFlag(PROPERTY_SHARE_INTERPOLATOR_MASK, attrs.getBoolean("shareInterpolator", true));
    init();

    if (attrs.hasAttribute("duration")) {
        mFlags |= PROPERTY_DURATION_MASK;
    }
    if (attrs.hasAttribute("fillBefore")) {
        mFlags |= PROPERTY_FILL_BEFORE_MASK;
    }
    if (attrs.hasAttribute("fillAfter")) {
        mFlags |= PROPERTY_FILL_AFTER_MASK;
    }
    if (attrs.hasAttribute("repeatMode")) {
        mFlags |= PROPERTY_REPEAT_MODE_MASK;
    }
    if (attrs.hasAttribute("startOffset")) {
        mFlags |= PROPERTY_START_OFFSET_MASK;
    }
}

AnimationSet::AnimationSet(bool shareInterpolator){
    setFlag(PROPERTY_SHARE_INTERPOLATOR_MASK, shareInterpolator);
    init();
}

AnimationSet::~AnimationSet(){
    for(auto a:mAnimations){
        if(mFlags&PROPERTY_SHARE_INTERPOLATOR_MASK)
            a->mInterpolator =nullptr;
        delete a;
    }
}

AnimationSet* AnimationSet::clone()const{
    AnimationSet* animation =new AnimationSet(*this);
    for (auto a:mAnimations){
        animation->mAnimations.push_back(a->clone());
    }
    return animation;
}

void AnimationSet::setFlag(int mask, bool value){
    if (value) {
        mFlags |= mask;
    } else {
        mFlags &= ~mask;
    }
}

void AnimationSet::init(){
    mStartTime = 0;
    mFlags = 0;
}

void AnimationSet::setFillAfter(bool fillAfter){
    mFlags |= PROPERTY_FILL_AFTER_MASK;
    Animation::setFillAfter(fillAfter);
}

void AnimationSet::setFillBefore(bool fillBefore){
    mFlags |= PROPERTY_FILL_BEFORE_MASK;
    Animation::setFillBefore(fillBefore);
}

void AnimationSet::setRepeatMode(int repeatMode){
    mFlags |= PROPERTY_REPEAT_MODE_MASK;
    Animation::setRepeatMode(repeatMode);
}

void AnimationSet::setStartOffset(long startOffset){
    mFlags |= PROPERTY_START_OFFSET_MASK;
    Animation::setStartOffset(startOffset);
}

bool AnimationSet::hasAlpha(){
    if (mDirty) {
        mDirty = mHasAlpha = false;
        for (auto a:mAnimations) {
            if (a->hasAlpha()) {
                mHasAlpha = true;
                break;
            }
        }
    }
    return mHasAlpha;
}

void AnimationSet::setDuration(long durationMillis){
    mFlags |= PROPERTY_DURATION_MASK;
    Animation::setDuration(durationMillis);
    mLastEnd = mStartOffset + mDuration;
}

void AnimationSet::addAnimation(Animation* a){
    mAnimations.push_back(a);
    const bool noMatrix = (mFlags & PROPERTY_MORPH_MATRIX_MASK) == 0;
    if (noMatrix && a->willChangeTransformationMatrix()) {
        mFlags |= PROPERTY_MORPH_MATRIX_MASK;
    }

    const bool changeBounds = (mFlags & PROPERTY_CHANGE_BOUNDS_MASK) == 0;

    if (changeBounds && a->willChangeBounds()) {
        mFlags |= PROPERTY_CHANGE_BOUNDS_MASK;
    }

    if ((mFlags & PROPERTY_DURATION_MASK) == PROPERTY_DURATION_MASK) {
        mLastEnd = mStartOffset + mDuration;
    } else {
        if (mAnimations.size() == 1) {
            mDuration = a->getStartOffset() + a->getDuration();
            mLastEnd  = mStartOffset + mDuration;
        } else {
            mLastEnd  = std::max(mLastEnd, mStartOffset + a->getStartOffset() + a->getDuration());
            mDuration = mLastEnd - mStartOffset;
        }
    }

    mDirty = true;
}

void AnimationSet::setStartTime(long startTimeMillis){
    Animation::setStartTime(startTimeMillis);

    for (Animation*a:mAnimations){
        a->setStartTime(startTimeMillis);
    }
}

long AnimationSet::getStartTime()const{
    long startTime = LONG_MAX;//Long.MAX_VALUE;
    for (Animation*a:mAnimations){
        startTime = std::min(startTime, a->getStartTime());
    }
    return startTime;
}

void AnimationSet::restrictDuration(long durationMillis){
    Animation::restrictDuration(durationMillis);

    for (Animation*a:mAnimations) {
        a->restrictDuration(durationMillis);
    }
}

long AnimationSet::getDuration()const{
     long duration = 0;
     if ((mFlags & PROPERTY_DURATION_MASK) == PROPERTY_DURATION_MASK) {
         duration = mDuration;
     } else {
         for (Animation*a:mAnimations)
             duration = std::max(duration, a->getDuration());
     }
     return duration;
}

long AnimationSet::computeDurationHint(){
    long duration = 0;
    int count = mAnimations.size();
    for (int i = count - 1; i >= 0; --i) {
        long d = mAnimations[i]->computeDurationHint();
        if (d > duration) duration = d;
    }
    return duration;
}

void AnimationSet::initializeInvalidateRegion(int left, int top, int right, int bottom){
    Rect& region = mPreviousRegion;
    region.set(left, top, right, bottom);
    region.inset(-1.0f, -1.0f);

    if (mFillBefore) {
        const int count = mAnimations.size();
        Transformation temp;

        Transformation& previousTransformation = mPreviousTransformation;

        for (int i = count - 1; i >= 0; --i) {
            Animation* a = mAnimations.at(i);
            if (!a->isFillEnabled() || a->getFillBefore() || a->getStartOffset() == 0) {
                temp.clear();
                Interpolator* interpolator = a->mInterpolator;
                a->applyTransformation(interpolator ? interpolator->getInterpolation(0.0f) : 0.0f, temp);
                previousTransformation.compose(temp);
            }
        }
    }
}

bool AnimationSet::getTransformation(long currentTime, Transformation& t){
    const int count = mAnimations.size();
    Transformation temp;

    bool more = false;
    bool started = false;
    bool ended = true;

    t.clear();

    for (int i = count - 1; i >= 0; --i) {
        Animation* a = mAnimations.at(i);

        temp.clear();
        more = a->getTransformation(currentTime, temp, getScaleFactor()) || more;
        t.compose(temp);
        started = started || a->hasStarted();
        ended = a->hasEnded() && ended;
    }

    if (started && !mStarted) {
        if (mListener.onAnimationStart) {
            mListener.onAnimationStart(*this);
        }
        mStarted = true;
    }

    if (ended != mEnded) {
        if (mListener.onAnimationEnd) {
            mListener.onAnimationEnd(*this);
        }
        mEnded = ended;
    }

    return more;
}

void AnimationSet::scaleCurrentDuration(float scale){
    for (Animation*a:mAnimations){
        a->scaleCurrentDuration(scale);
    }
}

void AnimationSet::initialize(int width, int height, int parentWidth, int parentHeight){
    Animation::initialize(width, height, parentWidth, parentHeight);

    bool durationSet = (mFlags & PROPERTY_DURATION_MASK) == PROPERTY_DURATION_MASK;
    bool fillAfterSet = (mFlags & PROPERTY_FILL_AFTER_MASK) == PROPERTY_FILL_AFTER_MASK;
    bool fillBeforeSet = (mFlags & PROPERTY_FILL_BEFORE_MASK) == PROPERTY_FILL_BEFORE_MASK;
    bool repeatModeSet = (mFlags & PROPERTY_REPEAT_MODE_MASK) == PROPERTY_REPEAT_MODE_MASK;
    bool shareInterpolator = (mFlags & PROPERTY_SHARE_INTERPOLATOR_MASK) == PROPERTY_SHARE_INTERPOLATOR_MASK;
    bool startOffsetSet = (mFlags & PROPERTY_START_OFFSET_MASK) == PROPERTY_START_OFFSET_MASK;

    if (shareInterpolator) {
        ensureInterpolator();
    }

    std::vector<Animation*>& children = mAnimations;
    const int count = children.size();

    const long duration = mDuration;
    const bool fillAfter = mFillAfter;
    const bool fillBefore = mFillBefore;
    const int repeatMode = mRepeatMode;
    Interpolator* interpolator = mInterpolator;
    const long startOffset = mStartOffset;


    std::vector<long>&storedOffsets = mStoredOffsets;
    if (startOffsetSet) {
        if (/*storedOffsets == null ||*/ storedOffsets.size() != count) {
            mStoredOffsets.resize(count);
        }
    } else if (storedOffsets.size()) {
        mStoredOffsets.clear();
    }

    for (int i = 0; i < count; i++) {
        Animation* a = children.at(i);
        if (durationSet)  a->setDuration(duration);

        if (fillAfterSet) a->setFillAfter(fillAfter);
        if (fillBeforeSet)a->setFillBefore(fillBefore);

        if (repeatModeSet)a->setRepeatMode(repeatMode);

        if (shareInterpolator) a->setInterpolator(interpolator);
 
        if (startOffsetSet) {
            long offset = a->getStartOffset();
            a->setStartOffset(offset + startOffset);
            storedOffsets[i] = offset;
        }
        a->initialize(width, height, parentWidth, parentHeight);
    }
}

void AnimationSet::reset(){
    Animation::reset();
    restoreChildrenStartOffset();
}

void AnimationSet::restoreChildrenStartOffset(){
    if (mStoredOffsets.size()==0) return;

    const int count = mAnimations.size();
    for (int i = 0; i < count; i++) {
        mAnimations[i]->setStartOffset(mStoredOffsets[i]);
    }
}

std::vector<Animation*> AnimationSet::getAnimations(){
    return mAnimations;
}

bool AnimationSet::willChangeTransformationMatrix()const{
    return (mFlags & PROPERTY_MORPH_MATRIX_MASK) == PROPERTY_MORPH_MATRIX_MASK;
}

bool AnimationSet::willChangeBounds()const{
    return (mFlags & PROPERTY_CHANGE_BOUNDS_MASK) == PROPERTY_CHANGE_BOUNDS_MASK;
}

}
