#include <widgetEx/qrcodeview.h>
#if ENABLE(QRCODE) ||1
#include <qrencode.h>
#include <cdlog.h>

//REF:https://github.com/zint/zint-gpl-only/blob/master/backend_qt4/qzint.h/cpp

namespace cdroid{

DECLARE_WIDGET(QRCodeView)

QRCodeView::QRCodeView(int w,int h):View(w,h){
    initView();
};

QRCodeView::QRCodeView(Context*ctx,const AttributeSet&attrs):View(ctx,attrs){
    initView();
    mEccLevel = attrs.getInt("eccLevel",std::map<const std::string,int>{
            {"low",QR_ECLEVEL_L},
            {"medium",QR_ECLEVEL_M},
            {"quartor",QR_ECLEVEL_Q},
            {"high",QR_ECLEVEL_H}
    },mEccLevel);
}

QRCodeView::~QRCodeView(){
    
}

void QRCodeView::initView(){
    mZoom = 1.0;
    mQrCodeWidth =0;
    mEccLevel = QR_ECLEVEL_M;
    mMode = QR_MODE_8;
    mBarColor = 0xFFFFFFFF;
}

void QRCodeView::setText(const std::string&text){
    if(mText!=text){
        if(mText.empty())
            requestLayout();
        mText = text;
        encode();
        invalidate(true);
    }
    float w,h;
}

void QRCodeView::setBarcodeColor(int color){
    if(mBarColor!=color){
        mBarColor = color;
        invalidate();
    }
}

int QRCodeView::getBarcodeColor()const{
    return mBarColor;
}

void  QRCodeView::setZoom(float zoom){
    mZoom = zoom;
    requestLayout();
}

float  QRCodeView::getZoom()const{
    return mZoom;
}

/*void QRCodeView::onSizeChanged(int w,int h,int ow,int oh){
    mZoom
}*/

void  QRCodeView::onMeasure(int widthMeasureSpec, int heightMeasureSpec){
    const int widthMode  = MeasureSpec::getMode(widthMeasureSpec);
    const int heightMode = MeasureSpec::getMode(heightMeasureSpec);
    const int widthSize  = MeasureSpec::getSize(widthMeasureSpec);
    const int heightSize = MeasureSpec::getSize(heightMeasureSpec);
    int width = widthSize,height=heightSize;
    switch(widthMode){
    case MeasureSpec::EXACTLY:
        height= width; break;
    case MeasureSpec::AT_MOST:
        if(heightMode==MeasureSpec::EXACTLY){
            height= heightSize;
        }else
            width = mQrCodeWidth*mZoom;
        break;
    case MeasureSpec::UNSPECIFIED:
        break;
    }
    mZoom = std::min(width,height);
    mZoom /= mQrCodeWidth;
    LOGV("setMeasuredDimension(%d,%d)  %dx%d mZoom=%f",width,height,widthSize,heightSize,mZoom);
    setMeasuredDimension(width, height);
}

extern "C" int QRspec_getMinimumVersion(int size, QRecLevel level);

void QRCodeView::encode(){
    const int version = QRspec_getMinimumVersion(mText.size(), (QRecLevel)mEccLevel);
    QRcode*mQRcode = QRcode_encodeString(mText.c_str(), version, (QRecLevel)mEccLevel, (QRencodeMode)mMode, 1);
    if(mQRcode){
        const uint8_t*qrd = mQRcode->data;
        mQrCodeWidth = mQRcode->width;
        mQRImage = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,mQrCodeWidth,mQrCodeWidth);
        const uint32_t image_stride = mQRImage->get_stride()/4;
        uint32_t*qimg = (uint32_t*)mQRImage->get_data();
        for(int32_t y = 0,idx = 0; y < mQRcode->width; y++){
            for(int32_t x = 0; x < mQRcode->width; x++){
                qimg[x]=(qrd[idx+x]&1)?mBarColor:0;
            }
            idx += mQRcode->width;
            qimg += image_stride;
        }
        const float wx = getWidth() - getPaddingLeft()- getPaddingRight();
        const float wy = getHeight()- getPaddingTop() - getPaddingBottom();
        mZoom = std::min(wx,wy)/mQRcode->width;
        mQRImage->mark_dirty();
    }
    QRcode_free(mQRcode);
}

void  QRCodeView::onDraw(Canvas&canvas){
    View::onDraw(canvas);
    const struct zint_vector_rect *rect;
    const struct zint_vector_hexagon *hex;
    const struct zint_vector_circle *circle;
    struct zint_vector_string *string;
    const RectF paintRect ={0,0,(float)getWidth(),(float)getHeight()};

    canvas.save();

    canvas.translate(getPaddingLeft(), getPaddingTop());
    canvas.scale(mZoom,mZoom);
    canvas.set_source(mQRImage,0,0);
    Cairo::RefPtr<Cairo::SurfacePattern>spat = canvas.get_source_for_surface();
    spat->set_filter(Cairo::SurfacePattern::Filter::NEAREST);
    canvas.rectangle(0,0,mQrCodeWidth,mQrCodeWidth);
    canvas.clip();
    canvas.paint();
    canvas.restore();
}

}/*endof namespace*/

#endif/*ENABLE(BARCODE)*/

