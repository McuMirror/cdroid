#include <drawables/ripplebackground.h>
#include <drawables/rippledrawable.h>

namespace cdroid{

RippleComponent::RippleComponent(RippleDrawable* owner,const Rect& bounds){
    mOwner=owner;
    mBounds = bounds;
    mHasMaxRadius =false;
}

void RippleComponent::onBoundsChange() {
    if (!mHasMaxRadius) {
        mTargetRadius = getTargetRadius(mBounds);
        onTargetRadiusChanged(mTargetRadius);
    }
}

void RippleComponent::setup(float maxRadius, int densityDpi) {
    if (maxRadius >= 0) {
        mHasMaxRadius = true;
        mTargetRadius = maxRadius;
    } else {
        mTargetRadius = getTargetRadius(mBounds);
    }

    mDensityScale = 1.0f;//densityDpi * DisplayMetrics.DENSITY_DEFAULT_SCALE;

    onTargetRadiusChanged(mTargetRadius);
}

float RippleComponent::getTargetRadius(const Rect& bounds){
    const float halfWidth = bounds.width / 2.0f;
    const float halfHeight = bounds.height / 2.0f;
    return (float) std::sqrt(halfWidth * halfWidth + halfHeight * halfHeight);
}

void RippleComponent::getBounds(Rect& bounds){
    const int r = (int)std::ceil(mTargetRadius);
    bounds.set(-r, -r, r+r, r+r);
}

void RippleComponent::invalidateSelf(){
    mOwner->invalidateSelf(false);
}

void RippleComponent::onHotspotBoundsChanged(){
}

void RippleComponent::onTargetRadiusChanged(float targetRadius){
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
RippleBackground::RippleBackground(RippleDrawable* owner,const Rect& bounds, bool isBounded):RippleComponent(owner,bounds){
    mIsBounded = isBounded;
}

bool RippleBackground::isVisible()const{
    return mOpacity > 0;
}

void RippleBackground::draw(Canvas& c,float paintAlpha){
    const int alpha = std::min( int(paintAlpha * mOpacity + 0.5f), 255);
    c.arc(0,0,mTargetRadius,0,M_PI*2.f);
    c.fill();
}

void RippleBackground::setState(bool focused, bool hovered, bool pressed){
    if (!mFocused) {
        focused = focused && !pressed;
    }
    if (!mHovered) {
        hovered = hovered && !pressed;
    }
    if (mHovered != hovered || mFocused != focused) {
        mHovered = hovered;
        mFocused = focused;
        onStateChanged();
    }
}

void RippleBackground::onStateChanged(){
    float newOpacity = mFocused ? .6f : mHovered ? .2f : .0f;
    if (mAnimator != nullptr) {
        mAnimator->cancel();
        mAnimator = nullptr;
    }
    mAnimator = ValueAnimator::ofFloat({mOpacity,newOpacity});//this, OPACITY, newOpacity);
    mAnimator->setDuration(OPACITY_DURATION);
    mAnimator->setInterpolator(LinearInterpolator::gLinearInterpolator.get());
    mAnimator->addUpdateListener(ValueAnimator::AnimatorUpdateListener([this](ValueAnimator&anim){
        PropertyValuesHolder*fp=anim.getValues(0);
        mOpacity = GET_VARIANT(fp->getAnimatedValue(),float);
        invalidateSelf();
    }));

    mAnimator->start();
}

void RippleBackground::jumpToFinal(){
    if (mAnimator) {
        mAnimator->end();
        mAnimator = nullptr;
    }
}


}
