#include <inputmethod.h>
#include <utf16char.h>
#include <textutils.h>
#include <cdtypes.h>
#include <cdlog.h>
#include <gui_features.h>
#ifdef ENABLE_PINYIN2HZ
#include <pinyinime.h>
#endif
using namespace ime_pinyin;

namespace cdroid{

InputMethod::InputMethod(const std::string&layout){
    keyboardlayout=layout;
}

int InputMethod::load_dicts(const std::string&sys,const std::string&user){
    sysdict=sys;
    userdict=user;
    return 0;
}

const std::string InputMethod::getSysdict()const{
    return sysdict;
}

const std::string InputMethod::getUserDict()const{
    return userdict;
}

const std::string InputMethod::getKeyboardLayout(int type)const{
    return keyboardlayout;
}

/*return :>=0 success,<0 predict is not supportrd*/
int InputMethod::search(const std::string&,std::vector<std::string>&candidates){
    return -1;
}

void InputMethod::close_search(){
}

/*return :>=0 success,<0 predict is not supportrd*/
int InputMethod::get_predicts(const std::string&,std::vector<std::string>&predicts){
   return -1;
}

#ifdef ENABLE_PINYIN2HZ
GooglePinyin::GooglePinyin(const std::string&layout)
 :InputMethod(layout){
}

int GooglePinyin::load_dicts(const std::string&sys,const std::string&user){
    sysdict=sys;
    userdict=user;
    handle=im_open_decoder(sysdict.c_str(),userdict.c_str());
    LOGD("%p's dict=%s,%s",handle,sys.c_str(),user.c_str());
    return handle!=nullptr;
}

/*pinyin to chinese words*/
int GooglePinyin::search(const std::string&pinyin,std::vector<std::string>&candidates){
    char16 canbuf[64];
    int num=im_search(handle,pinyin.c_str(),pinyin.length());
    for(int i=0;i<num;i++){/*拼音转汉字*/
        char16*scan=im_get_candidate(handle,i,canbuf,32);
        std::string u8s=TextUtils::utf16_utf8(scan,utf16_strlen(scan));
        if(u8s.size())candidates.push_back(u8s);
    }
    LOGD("search %s=%d",pinyin.c_str(),num);
    return num;
}

void GooglePinyin::close_search(){
    im_reset_search(handle);
}

//eg:如输入"大"，会返回 "家","学","概"等
int GooglePinyin::get_predicts(const std::string&txt,std::vector<std::string>&predicts){
    char16 (*predict_buf)[kMaxPredictSize + 1];
    const std::u16string u16txt=TextUtils::utf8_utf16(txt);
    int num=im_get_predicts(handle,(const char16*)u16txt.c_str(),predict_buf);
    LOGV("kMaxPredictSize=%d num=%d",kMaxPredictSize,num);
    for(int i=0;i<num;i++){
        std::string u8s = TextUtils::utf16_utf8(predict_buf[i],utf16_strlen(predict_buf[i]));
        if(u8s.size())predicts.push_back(u8s);
    }
    return num;
}

int GooglePinyin::get_spellings(std::vector<int>&vp){
//only can be called between im_search and im_reset_search
    const unsigned short *pos;
    int num=im_get_spl_start_pos(handle,pos);
    for(int i=0;i<num;i++){
        vp.push_back(pos[i]);
    }
    return num;
}

int GooglePinyin::get_spellings(std::vector<std::string>&sps){
    size_t slen=0;
    const char*str=im_get_sps_str(handle,&slen);
    const unsigned short*pos;
    int num=im_get_spl_start_pos(handle,pos);
    for(int i=0;i<num;i++){
        std::string s;
        if(i<num-1)
           s=std::string(str+pos[i],pos[i+1]-pos[i]);
        else s=std::string(str+pos[i]); 
        sps.push_back(s);
    }
    return sps.size();
}
#endif //ENABLE_PINYIN2HZ

}//namespace

