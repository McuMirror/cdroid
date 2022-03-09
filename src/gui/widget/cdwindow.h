/*
 * Copyright (C) 2015 UI project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CDROID_WINDOW_H__
#define __CDROID_WINDOW_H__
#include <cdtypes.h>
#include <widget/viewgroup.h>
#include <widget/framelayout.h>

namespace cdroid {
class Window : public ViewGroup {
protected:
    friend class WindowManager;
    friend class GraphDevice;
    class InvalidateOnAnimationRunnable:public Runnable{
    private:
        bool mPosted;
        Window*mOwner;
        std::vector<View*>mViews;
        void postIfNeededLocked();
    public:
        InvalidateOnAnimationRunnable();
        void setOwner(Window*w);
        void addView(View* view);
        void removeView(View* view);
        void run();
    };
private:
    bool mInLayout;
    bool mHandingLayoutInLayoutRequest;
    Rect mRectOfFocusedView;
    void doLayout();
    bool performFocusNavigation(KeyEvent& event);
    static View*inflate(Context*ctx,std::istream&stream);
protected:
    std::vector<View*>mLayoutRequesters;
    RefPtr<Region>mWindowRgn;
    RefPtr<Region>mVisibleRgn;
    int window_type;/*window type*/
    int mLayer;/*surface layer*/
    std::string mText;
    InvalidateOnAnimationRunnable mInvalidateOnAnimationRunnable;
    class UIEventSource*source;
    void onFinishInflate()override;
    void onSizeChanged(int w,int h,int oldw,int oldh)override;
    void onVisibilityChanged(View& changedView,int visibility)override;
    ViewGroup*invalidateChildInParent(int* location,Rect& dirty)override;
    int processInputEvent(InputEvent&event);
    int processKeyEvent(KeyEvent&event);
    int processPointerEvent(MotionEvent&event);
    RefPtr<Canvas>getCanvas();
    ViewGroup::LayoutParams* generateDefaultLayoutParams()const override;
    bool checkLayoutParams(const ViewGroup::LayoutParams* p)const override;
    ViewGroup::LayoutParams* generateLayoutParams(const ViewGroup::LayoutParams* lp)const override;
public:
    typedef enum{
        TYPE_WALLPAPER    =1,
        TYPE_APPLICATION  =2,
        TYPE_SYSTEM_WINDOW=2000,
        TYPE_STATUS_BAR   =2001,
        TYPE_SEARCH_BAR   =2002,
        TYPE_SYSTEM_ALERT =2003,
        TYPE_KEYGUARD     =2004,
        TYPE_TOAST        =2005,
    }WindowType;
    Window(int x,int y,int w,int h,int type=TYPE_APPLICATION);
    Window(Context*,const AttributeSet&);
    void setRegion(const RefPtr<Region>&region);
    void draw();
    virtual ~Window();
    virtual void setText(const std::string&);
    const std::string getText()const;
    virtual View& setPos(int x,int y)override;
    virtual bool onKeyUp(int keyCode,KeyEvent& evt) override;
    virtual bool onKeyDown(int keyCode,KeyEvent& evt) override;
    virtual void onBackPressed();
    virtual void onActive();
    virtual void onDeactive();
    bool dispatchKeyEvent(KeyEvent&event)override;
    bool isInLayout()const override;
    bool postDelayed(Runnable& what,uint32_t delay)override;
    bool removeCallbacks(const Runnable& what)override;
    void dispatchInvalidateOnAnimation(View* view)override;
    bool dispatchTouchEvent(MotionEvent& event)override;
    void cancelInvalidate(View* view)override;
    ViewGroup::LayoutParams* generateLayoutParams(const AttributeSet&)const override;
    void close();
protected:
};

}  // namespace cdroid

#endif  // UI_LIBUI_WINDOW_H_
