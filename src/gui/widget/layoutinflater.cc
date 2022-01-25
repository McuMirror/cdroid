#include <widget/layoutinflater.h>
#include <widget/viewgroup.h>
#include <cdroid.h>
#include <expat.h>
#include <cdlog.h>
#include <string.h>
#include <fstream>

namespace cdroid{

LayoutInflater::LayoutInflater(Context*context){
    mContext=context;
}

LayoutInflater*LayoutInflater::from(Context*context){
    return new LayoutInflater(context);
}

LayoutInflater::ViewInflater LayoutInflater::getInflater(const std::string&name){
    std::map<const std::string,ViewInflater>&maps=LayoutInflater::getMap();
    const size_t  pt = name.rfind('.');
    const std::string sname=(pt!=std::string::npos)?name.substr(pt+1):name;
    auto it=maps.find(sname);
    return (it!=maps.end())?it->second:nullptr;
}

std::map<const std::string,LayoutInflater::ViewInflater>&LayoutInflater::getMap(){
    static std::map<const std::string,LayoutInflater::ViewInflater> maps;
    return maps;
}

bool LayoutInflater::registInflater(const std::string&name,LayoutInflater::ViewInflater inflater){
    std::map<const std::string,ViewInflater>&maps=LayoutInflater::getMap();
    if(maps.find(name)!=maps.end())
        return false;
    maps.insert(std::map<const std::string,LayoutInflater::ViewInflater>::value_type(name,inflater)); 
    return true;
}

View* LayoutInflater::inflate(const std::string&resource,ViewGroup* root, bool attachToRoot){
    View*v=inflate(resource,root);
    if(root && attachToRoot) root->addView(v);
    return v;
}

View* LayoutInflater::inflate(const std::string&resource,ViewGroup*root){
    View*v=nullptr;
    if(mContext){
        std::unique_ptr<std::istream>stream=mContext->getInputStream(resource);
        if(stream && stream->good()) v=inflate(*stream,root);
    }else{
        std::ifstream fin(resource);
        v=inflate(fin,root);
    }
    return v;
}

typedef struct{
    Context*ctx;
    XML_Parser parser;
    std::vector<View*>views;//the first element is rootview setted by inflate
    ViewGroup*root;
    int parsedView;
}WindowParserData;

static void startElement(void *userData, const XML_Char *name, const XML_Char **satts){
    WindowParserData*pd=(WindowParserData*)userData;
    AttributeSet atts(satts);
    LayoutInflater::ViewInflater inflater=LayoutInflater::getInflater(name);
    ViewGroup*parent=nullptr;
    atts.setContext(pd->ctx);
    if(pd->views.size())
        parent=dynamic_cast<ViewGroup*>(pd->views.back());
    if(strcmp(name,"merge")==0)return;
    if(inflater==nullptr){
        XML_StopParser(pd->parser,false);
        LOGE("Unknown Parser for %s",name);
        return;
    }
    const std::string stname=atts.getString("style");
    if(stname.empty()){
        AttributeSet style=pd->ctx->obtainStyledAttributes(stname);
        atts.inherit(style);
    }
    View*v=inflater(pd->ctx,atts);
    pd->parsedView++;
    pd->views.push_back(v);
    if(parent){
        LayoutParams*lp=parent->generateLayoutParams(atts);
        LOGV("<%s> layoutSize=%dx%d id:%d",name,lp->width,lp->height,v->getId());
        parent->addViewInLayout(v,-1,lp,true);
    }else{
        LayoutParams*lp=((ViewGroup*)v)->generateLayoutParams(atts);
        ((ViewGroup*)v)->setLayoutParams(lp);
    }
}

static void endElement(void *userData, const XML_Char *name){
    WindowParserData*pd=(WindowParserData*)userData;
    ViewGroup*p=dynamic_cast<ViewGroup*>(pd->views.back());
    if(strcmp(name,"merge")==0)return;
	if((pd->views.size()==1)&&(pd->root==nullptr))
	    pd->root=p; 
    pd->views.pop_back();
}

View* LayoutInflater::inflate(std::istream&stream,ViewGroup*root){
    int len=0;
    char buf[256];
    XML_Parser parser=XML_ParserCreateNS(nullptr,' ');
    WindowParserData pd={mContext,parser};
    ULONGLONG tstart=SystemClock::uptimeMillis();

    pd.parsedView=0;
    if(root){pd.root=root;pd.views.push_back(root);}
    XML_SetUserData(parser,&pd);
    XML_SetElementHandler(parser, startElement, endElement);
    do {
        stream.read(buf,sizeof(buf));
        len=stream.gcount();
        if (XML_Parse(parser, buf,len,len==0) == XML_STATUS_ERROR) {
            const char*es=XML_ErrorString(XML_GetErrorCode(parser));
            LOGE("%s at line %ld",es, XML_GetCurrentLineNumber(parser));
            XML_ParserFree(parser);
            return nullptr;
        }
    } while(len!=0);
    XML_ParserFree(parser);
    pd.root->requestLayout();
    pd.root->startLayoutAnimation();
    LOGV("usedtime %dms  parsed %d views",SystemClock::uptimeMillis()-tstart,pd.parsedView);
    return pd.root;
}

}//endof namespace
