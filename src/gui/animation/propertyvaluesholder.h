#pragma once

#include <map>
#include <string>
#include <functional>
#include <animation/property.h>
#include <cmath>
#include <iostream>
#include <cdtypes.h>
#include <cdlog.h>
#include <core/color.h>
//reference:
//http://androidxref.com/9.0.0_r3/xref/frameworks/base/libs/hwui/PropertyValuesHolder.h
namespace cdroid{

class PropertyValuesHolder{
protected:
    std::string mPropertyName;
    Property*mProperty;
public:
    PropertyValuesHolder(){
        mProperty=nullptr;
    }
    void setPropertyName(const std::string& propertyName){
        mPropertyName=propertyName;
    }
    const std::string getPropertyName()const{
        return mPropertyName;
    }
    virtual void setFraction(void*target,float fraction)=0;
    virtual ~PropertyValuesHolder(){
        delete mProperty;
    }
};

template <typename T>
class Evaluator {
public:
    virtual void evaluate(T* out, const T& from, const T& to, float fraction) const {};
    virtual ~Evaluator() {}
};

class IntEvaluator:public Evaluator<int> {
public:
    virtual void evaluate(int* out, const int& from, const int& to, float fraction) const override{
        *out = (1.f - fraction)*from +  fraction * to;
    }
};

class FloatEvaluator : public Evaluator<float> {
public:
    virtual void evaluate(float* out, const float& from, const float& to,
            float fraction) const override {
        *out = from * (1 - fraction) + to * fraction;
    }
};

inline constexpr float lerp(float fromValue, float toValue, float fraction) {
    return float(fromValue * (1.f - fraction) + toValue * fraction);
}

class ColorEvaluator : public Evaluator<uint32_t> {
public:
    virtual void evaluate(uint32_t* outColor, const uint32_t& from, const uint32_t& to,
            float fraction) const override{
        float a=lerp((from>>24)/255.f,(to>>24)/255.f,fraction);
        float r=lerp(((from>>16)&0xFF)/255.f,((to>>16)&0xFF)/255.f,fraction);
        float g=lerp(((from>>8)&0xFF)/255.f,((to>>8)&0xFF)/255.f,fraction);
        float b=lerp((from&0xFF)/255.f,(to&0xFF)/255.f,fraction);
        *outColor=((int)(a*255.f)<<24)|((int)(r*255)<<16)|((int)(g*255)<<8)|((int)(b*255));
    }
};

template<typename T>
class PropertyValuesHolderImpl:public PropertyValuesHolder{
public:
    typedef std::function < void(void*target,float fraction,const T&value) > PropertySetter;
protected:
    Evaluator<T>* mEvaluator;
    std::vector<T>mDataSource;
    T mStartValue;
    T mEndValue;
    T mAnimateValue;
    PropertySetter mPropSetter;
public: 
    PropertyValuesHolderImpl(){
        mEvaluator=nullptr;
        mPropSetter=nullptr;
    }
    PropertyValuesHolderImpl(const T& startValue, const T& endValue)
         : mStartValue(startValue), mEndValue(endValue) {}
    void setValues(const T* dataSource, int length) {
        mDataSource.insert(mDataSource.begin(), dataSource, dataSource + length);
    }
    void setValues(const std::vector<T>&values){
         mDataSource=values; 
    }
    void setPropertySetter(PropertySetter setter){
        mPropSetter=setter;
    }
    void setFraction(void*target,float fraction)override{
        if (mDataSource.size()==0) mEvaluator->evaluate(&mAnimateValue, mStartValue, mEndValue, fraction);
        else if (fraction <= 0.0f) mAnimateValue=mDataSource.front();
        else if (fraction >= 1.0f) mAnimateValue=mDataSource.back();
        else{
            fraction *= mDataSource.size() - 1;
            int lowIndex = std::floor(fraction);
            fraction -= lowIndex;
            mEvaluator->evaluate(&mAnimateValue, mDataSource[lowIndex], mDataSource[lowIndex + 1], fraction);
        }
        if(mPropSetter)mPropSetter(target,fraction,mAnimateValue);
    }
    T getAnimatedValue()const{return mAnimateValue;}
};

#if 1
class IntPropertyValuesHolder:public PropertyValuesHolderImpl<int>{
public:
   IntPropertyValuesHolder():PropertyValuesHolderImpl<int>(){
      mEvaluator=new IntEvaluator();
   }
};

class FloatPropertyValuesHolder:public PropertyValuesHolderImpl<float>{
public:
   FloatPropertyValuesHolder():PropertyValuesHolderImpl<float>(){
      mEvaluator=new FloatEvaluator();
   }
};
class ColorPropertyValuesHolder:public PropertyValuesHolderImpl<uint32_t>{
public:
   ColorPropertyValuesHolder():PropertyValuesHolderImpl<uint32_t>(){
      mEvaluator=new ColorEvaluator();
   }
};
#else
typedef PropertyValuesHolderImpl<int>  IntPropertyValuesHolder;
typedef PropertyValuesHolderImpl<float> FloatPropertyValuesHolder;
#endif

}//endof namespace
