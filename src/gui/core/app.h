#ifndef __APPLICATION_H__
#define __APPLICATION_H__
#include <string>
#include <map>
#include <istream>
#include <cairomm/surface.h>
#include <looper/looper.h>
#include <context.h>
#include <assets.h>
struct option;
using namespace Cairo;

namespace cdroid{

class App:public Assets{
private:
    const std::string getAssetsPath();
    std::map<std::string,std::string>args;
protected:
    static App*mInst;
public:
     App(int argc=0,const char*argv[]=NULL,const option*extoptions=NULL);
     ~App();
     static App&getInstance();
     const std::string getDataPath()const;
     virtual void setOpacity(unsigned char alpha);
     virtual void setName(const std::string&appname);
     virtual const std::string&getName();

     virtual void setArg(const std::string&key,const std::string&value);
     virtual bool hasArg(const std::string&key)const;
     virtual const std::string&getArg(const std::string&key,const std::string&def="")const;

     virtual int getArgAsInt(const std::string&key,int def)const;
     virtual int addEventSource(EventSource *source, EventHandler handler);
     virtual int removeEventSource(EventSource*source);
     virtual int exec();
     virtual void exit(int code=0);
};

}//namespace
#endif
