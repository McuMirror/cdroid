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

#ifndef __NGLUI_GROUPVIEW_H__
#define __NGLUI_GROUPVIEW_H__

#include <widget/view.h>
#include <scroller.h>

namespace cdroid {

#define ATTR_ANIMATE_FOCUS (0x2000) /*flag to open animate focus*/

class ViewGroup : public View {
public:
    typedef cdroid::LayoutParams LayoutParams;
    typedef cdroid::MarginLayoutParams MarginLayoutParams;
    DECLARE_UIEVENT(void,OnAnimationFinished);
    DECLARE_UIEVENT(void,OnHierarchyChangeListener,ViewGroup&,View*,bool addremove);
    enum{
        FLAG_CLIP_CHILDREN   = 0x01,
        FLAG_CLIP_TO_PADDING = 0x02,
        FLAG_PADDING_NOT_NULL= 0x20,
        CLIP_TO_PADDING_MASK = FLAG_CLIP_TO_PADDING | FLAG_PADDING_NOT_NULL,
        FLAG_ADD_STATES_FROM_CHILDREN  = 0x2000,
        FLAG_ALWAYS_DRAWN_WITH_CACHE   = 0x4000,
        FLAG_CHILDREN_DRAWN_WITH_CACHE = 0x8000,
        FLAG_NOTIFY_CHILDREN_ON_DRAWABLE_STATE_CHANGE = 0x10000,

        FLAG_TOUCHSCREEN_BLOCKS_FOCUS = 0x4000000,
        FOCUS_BEFORE_DESCENDANTS= 0x20000,
        FOCUS_AFTER_DESCENDANTS = 0x40000,
        FOCUS_BLOCK_DESCENDANTS = 0x60000,
        FLAG_MASK_FOCUSABILITY  = 0x60000,
        FLAG_DISALLOW_INTERCEPT = 0x80000,
        FLAG_SPLIT_MOTION_EVENTS= 0x200000,
    };
    enum{
        LAYOUT_MODE_UNDEFINED  =-1,
        LAYOUT_MODE_CLIP_BOUNDS=0,
        LAYOUT_MODE_OPTICAL_BOUNDS=1,
        LAYOUT_MODE_DEFAULT = LAYOUT_MODE_CLIP_BOUNDS
    };
private:
    friend class View;
    int mLayoutMode;
    int mNestedScrollAxes;
    int mLastTouchDownX,mLastTouchDownY;
    int mLastTouchDownIndex=-1;
    long mLastTouchDownTime;
    View* mFocused;
    View* mDefaultFocus;
    View* mFocusedInCluster;
    int mChildCountWithTransientState;
    class TouchTarget* mFirstTouchTarget;
    RECT focusRectSrc;
    RECT focusRectDest;
    RECT focusRect;
    POINT animateTo;//save window boundray  while animating
    POINT animateFrom;//window animate from boundary
    void initGroup();
    void setBooleanFlag(int flag, bool value);
    bool hasBooleanFlag(int flag)const;
    TouchTarget* getTouchTarget(View* child);
    TouchTarget* addTouchTarget(View* child, int pointerIdBits);
    void resetTouchState();
    static bool resetCancelNextUpFlag(View* view);
    void clearTouchTargets();
    static bool canViewReceivePointerEvents(View& child);
    void cancelAndClearTouchTargets(MotionEvent*);
    void removePointersFromTouchTargets(int pointerIdBits);
    void cancelTouchTarget(View* view);
    void cancelHoverTarget(View*view);
    bool dispatchTransformedTouchEvent(MotionEvent& event, bool cancel,
            View* child, int desiredPointerIdBits);
    bool dispatchTransformedGenericPointerEvent(MotionEvent& event, View* child);

    void setTouchscreenBlocksFocusNoRefocus(bool touchscreenBlocksFocus);
    void moveFocusTo(const RECT&r);
    void invalidateChildrenInFocusRect();
    void addInArray(View* child, int index);
    bool removeViewInternal(View* view);
    void removeViewInternal(int index, View* view);
    void removeViewsInternal(int start, int count);
    void removeFromArray(int index);
    void removeFromArray(int start, int count);
    View&addViewInner(View* child, int index,LayoutParams* params,bool preventRequestLayout);
protected:
    int mGroupFlags;
    std::vector<View*> mChildren;
    RefPtr<Region>mInvalidRgn;
    LONGLONG time_lastframe;
    OnHierarchyChangeListener mOnHierarchyChangeListener;
    void setDefaultFocus(View* child);
    void clearDefaultFocus(View* child);
    bool hasFocusable(bool allowAutoFocus, bool dispatchExplicit)const override;
    bool hasFocusableChild(bool dispatchExplicit)const;
    bool dispatchGenericFocusedEvent(MotionEvent&event)override;
    virtual bool onRequestFocusInDescendants(int direction,const RECT* previouslyFocusedRect);
    virtual bool requestChildRectangleOnScreen(View* child,RECT& rectangle, bool immediate);
    bool performKeyboardGroupNavigation(int direction);

    bool addViewInLayout(View* child, int index,LayoutParams* params);
    bool addViewInLayout(View* child, int index,LayoutParams* params,bool preventRequestLayout);
    void attachViewToParent(View* child, int index, LayoutParams* params);
    void dispatchViewAdded(View* child);
    void dispatchViewRemoved(View* child);
    void removeDetachedView(View* child, bool animate);
    void detachViewsFromParent(int start, int count);
    void detachViewFromParent(View* child);
    void detachAllViewsFromParent();
    void clearFocusedInCluster(View* child);
    void clearFocusedInCluster();
    virtual LayoutParams* generateLayoutParams(const LayoutParams* p)const;
    virtual LayoutParams* generateDefaultLayoutParams()const;
    virtual bool checkLayoutParams(const LayoutParams* p)const;

    virtual void onSetLayoutParams(View* child,LayoutParams* layoutParams);

    void measureChildren(int widthMeasureSpec, int heightMeasureSpec);
    void measureChild(View* child, int parentWidthMeasureSpec,int parentHeightMeasureSpec);

    virtual void measureChildWithMargins(View* child,int parentWidthMeasureSpec, int widthUsed,
            int parentHeightMeasureSpec, int heightUsed);
    void dispatchDraw(Canvas&)override;
    bool hasActiveAnimations();
    void transformPointToViewLocal(POINT&point,View&);
    bool isTransformedTouchPointInView(int x,int y,View& child,Point*outLocalPoint);

    void drawableStateChanged()override;
    std::vector<int> onCreateDrawableState()const override;
    virtual void onDraw(Canvas& canvas) override;
    void dispatchSetPressed(bool pressed)override;
    virtual int getChildDrawingOrder(int childCount, int i);
public:
    ViewGroup(Context*ctx,const AttributeSet& attrs);
    ViewGroup(int w,int h);
    ViewGroup(int x,int y,int w,int h);
    virtual ~ViewGroup();
    bool getTouchscreenBlocksFocus()const;
    bool shouldBlockFocusForTouchscreen()const;
    int getDescendantFocusability()const;
    void setDescendantFocusability(int);

    bool getClipChildren()const;
    void setClipChildren(bool clipChildren);
    bool getClipToPadding()const;
    void setClipToPadding(bool clipToPadding);

    void dispatchSetSelected(bool selected)override;
    void dispatchSetActivated(bool activated)override;

    View*focusSearch(View*focused,int direction)const;
    View*getFocusedChild(){return mFocused;}
    View*findFocus()override;
    bool restoreDefaultFocus()override;
    virtual void requestChildFocus(View*child,View*focused);
    void unFocus(View* focused)override;
    void clearChildFocus(View* child);
    void requestDisallowInterceptTouchEvent(bool disallowIntercept);
    bool hasTransientState()override;
    void childHasTransientStateChanged(View* child, bool childHasTransientState);

    void offsetDescendantRectToMyCoords(View* descendant,RECT& rect)const;
    void offsetRectIntoDescendantCoords(View* descendant, RECT& rect)const;
    void offsetRectBetweenParentAndChild(View* descendant,RECT& rect,bool offsetFromChildToParent, bool clipToBounds)const;
    void offsetChildrenTopAndBottom(int offset);

    void addFocusables(std::vector<View*>& views, int direction, int focusableMode)const override;
    void addKeyboardNavigationClusters(std::vector<View*>&views,int drection)const override;
    void setOnHierarchyChangeListener(OnHierarchyChangeListener listener);
    bool restoreFocusNotInCluster();
    View*keyboardNavigationClusterSearch(View* currentCluster,int direction)override;
    bool requestFocus(int direction=FOCUS_DOWN,const RECT*previouslyFocusedRect=nullptr)override;

    int getChildCount()const;
    View*getChildAt(int idx)const;
    int indexOfChild(View* child)const;
    bool isLayoutModeOptical()const;
    void cleanupLayoutState(View* child)const;

    virtual View&addView(View* view);
    virtual View& addView(View* child, int index);
    virtual View& addView(View* child, LayoutParams* params);
    virtual View& addView(View* child, int index, LayoutParams* params);
  
    virtual void onViewAdded(View* child);
    virtual void onViewRemoved(View* child);
    void invalidateChild(View*child,const RECT*);

    virtual LayoutParams* generateLayoutParams(const AttributeSet& attrs)const;
    static int getChildMeasureSpec(int spec, int padding, int childDimension);
    virtual void removeView(View* view);/*only remove view from children,no deleteion*/
    virtual void removeViewAt(int idx);
    virtual void removeAllViews();
    void removeViews(int start, int count);
    void removeViewInLayout(View* view);
    void removeViewInLayout(int start,int count);
    void removeAllViewsInLayout();
    virtual View* findViewById(int id)const;
    virtual bool shouldDelayChildPressedState()const;

    virtual void onSizeChanged(int w,int h,int ow,int oh) override;
    
    virtual bool onStartNestedScroll(View* child, View* target, int nestedScrollAxes);
    virtual void onNestedScrollAccepted(View* child, View* target, int axes);
    virtual void onStopNestedScroll(View* child);
    virtual void onNestedScroll(View* target, int dxConsumed, int dyConsumed,int dxUnconsumed, int dyUnconsumed);
    virtual void onNestedPreScroll(View* target, int dx, int dy, int*consumed);
    bool onNestedFling(View* target, float velocityX, float velocityY, bool consumed);
    int getNestedScrollAxes()const;
    bool onNestedPreFling(View* target, float velocityX, float velocityY);

    bool dispatchKeyEvent(KeyEvent&)override;
    bool dispatchUnhandledMove(View* focused, int direction)override;
    bool dispatchTouchEvent(MotionEvent& event)override;
    bool onInterceptTouchEvent(MotionEvent& evt);
    virtual void onDrawFocusRect(Canvas&,const RECT&);

    void jumpDrawablesToCurrentState()override;
    void setAddStatesFromChildren(bool addsStates);
    bool addStatesFromChildren();
    virtual void childDrawableStateChanged(View* child);
};

}  // namespace ui

#endif  // __NGLUI_GROUPVIEW_H__
