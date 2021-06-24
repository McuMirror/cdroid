#include <widget/togglebutton.h>
#include <cdlog.h>


namespace cdroid{
#define TOGGLE 1
#define NO_ALPHA 0xFF

ToggleButton::ToggleButton(Context*ctx,const AttributeSet& attrs):CompoundButton(ctx,attrs){
    mIndicatorDrawable=nullptr;
    setTextOn(ctx->getString(attrs.getString("textOn")));
    setTextOff(ctx->getString(attrs.getString("textOff")));
    mDisabledAlpha=attrs.getFloat("disabledAlpha",0.5f);
}

ToggleButton::ToggleButton(int w,int h):CompoundButton(std::string(),w,h){
    mIndicatorDrawable=nullptr;
    mDisabledAlpha=0.5f;
}

const std::string ToggleButton::getTextOn()const{
    return mTextOn;
}

void ToggleButton::setTextOn(const std::string& textOn){
    mTextOn=textOn;
    syncTextState();
}

const std::string ToggleButton::getTextOff()const{
    return mTextOff;
}

void ToggleButton::setTextOff(const std::string& textOff){
    mTextOff=textOff;
    syncTextState();
}

void ToggleButton::setChecked(bool checked) {
    CompoundButton::setChecked(checked);
    LOGV("%p :%d checked=%d",this,getId(),checked);	
    syncTextState();
}

void ToggleButton::syncTextState(){
    bool checked = isChecked();
    if (checked && !mTextOn.empty()) {
        setText(mTextOn);
    } else if (!checked && !mTextOff.empty()) {
        setText(mTextOff);
    }
}

void ToggleButton::drawableStateChanged() {
    CompoundButton::drawableStateChanged();
    if (mIndicatorDrawable != nullptr) {
        mIndicatorDrawable->setAlpha(isEnabled() ? NO_ALPHA : (int) (NO_ALPHA * mDisabledAlpha));
    }
}

void ToggleButton::updateReferenceToIndicatorDrawable(Drawable* backgroundDrawable) {
    if (dynamic_cast<LayerDrawable*>(backgroundDrawable)) {
        LayerDrawable* layerDrawable = (LayerDrawable*) backgroundDrawable;
        mIndicatorDrawable =layerDrawable->findDrawableByLayerId(TOGGLE);//com.android.internal.R.id.toggle);
    } else {
        mIndicatorDrawable = nullptr;
    }
}

View& ToggleButton::setBackgroundDrawable(Drawable* d){
    CompoundButton::setBackgroundDrawable(d);
    updateReferenceToIndicatorDrawable(d);
    return *this;
}

}

