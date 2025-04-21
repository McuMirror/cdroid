#include <drawables/scaledrawable.h>

namespace cdroid{

#define MAX_LEVEL 10000
#define DO_NOT_SCALE 1.0f
ScaleDrawable::ScaleState::ScaleState():DrawableWrapperState(){
    mScaleWidth = DO_NOT_SCALE;
    mScaleHeight= DO_NOT_SCALE;
    mGravity = Gravity::LEFT;
    mUseIntrinsicSizeAsMin = false;
}

ScaleDrawable::ScaleState::ScaleState(const ScaleState& orig)
    :DrawableWrapperState(orig){
    mScaleWidth = orig.mScaleWidth;
    mScaleHeight = orig.mScaleHeight;
    mGravity = orig.mGravity;
    mUseIntrinsicSizeAsMin = orig.mUseIntrinsicSizeAsMin;
    mInitialLevel = orig.mInitialLevel;
}

ScaleDrawable* ScaleDrawable::ScaleState::newDrawable(){
    return new ScaleDrawable(std::dynamic_pointer_cast<ScaleState>(shared_from_this()));
}

////////////////////////////////////////////////////////////////////////////////////////////
ScaleDrawable::ScaleDrawable():ScaleDrawable(std::make_shared<ScaleState>()){
}

ScaleDrawable::ScaleDrawable(std::shared_ptr<ScaleState> state):DrawableWrapper(state){
    mState = state;
}

ScaleDrawable::ScaleDrawable(Drawable* drawable, int gravity,float scaleWidth,float scaleHeight)
    :ScaleDrawable(std::make_shared<ScaleState>()){
    mState->mGravity    = gravity;
    mState->mScaleWidth = scaleWidth;
    mState->mScaleHeight= scaleHeight;
    mState->mUseIntrinsicSizeAsMin = false;
    setDrawable(drawable);
}

std::shared_ptr<Drawable::ConstantState>ScaleDrawable::getConstantState(){
    return mState;
}

std::shared_ptr<DrawableWrapper::DrawableWrapperState>ScaleDrawable::mutateConstantState(){
    mState = std::make_shared<ScaleState>(*mState);
    return mState;
}

bool ScaleDrawable::onLevelChange(int level) {
    DrawableWrapper::onLevelChange(level);
    onBoundsChange(getBounds());
    invalidateSelf();
    return true;
}

void ScaleDrawable::onBoundsChange(const Rect& bounds){
    Drawable*d = getDrawable();
    Rect r;
    const bool min = mState->mUseIntrinsicSizeAsMin;
    const int level = getLevel();

    int w = bounds.width;
    if (mState->mScaleWidth > 0.f) {
        const int iw = min ? d->getIntrinsicWidth() : 0;
        w -= (int) ((w - iw) * (MAX_LEVEL - level) * mState->mScaleWidth / MAX_LEVEL);
    }

    int h = bounds.height;
    if (mState->mScaleHeight > 0.f) {
        const int ih = min ? d->getIntrinsicHeight() : 0;
        h -= (int) ((h - ih) * (MAX_LEVEL - level) * mState->mScaleHeight / MAX_LEVEL);
    }

    const int layoutDirection = getLayoutDirection();
    Gravity::apply(mState->mGravity, w, h, bounds, r, layoutDirection);

    if (w > 0 && h > 0) {
        d->setBounds(r);
    }
}

int ScaleDrawable::getGravity()const{
    return mState->mGravity;
}

int ScaleDrawable::getOpacity(){
    Drawable* d = getDrawable();
    if (d->getLevel() == 0) {
        return PixelFormat::TRANSPARENT;
    }
    const int opacity = d->getOpacity();
    if (opacity == PixelFormat::OPAQUE && d->getLevel() < MAX_LEVEL) {
        return PixelFormat::TRANSLUCENT;
    }
    return opacity;
}

void ScaleDrawable::draw(Canvas& canvas) {
    Drawable*d = getDrawable();
    if (d && d->getLevel() != 0) {
        d->draw(canvas);
    }
}

extern int getDimensionOrFraction(const AttributeSet&attrs,const std::string&key,int base,int def);

void ScaleDrawable::inflate(XmlPullParser&parser,const AttributeSet&atts){
    mState->mScaleWidth = getDimensionOrFraction(atts,"scaleWidth", 100, mState->mScaleWidth);
    mState->mScaleHeight = getDimensionOrFraction(atts,"scaleHeight", 100, mState->mScaleHeight);
    mState->mGravity = atts.getGravity("scaleGravity", mState->mGravity);
    mState->mUseIntrinsicSizeAsMin = atts.getBoolean("useIntrinsicSizeAsMinimum", mState->mUseIntrinsicSizeAsMin);
    mState->mInitialLevel = atts.getInt("level", mState->mInitialLevel);
    DrawableWrapper::inflate(parser,atts);
}

}
