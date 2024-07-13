#include <textutils.h>
#include <string.h>
#include <cdtypes.h>
#include <cdlog.h>

#ifdef USE_ICONV
#include <iconv.h>
#endif
namespace cdroid{
static int utf8ToUnicodeChar (unsigned char *ch, wchar_t *unicode);

#ifdef USE_ICONV
static int convert(const char*from_charset,const char*to_charset,const char*inbuf,size_t inlen,char*outbuf,size_t outlen){
    char **pin = (char**)&inbuf;
    char **pout = &outbuf;
    size_t outlen0=outlen;
    iconv_t cd = iconv_open(to_charset,from_charset);
    if (cd==0) return -1;
    memset(outbuf,0,outlen);
    iconv(cd, pin, &inlen,pout, &outlen);
    iconv_close(cd);
    return outlen0-outlen;
}
#endif

const char* TextUtils::UCSWCHAR(){
    union { unsigned short bom; unsigned char byte; } endian_test;
    endian_test.bom = 0xFFFE;
    if(sizeof(wchar_t) == 4 && endian_test.byte == 0xFE)
        return "UCS-4LE";
    else if(sizeof(wchar_t) == 2 && endian_test.byte == 0xFE)
        return "UCS-2LE";
    else if(sizeof(wchar_t) == 4 && endian_test.byte != 0xFE)
         return "UCS-4BE";
    else if(sizeof(wchar_t) == 2 && endian_test.byte != 0xFE)
        return "UCS-2BE";
    else
        return "ASCII";
}

const char*TextUtils::UCS16(){
    union { unsigned short bom; unsigned char byte; } endian_test;
    endian_test.bom = 0xFFFE;
    if(endian_test.byte == 0xFE)
        return "UCS-2LE";
    return "UCS-2BE";
}

const std::string TextUtils::utf16_utf8(const unsigned short*utf16,int len){
    char*out=new char[len*4];
    #ifdef USE_ICONV
    int rc = convert(UCS16(),"UTF-8",(const char*)utf16,len*2,out,len*4);
    LOGD_IF(rc==-1,"convert error");
    #else
    char*pout = out;
    for(int i=0;i<len;i++){
        int n=UCS2UTF(utf16[i],pout,4);
        pout+=n;
    }
    *pout = 0;
    #endif
    std::string u8s=out;
    delete[] out;
    return u8s; 
}

const std::wstring TextUtils::utf8tounicode(const std::string&utf8){
    size_t u8len = utf8.size() + 8;
    wchar_t *out = new wchar_t[u8len];
    #ifdef USE_ICONV
    int rc = convert("UTF-8",UCSWCHAR(),utf8.c_str(),utf8.size(),(char*)out,sizeof(wchar_t)*u8len);
    LOGD_IF(rc==-1,"convert error");
    #else
    wchar_t*pout = out;
    for(int i=0;i< utf8.length() ;){
        int n=UTF2UCS((utf8.c_str()+i),pout++);
        i+=n;
    }
    *pout = 0;
    #endif
    std::wstring u32s(out,wcslen(out));
    delete[] out;
    return u32s;
}

const std::u16string TextUtils::utf8_utf16(const std::string&utf8){
    size_t u8len = utf8.size()+8;
    char16_t *out= new char16_t[u8len];
    #ifdef USE_ICONV
    int rc = convert("UTF-8",UCSWCHAR(),utf8.c_str(),utf8.size(),(char*)out,sizeof(wchar_t)*u8len);
    LOGD_IF(rc==-1,"convert error");
    #else
    char16_t*pout = out;
    for(int i = 0;i < utf8.length() ;){
	wchar_t oc;
        int n=UTF2UCS((utf8.c_str()+i),&oc);
	*pout++= oc;
        i += n;
    }
    *pout = 0;
    #endif
    std::u16string u16s(out);//,wcslen(out));
    delete[] out;
    return u16s;
}

const std::string TextUtils::unicode2utf8(const std::wstring&u32s){
    int u8len = u32s.length()*4+8;
    char*out  = new char[u8len];
    #ifdef USE_ICONV
    int rc = convert(UCSWCHAR(),"UTF-8",(char*)u32s.c_str(),u32s.length()*sizeof(wchar_t),out,u8len);
    LOGD_IF(rc==-1,"convert error");
    #else
    //static int ucs4ToUtf8 (unsigned char *s, wchar_t uc, int n)
    char*pout = out;
    for(int i = 0 ;i < u32s.length() ;i++){
        int n = UCS2UTF(u32s[i],pout,4);
        pout+=n;
    }
    *pout=0;
    #endif
    std::string u8s = out;
    delete[] out;
    return u8s;
}

bool TextUtils::startWith(const std::string&str,const std::string&head){
    return str.compare(0, head.size(), head) == 0;
}

bool TextUtils::endWith(const std::string&str,const std::string&tail){
    return str.size()>=tail.size()&&(str.compare(str.size() - tail.size(), tail.size(), tail) == 0);
}

std::string& TextUtils::trim(std::string&s){
    if (s.empty()){  
        return s;  
    }  
    s.erase(0,s.find_first_not_of(" \t\r\n"));  
    s.erase(s.find_last_not_of(" \t\r\n") + 1);  
    return s;
}

std::string& TextUtils::replace(std::string&src,const std::string&old_value,const std::string&new_value){
    for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
        if ((pos = src.find(old_value, pos)) != std::string::npos) {
             src.replace(pos, old_value.length(), new_value);
        }
        else break;
    }
    return src;
}

long TextUtils::strtol(const std::string&value){
    if(value.empty())return 0;
    if(value[0]=='#')return std::strtoul(value.c_str()+1,nullptr,16);
    if((value[0]=='0')&&((value[1]=='x')||(value[1]=='X')))
        return std::strtoul(value.c_str()+2,nullptr,16);
    return std::strtoul(value.c_str(),nullptr,10);
}

std::vector<std::string> TextUtils::split(const std::string& s,const std::string& delim){
    std::vector<std::string> elems;
    size_t pos = 0;
    size_t len = s.length();
    size_t delim_len = delim.length();
    if (delim_len == 0) return elems;
    while (pos < len){
        int find_pos = s.find(delim, pos);
        if (find_pos < 0){
            elems.push_back(s.substr(pos, len - pos));
            break;
        }
        elems.push_back(s.substr(pos, find_pos - pos));
        pos = find_pos + delim_len;
    }
    return elems;
}

int TextUtils::UTF2UCS(const char*utf,wchar_t*unicode){
    const unsigned char *p = (const unsigned char*)utf;
    int e = 0, n = 0;
    if(!p || !unicode) return 0;

    if(*p >= 0xfc) {          /* 6:<11111100> */
        e = (p[0] & 0x01) << 30;
        e |= (p[1] & 0x3f) << 24;
        e |= (p[2] & 0x3f) << 18;
        e |= (p[3] & 0x3f) << 12;
        e |= (p[4] & 0x3f) << 6;
        e |= (p[5] & 0x3f);
        n = 6;
    } else if(*p >= 0xf8) {  /* 5:<11111000> */
        e = (p[0] & 0x03) << 24;
        e |= (p[1] & 0x3f) << 18;
        e |= (p[2] & 0x3f) << 12;
        e |= (p[3] & 0x3f) << 6;
        e |= (p[4] & 0x3f);
        n = 5;
    } else if(*p >= 0xf0) {  /* 4:<11110000> */
        e = (p[0] & 0x07) << 18;
        e |= (p[1] & 0x3f) << 12;
	e |= (p[2] & 0x3f) << 6;
        e |= (p[3] & 0x3f);
        n = 4;
    } else if(*p >= 0xe0) {  /* 3:<11100000> */
        e = (p[0] & 0x0f) << 12;
        e |= (p[1] & 0x3f) << 6;
        e |= (p[2] & 0x3f);
        n = 3;
    } else if(*p >= 0xc0) {  /* 2:<11000000> */
        e = (p[0] & 0x1f) << 6;
        e |= (p[1] & 0x3f);
        n = 2;
    } else {
        e = p[0];
        n = 1;
    }
    *unicode = e;
    /* Return bytes count of this utf-8 character */
    return n;
}

int TextUtils::UCS2UTF(wchar_t wc,char*oututf,int outlen){
    int count=0;
    unsigned char*s=(unsigned char*)oututf;
    if (wc < 0x80)         count = 1;
    else if (wc < 0x800)   count = 2;
    else if (wc < 0x10000) count = 3;
    else if (wc < 0x110000)count = 4;
    else  return -1;

    if(oututf==nullptr)return count;
    if(outlen<count)return -1;

    switch (count){ /* note: code falls through cases! */
    case 4: s[3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
    case 3: s[2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
    case 2: s[1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
    case 1: s[0] = wc;
    }
    return count;
}

}//namespace
