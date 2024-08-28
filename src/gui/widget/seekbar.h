#ifndef __SEEK_BAR_H__
#define __SEEK_BAR_H__
#include <widget/absseekbar.h>

namespace cdroid{
class SeekBar:public AbsSeekBar{
public:
    struct OnSeekBarChangeListener{
        CallbackBase<void,SeekBar&,int,bool> onProgressChanged;//(SeekBar seekBar, int progress, boolean fromUser);
        CallbackBase<void,SeekBar&>onStartTrackingTouch;
        CallbackBase<void,SeekBar&>onStopTrackingTouch;
    };
    //DECLARE_UIEVENT(void,OnSeekBarChangeListener,AbsSeekBar&seek,int progress,bool fromuser);
protected:
    OnSeekBarChangeListener  mOnSeekBarChangeListener;
    void onProgressRefresh(float scale, bool fromUser, int progress)override;
public:
    SeekBar(int w,int h);
    SeekBar(Context*ctx,const AttributeSet& attrs);
    void onStartTrackingTouch()override;
    void onStopTrackingTouch()override;
    void setOnSeekBarChangeListener(const OnSeekBarChangeListener& l);
};

}//namespace
#endif 
