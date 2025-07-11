/*********************************************************************************
 * Copyright (C) [2019] [houzh@msn.com]
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *********************************************************************************/
#include <memory>
#include <cstring>
#include <fstream>
#include <gui_features.h>
#include <drawables/drawable.h>
#include <drawables/bitmapdrawable.h>
#include <drawables/ninepatchdrawable.h>
#include <drawables/animatedimagedrawable.h>
#include <image-decoders/imagedecoder.h>
#include <core/textutils.h>
#include <core/context.h>
#include <core/atexit.h>
#include <png.h>
#include <porting/cdlog.h>
#if ENABLE(LCMS)
#include <lcms2.h>
#endif

namespace cdroid{
using namespace Cairo;
void*ImageDecoder::mCMSProfile = nullptr;

ImageDecoder::ImageDecoder(std::istream&stream):mStream(stream){
    mImageWidth = -1;
    mImageHeight= -1;
    mFrameCount = 1;
    mPrivate = nullptr;
    mTransform= nullptr;
#if ENABLE(LCMS)
    if(mCMSProfile==nullptr){
        mCMSProfile=cmsOpenProfileFromFile("/home/houzh/sRGB Color Space Profile.icm","r");
        if(mCMSProfile){
            AtExit::registerCallback([](){
                cmsCloseProfile(mCMSProfile);
            });
        }
    }
#endif
}

ImageDecoder::~ImageDecoder(){
#if ENABLE(LCMS)
    if(mTransform)cmsDeleteTransform(mTransform);
#endif
}

uint32_t ImageDecoder::mHeaderBytesRequired = 0;
std::unordered_map<std::string,ImageDecoder::Registry> ImageDecoder::mFactories;

ImageDecoder::Registry::Registry(uint32_t msize,Factory& fun,Verifier& v)
  :magicSize(msize),factory(fun),verifier(v){

}

int ImageDecoder::registerFactory(const std::string&mime,uint32_t magicSize,Verifier v,Factory factory){
    auto it = mFactories.find(mime);
    if(it==mFactories.end()){
        mFactories.insert({mime,Registry(magicSize,factory,v)});
        mHeaderBytesRequired = std::max(magicSize,mHeaderBytesRequired);
        LOGD("Register FrameSequence factory[%d] %s", mFactories.size()-1,mime.c_str());
        return 0;
    }else{
        it->second.factory = factory;
    }
    return 0;
}

int ImageDecoder::getWidth()const{
    return mImageWidth;
}

int ImageDecoder::getHeight()const{
    return mImageHeight;
}

int ImageDecoder::getFrameCount()const{
    return mFrameCount;
}

int ImageDecoder::computeTransparency(Cairo::RefPtr<Cairo::ImageSurface>bmp){
    if((bmp==nullptr)||(bmp->get_width()==0)||(bmp->get_height()==0))
        return PixelFormat::TRANSPARENT;
    if((bmp->get_content()&&(Cairo::Content::CONTENT_ALPHA)==0))
        return PixelFormat::OPAQUE;

    if( (bmp->get_content()&CONTENT_COLOR) ==0){
        switch(bmp->get_format()){
        case Surface::Format::A1:
            return PixelFormat::TRANSPARENT;//CAIRO_IMAGE_HAS_BILEVEL_ALPHA;
        case Surface::Format::A8:
            for(int y=0;y<bmp->get_height();y++){
                uint8_t*alpha=bmp->get_data()+bmp->get_stride()*y;
                for(int x=0;x<bmp->get_width();x++,alpha++)
                    if(*alpha > 0 && *alpha < 255)
                        return PixelFormat::TRANSLUCENT;//CAIRO_IMAGE_HAS_ALPHA;
            }
            return PixelFormat::TRANSPARENT;//CAIRO_IMAGE_HAS_BILEVEL_ALPHA;
        default:
            return PixelFormat::TRANSLUCENT;
        }
    }

    if((bmp->get_format()==Surface::Format::RGB16_565)||(bmp->get_format()==Surface::Format::RGB24))
        return PixelFormat::OPAQUE;

    if(bmp->get_format()!=Surface::Format::ARGB32)
        return PixelFormat::TRANSLUCENT;

    int transparentCount = 0, opaqueCount = 0;
    for(int y = 0;y < bmp->get_height() ;y++){
        uint8_t*pixels = (bmp->get_data() + bmp->get_stride()*y);
        for (int x = 0; x < bmp->get_width(); x++, pixels+=4){
            const uint8_t a = pixels[3];
            if(a==0) transparentCount++;
            else if(a!=255) return PixelFormat::TRANSLUCENT;//CAIRO_IMAGE_HAS_BILEVEL_ALPHA
            else opaqueCount++;//return PixelFormat::TRANSLUCENT;//CAIRO_IMAGE_HAS_ALPHA;

            if(transparentCount&&opaqueCount) return PixelFormat::TRANSLUCENT;
        }
    }
    return PixelFormat::OPAQUE;
}

#define TRANSPARENCY "TRANSPARENCY"

int ImageDecoder::getTransparency(Cairo::RefPtr<Cairo::ImageSurface>bmp){
    if(bmp){
        unsigned long len;
        const unsigned char*data= bmp->get_mime_data((const char*)TRANSPARENCY,len);
        const int transparency = int((unsigned long)data);
        return transparency?transparency:int(PixelFormat::OPAQUE);
    }
    return PixelFormat::TRANSPARENT;
}

void ImageDecoder::setTransparency(Cairo::RefPtr<Cairo::ImageSurface>bmp,int transparency){
    if(bmp)
        bmp->set_mime_data((const char*)TRANSPARENCY,(unsigned char*)(long(transparency)),0,nullptr);
}

#if USE(OPENJPEG)
static bool matchesJP2Signature(char* contents){
    return !memcmp(contents, "\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A", 12)
        || !memcmp(contents, "\x0D\x0A\x87\x0A", 4);
}

static bool matchesJ2KSignature(char* contents){
    return !memcmp(contents, "\xFF\x4F\xFF\x51", 4);
}
#endif

#if ENABLE(WEBP)
static bool matchesWebPSignature(char* contents){
    return !memcmp(contents, "RIFF", 4) && !memcmp(contents + 8, "WEBPVP", 6);
}
#endif

static bool matchesBMPSignature(char* contents){
    return !memcmp(contents, "BM", 2);
}

static bool matchesICOSignature(char* contents){
    return !memcmp(contents, "\x00\x00\x01\x00", 4);
}

static bool matchesCURSignature(char* contents){
    return !memcmp(contents, "\x00\x00\x02\x00", 4);
}

static bool isApng(std::istream*istm){
    char buf[64];
    //Chuk IHDR is the pngs's 1st Chunk
    //Chunk acTL is the 1st Chunk after IHDR
    istm->read(buf,48);
    const uint32_t frames = png_get_uint_32(buf+41);
    LOGV("chunk:%c%c%c%c isapng=%d frames:%d",buf[37],buf[38],buf[39],buf[40],!memcmp(buf+37,"acTL",4),frames);
    return (memcmp(buf+37,"acTL",4)==0) && (frames>1);
}

static int registerBuildinCodesc(){
    ImageDecoder::registerFactory(std::string("mime/png"),8,PNGDecoder::isPNG,
            [](std::istream&stream){return std::make_unique<PNGDecoder>(stream);});
#if ENABLE(GIF)
    ImageDecoder::registerFactory("mime/gif",6,GIFDecoder::isGIF,
            [](std::istream&stream){return std::make_unique<GIFDecoder>(stream);});
#endif

#if ENABLE(JPEG)
    ImageDecoder::registerFactory("mime/jpeg",12,JPEGDecoder::isJPEG,
            [](std::istream&stream){return std::make_unique<JPEGDecoder>(stream);});
#endif

#if USE(OPENJPEG)
    if (matchesJP2Signature(contents))
        return JPEG2000ImageDecoder::create(JPEG2000ImageDecoder::Format::JP2, alphaOption, gammaAndColorProfileOption);

    if (matchesJ2KSignature(contents))
        return JPEG2000ImageDecoder::create(JPEG2000ImageDecoder::Format::J2K, alphaOption, gammaAndColorProfileOption);
#endif

#if USE(ICO)
    if (matchesICOSignature(contents) || matchesCURSignature(contents))
        return ICOImageDecoder::create(alphaOption, gammaAndColorProfileOption);
#endif

#if USE(BITMAP)
    if (matchesBMPSignature(contents))
        return BMPImageDecoder::create(alphaOption, gammaAndColorProfileOption);
#endif
    return 0;
}

std::unique_ptr<ImageDecoder>ImageDecoder::getDecoder(std::istream&istm){
    constexpr unsigned lengthOfLongestSignature = 14; /* To wit: "RIFF????WEBPVP"*/
    uint8_t contents[lengthOfLongestSignature];
    std::unique_ptr<ImageDecoder>decoder;
    
    istm.read((char*)contents,lengthOfLongestSignature);
    const std::streamsize length = istm.gcount();
    if (length < lengthOfLongestSignature)
        return nullptr;
    istm.seekg(0,std::ios::beg);
    if(mFactories.empty()){
        registerBuildinCodesc();
    }
    for(auto fac:mFactories){
        auto f = fac.second;
        if(f.verifier(contents,lengthOfLongestSignature)){
            float scale = 1.f;
            decoder = f.factory(istm);
            return decoder;
        }
    }
    return nullptr;
}

Cairo::RefPtr<Cairo::ImageSurface> ImageDecoder::loadImage(std::istream&istm,int width,int height){
    float scale = 1.f;
    std::unique_ptr<ImageDecoder>decoder = getDecoder(istm);
    if(decoder == nullptr)
        return nullptr;
    if((width > 0) && (height > 0))
        scale = std::min(float(width)/decoder->getWidth(),float(height)/decoder->getHeight());
    else if(width > 0)
        scale = std::min(scale,float(width)/decoder->getWidth());
    else if(height > 0)
        scale = std::max(scale,float(height)/decoder->getHeight());
    return decoder->decode(scale);
}

Cairo::RefPtr<Cairo::ImageSurface>ImageDecoder::loadImage(Context*ctx,const std::string&resourceId,int width,int height){
    std::unique_ptr<ImageDecoder>decoder;
    std::unique_ptr<std::istream>istm = ctx ? ctx->getInputStream(resourceId) : std::make_unique<std::ifstream>(resourceId);
    if((istm == nullptr)||(!*istm))
        return nullptr;
    return loadImage(*istm,width,height);
}

Drawable*ImageDecoder::createAsDrawable(Context*ctx,const std::string&resourceId){
    std::unique_ptr<std::istream> istm = ctx ? ctx->getInputStream(resourceId) : std::make_unique<std::ifstream>(resourceId);
    std::unique_ptr<ImageDecoder> decoder = ((istm==nullptr)||(!*istm))?nullptr:getDecoder(*istm);
    Cairo::RefPtr<Cairo::ImageSurface> image = decoder?decoder->decode(1.0):nullptr;

    if(image && decoder && (decoder->getFrameCount()==1)){
        Drawable*d = nullptr;
        if(TextUtils::endWith(resourceId,".9.png"))
            d = new NinePatchDrawable(image);
        else if(TextUtils::endWith(resourceId,".png")||TextUtils::endWith(resourceId,".jpg"))
            d = new BitmapDrawable(image);
        if(d != nullptr) {
            d->getConstantState()->mResource=resourceId;
            return d;
        }
    }

    if( ((istm!=nullptr)&&(*istm)) && (TextUtils::endWith(resourceId,".gif")||TextUtils::endWith(resourceId,".webp")
            ||TextUtils::endWith(resourceId,".apng")||TextUtils::endWith(resourceId,".png"))){
	    Drawable* d = new AnimatedImageDrawable(ctx,resourceId);
        LOGD_IF(d==nullptr,"%s load failed!",resourceId.c_str());
        if(d != nullptr){
            d->getConstantState()->mResource=resourceId;
            return d;
        }
    }
    return nullptr;
}

}/*endof namespace*/
