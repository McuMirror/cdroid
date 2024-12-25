#ifndef __CONTEXT_H__
#define __CONTEXT_H__
#include <string>
#include <iostream>
#include <functional>
#include <cairomm/refptr.h>
#include <cairomm/surface.h>
#include <core/callbackbase.h>
#include <core/attributeset.h>
#include <core/displaymetrics.h>

#define USE(FEATURE) (defined(USE_##FEATURE) && USE_##FEATURE)
#define ENABLE(FEATURE) (defined(ENABLE_##FEATURE) && ENABLE_##FEATURE)

namespace cdroid{

class Drawable;
class ColorStateList;
class Context{
public:
    virtual const std::string getPackageName()const=0;
    virtual const std::string getTheme()const=0;
    virtual void setTheme(const std::string&theme)=0;
    virtual const DisplayMetrics&getDisplayMetrics()const=0;
    virtual int getId(const std::string&)const=0;
    virtual int getNextAutofillId()=0;
    virtual const std::string getString(const std::string&id,const std::string&lan="")=0;
    virtual std::unique_ptr<std::istream>getInputStream(const std::string&,std::string*outpkg=nullptr)=0;
    Cairo::RefPtr<Cairo::ImageSurface> loadImage(const std::string&resname){
        return loadImage(resname,-1,-1);
    }
    Cairo::RefPtr<Cairo::ImageSurface> loadImage(const std::string&resname,int width,int height){
        std::unique_ptr<std::istream> stm = getInputStream(resname);
        if(stm)return loadImage(*stm,width,height);
        return nullptr;
    }
    virtual Cairo::RefPtr<Cairo::ImageSurface> loadImage(std::istream&,int width,int height)=0;
    virtual Drawable* getDrawable(const std::string&resid)=0;
    virtual int getColor(const std::string&resid,int def=0)=0;
    virtual bool getBoolean(const std::string&resid,bool def=false)=0;
    virtual int getDimension(const std::string&resid,int def=0)=0;
    virtual float getFloat(const std::string&resid,float def=0)=0;
    virtual size_t getArray(const std::string&resname,std::vector<std::string>&)=0;
    virtual size_t getArray(const std::string&resname,std::vector<int>&)=0;
    virtual ColorStateList* getColorStateList(const std::string&resid)=0;
    virtual AttributeSet obtainStyledAttributes(const std::string&resid)=0;
};

}
#endif
