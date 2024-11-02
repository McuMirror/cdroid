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

#include <view/viewgroup.h>
#include <view/viewoverlay.h>
#include <animation/layouttransition.h>
#include <animation/layoutanimationcontroller.h>
#include <cdlog.h>
#include <focusfinder.h>
#include <systemclock.h>

#define CHILD_LEFT_INDEX 0
#define CHILD_TOP_INDEX  1

using namespace Cairo;
namespace cdroid {

DECLARE_WIDGET(ViewGroup)

class TouchTarget{
private:
    static TouchTarget* sRecycleBin;
    static int sRecycledCount;
public:
    enum{
        ALL_POINTER_IDS= -1,
        MAX_RECYCLED   = 32
    };
    View*child;
    int pointerIdBits;
    TouchTarget*next;
public:
    TouchTarget(){
        child= nullptr;
        next = nullptr;
        pointerIdBits = 0;
    }
    static TouchTarget*obtain(View*child,int pointerIdBits){
        TouchTarget*target;
        if(sRecycleBin==nullptr)
             target=new TouchTarget();
        else{
            target=sRecycleBin;
            sRecycleBin=target->next;
            sRecycledCount--;
            target->next=nullptr;
        }
        target->child = child;
        target->pointerIdBits = pointerIdBits;
        return target;
    }
    void recycle(){
        if (sRecycledCount < MAX_RECYCLED) {
            next = sRecycleBin;
            sRecycleBin = this;
            sRecycledCount += 1;
        } else {
            next = nullptr;
        }
        child = nullptr;
    }
};

class HoverTarget {
private:
    static constexpr int MAX_RECYCLED = 32;
    static HoverTarget* sRecycleBin;
    static int sRecycledCount;
    HoverTarget() {
    }
public:
    // The hovered child view.
    View* child;
    // The next target in the target list.
    HoverTarget* next;

    static HoverTarget* obtain(View* child) {
        if (child == nullptr) {
            throw "child must be non-null";
        }

        HoverTarget* target;
        if (sRecycleBin == nullptr) {
            target = new HoverTarget();
        } else {
            target = sRecycleBin;
            sRecycleBin = target->next;
            sRecycledCount--;
            target->next = nullptr;
        }
        target->child = child;
        return target;
    }

    void recycle() {
        if (child == nullptr) {
            throw "already recycled once";
        }
        if (sRecycledCount < MAX_RECYCLED) {
            next = sRecycleBin;
            sRecycleBin = this;
            sRecycledCount += 1;
        } else {
            next = nullptr;
        }
        child = nullptr;
    }
};
TouchTarget*TouchTarget::sRecycleBin = nullptr;
HoverTarget*HoverTarget::sRecycleBin = nullptr;
int TouchTarget::sRecycledCount = 0;
int HoverTarget::sRecycledCount = 0;

ViewGroup::ViewGroup(Context*ctx,const AttributeSet& attrs):View(ctx,attrs){
    initGroup();
    initFromAttributes(ctx,attrs);
}

ViewGroup::ViewGroup(int w,int h)
  : ViewGroup(0,0,w,h) {
}

ViewGroup::ViewGroup(int x,int y,int w,int h)
:View(w,h){
    mLeft = x;
    mTop  = y;
    initGroup();
}

void ViewGroup::initGroup(){
    // ViewGroup doesn't draw by default
    if (!isShowingLayoutBounds()) {
        setFlags(WILL_NOT_DRAW, DRAW_MASK);
    }
    mGroupFlags = FLAG_CLIP_CHILDREN;
    mGroupFlags|= FLAG_CLIP_TO_PADDING;
    mGroupFlags|= FLAG_ANIMATION_DONE;
    mGroupFlags|= FLAG_ANIMATION_CACHE;
    mGroupFlags|= FOCUS_BEFORE_DESCENDANTS;
    //mGroupFlags!= FLAG_ALWAYS_DRAWN_WITH_CACHE;
    mLayoutMode = LAYOUT_MODE_UNDEFINED;
    mFocused    = nullptr;
    mDefaultFocus = nullptr;
    mFocusedInCluster = nullptr;
    mFirstTouchTarget = nullptr;
    mFirstHoverTarget = nullptr;
    mTooltipHoverTarget = nullptr;
    mLayoutAnimationController = nullptr;
    mHoveredSelf = false;
    mPointerCapture  = false;
    mIsInterestedInDrag = false;
    mTooltipHoveredSelf = false;
    mChildCountWithTransientState= 0;
    mChildUnhandledKeyListeners  = 0;
    mInvalidRgn=Region::create();
    mChildTransformation =nullptr;
    mInvalidationTransformation =nullptr;
    mTransition = nullptr;
    mPersistentDrawingCache = PERSISTENT_SCROLLING_CACHE;
    setDescendantFocusability(FOCUS_BEFORE_DESCENDANTS);
    mLayoutTransitionListener.startTransition=[this](LayoutTransition&transition,ViewGroup*container,View*view,int transitionType){
         // We only care about disappearing items, since we need special logic to keep
         // those items visible after they've been 'removed'
         if(transitionType==LayoutTransition::DISAPPEARING)
             startViewTransition(view);
    };
    mLayoutTransitionListener.endTransition=[this](LayoutTransition&transition,ViewGroup*container,View*view,int transitionType){
         if(mLayoutCalledWhileSuppressed && !transition.isChangingLayout()){
              requestLayout();
              mLayoutCalledWhileSuppressed=false;
         }
         if(transitionType==LayoutTransition::DISAPPEARING && mTransitioningViews.size())
              endViewTransition(view);
    };
}

void ViewGroup::initFromAttributes(Context*ctx,const AttributeSet&atts){
    setClipChildren(atts.getBoolean("clipChildren",true));
    setClipToPadding(atts.getBoolean("clipToPadding",true));
    //setAnimationCacheEnabled
    std::string resid = atts.getString("layoutAnimation");
    setLayoutAnimation(AnimationUtils::loadLayoutAnimation(ctx,resid));

    const int flags=atts.getInt("descendantFocusability",std::map<const std::string,int>{
        {"beforeDescendants",FOCUS_BEFORE_DESCENDANTS},
        {"afterDescendants",FOCUS_AFTER_DESCENDANTS},
        {"blocksDescendants",FOCUS_BLOCK_DESCENDANTS}
    },FOCUS_BEFORE_DESCENDANTS);
    setDescendantFocusability(flags);

    if(atts.getBoolean("animateLayoutChanges",false))
        setLayoutTransition(new LayoutTransition());
    const int layoutMode = atts.getInt("layoutMode",std::map<const std::string,int>{
        {"undefined",LAYOUT_MODE_UNDEFINED},
        {"clipBounds",LAYOUT_MODE_CLIP_BOUNDS},
        {"opticalBounds",LAYOUT_MODE_OPTICAL_BOUNDS}
    },LAYOUT_MODE_UNDEFINED);
    setAddStatesFromChildren(atts.getBoolean("addStatesFromChildren",false));
    setMotionEventSplittingEnabled(atts.getBoolean("splitMotionEvents",false));
    setAlwaysDrawnWithCacheEnabled(atts.getBoolean("alwaysDrawnWithCache",false));
    setLayoutMode(layoutMode);
    setTransitionGroup(atts.getBoolean("transitionGroup",false));
    setTouchscreenBlocksFocus(atts.getBoolean("touchscreenBlocksFocus",false));
}

ViewGroup::~ViewGroup() {
    while(mChildren.size()){
        View*v = mChildren[0];
        removeViewAt(0);
        delete v;
    }
    mChildren.clear();
    delete mChildTransformation;
    delete mInvalidationTransformation;
    delete mLayoutAnimationController;
    delete mTransition;
}

bool ViewGroup::canResolveLayoutDirection()const{
    return true;
}

bool ViewGroup::isLayoutDirectionResolved()const{
    return true;
}

int ViewGroup::getLayoutDirection()const{
    return View::LAYOUT_DIRECTION_RESOLVED_DEFAULT;
}

bool ViewGroup::canResolveTextDirection()const{
    return true;
}

bool ViewGroup::isTextDirectionResolved()const{
    return true;
}

int ViewGroup::getTextDirection()const{
    return View::TEXT_DIRECTION_RESOLVED_DEFAULT;
}

bool ViewGroup::canResolveTextAlignment()const{
    return true;
}

bool ViewGroup::isTextAlignmentResolved()const{
    return true;
}

int ViewGroup::getTextAlignment()const{
    return View::TEXT_ALIGNMENT_RESOLVED_DEFAULT;
}

bool ViewGroup::ensureTouchMode(bool){
    return false;
}

void ViewGroup::cancelAndClearTouchTargets(MotionEvent* event){
    if (mFirstTouchTarget==nullptr)return;
 
    long now = SystemClock::uptimeMillis();
    bool syntheticEvent = (event==nullptr);
    if(event==nullptr){
        event = MotionEvent::obtain(now, now,MotionEvent::ACTION_CANCEL, 0.0f, 0.0f, 0);
        event->setAction(MotionEvent::ACTION_CANCEL);
        event->setSource(InputDevice::SOURCE_TOUCHSCREEN);
    }
    for (TouchTarget* target = mFirstTouchTarget; target ; target = target->next) {
        resetCancelNextUpFlag(target->child);
        dispatchTransformedTouchEvent(*event, true, target->child, target->pointerIdBits);
    }
    if(syntheticEvent)
        event->recycle();
    clearTouchTargets();
}

bool ViewGroup::dispatchTransformedTouchEvent(MotionEvent& event, bool cancel,
       View* child, int desiredPointerIdBits){
    bool handled;

    // Canceling motions is a special case.  We don't need to perform any transformations
    // or filtering.  The important part is the action, not the contents.
    const int oldAction = event.getAction();
    if (cancel || (oldAction == MotionEvent::ACTION_CANCEL) ) {
        event.setAction(MotionEvent::ACTION_CANCEL);
        if (child == nullptr) {
            handled = View::dispatchTouchEvent(event);
        } else {
            handled = child->dispatchTouchEvent(event);
        }
        event.setAction(oldAction);
        return handled;
    }

    // Calculate the number of pointers to deliver.
    const int oldPointerIdBits = event.getPointerIdBits();
    const int newPointerIdBits = oldPointerIdBits & desiredPointerIdBits;

    // If for some reason we ended up in an inconsistent state where it looks like we
    // might produce a motion event with no pointers in it, then drop the event.
    if (newPointerIdBits == 0) {
        return false;
    }

    // If the number of pointers is the same and we don't need to perform any fancy
    // irreversible transformations, then we can reuse the motion event for this
    // dispatch as long as we are careful to revert any changes we make.
    // Otherwise we need to make a copy.
    MotionEvent* transformedEvent;
    if (newPointerIdBits == oldPointerIdBits) {
        if ((child == nullptr) || child->hasIdentityMatrix()) {
            if (child == nullptr) {
                handled = View::dispatchTouchEvent(event);
            } else {
                const float offsetX = mScrollX - child->mLeft;
                const float offsetY = mScrollY - child->mTop;
                event.offsetLocation(offsetX, offsetY);
                handled = child->dispatchTouchEvent(event);
                event.offsetLocation(-offsetX, -offsetY);
            }
            return handled;
        }
        transformedEvent = MotionEvent::obtain(event);
    } else {
        transformedEvent = event.split(newPointerIdBits);
    }

    // Perform any necessary transformations and dispatch.
    if (child == nullptr) {
        handled = View::dispatchTouchEvent(*transformedEvent);
    } else {
        float offsetX = mScrollX - child->mLeft;
        float offsetY = mScrollY - child->mTop;
        transformedEvent->offsetLocation(offsetX, offsetY);
        if (! child->hasIdentityMatrix()) {
            LOGV_IF(event.getAction()==MotionEvent::ACTION_DOWN,"xy=(%f,%f) , (%f,%f)",event.getX(),event.getY(),transformedEvent->getX(),transformedEvent->getY());
            transformedEvent->transform(child->getInverseMatrix());
            LOGV_IF(event.getAction()==MotionEvent::ACTION_DOWN,"XY=(%f,%f)",transformedEvent->getX(),transformedEvent->getY());
        }
        handled = child->dispatchTouchEvent(*transformedEvent);
    }
    // Done. 
    transformedEvent->recycle();
    return handled;
}

TouchTarget* ViewGroup::getTouchTarget(View* child) {
    for (TouchTarget* target = mFirstTouchTarget; target ; target = target->next) {
        if (target->child == child) {
            return target;
        }
    }
    return nullptr;
}

TouchTarget* ViewGroup::addTouchTarget(View* child, int pointerIdBits) {
    TouchTarget* target = TouchTarget::obtain(child, pointerIdBits);
    target->next = mFirstTouchTarget;
    mFirstTouchTarget = target;
    return target;
}

void ViewGroup::resetTouchState() {
    clearTouchTargets();
    resetCancelNextUpFlag(this);
    mGroupFlags &= ~FLAG_DISALLOW_INTERCEPT;
    mNestedScrollAxes = SCROLL_AXIS_NONE;
}

bool ViewGroup::resetCancelNextUpFlag(View* view){
    if ((view->mPrivateFlags & PFLAG_CANCEL_NEXT_UP_EVENT) != 0) {
        view->mPrivateFlags &= ~PFLAG_CANCEL_NEXT_UP_EVENT;
        return true;
    }
    return false;
}

void ViewGroup::clearTouchTargets(){
    TouchTarget* target = mFirstTouchTarget;
    if (target != nullptr) {
        do {
            TouchTarget* next = target->next;
            target->recycle();
            target = next;
        } while (target != nullptr);
        mFirstTouchTarget = nullptr;
    }
}

void ViewGroup::removePointersFromTouchTargets(int pointerIdBits) {
    TouchTarget* predecessor = nullptr;
    TouchTarget* target = mFirstTouchTarget;
    while (target != nullptr) {
        TouchTarget* next = target->next;
        if ((target->pointerIdBits & pointerIdBits) != 0) {
            target->pointerIdBits &= ~pointerIdBits;
            if (target->pointerIdBits == 0) {
                if (predecessor == nullptr) mFirstTouchTarget = next;
                else    predecessor->next = next;
                target->recycle();
                target = next;
                continue;
            }
        }
        predecessor = target;
        target = next;
    }
}

void ViewGroup::cancelTouchTarget(View* view){
    TouchTarget* predecessor = nullptr;
    TouchTarget* target = mFirstTouchTarget;
    while (target != nullptr) {
        TouchTarget* next = target->next;
        if (target->child == view) {
            if (predecessor == nullptr) {
                mFirstTouchTarget = next;
            } else {
                predecessor->next = next;
            }
            target->recycle();
            long now = SystemClock::uptimeMillis();
            MotionEvent* event=MotionEvent::obtain(now, now, MotionEvent::ACTION_CANCEL, 0.0f, 0.0f, 0);
            event->setSource(InputDevice::SOURCE_TOUCHSCREEN);
            view->dispatchTouchEvent(*event);
            event->recycle();
            return;
        }
        predecessor = target;
        target = next;
    }
}

void ViewGroup::exitTooltipHoverTargets() {
    if (mTooltipHoveredSelf || mTooltipHoverTarget) {
        const long now = SystemClock::uptimeMillis();
        MotionEvent* event = MotionEvent::obtain(now, now,
                MotionEvent::ACTION_HOVER_EXIT, 0.0f, 0.0f, 0);
        event->setSource(InputDevice::SOURCE_TOUCHSCREEN);
        dispatchTooltipHoverEvent(*event);
        event->recycle();
    }
}

bool ViewGroup::dispatchTooltipHoverEvent(MotionEvent& event) {
    const int action = event.getAction();
    View* newTarget = nullptr;
    const int childrenCount = mChildren.size();
    switch (action) {
    case MotionEvent::ACTION_HOVER_ENTER:
        break;

    case MotionEvent::ACTION_HOVER_MOVE:

        // Check what the child under the pointer says about the tooltip.
        if (childrenCount != 0) {
            const float x = event.getXDispatchLocation(0);
            const float y = event.getYDispatchLocation(0);

            std::vector<View*> preorderedList = buildOrderedChildList();
            const bool customOrder = preorderedList.size()==0 && isChildrenDrawingOrderEnabled();
            auto children = mChildren;
            for (int i = childrenCount - 1; i >= 0; i--) {
                const int childIndex = getAndVerifyPreorderedIndex(childrenCount, i, customOrder);
                View* child = getAndVerifyPreorderedView(preorderedList, children, childIndex);
                if (!child->canReceivePointerEvents()
                        || !isTransformedTouchPointInView(x, y, *child, nullptr)) {
                    continue;
                }
                if (dispatchTooltipHoverEvent(event, child)) {
                    newTarget = child;
                    break;
                }
            }
            preorderedList.clear();
        }

        if (mTooltipHoverTarget != newTarget) {
            if (mTooltipHoverTarget != nullptr) {
                event.setAction(MotionEvent::ACTION_HOVER_EXIT);
                mTooltipHoverTarget->dispatchTooltipHoverEvent(event);
                event.setAction(action);
            }
            mTooltipHoverTarget = newTarget;
        }

        if (mTooltipHoverTarget != nullptr) {
            if (mTooltipHoveredSelf) {
                mTooltipHoveredSelf = false;
                event.setAction(MotionEvent::ACTION_HOVER_EXIT);
		View::dispatchTooltipHoverEvent(event);
                event.setAction(action);
             }
             return true;
         }

         mTooltipHoveredSelf = View::dispatchTooltipHoverEvent(event);
         return mTooltipHoveredSelf;

    case MotionEvent::ACTION_HOVER_EXIT:
         if (mTooltipHoverTarget != nullptr) {
             mTooltipHoverTarget->dispatchTooltipHoverEvent(event);
             mTooltipHoverTarget = nullptr;
         } else if (mTooltipHoveredSelf) {
             View::dispatchTooltipHoverEvent(event);
             mTooltipHoveredSelf = false;
         }
         break;
    }
    return false;
}

bool ViewGroup::dispatchTooltipHoverEvent(MotionEvent& event, View* child) {
    bool result;
    if (!child->hasIdentityMatrix()) {
        MotionEvent* transformedEvent = getTransformedMotionEvent(event, child);
        result = child->dispatchTooltipHoverEvent(*transformedEvent);
        transformedEvent->recycle();
    } else {
        const float offsetX = mScrollX - child->mLeft;
        const float offsetY = mScrollY - child->mTop;
        event.offsetLocation(offsetX, offsetY);
        result = child->dispatchTooltipHoverEvent(event);
        event.offsetLocation(-offsetX, -offsetY);
    }
    return result;
}

bool ViewGroup::hasHoveredChild() const{
    return mFirstHoverTarget != nullptr;
}

bool ViewGroup::pointInHoveredChild(MotionEvent& event) {
    if (mFirstHoverTarget) {
        return isTransformedTouchPointInView(event.getXDispatchLocation(0),
                event.getYDispatchLocation(0),*mFirstHoverTarget->child, nullptr);
    }
    return false;
}

void ViewGroup::exitHoverTargets(){
    if (mHoveredSelf || mFirstHoverTarget) {
        const long now = SystemClock::uptimeMillis();
        MotionEvent* event = MotionEvent::obtain(now, now,
                MotionEvent::ACTION_HOVER_EXIT, 0.0f, 0.0f, 0);
        event->setSource(InputDevice::SOURCE_TOUCHSCREEN);
        dispatchHoverEvent(*event);
        event->recycle();
    }
}

void ViewGroup::cancelHoverTarget(View*view){
    HoverTarget* predecessor = nullptr;
    HoverTarget* target = mFirstHoverTarget;
    while (target != nullptr) {
        HoverTarget* next = target->next;
        if (target->child == view) {
            if (predecessor == nullptr) {
                mFirstHoverTarget = next;
            } else {
                predecessor->next = next;
            }
            target->recycle();

            const long now = SystemClock::uptimeMillis();
            MotionEvent* event = MotionEvent::obtain(now, now,
                    MotionEvent::ACTION_HOVER_EXIT, 0.0f, 0.0f, 0);
            event->setSource(InputDevice::SOURCE_TOUCHSCREEN);
            view->dispatchHoverEvent(*event);
            event->recycle();
            return;
        }
        predecessor = target;
        target = next;
    }
}

bool ViewGroup::canViewReceivePointerEvents(View& child) {
    return (child.mViewFlags & VISIBILITY_MASK) == VISIBLE || child.getAnimation();
}

void ViewGroup::setOnHierarchyChangeListener(const OnHierarchyChangeListener& listener){
    mOnHierarchyChangeListener =listener;
}

void ViewGroup::onViewAdded(View*v){
}

void ViewGroup::onViewRemoved(View*v){
}

bool ViewGroup::hasTransientState(){
    return mChildCountWithTransientState > 0 || View::hasTransientState();
}

void ViewGroup::dispatchAttachedToWindow(AttachInfo* info, int visibility){
     mGroupFlags |= FLAG_PREVENT_DISPATCH_ATTACHED_TO_WINDOW;
     View::dispatchAttachedToWindow(info, visibility);
     mGroupFlags &= ~FLAG_PREVENT_DISPATCH_ATTACHED_TO_WINDOW;

     for (View*child:mChildren){
         child->dispatchAttachedToWindow(info, combineVisibility(visibility, child->getVisibility()));
     }
     for (View*view:mTransientViews){
         view->dispatchAttachedToWindow(info, combineVisibility(visibility, view->getVisibility()));
     }
}

bool ViewGroup::dispatchGenericPointerEvent(MotionEvent& event) {
    // Send the event to the child under the pointer.
    const int childrenCount = mChildren.size();
    if (childrenCount != 0) {
        const float x = event.getXDispatchLocation(0);
        const float y = event.getYDispatchLocation(0);
        std::vector<View*> preorderedList = buildOrderedChildList();
        bool customOrder = preorderedList.empty()  && isChildrenDrawingOrderEnabled();
        for (int i = childrenCount - 1; i >= 0; i--) {
            int childIndex = getAndVerifyPreorderedIndex(childrenCount, i, customOrder);
            View* child = getAndVerifyPreorderedView(preorderedList, mChildren, childIndex);
            if (!canViewReceivePointerEvents(*child)
                    || !isTransformedTouchPointInView(x, y, *child, nullptr)) {
                continue;
            }
            if (dispatchTransformedGenericPointerEvent(event, child)) {
                preorderedList.clear();
                return true;
            }
        }
        preorderedList.clear();
    }
    // No child handled the event.  Send it to this view group.
    return View::dispatchGenericPointerEvent(event);
}

bool ViewGroup::dispatchGenericFocusedEvent(MotionEvent&event){
    if ((mPrivateFlags & (PFLAG_FOCUSED | PFLAG_HAS_BOUNDS))
            == (PFLAG_FOCUSED | PFLAG_HAS_BOUNDS)) {
        return View::dispatchGenericFocusedEvent(event);
    } else if (mFocused && (mFocused->mPrivateFlags & PFLAG_HAS_BOUNDS)
            == PFLAG_HAS_BOUNDS) {
        return mFocused->dispatchGenericMotionEvent(event);
    }
    return false;
}

bool ViewGroup::dispatchTransformedGenericPointerEvent(MotionEvent& event, View* child) {
    bool handled = false;;
    if (!child->hasIdentityMatrix()) {
        MotionEvent* transformedEvent = getTransformedMotionEvent(event, child);
        handled = child->dispatchGenericMotionEvent(*transformedEvent);
        transformedEvent->recycle();
    } else {
        const float offsetX = mScrollX - child->mLeft;
        const float offsetY = mScrollY - child->mTop;
        event.offsetLocation(offsetX, offsetY);
        handled = child->dispatchGenericMotionEvent(event);
        event.offsetLocation(-offsetX, -offsetY);
    }
    return handled;
}

bool ViewGroup::dispatchUnhandledMove(View* focused, int direction){
    return mFocused && mFocused->dispatchUnhandledMove(focused, direction);
}

void ViewGroup::childHasTransientStateChanged(View* child, bool childHasTransientState){
    const bool oldHasTransientState = hasTransientState();
    if (childHasTransientState) {
        mChildCountWithTransientState++;
    } else {
        mChildCountWithTransientState--;
    }

    const bool newHasTransientState = hasTransientState();
    if (mParent  && oldHasTransientState != newHasTransientState)
        mParent->childHasTransientStateChanged(this, newHasTransientState);
}

void ViewGroup::dispatchViewAdded(View*v){
    onViewAdded(v);
    if(mOnHierarchyChangeListener.onChildViewAdded)
        mOnHierarchyChangeListener.onChildViewAdded(*this,v);
}

void ViewGroup::dispatchViewRemoved(View*v){
    onViewRemoved(v);
    if(mOnHierarchyChangeListener.onChildViewRemoved)
        mOnHierarchyChangeListener.onChildViewRemoved(*this,v);
}

void ViewGroup::removeDetachedView(View* child, bool animate){
    if (mTransition)mTransition->removeChild(this, child);

    if (child == mFocused) child->clearFocus();
    if (child == mDefaultFocus) clearDefaultFocus(child);
    if (child == mFocusedInCluster) clearFocusedInCluster(child);

    child->clearAccessibilityFocus();

    cancelTouchTarget(child);
    cancelHoverTarget(child);

    if ((animate && child->getAnimation()) ||isViewTransitioning(child)){
        addDisappearingView(child);
    } else if (child->mAttachInfo != nullptr) {
        child->dispatchDetachedFromWindow();
    }

    if (child->hasTransientState()) childHasTransientStateChanged(child, false);

    dispatchViewRemoved(child);
}

void ViewGroup::detachViewFromParent(View* child){
    removeFromArray(indexOfChild(child));
}

void ViewGroup::detachViewFromParent(int index){
    removeFromArray(index);
}

void ViewGroup::detachViewsFromParent(int start, int count){
    removeFromArray(start, count);
}

void ViewGroup::removeFromArray(int index){
    if (!isViewTransitioning(mChildren[index])){
        mChildren[index]->mParent = nullptr;
    }
    if ((index>=0)&&(index<mChildren.size())) {
        /*auto it=*/mChildren.erase(mChildren.begin()+index);
        //delete *it;cant delete here 
    } else {
        LOGE("IndexOutOfBounds %d",index);
    }
    if (mLastTouchDownIndex == index) {
        mLastTouchDownTime = 0;
        mLastTouchDownIndex = -1;
    } else if (mLastTouchDownIndex > index) {
        mLastTouchDownIndex--;
    }
}

void ViewGroup::removeFromArray(int start, int count){
    const int childrenCount = mChildren.size();

    start = std::max(0, start);
    const int end = std::min(childrenCount, start + count);

    if (start == end)  return;
    for (int i = start; i < end; i++) {
        mChildren[i]->mParent = nullptr;
        //Do not  delete mChildren[i];
        mChildren[i] = nullptr;
    }
    mChildren.erase(mChildren.begin()+start,mChildren.begin()+start+count);
}

void ViewGroup::detachAllViewsFromParent(){
    int count = mChildren.size();
    if (count <= 0) {
        return;
    }
    for (int i = count - 1; i >= 0; i--) {
        mChildren[i]->mParent = nullptr;
        mChildren[i] = nullptr;
    }
    mChildren.clear();
}

bool ViewGroup::addViewInLayout(View* child, int index,LayoutParams* params){
    return addViewInLayout(child, index, params, false);
}

bool ViewGroup::addViewInLayout(View* child, int index,LayoutParams* params,bool preventRequestLayout){
    if (child == nullptr) {
        LOGE("Cannot add a null child view to a ViewGroup");
        return false;
    }
    child->mParent = nullptr;
    addViewInner(child, index, params, preventRequestLayout);
    child->mPrivateFlags = (child->mPrivateFlags & ~PFLAG_DIRTY_MASK) | PFLAG_DRAWN;
    return true;
}

void ViewGroup::addTransientView(View*view,int index){
    if (index < 0) {
        return;
    }
    int oldSize = mTransientIndices.size();
    if (oldSize > 0) {
        int insertionIndex;
        for (insertionIndex = 0; insertionIndex < oldSize; ++insertionIndex) {
            if (index < mTransientIndices.at(insertionIndex)) {
                break;
            }
        }
        mTransientIndices.insert(mTransientIndices.begin()+insertionIndex, index);
        mTransientViews.insert(mTransientViews.begin()+insertionIndex, view);
    } else {
        mTransientIndices.push_back(index);
        mTransientViews.push_back(view);
    }
    view->mParent = this;
    view->dispatchAttachedToWindow(mAttachInfo, (mViewFlags&VISIBILITY_MASK));
    invalidate(true);
}

void ViewGroup::removeTransientView(View*view){
    int size = mTransientViews.size();
    for (int i = 0; i < size; ++i) {
        if (view == mTransientViews.at(i)) {
            mTransientViews.erase(mTransientViews.begin()+i);
            mTransientIndices.erase(mTransientIndices.begin()+i);
            view->mParent = nullptr;
            view->dispatchDetachedFromWindow();
            invalidate(true);
            return;
        }
    }
}

int ViewGroup::getTransientViewCount() const{
    return mTransientIndices.size();
}

int ViewGroup::getTransientViewIndex(int position) const{
    if ((position < 0) || (position >= mTransientIndices.size())) {
        return -1;
    }
    return mTransientIndices.at(position);
}

View* ViewGroup::getTransientView(int position) const{
    if ((position < 0) || (position >= mTransientViews.size())) {
        return nullptr;
    }
    return mTransientViews.at(position);
}

void ViewGroup::addDisappearingView(View* v) {
    mDisappearingChildren.push_back(v);
}

void ViewGroup::clearDisappearingChildren() {
    std::vector<View*> disappearingChildren = mDisappearingChildren;
    int count = disappearingChildren.size();
    for (int i = 0; i < count; i++) {
        View* view = disappearingChildren.at(i);
        if (view->mAttachInfo)view->dispatchDetachedFromWindow();
        view->clearAnimation();
    }
    disappearingChildren.clear();
    invalidate();
}

void ViewGroup::startViewTransition(View* view){
    if (view->mParent == this)
        mTransitioningViews.push_back(view);
}

void ViewGroup::endViewTransition(View* view){
    auto it= std::find(mTransitioningViews.begin(),mTransitioningViews.end(),view);
    if(it!=mTransitioningViews.end())mTransitioningViews.erase(it);

    it= std::find(mDisappearingChildren.begin(),mDisappearingChildren.end(),view);
    if (it!=mDisappearingChildren.end()) {
        mDisappearingChildren.erase(it);

        it=std::find(mVisibilityChangingChildren.begin(),mVisibilityChangingChildren.end(),view);
        if (it!=mVisibilityChangingChildren.end()) {
            mVisibilityChangingChildren.erase(it);
        } else {
            if (view->mAttachInfo) view->dispatchDetachedFromWindow();
            if (view->mParent) view->mParent = nullptr;
        }
       invalidate();
   }
}

void ViewGroup::finishAnimatingView(View* view, Animation* animation) {
    auto it=std::find(mDisappearingChildren.begin(),mDisappearingChildren.end(),view);
    if (it!=mDisappearingChildren.end()) {
        mDisappearingChildren.erase(it);
        if (view->mAttachInfo) view->dispatchDetachedFromWindow();
        view->clearAnimation();
        mGroupFlags |= FLAG_INVALIDATE_REQUIRED;
    }

    if (animation && !animation->getFillAfter()) {
        view->clearAnimation();
    }

    if ((view->mPrivateFlags & PFLAG_ANIMATION_STARTED) == PFLAG_ANIMATION_STARTED) {
        view->onAnimationEnd();
        // Should be performed by onAnimationEnd() but this avoid an infinite loop,
        // so we'd rather be safe than sorry
        view->mPrivateFlags &= ~PFLAG_ANIMATION_STARTED;
        // Draw one more frame after the animation is done
        mGroupFlags |= FLAG_INVALIDATE_REQUIRED;
    }
}

void ViewGroup::dispatchInvalidateOnAnimation(View* view){
}

void ViewGroup::dispatchInvalidateRectOnAnimation(View*view,const Rect&rect){
}

void ViewGroup::dispatchInvalidateDelayed(View* view,long){
}

void ViewGroup::dispatchInvalidateRectDelayed(const AttachInfo::InvalidateInfo*,long){
}

void ViewGroup::cancelInvalidate(View* view){
}

bool ViewGroup::isViewTransitioning(View* view){
    return  std::find(mTransitioningViews.begin(),mTransitioningViews.end(),view)!= mTransitioningViews.end();
}

void ViewGroup::onChildVisibilityChanged(View* child, int oldVisibility, int newVisibility){
    LOGV("view %p visibility %d->%d",child,oldVisibility, newVisibility);
    if (mTransition != nullptr) {
        if (newVisibility == VISIBLE) {
            mTransition->showChild(this, child, oldVisibility);
        } else {
            mTransition->hideChild(this, child, newVisibility);
            if ( isViewTransitioning(child) ){
                // Only track this on disappearing views - appearing views are already visible
                // and don't need special handling during drawChild()
                mVisibilityChangingChildren.push_back(child);
                addDisappearingView(child);
            }
        }
    }
    // in all cases, for drags
    /*if (newVisibility == VISIBLE && mCurrentDragStartEvent != null) {
        if (!mChildrenInterestedInDrag.contains(child)) {
            notifyChildOfDragStart(child);
        }
    }*/
}

void ViewGroup::attachViewToParent(View* child, int index, LayoutParams* params){
    child->mLayoutParams = params;

    if (index < 0) {
        index = getChildCount();
    }

    addInArray(child, index);

    child->mParent = this;
    child->mPrivateFlags = (child->mPrivateFlags & ~PFLAG_DIRTY_MASK & ~PFLAG_DRAWING_CACHE_VALID)
         | PFLAG_DRAWN|PFLAG_INVALIDATED;
    this->mPrivateFlags |= PFLAG_INVALIDATED;
    if (child->hasFocus()) {
        requestChildFocus(child, child->findFocus());
    }
    dispatchVisibilityAggregated(isAttachedToWindow() && getWindowVisibility() == VISIBLE&& isShown());
    //notifySubtreeAccessibilityStateChangedIfNeeded();
}

void ViewGroup::dispatchWindowVisibilityChanged(int visibility) {
    View::dispatchWindowVisibilityChanged(visibility);
    for (auto child:mChildren) {
        child->dispatchWindowVisibilityChanged(visibility);
    }
}

void ViewGroup::dispatchVisibilityChanged(View& changedView, int visibility) {
    View::dispatchVisibilityChanged(changedView, visibility);
    for (View*child:mChildren) {
        child->dispatchVisibilityChanged(changedView, visibility);
    }
}

bool ViewGroup::isAlwaysDrawnWithCacheEnabled()const{
    return (mGroupFlags & FLAG_ALWAYS_DRAWN_WITH_CACHE) == FLAG_ALWAYS_DRAWN_WITH_CACHE;
}

void ViewGroup::setAlwaysDrawnWithCacheEnabled(bool always){
    setBooleanFlag(FLAG_ALWAYS_DRAWN_WITH_CACHE, always);
}

bool ViewGroup::isChildrenDrawnWithCacheEnabled()const{
    return (mGroupFlags & FLAG_CHILDREN_DRAWN_WITH_CACHE) == FLAG_CHILDREN_DRAWN_WITH_CACHE;
}

void ViewGroup::setChildrenDrawnWithCacheEnabled(bool enabled){
    setBooleanFlag(FLAG_CHILDREN_DRAWN_WITH_CACHE, enabled);
}

void ViewGroup::setChildrenDrawingCacheEnabled(bool enabled){
    if (enabled || (mPersistentDrawingCache & PERSISTENT_ALL_CACHES) != PERSISTENT_ALL_CACHES) {
        for (auto c:mChildren) {
            c->setDrawingCacheEnabled(enabled);
        }
    }
}

bool ViewGroup::isLayoutModeOptical()const{
    return mLayoutMode == LAYOUT_MODE_OPTICAL_BOUNDS;
}

int ViewGroup::getDescendantFocusability()const{
    return mGroupFlags&FLAG_MASK_FOCUSABILITY;
}

void ViewGroup::setDescendantFocusability(int focusability){
    switch (focusability) {
    case FOCUS_BEFORE_DESCENDANTS:
    case FOCUS_AFTER_DESCENDANTS:
    case FOCUS_BLOCK_DESCENDANTS:
        break;
    default:
        FATAL("must be one of FOCUS_BEFORE_DESCENDANTS, "
              "FOCUS_AFTER_DESCENDANTS, FOCUS_BLOCK_DESCENDANTS");
    }
    mGroupFlags &= ~FLAG_MASK_FOCUSABILITY;
    mGroupFlags |= (focusability & FLAG_MASK_FOCUSABILITY);
}

bool ViewGroup::isChildrenDrawingOrderEnabled()const{
    return (mGroupFlags & FLAG_USE_CHILD_DRAWING_ORDER) == FLAG_USE_CHILD_DRAWING_ORDER;
}

void ViewGroup::setChildrenDrawingOrderEnabled(bool enabled) {
    setBooleanFlag(FLAG_USE_CHILD_DRAWING_ORDER, enabled);
}

void ViewGroup::setBooleanFlag(int flag, bool value) {
    if (value) {
        mGroupFlags |= flag;
    } else {
        mGroupFlags &= ~flag;
    }
}

bool ViewGroup::hasBooleanFlag(int flag)const{
    return (mGroupFlags & flag) == flag;
}

bool ViewGroup::getClipChildren() const{
    return ((mGroupFlags & FLAG_CLIP_CHILDREN) != 0);
}

void ViewGroup::setClipChildren(bool clipChildren){
    bool previousValue = (mGroupFlags & FLAG_CLIP_CHILDREN) == FLAG_CLIP_CHILDREN;
    if (clipChildren != previousValue){ 
        setBooleanFlag(FLAG_CLIP_CHILDREN, clipChildren);
	invalidate();
    }
}

void ViewGroup::setClipToPadding(bool clipToPadding) {
    if (hasBooleanFlag(FLAG_CLIP_TO_PADDING) != clipToPadding) {
        setBooleanFlag(FLAG_CLIP_TO_PADDING, clipToPadding);
        invalidate();
    }
}

bool ViewGroup::getClipToPadding()const{
   return hasBooleanFlag(FLAG_CLIP_TO_PADDING);
}

void ViewGroup::dispatchStartTemporaryDetach() {
    View::dispatchStartTemporaryDetach();
    for (View*child:mChildren){
        child->dispatchStartTemporaryDetach();
    }
}

void ViewGroup::dispatchFinishTemporaryDetach(){
    View::dispatchFinishTemporaryDetach();
    for (View*child:mChildren){
        child->dispatchFinishTemporaryDetach();
    }
}

void ViewGroup::dispatchSetSelected(bool selected) {
    for (auto child:mChildren){
        child->setSelected(selected);
    }
}

void ViewGroup::dispatchSetActivated(bool activated) {
    for (auto child:mChildren){
        child->setActivated(activated);
    }
}

void ViewGroup::dispatchSetPressed(bool pressed) {
    for (auto child:mChildren){
        // Children that are clickable on their own should not
        // show a pressed state when their parent view does.
        // Clearing a pressed state always propagates.
        if (!pressed || (!child->isClickable() && !child->isLongClickable())) {
            child->setPressed(pressed);
        }
    }
}

void ViewGroup::dispatchDrawableHotspotChanged(float x,float y){
    for(auto child:mChildren){
        const bool nonActivationable = !child->isClickable()&&!child->isLongClickable();
        const bool duplicateState    = (child->mViewFlags & DUPLICATE_PARENT_STATE)!=0;
        if( nonActivationable||duplicateState){
            float point[2]={x,y};
            transformPointToViewLocal(point,*child);
            child->drawableHotspotChanged(point[0],point[1]);
        }
    }
}

int ViewGroup::getChildCount()const{
    return mChildren.size();
}

View*ViewGroup::getChildAt(int idx)const{
    if(idx<0||idx>=mChildren.size())return nullptr;
    return mChildren.at(idx);
}

int ViewGroup::indexOfChild(View* child)const{
    int count=mChildren.size();
    for(int i=0;i<count;i++)
        if(mChildren[i]==child)return i;
    return -1;
}

ViewOverlay* ViewGroup::getOverlay() {
    if (mOverlay == nullptr) {
        mOverlay = new ViewGroupOverlay(mContext, this);
        mOverlay->getOverlayView()->setFrame(mLeft,mTop,mRight-mLeft,mBottom-mTop);
    }
    return mOverlay;
}

int ViewGroup::getChildDrawingOrder(int count,int i){
    return i;
}

bool ViewGroup::hasChildWithZ()const{
    for (auto child:mChildren) {
        if (child->getZ() != 0) return true;
    }
    return false;
}

std::vector<View*> ViewGroup::buildOrderedChildList() {
    std::vector<View*>preSortedChildren;
    const int childrenCount =  mChildren.size();
    if (childrenCount <= 1 || !hasChildWithZ()) return preSortedChildren;

    const bool customOrder = isChildrenDrawingOrderEnabled();
    for (int i = 0; i < childrenCount; i++) {
        // add next child (in child order) to end of list
        const int childIndex = getAndVerifyPreorderedIndex(childrenCount, i, customOrder);
        View* nextChild = mChildren[childIndex];
        const float currentZ = nextChild->getZ();

        // insert ahead of any Views with greater Z
        int insertIndex = i;
        while (insertIndex > 0 && preSortedChildren.at(insertIndex - 1)->getZ() > currentZ) {
            insertIndex--;
        }
        preSortedChildren.insert(preSortedChildren.begin()+insertIndex, nextChild);
    }
    return preSortedChildren;
}

std::vector<View*> ViewGroup::buildTouchDispatchChildList(){
    return buildOrderedChildList();
}

int ViewGroup::getAndVerifyPreorderedIndex(int childrenCount, int i, bool customOrder){
    int childIndex;
    if (customOrder) {
        int childIndex1 = getChildDrawingOrder(childrenCount, i);
        LOGE_IF(childIndex1 >= childrenCount,"getChildDrawingOrder() returned invalid index %d (child count is %d)",childIndex1,childrenCount);
        childIndex = childIndex1;
    } else {
        childIndex = i;
    }
    return childIndex;
}

View* ViewGroup::getAndVerifyPreorderedView(const std::vector<View*>& preorderedList,const std::vector<View*>& children,int childIndex) {
    View* child;
    if (preorderedList.size()) {
        child = preorderedList.at(childIndex);
        LOGE_IF(child == nullptr,"Invalid preorderedList contained null child at index %d",childIndex);
    } else {
        child = children[childIndex];
    }
    return child;
}

bool ViewGroup::getChildStaticTransformation(View* child, Transformation* t){
    return false;
}

Transformation* ViewGroup::getChildTransformation(){
    if(mChildTransformation==nullptr)
        mChildTransformation=new Transformation();
    return mChildTransformation;
}

int ViewGroup::getChildMeasureSpec(int spec, int padding, int childDimension){
    int specMode = MeasureSpec::getMode(spec);
    int specSize = MeasureSpec::getSize(spec);

    int size = std::max(0, specSize - padding);

    int resultSize = 0;
    int resultMode = 0;

    switch (specMode) {
    // Parent has imposed an exact size on us
    case MeasureSpec::EXACTLY:
        if (childDimension >= 0) {
            resultSize = childDimension;
            resultMode = MeasureSpec::EXACTLY;
        } else if (childDimension == LayoutParams::MATCH_PARENT) {
            // Child wants to be our size. So be it.
            resultSize = size;
            resultMode = MeasureSpec::EXACTLY;
        } else if (childDimension == LayoutParams::WRAP_CONTENT) {
            // Child wants to determine its own size. It can't be
            // bigger than us.
            resultSize = size;
            resultMode = MeasureSpec::AT_MOST;
        }
        break;

    // Parent has imposed a maximum size on us
    case MeasureSpec::AT_MOST:
        if (childDimension >= 0) {
            // Child wants a specific size... so be it
            resultSize = childDimension;
            resultMode = MeasureSpec::EXACTLY;
        } else if (childDimension == LayoutParams::MATCH_PARENT) {
            // Child wants to be our size, but our size is not fixed.
            // Constrain child to not be bigger than us.
            resultSize = size;
            resultMode = MeasureSpec::AT_MOST;
        } else if (childDimension == LayoutParams::WRAP_CONTENT) {
            // Child wants to determine its own size. It can't be
            // bigger than us.
            resultSize = size;
            resultMode = MeasureSpec::AT_MOST;
        }
        break;

    // Parent asked to see how big we want to be
    case MeasureSpec::UNSPECIFIED:
        if (childDimension >= 0) {
            // Child wants a specific size... let him have it
            resultSize = childDimension;
            resultMode = MeasureSpec::EXACTLY;
        } else if (childDimension == LayoutParams::MATCH_PARENT) {
            // Child wants to be our size... find out how big it should
            // be
            resultSize = size;// View.sUseZeroUnspecifiedMeasureSpec ? 0 : size;
            resultMode = MeasureSpec::UNSPECIFIED;
        } else if (childDimension == LayoutParams::WRAP_CONTENT) {
            // Child wants to determine its own size.... find out how
            // big it should be
            resultSize = size;//View.sUseZeroUnspecifiedMeasureSpec ? 0 : size;
            resultMode = MeasureSpec::UNSPECIFIED;
        }
        break;
    }
    //noinspection ResourceType
    return MeasureSpec::makeMeasureSpec(resultSize, resultMode);
}

View&ViewGroup::addView(View* view){
    return addView(view,-1);
}

View& ViewGroup::addView(View* child, int index){
    LayoutParams* params = child->getLayoutParams();
    if (params == nullptr) {
        params = generateDefaultLayoutParams();
        LOGE_IF(params == nullptr,"generateDefaultLayoutParams() cannot return null");
    }
    return addView(child, index, params);
}

View& ViewGroup::addView(View* child, int width, int height){
    LayoutParams* params = generateDefaultLayoutParams();
    params->width = width;
    params->height = height;
    return addView(child, -1, params);
}

View& ViewGroup::addView(View* child, LayoutParams* params){
    return addView(child, -1, params);
}

void ViewGroup::removeView(View* view){
    if(removeViewInternal(view)){
        requestLayout();
        invalidate();
    }
}

View&ViewGroup::addView(View* child, int index,LayoutParams* params){
    if(child==nullptr)
         throw "Cannot add a null child view to a ViewGroup";
    requestLayout();
    invalidate(true);
    return addViewInner(child, index, params, false);
}

void ViewGroup::addInArray(View* child, int index){
    if(index>=mChildren.size())
        mChildren.push_back(child);
    else
        mChildren.insert(mChildren.begin()+index,child);
}

void ViewGroup::cleanupLayoutState(View* child)const{
    child->mPrivateFlags &= ~PFLAG_FORCE_LAYOUT;
}

View& ViewGroup::addViewInner(View* child, int index,LayoutParams* params,bool preventRequestLayout){
    if(mTransition){
        mTransition->cancel(LayoutTransition::DISAPPEARING);
    }

    if(child->mParent){
        throw "The specified child already has a parent. you must call removeView() on the child's parent first.";
    }
    if(mTransition){
        mTransition->addChild(this,child);
    }
    if (!checkLayoutParams(params)) {
        LayoutParams*olp =params;
        params = generateLayoutParams(params);
        if(child->mLayoutParams!=olp){
            delete olp;//params is unbinded ,must be destroied!
        }
    }

    if (preventRequestLayout) {
        if(child->mLayoutParams!=params)
            delete child->mLayoutParams;
        child->mLayoutParams = params;
    } else {
        child->setLayoutParams(params);
    }

    if (index < 0) index =mChildren.size();

    addInArray(child, index);

    // tell our children
    if (preventRequestLayout) {
        child->assignParent(this);
    } else {
        child->mParent = this;
    }
    if (child->hasUnhandledKeyListener()) incrementChildUnhandledKeyListeners();

    const bool childHasFocus = child->hasFocus();
    if (childHasFocus) requestChildFocus(child, child->findFocus());

    if (mAttachInfo  && (mGroupFlags & FLAG_PREVENT_DISPATCH_ATTACHED_TO_WINDOW) == 0) {
        bool lastKeepOn = mAttachInfo->mKeepScreenOn;
        mAttachInfo->mKeepScreenOn = false;
        child->dispatchAttachedToWindow(mAttachInfo, (mViewFlags&VISIBILITY_MASK));
        if (mAttachInfo->mKeepScreenOn) {
            //needGlobalAttributesUpdate(true);
        }
        mAttachInfo->mKeepScreenOn = lastKeepOn;
    }

    if (child->isLayoutDirectionInherited()) child->resetRtlProperties();

    dispatchViewAdded(child);

    if ((child->mViewFlags & DUPLICATE_PARENT_STATE) == DUPLICATE_PARENT_STATE) {
        mGroupFlags |= FLAG_NOTIFY_CHILDREN_ON_DRAWABLE_STATE_CHANGE;
    }

    if (child->hasTransientState()) childHasTransientStateChanged(child, true);

    //if (child->getVisibility() !=GONE) notifySubtreeAccessibilityStateChangedIfNeeded();

    int transientCount = mTransientIndices.size();
    for (int i = 0; i < transientCount; ++i) {
        int oldIndex = mTransientIndices.at(i);
        if (index <= oldIndex) {
            mTransientIndices[i]= oldIndex + 1;
        }
    }

    /*if (mCurrentDragStartEvent != nullptr && child->getVisibility() == VISIBLE) {
        notifyChildOfDragStart(child);
    }*/

    if (child->hasDefaultFocus()) {
        // When adding a child that contains default focus, either during inflation or while
        // manually assembling the hierarchy, update the ancestor default-focus chain.
        setDefaultFocus(child);
    }
    //touchAccessibilityNodeProviderIfNeeded(child);
    return *child;
}

LayoutParams* ViewGroup::generateLayoutParams(const AttributeSet& attrs)const{
    return new LayoutParams(getContext(), attrs);
}

LayoutParams*ViewGroup::generateLayoutParams(const LayoutParams* p)const{
    return (LayoutParams*)p;
}

LayoutParams* ViewGroup::generateDefaultLayoutParams()const{
    return new LayoutParams(LayoutParams::WRAP_CONTENT,LayoutParams::WRAP_CONTENT);
}

bool ViewGroup::checkLayoutParams(const LayoutParams* p)const{
    return p!=nullptr;
}

void ViewGroup::onSetLayoutParams(View* child,const LayoutParams* layoutParams){
    requestLayout();
}

bool ViewGroup::hasUnhandledKeyListener()const{
    return (mChildUnhandledKeyListeners > 0) || View::hasUnhandledKeyListener();
}

void ViewGroup::incrementChildUnhandledKeyListeners(){
    mChildUnhandledKeyListeners += 1;
    if (mChildUnhandledKeyListeners == 1) {
        if (mParent) {
            mParent->incrementChildUnhandledKeyListeners();
        }
    }
}

void ViewGroup::decrementChildUnhandledKeyListeners(){
    mChildUnhandledKeyListeners -= 1;
    if (mChildUnhandledKeyListeners == 0) {
        if (mParent) {
            mParent->decrementChildUnhandledKeyListeners();
        }
    }
}

View* ViewGroup::dispatchUnhandledKeyEvent(KeyEvent& evt){
    if (!hasUnhandledKeyListener()) {
        return nullptr;
    }
    std::vector<View*> orderedViews = buildOrderedChildList();
    if (orderedViews.size()) {
        for (int i = orderedViews.size() - 1; i >= 0; --i) {
            View* v = orderedViews.at(i);
            View* consumer = v->dispatchUnhandledKeyEvent(evt);
            if (consumer != nullptr) {
                return consumer;
            }
        }
    } else {
        for (int i = getChildCount() - 1; i >= 0; --i) {
            View* v = getChildAt(i);
            View* consumer = v->dispatchUnhandledKeyEvent(evt);
            if (consumer != nullptr) {
                return consumer;
            }
        }
    }
    if (onUnhandledKeyEvent(evt)) {
        return this;
    }
    return nullptr;
}

void ViewGroup::measureChildren(int widthMeasureSpec, int heightMeasureSpec){
    for (auto child:mChildren) {
        if ((child->mViewFlags & VISIBILITY_MASK) != GONE) {
            measureChild(child, widthMeasureSpec, heightMeasureSpec);
        }
    }
}

void ViewGroup::measureChild(View* child, int parentWidthMeasureSpec,int parentHeightMeasureSpec){
    LayoutParams* lp = child->getLayoutParams();
    int childWidthMeasureSpec = getChildMeasureSpec(parentWidthMeasureSpec,
        mPaddingLeft + mPaddingRight, lp->width);
    int childHeightMeasureSpec = getChildMeasureSpec(parentHeightMeasureSpec,
        mPaddingTop + mPaddingBottom, lp->height);
    child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
}

void ViewGroup::measureChildWithMargins(View* child,int parentWidthMeasureSpec, int widthUsed,
            int parentHeightMeasureSpec, int heightUsed){
    MarginLayoutParams* lp = (MarginLayoutParams*) child->getLayoutParams();

    int childWidthMeasureSpec = getChildMeasureSpec(parentWidthMeasureSpec,
        mPaddingLeft + mPaddingRight + lp->leftMargin + lp->rightMargin
                + widthUsed, lp->width);
    int childHeightMeasureSpec = getChildMeasureSpec(parentHeightMeasureSpec,
        mPaddingTop + mPaddingBottom + lp->topMargin + lp->bottomMargin
        + heightUsed, lp->height);
    child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
}

void ViewGroup::removeViewAt(int index){
    removeViewInternal(index, getChildAt(index));
    requestLayout();
    invalidate();
}

void ViewGroup::removeViews(int start, int count){
    removeViewsInternal(start, count);
    requestLayout();
    invalidate();
}

void ViewGroup::removeViewInLayout(View* view){
    removeViewInternal(view);
}

void ViewGroup::removeViewsInLayout(int start,int count){
    removeViewsInternal(start, count);
}

void ViewGroup::removeAllViews(){
    removeAllViewsInLayout();
    requestLayout();
    invalidate();
}

void ViewGroup::removeAllViewsInLayout() {
    int count = mChildren.size();
    if (count <= 0) {
        return;
    }

    View* focused = mFocused;
    bool detach = mAttachInfo != nullptr;
    bool bclearChildFocus = false;

    //needGlobalAttributesUpdate(false);
    for (int i = count - 1; i >= 0; i--) {
        View* view = mChildren[i];

        if (mTransition) mTransition->removeChild(this, view);

        if (view == focused) {
            view->unFocus(nullptr);
            bclearChildFocus = true;
        }

        view->clearAccessibilityFocus();

        cancelTouchTarget(view);
        cancelHoverTarget(view);

        if (view->getAnimation() ||std::find(mTransitioningViews.begin(),
		        mTransitioningViews.end(),view)!=mTransitioningViews.end()) {
            addDisappearingView(view);
        } else if (detach) {
            view->dispatchDetachedFromWindow();
        }

        if (view->hasTransientState()) {
            childHasTransientStateChanged(view, false);
        }

        dispatchViewRemoved(view);

        view->mParent = nullptr;
        mChildren[i] = nullptr;//cant delete mChilden[i]
    }

    mChildren.clear();
    if (mDefaultFocus)  clearDefaultFocus(mDefaultFocus);

    if (mFocusedInCluster) clearFocusedInCluster(mFocusedInCluster);

    if (bclearChildFocus) {
        clearChildFocus(focused);
        if (!rootViewRequestFocus()) {
            notifyGlobalFocusCleared(focused);
        }
    }
}

bool ViewGroup::removeViewInternal(View* view){
    const int index = indexOfChild(view);
    if (index >= 0) {
        removeViewInternal(index, view);
        return true;
    }
    return false;
}

void ViewGroup::removeViewInternal(int index, View* view){
    if (mTransition)mTransition->removeChild(this, view);

    bool _clearChildFocus = false;
    if(view==nullptr) return ;
    if (view == mFocused) {
        view->unFocus(nullptr);
        _clearChildFocus = true;
    }
    if (view == mFocusedInCluster)clearFocusedInCluster(view);

    view->clearAccessibilityFocus();

    cancelTouchTarget(view);
    cancelHoverTarget(view);

    if (view->getAnimation() ||isViewTransitioning(view)){
        addDisappearingView(view);
    } else if (view->mAttachInfo) {
        view->dispatchDetachedFromWindow();
    }

    if (view->hasTransientState())childHasTransientStateChanged(view, false);

    //needGlobalAttributesUpdate(false);
    removeFromArray(index);

    if (view->hasUnhandledKeyListener()) decrementChildUnhandledKeyListeners();

    if (view == mDefaultFocus) clearDefaultFocus(view);

    if (_clearChildFocus) {
        clearChildFocus(view);
        if (!rootViewRequestFocus()) notifyGlobalFocusCleared(this);
    }
    dispatchViewRemoved(view);

    //if (view->getVisibility() != View::GONE)notifySubtreeAccessibilityStateChangedIfNeeded();

    const int transientCount =  mTransientIndices.size();
    for (int i = 0; i < transientCount; ++i) {
        int oldIndex = mTransientIndices.at(i);
        if (index < oldIndex) {
            mTransientIndices[i]=oldIndex-1;//mTransientIndices.set(i, oldIndex - 1);
        }
    }
    /*if (mCurrentDragStartEvent != nullptr){
        auto it =std::find(mChildrenInterestedInDrag.begin(),mChildrenInterestedInDrag.end(),view);
        if(it!=mChildrenInterestedInDrag.end())mChildrenInterestedInDrag.erase(it);
    }*/
}

void ViewGroup::removeViewsInternal(int start, int count){
    int end = start + count;

    if (start < 0 || count < 0 || end > mChildren.size()) {
        throw "IndexOutOfBoundsException";//new IndexOutOfBoundsException();
    }

    View* focused = mFocused;
    bool detach = mAttachInfo != nullptr;
    bool _clearChildFocus = false;
    View* _clearDefaultFocus = nullptr;

    for (int i = start; i < end; i++) {
        View* view = mChildren[i];

        if (mTransition) mTransition->removeChild(this, view);

        if (view == focused) {
            view->unFocus(nullptr);
            _clearChildFocus = true;
        }
        if (view == mDefaultFocus)_clearDefaultFocus = view;
        if (view == mFocusedInCluster) clearFocusedInCluster(view);

        view->clearAccessibilityFocus();

        cancelTouchTarget(view);
        cancelHoverTarget(view);

        if (view->getAnimation() ||(std::find(mTransitioningViews.begin(),
                mTransitioningViews.end(),view)==mTransitioningViews.end())) {
            addDisappearingView(view);
        } else if (detach) {
            view->dispatchDetachedFromWindow();
        }

        if (view->hasTransientState()) childHasTransientStateChanged(view, false);

        //needGlobalAttributesUpdate(false);

        dispatchViewRemoved(view);
    }

    removeFromArray(start, count);

    if (_clearDefaultFocus != nullptr) {
        clearDefaultFocus(_clearDefaultFocus);
    }
    if (_clearChildFocus) {
        clearChildFocus(focused);
        if (!rootViewRequestFocus()) notifyGlobalFocusCleared(focused);
    }
}

View* ViewGroup::findViewByPredicateTraversal(std::function<bool(const View*)>predicate,View* childToSkip){
    if (predicate(this)) {
        return (View*)this;
    }

    for (auto v:mChildren) {

        if (v != childToSkip && (v->mPrivateFlags & PFLAG_IS_ROOT_NAMESPACE) == 0) {
            v = v->findViewByPredicate(predicate);

            if (v != nullptr) {
                return  v;
            }
        }
    }
    return nullptr;

}

View* ViewGroup::findViewWithTagTraversal(void*tag){
    //if (tag  && tag.equals(mTag)) return (View*)this;

    for (View*v:mChildren){
        if ((v->mPrivateFlags & PFLAG_IS_ROOT_NAMESPACE) == 0) {
            v = v->findViewWithTag(tag);
            if (v != nullptr) return v;
        }
    }
    return nullptr;
}

View*ViewGroup::findViewById(int id){
    for(auto v:mChildren){
        View*c=v->findViewById(id);
        if(c)return c;
    }
    return View::findViewById(id);
}

void ViewGroup::dispatchWindowFocusChanged(bool hasFocus) {
    View::dispatchWindowFocusChanged(hasFocus);
    for (View*child:mChildren){
        child->dispatchWindowFocusChanged(hasFocus);
    }
}

bool ViewGroup::drawChild(Canvas& canvas, View* child, long drawingTime){
    return child->draw(canvas,this,drawingTime);
}

void ViewGroup::onDebugDrawMargins(Canvas& canvas){
    for (View*c:mChildren){
        c->getLayoutParams()->onDebugDraw(*c, canvas);
    }
}

void ViewGroup::drawInvalidateRegion(Canvas&canvas){
    int num=mInvalidRgn->get_num_rectangles();
    canvas.set_source_rgb(0,1,0);
    for(int i=0;i<num;i++){
        RectangleInt r = mInvalidRgn->get_rectangle(i);
        canvas.rectangle(r.x,r.y,r.width,r.height);
    }
    canvas.stroke();
}

void ViewGroup::fillRect(Canvas& canvas,int x1, int y1, int x2, int y2) {
    if (x1 == x2 || y1 == y2) return;
    if (x1 > x2) {
        int tmp = x1; x1 = x2; x2 = tmp;
    }
    if (y1 > y2) {
        int tmp = y1; y1 = y2; y2 = tmp;
    }
    canvas.rectangle(x1, y1, x2-x1, y2-y1);
}

void ViewGroup::drawRect(Canvas& canvas,int x1, int y1, int x2, int y2){
    canvas.move_to(x1,y1);
    canvas.line_to(x2,y1);
    canvas.line_to(x2,y2);
    canvas.line_to(x1,y2);
    canvas.line_to(x1,y1);
}

void ViewGroup::drawCorner(Canvas& c,int x1, int y1, int dx, int dy, int lw) {
#define SIGN(x) ((x)>=0?1:-1)
    fillRect(c, x1, y1, x1 + dx, y1 + lw * SIGN(dy));
    fillRect(c, x1, y1, x1 + lw * SIGN(dx), y1 + dy);
}

void ViewGroup::drawRectCorners(Canvas& canvas, int x1, int y1, int x2, int y2,int lineLength, int lineWidth) {
    drawCorner(canvas, x1, y1, lineLength, lineLength, lineWidth);
    drawCorner(canvas, x1, y2, lineLength, -lineLength, lineWidth);
    drawCorner(canvas, x2, y1, -lineLength, lineLength, lineWidth);
    drawCorner(canvas, x2, y2, -lineLength, -lineLength, lineWidth);
}

void ViewGroup::onDebugDraw(Canvas& canvas){
    // Draw optical bounds
    canvas.set_color(Color::RED);
    canvas.set_line_width(.5f);
    for (View*c :mChildren){
        if (c->getVisibility() != View::GONE) {
            Insets insets = c->getOpticalInsets();
            drawRect(canvas,c->getLeft() + insets.left,c->getTop() + insets.top,
                c->getRight() - insets.right - 1,c->getBottom() - insets.bottom - 1);
        }
    }
    canvas.stroke();

    // Draw margins
    canvas.set_source_rgba(1.f ,.0f, 1.f,.25f);
    onDebugDrawMargins(canvas);
    canvas.fill();

    // Draw clip bounds
    canvas.set_color(DEBUG_CORNERS_COLOR); 
    int lineLength = dipsToPixels(DEBUG_CORNERS_SIZE_DIP);
    int lineWidth  = dipsToPixels(1);
    for (View* c:mChildren){
        if (c->getVisibility() != View::GONE) {
            drawRectCorners(canvas, c->getLeft(), c->getTop(), c->getRight(), c->getBottom(),lineLength, lineWidth);
        }
    }
    canvas.fill();
}

void ViewGroup::dispatchDraw(Canvas&canvas){
    const int childrenCount = mChildren.size();
    int flags = mGroupFlags;

    if ((flags & FLAG_RUN_ANIMATION) != 0 && canAnimate()) {
        bool buildCache = !isHardwareAccelerated();
        for (int i=0;i<mChildren.size();i++){
            View* child=mChildren[i];
            if ((child->mViewFlags & VISIBILITY_MASK) == VISIBLE) {
                LayoutParams* params = child->getLayoutParams();
                attachLayoutAnimationParameters(child, params, i, childrenCount);
                bindLayoutAnimation(child);
            }
        }

        LayoutAnimationController* controller = mLayoutAnimationController;
        if (controller->willOverlap())  mGroupFlags |= FLAG_OPTIMIZE_INVALIDATE;

        controller->start();

        mGroupFlags &= ~FLAG_RUN_ANIMATION;
        mGroupFlags &= ~FLAG_ANIMATION_DONE;

        if (mAnimationListener.onAnimationStart) {
            mAnimationListener.onAnimationStart(*controller->getAnimation());
        }
    }

    int clipSaveCount = 0;
    const bool clipToPadding = (flags & CLIP_TO_PADDING_MASK) == CLIP_TO_PADDING_MASK;
    if (clipToPadding) {
        canvas.save();
        clipSaveCount++;
        canvas.rectangle(mScrollX + mPaddingLeft, mScrollY + mPaddingTop,
                mRight-mLeft-mPaddingLeft - mPaddingRight, mBottom-mTop -mPaddingTop- mPaddingBottom);
        canvas.clip();
    }

    // We will draw our child's animation, let's reset the flag
    mPrivateFlags &= ~PFLAG_DRAW_ANIMATION;
    mGroupFlags   &= ~FLAG_INVALIDATE_REQUIRED;

    bool more = false;
    const long drawingTime = getDrawingTime();

    //if (usingRenderNodeProperties) canvas.insertReorderBarrier();
    const int transientCount = mTransientIndices.size();
    int transientIndex = transientCount != 0 ? 0 : -1;
    // Only use the preordered list if not HW accelerated, since the HW pipeline will do the
    // draw reordering internally
    std::vector<View*> preorderedList=buildOrderedChildList();
    const bool customOrder = preorderedList.empty() && isChildrenDrawingOrderEnabled();
    for (int i = 0; i < childrenCount; i++) {
        while (transientIndex >= 0 && mTransientIndices.at(transientIndex) == i) {
            View* transientChild = mTransientViews.at(transientIndex);
            if ((transientChild->mViewFlags & VISIBILITY_MASK) == VISIBLE ||
                    transientChild->getAnimation() != nullptr) {
                more |= drawChild(canvas, transientChild, drawingTime);
            }
            transientIndex++;
            if (transientIndex >= transientCount) {
                transientIndex = -1;
            }
        }

        const int childIndex = getAndVerifyPreorderedIndex(childrenCount, i, customOrder);
        View* child = getAndVerifyPreorderedView(preorderedList, mChildren, childIndex);
        if ((child->mViewFlags & VISIBILITY_MASK) == VISIBLE || child->getAnimation() != nullptr) {
            more |= drawChild(canvas, child, drawingTime);
        }
    }
    while (transientIndex >= 0) {
        // there may be additional transient views after the normal views
        View* transientChild = mTransientViews.at(transientIndex);
        if ((transientChild->mViewFlags & VISIBILITY_MASK) == VISIBLE ||
                transientChild->getAnimation() != nullptr) {
            more |= drawChild(canvas, transientChild, drawingTime);
        }
        transientIndex++;
        if (transientIndex >= transientCount) {
            break;
        }
    }
    preorderedList.clear();

    // Draw any disappearing views that have animations
    if (mDisappearingChildren.size())  {
        std::vector<View*>&disappearingChildren = mDisappearingChildren;
        int disappearingCount = disappearingChildren.size() - 1;
        // Go backwards -- we may delete as animations finish
        for (int i = disappearingCount; i >= 0; i--) {
            View* child = disappearingChildren.at(i);
            more |= drawChild(canvas, child, drawingTime);
        }
    }
    //if (usingRenderNodeProperties) canvas.insertInorderBarrier();
    
    if (isShowingLayoutBounds()) onDebugDraw(canvas);

    if (clipToPadding) {
        while(clipSaveCount--)canvas.restore();
    }

    // mGroupFlags might have been updated by drawChild()
    flags = mGroupFlags;

    if ((flags & FLAG_INVALIDATE_REQUIRED) == FLAG_INVALIDATE_REQUIRED) {
        invalidate(true);
    }

    if ((flags & FLAG_ANIMATION_DONE) == 0 && (flags & FLAG_NOTIFY_ANIMATION_LISTENER) == 0 &&
            mLayoutAnimationController->isDone() && !more) {
        // We want to erase the drawing cache and notify the listener after the
        // next frame is drawn because one extra invalidate() is caused by
        // drawChild() after the animation is over
        mGroupFlags |= FLAG_NOTIFY_ANIMATION_LISTENER;
        post([this](){notifyAnimationListener();});
    }
}

void ViewGroup::invalidateChild(View*child,Rect&dirty){
    if(mAttachInfo==nullptr) return;
    
    ViewGroup* parent=this;
    
    const bool drawAnimation = (child->mPrivateFlags & PFLAG_DRAW_ANIMATION) != 0;

    const bool isOpaque = child->isOpaque() && !drawAnimation 
                 && child->getAnimation() == nullptr && child->hasIdentityMatrix();//&& childMatrix.isIdentity();

    int opaqueFlag = isOpaque ? PFLAG_DIRTY_OPAQUE : PFLAG_DIRTY;

    int location[2] = {child->mLeft,child->mTop};

    if (child->mLayerType != LAYER_TYPE_NONE){
        mPrivateFlags |= PFLAG_INVALIDATED;
        mPrivateFlags &= ~PFLAG_DRAWING_CACHE_VALID;
    }

    Rect boundingRect = dirty;
    if(!child->hasIdentityMatrix()||(mGroupFlags & FLAG_SUPPORT_STATIC_TRANSFORMATIONS) != 0){
         Matrix transformMatrix;
         Matrix childMatrix = child->getMatrix();
         if((mGroupFlags & FLAG_SUPPORT_STATIC_TRANSFORMATIONS)!=0){
             Transformation t;
             const bool transformed = getChildStaticTransformation(child,&t);
             if(transformed){
                 transformMatrix = t.getMatrix();
                 if(!child->hasIdentityMatrix())
                      transformMatrix.multiply(transformMatrix,childMatrix);
             }else
                 transformMatrix = childMatrix;
         }else{
             transformMatrix = childMatrix;
         }
         transformMatrix.transform_rectangle((RectangleInt&)dirty);
         LOGV("(%d,%d,%d,%d)-->(%d,%d,%d,%d) rotation=%f",boundingRect.left,boundingRect.top,boundingRect.width,boundingRect.height,
                dirty.left,dirty.top,dirty.width,dirty.height,child->getRotation());
    }
	
    do {
        View*view=parent;
        if (drawAnimation) {
            if (view) {
                 view->mPrivateFlags |= PFLAG_DRAW_ANIMATION;
            }/* else if (parent) {
                 parent->mIsAnimating = true;
            }*/
        }

        // If the parent is dirty opaque or not dirty, mark it dirty with the opaque
        // flag coming from the child that initiated the invalidate
        if (view) {
            if ((view->mViewFlags & FADING_EDGE_MASK) != 0 && view->getSolidColor() == 0) {
                  opaqueFlag = PFLAG_DIRTY;
            }
            if ((view->mPrivateFlags & PFLAG_DIRTY_MASK) != PFLAG_DIRTY) {
                view->mPrivateFlags = (view->mPrivateFlags & ~PFLAG_DIRTY_MASK) | opaqueFlag;
            }
        }

        parent = parent->invalidateChildInParent(location, dirty);
        if ( view && !view->hasIdentityMatrix() ) { // Account for transform on current parent
            Matrix m = view->getMatrix();
            m.transform_rectangle((RectangleInt&)dirty);
        }
    } while (parent);

    //set invalidate region to rootview
    if(child->isTemporarilyDetached()==false){
        ViewGroup*root = getRootView();
        dirty.intersect(0,0,root->getWidth(),root->getHeight());
        root->mInvalidRgn->do_union((const RectangleInt&)dirty);
    }
}

ViewGroup*ViewGroup::invalidateChildInParent(int* location, Rect& dirty){
    if (1||(mPrivateFlags & (PFLAG_DRAWN | PFLAG_DRAWING_CACHE_VALID)) != 0) {
        // either DRAWN, or DRAWING_CACHE_VALID
        if ((mGroupFlags & (FLAG_OPTIMIZE_INVALIDATE | FLAG_ANIMATION_DONE)) != FLAG_OPTIMIZE_INVALIDATE) {
            dirty.offset(location[CHILD_LEFT_INDEX] - mScrollX,location[CHILD_TOP_INDEX] - mScrollY);
            if ((mGroupFlags & FLAG_CLIP_CHILDREN) == 0) {
                dirty.Union(0, 0, mRight - mLeft,mBottom - mTop);
            }

            if ((mGroupFlags & FLAG_CLIP_CHILDREN) == FLAG_CLIP_CHILDREN) {
                if (!dirty.intersect(0, 0, mRight - mLeft,mBottom - mTop)) {
                    dirty.setEmpty();
                }
            }

            location[CHILD_LEFT_INDEX]= mLeft;
            location[CHILD_TOP_INDEX] = mTop;
        } else {
            if ((mGroupFlags & FLAG_CLIP_CHILDREN) == FLAG_CLIP_CHILDREN) {
                dirty.set(0, 0, mRight - mLeft,mBottom - mTop);
            } else {
                // in case the dirty rect extends outside the bounds of this container
                dirty.Union(0, 0, mRight - mLeft,mBottom - mTop);
            }
            location[CHILD_LEFT_INDEX]= mLeft;
            location[CHILD_TOP_INDEX] = mTop;

            mPrivateFlags &= ~PFLAG_DRAWN;
        }
        mPrivateFlags &= ~PFLAG_DRAWING_CACHE_VALID;
        if (mLayerType != LAYER_TYPE_NONE) 
            mPrivateFlags |= PFLAG_INVALIDATED;

        return mParent;
    }
    return nullptr;
}

void ViewGroup::resetResolvedPadding() {
    View::resetResolvedPadding();

    for (View*child:mChildren) {
        if (child->isLayoutDirectionInherited()) {
            child->resetResolvedPadding();
        }
    }
}

void ViewGroup::resetResolvedDrawables() {
    View::resetResolvedDrawables();

    for (View*child:mChildren) {
        if (child->isLayoutDirectionInherited()) {
            child->resetResolvedDrawables();
        }
    }
}

bool ViewGroup::shouldDelayChildPressedState(){
    return true;
}

void ViewGroup::unFocus(View* focused){
   if (mFocused == nullptr) {
         View::unFocus(focused);
   } else {
         mFocused->unFocus(focused);
         mFocused = nullptr;
   }
}

View*ViewGroup::getFocusedChild(){
    return mFocused;
}

View*ViewGroup::getDeepestFocusedChild(){
    View*v=this;
    while(v){
        if(v->isFocused())return v;
        if(dynamic_cast<ViewGroup*>(v))
            v=((ViewGroup*)v)->getFocusedChild();
        else v=nullptr;
    }
    return nullptr;
}

bool ViewGroup::hasFocus()const{
    return (mPrivateFlags&PFLAG_FOCUSED)||(mFocused!=nullptr);
}

View*ViewGroup::findFocus(){
    if (isFocused()) {
        return this;
    }

    if (mFocused != nullptr) {
        return mFocused->findFocus();
    }
    return nullptr;
}

bool ViewGroup::restoreDefaultFocus(){
    if (mDefaultFocus != nullptr
            && getDescendantFocusability() != FOCUS_BLOCK_DESCENDANTS
            && (mDefaultFocus->mViewFlags & VISIBILITY_MASK) == VISIBLE
            && mDefaultFocus->restoreDefaultFocus()) {
        return true;
    }
    return View::restoreDefaultFocus();
}

bool ViewGroup::hasFocusable(bool allowAutoFocus, bool dispatchExplicit)const{
    if ((mViewFlags & VISIBILITY_MASK) != VISIBLE) {
        return false;
    }

    // Only use effective focusable value when allowed.
    if ((allowAutoFocus || getFocusable() != FOCUSABLE_AUTO) && isFocusable()) {
        return true;
    }

    // Determine whether we have a focused descendant.
    int descendantFocusability = getDescendantFocusability();
    if (descendantFocusability != FOCUS_BLOCK_DESCENDANTS) {
        return hasFocusableChild(dispatchExplicit);
    }
    return false;
}

bool ViewGroup::hasFocusableChild(bool dispatchExplicit)const{
    for (auto child:mChildren){
        // In case the subclass has overridden has[Explicit]Focusable, dispatch
        // to the expected one for each child even though we share logic here.
        if ((dispatchExplicit && child->hasExplicitFocusable())
                || (!dispatchExplicit && child->hasFocusable())) {
            return true;
        }
    }
    return false;
}

void ViewGroup::setDefaultFocus(View* child){
    if (mDefaultFocus != nullptr && mDefaultFocus->isFocusedByDefault()) {
        return;
    }
    mDefaultFocus = child;
    if (mParent) mParent->setDefaultFocus(this);
}

void ViewGroup::clearDefaultFocus(View* child){
    if (mDefaultFocus != child && mDefaultFocus != nullptr
                && mDefaultFocus->isFocusedByDefault()) {
        return;
    }

    mDefaultFocus = nullptr;
    // Search child siblings for default focusables.
    for (View*sibling:mChildren) {
        if (sibling->isFocusedByDefault()) {
            mDefaultFocus = sibling;
            return;
        } else if (mDefaultFocus == nullptr && sibling->hasDefaultFocus()) {
            mDefaultFocus = sibling;
        }
    }

    if (mParent)mParent->clearDefaultFocus(this);
}

bool ViewGroup::hasDefaultFocus()const{
    return mDefaultFocus || View::hasDefaultFocus();
}

bool ViewGroup::onRequestFocusInDescendants(int direction,Rect* previouslyFocusedRect){
    int index;
    int increment;
    int end;
    int count = mChildren.size();
    if ((direction & FOCUS_FORWARD) != 0) {
        index = 0;
        increment = 1;
        end = count;
    } else {
        index = count - 1;
        increment = -1;
        end = -1;
    }
    for (int i = index; i != end; i += increment) {
        View* child = mChildren[i];
        if ((child->mViewFlags & VISIBILITY_MASK) == VISIBLE) {
            if (child->requestFocus(direction, previouslyFocusedRect)) {
                return true;
            }
        }
    }
    return false;
}

bool ViewGroup::requestChildRectangleOnScreen(View* child,Rect& rectangle, bool immediate) {
    return false;
}

void ViewGroup::requestChildFocus(View*child,View*focused){
    if (getDescendantFocusability() == FOCUS_BLOCK_DESCENDANTS) {
       return;
    }
    // Unfocus us, if necessary
    View::unFocus(focused);

    // We had a previous notion of who had focus. Clear it.
    if (mFocused != child) {
        if (mFocused != nullptr)
            mFocused->unFocus(focused);
        mFocused = child;
    }
    if (mParent != nullptr) 
        mParent->requestChildFocus(this, focused);
}

bool ViewGroup::hasPointerCapture()const{
    return mPointerCapture;
}

void ViewGroup::requestPointerCapture(bool){
    //TODO
}

void ViewGroup::handlePointerCaptureChanged(bool hasCapture) {
    if (mPointerCapture == hasCapture) {
        return;
    }
    mPointerCapture = hasCapture;
    /*if (mView != nullptr) {
        mView->dispatchPointerCaptureChanged(hasCapture);
    }*/
}

void ViewGroup::clearChildFocus(View* child){
    if (mFocused == nullptr) {
         View::clearFocus();
    } else {
         View* focused = mFocused;
         mFocused = nullptr;
         focused->clearFocus();
    }
}

void ViewGroup::focusableViewAvailable(View*v){
    if (mParent 
             // shortcut: don't report a new focusable view if we block our descendants from
             // getting focus or if we're not visible
             && (getDescendantFocusability() != FOCUS_BLOCK_DESCENDANTS)
             && ((mViewFlags & VISIBILITY_MASK) == VISIBLE)
             && (isFocusableInTouchMode() || !shouldBlockFocusForTouchscreen())
             // shortcut: don't report a new focusable view if we already are focused
             // (and we don't prefer our descendants)
             //
             // note: knowing that mFocused is non-null is not a good enough reason
             // to break the traversal since in that case we'd actually have to find
             // the focused view and make sure it wasn't FOCUS_AFTER_DESCENDANTS and
             // an ancestor of v; this will get checked for at ViewAncestor
             && !(isFocused() && getDescendantFocusability() != FOCUS_AFTER_DESCENDANTS)) {
         mParent->focusableViewAvailable(v);
     }
}

bool ViewGroup::getTouchscreenBlocksFocus()const{ 
    return mGroupFlags & FLAG_TOUCHSCREEN_BLOCKS_FOCUS;
}

bool ViewGroup::shouldBlockFocusForTouchscreen()const{
    return getTouchscreenBlocksFocus()&&!( isKeyboardNavigationCluster()&& (hasFocus()||(findKeyboardNavigationCluster() != this)));
}

bool ViewGroup::dispatchActivityResult(const std::string& who, int requestCode, int resultCode, Intent data){
    if (View::dispatchActivityResult(who, requestCode, resultCode, data)) {
        return true;
    }
    for (auto child:mChildren){
        if (child->dispatchActivityResult(who, requestCode, resultCode, data)) {
            return true;
        }
    }
    return false;
}

View* ViewGroup::focusSearch(View* focused, int direction){
    if (nullptr==mParent){//isRootNamespace()) {
        // root namespace means we should consider ourselves the top of the
        // tree for focus searching; otherwise we could be focus searching
        // into other tabs.  see LocalActivityManager and TabHost for more info.
        return FocusFinder::getInstance().findNextFocus((ViewGroup*)this, focused, direction);
    } else {
        return mParent->focusSearch(focused, direction);
    }
    return nullptr;
}

void ViewGroup::offsetDescendantRectToMyCoords(const View* descendant,Rect& rect)const{
    offsetRectBetweenParentAndChild(descendant, rect, true, false);
}

void ViewGroup::offsetRectIntoDescendantCoords(const View* descendant, Rect& rect)const{
    offsetRectBetweenParentAndChild(descendant, rect, false, false);
}

void ViewGroup::offsetRectBetweenParentAndChild(const View* descendant,Rect&rect,bool offsetFromChildToParent, bool clipToBounds)const{
    // already in the same coord system :)
    if (descendant == this) return;

    View* theParent = descendant->getParent();

        // search and offset up to the parent
    while ((theParent != nullptr) && (theParent != this)) {

        if (offsetFromChildToParent) {
            rect.offset(descendant->mLeft - descendant->mScrollX,
                    descendant->mTop - descendant->mScrollY);
            if (clipToBounds) {
               View* p =  theParent;
               bool intersected = rect.intersect(0, 0,p->getWidth(),p->getHeight());
               if (!intersected)  rect.setEmpty();
            }
        } else {
            if (clipToBounds) {
                View* p =theParent;
                bool intersected = rect.intersect(0, 0,p->getWidth(),p->getHeight());
                if (!intersected) rect.setEmpty();
            }
            rect.offset(descendant->mScrollX - descendant->mLeft,
                        descendant->mScrollY - descendant->mTop);
       }

       descendant = theParent;
       theParent = descendant->getParent();
   }

   // now that we are up to this view, need to offset one more time
   // to get into our coordinate space
   if (theParent == this) {
        if (offsetFromChildToParent) {
            rect.offset(descendant->mLeft - descendant->mScrollX,
                    descendant->mTop - descendant->mScrollY);
        } else {
            rect.offset(descendant->mScrollX - descendant->mLeft,
                    descendant->mScrollY - descendant->mTop);
        }
   } else {
         LOGE("parameter must be a descendant of this view");
   }
}

void ViewGroup::offsetChildrenTopAndBottom(int offset){
    for (auto v:mChildren) {
        v->setPos(v->mLeft,v->mTop + offset);
    }
}

void ViewGroup::offsetChildrenLeftAndRight(int offset){
    for (auto v:mChildren) {
        v->setPos(v->mLeft + offset,v->mTop);
    }
}

bool ViewGroup::showContextMenuForChild(View* originalView){
    return false;
}

bool ViewGroup::showContextMenuForChild(View* originalView, float x, float y){
    return false;
}

bool ViewGroup::getChildVisibleRect(View*child,Rect&r,Point*offset){
    return getChildVisibleRect(child,r,offset,false);
}

bool ViewGroup::getChildVisibleRect(View*child,Rect&r,Point*offset,bool forceParentCheck){
    // It doesn't make a whole lot of sense to call this on a view that isn't attached,
    // but for some simple tests it can be useful. If we don't have attach info this
    // will allocate memory.
    RectF rect;
    rect.set(r.left,r.top,r.width,r.height);

    if (!child->hasIdentityMatrix()) {
        child->getMatrix().transform_rectangle((Cairo::Rectangle&)rect);
    }

    const int dx = child->mLeft - mScrollX;
    const int dy = child->mTop - mScrollY;

    rect.offset(dx, dy);

    if (offset != nullptr) {
        if (!child->hasIdentityMatrix()) {
            double position[2];
            position[0] = offset->x;
            position[1] = offset->y;
            child->getMatrix().transform_point(position[0],position[1]);
            offset->x = std::round(position[0]);
            offset->y = std::round(position[1]);
        }
        offset->x += dx;
        offset->y += dy;
    }

    const int width = mRight - mLeft;
    const int height = mBottom - mTop;

    bool rectIsVisible = true;
    if (mParent == nullptr || mParent->getClipChildren()) {
        // Clip to bounds.
        rectIsVisible = rect.intersect(0, 0, width, height);
    }

    if ((forceParentCheck || rectIsVisible)  && (mGroupFlags & CLIP_TO_PADDING_MASK) == CLIP_TO_PADDING_MASK) {
        // Clip to padding.
        rectIsVisible = rect.intersect(mPaddingLeft, mPaddingTop,  
			width - mPaddingRight-mPaddingLeft, 
			height - mPaddingBottom-mPaddingTop);
    }

    if ((forceParentCheck || rectIsVisible) && (mClipBounds.empty()==false)) {
        // Clip to clipBounds.
        rectIsVisible = rect.intersect(mClipBounds.left, mClipBounds.top, mClipBounds.width, mClipBounds.height);
    }
    r.set((int) floor(rect.left), (int) floor(rect.top), (int) ceil(rect.width), (int) ceil(rect.height));

    if ((forceParentCheck || rectIsVisible) && mParent) {
        if (1/*mParent instanceof ViewGroup*/) {
            rectIsVisible = mParent->getChildVisibleRect(this, r, offset, forceParentCheck);
        } else {
            rectIsVisible = mParent->getChildVisibleRect(this, r, offset);
        }
    }
    return rectIsVisible;
}

bool ViewGroup::canAnimate()const{
    return mLayoutAnimationController!=nullptr;
}

void ViewGroup::startLayoutAnimation() {
    if (mLayoutAnimationController) {
        mGroupFlags |= FLAG_RUN_ANIMATION;
        requestLayout();
    }
}

void ViewGroup::scheduleLayoutAnimation() {
    mGroupFlags |= FLAG_RUN_ANIMATION;
}

void ViewGroup::setLayoutAnimation(LayoutAnimationController* controller) {
    mLayoutAnimationController = controller;
    if (mLayoutAnimationController) {
        mGroupFlags |= FLAG_RUN_ANIMATION;
    }
}

LayoutAnimationController*ViewGroup::getLayoutAnimation() {
    return mLayoutAnimationController;
}

void ViewGroup::setLayoutAnimationListener(Animation::AnimationListener animationListener){
    mAnimationListener = animationListener;
}

void ViewGroup::requestTransitionStart(LayoutTransition* transition){
    ViewGroup*root = getRootView();
    if(root)root->requestTransitionStart(transition);
}

Animation::AnimationListener ViewGroup::getLayoutAnimationListener(){
    return mAnimationListener;
}

void ViewGroup::setLayoutTransition(LayoutTransition* transition) {
    if (mTransition != nullptr) {
        mTransition->cancel();
        mTransition->removeTransitionListener(mLayoutTransitionListener);
    }
    mTransition = transition;
    if (mTransition != nullptr) {
        mTransition->addTransitionListener(mLayoutTransitionListener);
    }
}

LayoutTransition* ViewGroup::getLayoutTransition()const{
    return mTransition;
}

void ViewGroup::bindLayoutAnimation(View* child){
    Animation* a = mLayoutAnimationController->getAnimationForView(child);
    child->setAnimation(a);
}

void ViewGroup::attachLayoutAnimationParameters(View* child,LayoutParams* params, int index, int count) {
    LayoutAnimationController::AnimationParameters* animationParams = params->layoutAnimationParameters;
    if (animationParams == nullptr) {
        animationParams = new LayoutAnimationController::AnimationParameters();
        params->layoutAnimationParameters = animationParams;
    }

    animationParams->count = count;
    animationParams->index = index;
}

void ViewGroup::notifyAnimationListener(){
    mGroupFlags &= ~FLAG_NOTIFY_ANIMATION_LISTENER;
    mGroupFlags |= FLAG_ANIMATION_DONE;

    if (mAnimationListener.onAnimationEnd){
        post([this](){
            mAnimationListener.onAnimationEnd(*mLayoutAnimationController->getAnimation());
        });
    }
    invalidate(true);
}

void ViewGroup::addFocusables(std::vector<View*>& views, int direction, int focusableMode){
    int focusableCount = views.size();

    const int descendantFocusability = getDescendantFocusability();
    const bool blockFocusForTouchscreen = shouldBlockFocusForTouchscreen();
    const bool focusSelf = (isFocusableInTouchMode() || !blockFocusForTouchscreen);

    if (descendantFocusability == FOCUS_BLOCK_DESCENDANTS) {
        if (focusSelf) {
            View::addFocusables(views, direction, focusableMode);
        }
        return;
    }

    if (blockFocusForTouchscreen) {
        focusableMode |= FOCUSABLES_TOUCH_MODE;
    }

    if ((descendantFocusability == FOCUS_BEFORE_DESCENDANTS) && focusSelf) {
        View::addFocusables(views, direction, focusableMode);
    }

    std::vector<View*> children;
    for(auto c:mChildren){
        if(c->getVisibility()==View::VISIBLE)
           children.push_back(c);
    };
    FocusFinder::sort(children, 0, children.size(), this, isLayoutRtl());
    for (int i = 0; i < children.size(); ++i) {
        children[i]->addFocusables(views, direction, focusableMode);
    }

    // When set to FOCUS_AFTER_DESCENDANTS, we only add ourselves if
    // there aren't any focusable descendants.  this is
    // to avoid the focus search finding layouts when a more precise search
    // among the focusable children would be more interesting.
    if ((descendantFocusability == FOCUS_AFTER_DESCENDANTS) && focusSelf
            && focusableCount == views.size()) {
        View::addFocusables(views, direction, focusableMode);
    }
}

void ViewGroup::clearFocusedInCluster(){
    View* top = findKeyboardNavigationCluster();
    ViewGroup* parent = this;
    do {
        parent->mFocusedInCluster = nullptr;
        if (parent == top)break;
        parent = parent->getParent();
    } while (parent);
}

void ViewGroup::clearFocusedInCluster(View* child) {
    if (mFocusedInCluster != child) {
        return;
    }
    clearFocusedInCluster();
}

void ViewGroup::addKeyboardNavigationClusters(std::vector<View*>&views,int direction){
    int focusableCount = views.size();

    if (isKeyboardNavigationCluster()) {
        // Cluster-navigation can enter a touchscreenBlocksFocus cluster, so temporarily
        // disable touchscreenBlocksFocus to evaluate whether it contains focusables.
        bool blockedFocus = getTouchscreenBlocksFocus();
        setTouchscreenBlocksFocusNoRefocus(false);
        View::addKeyboardNavigationClusters(views, direction);
        setTouchscreenBlocksFocusNoRefocus(blockedFocus);
    } else {
        View::addKeyboardNavigationClusters(views, direction);
    }

    if (focusableCount != views.size()) {
        // No need to look for groups inside a group.
        return;
    }

    if (getDescendantFocusability() == FOCUS_BLOCK_DESCENDANTS) {
        return;
    }

    int count = 0;
    std::vector<View*>visibleChildren;
    for (View*child:mChildren){
        if ((child->mViewFlags & VISIBILITY_MASK) == VISIBLE) {
            visibleChildren.push_back(child);
        }
    }
    FocusFinder::sort(visibleChildren, 0, count, this, isLayoutRtl());
    for (View*view:visibleChildren){
        view->addKeyboardNavigationClusters(views, direction);
    }
}

void ViewGroup::setTouchscreenBlocksFocus(bool touchscreenBlocksFocus){
    if (touchscreenBlocksFocus) {
        mGroupFlags |= FLAG_TOUCHSCREEN_BLOCKS_FOCUS;
        if (hasFocus() && !isKeyboardNavigationCluster()) {
            View* focusedChild = getDeepestFocusedChild();
            if (!focusedChild->isFocusableInTouchMode()) {
                View* newFocus = View::focusSearch((int)FOCUS_FORWARD);
                if (newFocus) newFocus->requestFocus();
            }
        }
    } else {
        mGroupFlags &= ~FLAG_TOUCHSCREEN_BLOCKS_FOCUS;
    }    
}

void ViewGroup::setTouchscreenBlocksFocusNoRefocus(bool touchscreenBlocksFocus) {
    if (touchscreenBlocksFocus) {
        mGroupFlags |= FLAG_TOUCHSCREEN_BLOCKS_FOCUS;
    } else {
        mGroupFlags &= ~FLAG_TOUCHSCREEN_BLOCKS_FOCUS;
    }
}

void ViewGroup::transformPointToViewLocal(float point[2],View&child) {
     point[0] += mScrollX - child.mLeft;
     point[1] += mScrollY - child.mTop;
     double x= point[0];
     double y= point[1];
     if (!child.hasIdentityMatrix()) {
         child.getInverseMatrix().transform_point(x,y);
         point[0]=x;
         point[1]=y;
     }
}

int ViewGroup::getLayoutMode(){
    if(mLayoutMode==LAYOUT_MODE_UNDEFINED){
        const int inheritedLayoutMode=mParent?mParent->getLayoutMode():LAYOUT_MODE_DEFAULT;
        setLayoutMode(inheritedLayoutMode,false);
    }
    return mLayoutMode;
}

void ViewGroup::setLayoutMode(int layoutMode) {
    if (mLayoutMode != layoutMode) {
        invalidateInheritedLayoutMode(layoutMode);
        setLayoutMode(layoutMode, layoutMode != LAYOUT_MODE_UNDEFINED);
        requestLayout();
    }
}

void ViewGroup::setLayoutMode(int layoutMode, bool explicitly) {
    mLayoutMode = layoutMode;
    setBooleanFlag(FLAG_LAYOUT_MODE_WAS_EXPLICITLY_SET, explicitly);
}

void ViewGroup::invalidateInheritedLayoutMode(int layoutModeOfRoot){
    if (mLayoutMode == LAYOUT_MODE_UNDEFINED ||
        mLayoutMode == layoutModeOfRoot ||
        hasBooleanFlag(FLAG_LAYOUT_MODE_WAS_EXPLICITLY_SET)) {
        return;
    }
    setLayoutMode(LAYOUT_MODE_UNDEFINED, false);
    // apply recursively
    for (auto child:mChildren) {
        child->invalidateInheritedLayoutMode(layoutModeOfRoot);
    }
}

bool ViewGroup::isTransformedTouchPointInView(float x,float y, View& child,Point*outLocalPoint) {
    float point[2] ={x,y};
    transformPointToViewLocal(point,child);
    const bool isInView=child.pointInView(point[0],point[1],0);
    if(isInView && outLocalPoint != nullptr) {
        outLocalPoint->set(x, y);
    }
    return isInView;
}

bool ViewGroup::onStartNestedScroll(View* child, View* target, int nestedScrollAxes){
    return false;
}

void ViewGroup::onNestedScrollAccepted(View* child, View* target, int axes){
    mNestedScrollAxes = axes;
}

void ViewGroup::onStopNestedScroll(View* child){
    // Stop any recursive nested scrolling.
    stopNestedScroll();
    mNestedScrollAxes = 0;
}

void ViewGroup::onNestedScroll(View* target, int dxConsumed, int dyConsumed,int dxUnconsumed, int dyUnconsumed){
    // Re-dispatch up the tree by default
    dispatchNestedScroll(dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed, nullptr);
}

void ViewGroup::onNestedPreScroll(View* target, int dx, int dy, int*consumed){
    dispatchNestedPreScroll(dx, dy, consumed, nullptr);
}

int  ViewGroup::getNestedScrollAxes()const{
    return mNestedScrollAxes;
}

bool ViewGroup::onNestedFling(View* target, float velocityX, float velocityY, bool consumed){
    return true;
}

bool ViewGroup::onNestedPreFling(View* target, float velocityX, float velocityY){
    return true;
}

void ViewGroup::setMotionEventSplittingEnabled(bool split) {
    // TODO Applications really shouldn't change this setting mid-touch event,
    // but perhaps this should handle that case and send ACTION_CANCELs to any child views
    // with gestures in progress when this is changed.
    if (split) {
        mGroupFlags |= FLAG_SPLIT_MOTION_EVENTS;
    } else {
        mGroupFlags &= ~FLAG_SPLIT_MOTION_EVENTS;
    }
}

bool ViewGroup::isMotionEventSplittingEnabled()const{
   return (mGroupFlags & FLAG_SPLIT_MOTION_EVENTS) == FLAG_SPLIT_MOTION_EVENTS;
}

bool ViewGroup::isTransitionGroup() {
    if ((mGroupFlags & FLAG_IS_TRANSITION_GROUP_SET) != 0) {
        return ((mGroupFlags & FLAG_IS_TRANSITION_GROUP) != 0);
    } else {
        /*ViewOutlineProvider outlineProvider = getOutlineProvider();
        return getBackground() != null || getTransitionName() != null ||
                (outlineProvider != null && outlineProvider != ViewOutlineProvider.BACKGROUND);*/
        return false;
    }
}

void ViewGroup::setTransitionGroup(bool isTransitionGroup) {
    mGroupFlags |= FLAG_IS_TRANSITION_GROUP_SET;
    if (isTransitionGroup) {
        mGroupFlags |= FLAG_IS_TRANSITION_GROUP;
    } else {
        mGroupFlags &= ~FLAG_IS_TRANSITION_GROUP;
    }
}

void ViewGroup::requestDisallowInterceptTouchEvent(bool disallowIntercept){
    if (disallowIntercept == ((mGroupFlags & FLAG_DISALLOW_INTERCEPT) != 0)) {
        // We're already in this state, assume our ancestors are too
        return;
    }

    if (disallowIntercept) {
        mGroupFlags |= FLAG_DISALLOW_INTERCEPT;
    } else {
        mGroupFlags &= ~FLAG_DISALLOW_INTERCEPT;
    }

    // Pass it up to our parent
    if (mParent) mParent->requestDisallowInterceptTouchEvent(disallowIntercept);
}

bool ViewGroup::onInterceptTouchEvent(MotionEvent& ev){
    if( ev.isFromSource(InputDevice::SOURCE_MOUSE)
        && (ev.getAction() == MotionEvent::ACTION_DOWN)
        && ev.isButtonPressed(MotionEvent::BUTTON_PRIMARY)
        && isOnScrollbarThumb(ev.getX(), ev.getY()) )
        return true;
    return false; 
}

void ViewGroup::onDescendantInvalidated(View* child,View* target){
    mPrivateFlags |= (target->mPrivateFlags & PFLAG_DRAW_ANIMATION);

    if ((target->mPrivateFlags & ~PFLAG_DIRTY_MASK) != 0) {
        // We lazily use PFLAG_DIRTY, since computing opaque isn't worth the potential
        // optimization in provides in a DisplayList world.
        mPrivateFlags = (mPrivateFlags & ~PFLAG_DIRTY_MASK) | PFLAG_DIRTY;

        // simplified invalidateChildInParent behavior: clear cache validity to be safe...
        mPrivateFlags &= ~PFLAG_DRAWING_CACHE_VALID;
    }

    // ... and mark inval if in software layer that needs to repaint (hw handled in native)
    if (mLayerType == LAYER_TYPE_SOFTWARE) {
        // Layered parents should be invalidated. Escalate to a full invalidate (and note that
        // we do this after consuming any relevant flags from the originating descendant)
        mPrivateFlags |= PFLAG_INVALIDATED | PFLAG_DIRTY;
        target = this;
    }

    if (mParent != nullptr) {
        mParent->onDescendantInvalidated(this, target);
    }
}

bool ViewGroup::dispatchKeyEvent(KeyEvent&event){
    if (mInputEventConsistencyVerifier)
        mInputEventConsistencyVerifier->onKeyEvent(event, 1);
    if ((mPrivateFlags & (PFLAG_FOCUSED | PFLAG_HAS_BOUNDS))
          == (PFLAG_FOCUSED | PFLAG_HAS_BOUNDS)) {
        if (View::dispatchKeyEvent(event)) {
            return true;
        }
    } else if (mFocused != nullptr && (mFocused->mPrivateFlags & PFLAG_HAS_BOUNDS)
            == PFLAG_HAS_BOUNDS) {
        if (mFocused->dispatchKeyEvent(event)) {
            return true;
        }
    }
    if (mInputEventConsistencyVerifier)
        mInputEventConsistencyVerifier->onUnhandledEvent(event, 1);
    return View::dispatchKeyEvent(event);
}

MotionEvent* ViewGroup::getTransformedMotionEvent(MotionEvent& event, View* child)const{
    const float offsetX = mScrollX - child->mLeft;
    const float offsetY = mScrollY - child->mTop;
    MotionEvent* transformedEvent = MotionEvent::obtain(event);
    transformedEvent->offsetLocation(offsetX, offsetY);
    if (!child->hasIdentityMatrix()) {
        transformedEvent->transform(child->getInverseMatrix());
    }
    return transformedEvent;
}

bool ViewGroup::dispatchTouchEvent(MotionEvent&ev){
    const int action = ev.getAction();
    const int actionMasked=ev.getActionMasked();
    const float xf = ev.getX();
    const float yf = ev.getY();
    const float scrolledXFloat = xf + mScrollX;
    const float scrolledYFloat = yf + mScrollY;

    if (mInputEventConsistencyVerifier)
        mInputEventConsistencyVerifier->onTouchEvent(ev, 1);

    if (ev.isTargetAccessibilityFocus() && isAccessibilityFocusedViewOrHost()) {
         ev.setTargetAccessibilityFocus(false);
    }

    bool handled = false;
    if(onFilterTouchEventForSecurity(ev)){
        if (actionMasked == MotionEvent::ACTION_DOWN) {
            cancelAndClearTouchTargets(&ev);
            resetTouchState();
        }
        // Check for interception.
        bool intercepted=false;
        if((actionMasked == MotionEvent::ACTION_DOWN)||mFirstTouchTarget){
            const bool disallowIntercept = (mGroupFlags & FLAG_DISALLOW_INTERCEPT) != 0;
            if (!disallowIntercept) {
                intercepted = onInterceptTouchEvent(ev);
                ev.setAction(action); // restore action in case it was changed
            }
        }else{
            intercepted=true;
        }
        
        if (intercepted || mFirstTouchTarget != nullptr) {
             ev.setTargetAccessibilityFocus(false);
        }
        // Check for cancelation.
        const  bool canceled = resetCancelNextUpFlag(this)|| actionMasked == MotionEvent::ACTION_CANCEL;

        const bool split = (mGroupFlags & FLAG_SPLIT_MOTION_EVENTS) != 0;
        TouchTarget* newTouchTarget = nullptr;
        bool alreadyDispatchedToNewTouchTarget = false;

        if(!canceled && !intercepted){
            if( (actionMasked == MotionEvent::ACTION_DOWN) || (split && (actionMasked == MotionEvent::ACTION_POINTER_DOWN))
               || (actionMasked == MotionEvent::ACTION_HOVER_MOVE) ){
                const int actionIndex = ev.getActionIndex(); // always 0 for down
                const int idBitsToAssign = split ? (1 << ev.getPointerId(actionIndex)) : TouchTarget::ALL_POINTER_IDS;

                removePointersFromTouchTargets(idBitsToAssign);
                const int childrenCount = mChildren.size();
                if ((newTouchTarget == nullptr) && childrenCount){
                    const int x = ev.getXDispatchLocation(actionIndex);
                    const int y = ev.getYDispatchLocation(actionIndex);

                    std::vector<View*>preorderedList = buildTouchDispatchChildList();
                    const bool customOrder = preorderedList.empty() && isChildrenDrawingOrderEnabled();
                    std::vector<View*>&children = mChildren;
                    for(int i = childrenCount-1;i >= 0;i--){
                        const int childIndex = getAndVerifyPreorderedIndex(childrenCount, i, customOrder);
                        View* child = getAndVerifyPreorderedView(preorderedList, children, childIndex);
                        if (!canViewReceivePointerEvents(*child) || !isTransformedTouchPointInView(x, y,*child, nullptr)) {
                            ev.setTargetAccessibilityFocus(false);
                            continue;
                        }

                        newTouchTarget = getTouchTarget(child);
                        if (newTouchTarget) {
                            // Child is already receiving touch within its bounds.
                            // Give it the new pointer in addition to the ones it is handling.
                            newTouchTarget->pointerIdBits |= idBitsToAssign;
                            break;
                        }

                        resetCancelNextUpFlag(child);
                        if(dispatchTransformedTouchEvent(ev, false, child, idBitsToAssign)){
                            mLastTouchDownTime = ev.getDownTime();
                            if(preorderedList.size()){
                                for(int j=0;j<childrenCount;j++){
                                    if(children[childIndex]==mChildren[j]){
                                        mLastTouchDownIndex=j; break;
                                    }
                                } 
                            }else{
                                mLastTouchDownIndex = childIndex;
                            }
                            mLastTouchDownX = ev.getX() ;
                            mLastTouchDownY = ev.getY() ;
                            newTouchTarget = addTouchTarget(child,idBitsToAssign);
                            alreadyDispatchedToNewTouchTarget = true;
                            break;
                        }
                        ev.setTargetAccessibilityFocus(false);
                    }
                    preorderedList.clear();
                }
                if ( (newTouchTarget == nullptr) && mFirstTouchTarget) {
                    // Did not find a child to receive the event.
                    // Assign the pointer to the least recently added target.
                    newTouchTarget = mFirstTouchTarget;
                    while (newTouchTarget->next) {
                        newTouchTarget = newTouchTarget->next;
                    }
                    newTouchTarget->pointerIdBits |= idBitsToAssign;
                }
            }
        }
        
        // Dispatch to touch targets.
        if (mFirstTouchTarget == nullptr){
            handled = dispatchTransformedTouchEvent(ev, canceled, nullptr,TouchTarget::ALL_POINTER_IDS);
        }else{
            TouchTarget* predecessor = nullptr;
            TouchTarget* target = mFirstTouchTarget;
            while (target ) {
                TouchTarget* next = target->next;
                if (alreadyDispatchedToNewTouchTarget && (target == newTouchTarget) ) {
                    handled = true;
                } else {
                    const bool cancelChild = resetCancelNextUpFlag(target->child)|| intercepted;
                    if (dispatchTransformedTouchEvent(ev, cancelChild, target->child, target->pointerIdBits)) {
                        handled = true;
                    }
                    if (cancelChild) {
                       if (predecessor == nullptr) mFirstTouchTarget = next;
                       else  predecessor->next = next;
                       target->recycle();
                       target = next;
                       continue;
                    }
               }
               predecessor = target;
               target = next;
           }
        } 

        // Update list of touch targets for pointer up or cancel, if needed.
        if (canceled || (actionMasked == MotionEvent::ACTION_UP)
            || (actionMasked == MotionEvent::ACTION_HOVER_MOVE) ) {
            resetTouchState();
        } else if (split && (actionMasked == MotionEvent::ACTION_POINTER_UP) ) {
            int actionIndex = ev.getActionIndex();
            int idBitsToRemove = 1 << ev.getPointerId(actionIndex);
            removePointersFromTouchTargets(idBitsToRemove);
        } 
    }
    if (!handled && mInputEventConsistencyVerifier)
        mInputEventConsistencyVerifier->onUnhandledEvent(ev, 1);
    return handled;
}

bool ViewGroup::dispatchHoverEvent(MotionEvent&event){
    bool handled = false;
    int action = event.getAction();
    // First check whether the view group wants to intercept the hover event.
    bool interceptHover = onInterceptHoverEvent(event);
    event.setAction(action); // restore action in case it was changed

    MotionEvent* eventNoHistory = &event;
    // Send events to the hovered children and build a new list of hover targets until
    // one is found that handles the event.
    HoverTarget* firstOldHoverTarget = mFirstHoverTarget;
    mFirstHoverTarget = nullptr;
    if (!interceptHover && action != MotionEvent::ACTION_HOVER_EXIT) {
        float x = event.getXDispatchLocation(0);
        float y = event.getYDispatchLocation(0);
        int childrenCount = mChildren.size();
        if (childrenCount != 0) {
            std::vector<View*> preorderedList = buildOrderedChildList();
            bool customOrder = preorderedList.empty() && isChildrenDrawingOrderEnabled();
            HoverTarget* lastHoverTarget = nullptr;
            for (int i = childrenCount - 1; i >= 0; i--) {
                int childIndex = getAndVerifyPreorderedIndex(childrenCount, i, customOrder);
                View* child = getAndVerifyPreorderedView(preorderedList, mChildren, childIndex);
                if (!child->canReceivePointerEvents() || !isTransformedTouchPointInView(x, y, *child, nullptr)) {
                    continue;
                }

                // Obtain a hover target for this child.  Dequeue it from the
                // old hover target list if the child was previously hovered.
                HoverTarget* hoverTarget = firstOldHoverTarget;
                bool wasHovered;
                for (HoverTarget* predecessor = nullptr; ;) {
                    if (hoverTarget == nullptr) {
                        hoverTarget = HoverTarget::obtain(child);
                        wasHovered = false;
                        break;
                    }

                    if (hoverTarget->child == child) {
                        if (predecessor != nullptr) {
                            predecessor->next = hoverTarget->next;
                        } else {
                            firstOldHoverTarget = hoverTarget->next;
                        }
                        hoverTarget->next = nullptr;
                        wasHovered = true;
                        break;
                    }

                    predecessor = hoverTarget;
                    hoverTarget = hoverTarget->next;
                }

                // Enqueue the hover target onto the new hover target list.
                if (lastHoverTarget != nullptr) {
                    lastHoverTarget->next = hoverTarget;
                } else {
                    mFirstHoverTarget = hoverTarget;
                }
                lastHoverTarget = hoverTarget;

                // Dispatch the event to the child.
                if (action == MotionEvent::ACTION_HOVER_ENTER) {
                    if (!wasHovered) { // Send the enter as is.
                        handled |= dispatchTransformedGenericPointerEvent(event, child); // enter
                    }
                } else if (action == MotionEvent::ACTION_HOVER_MOVE) {
                    if (!wasHovered) { // Synthesize an enter from a move.
                        eventNoHistory = obtainMotionEventNoHistoryOrSelf(eventNoHistory);
                        eventNoHistory->setAction(MotionEvent::ACTION_HOVER_ENTER);
                        handled |= dispatchTransformedGenericPointerEvent(*eventNoHistory, child); // enter
                        eventNoHistory->setAction(action);

                        handled |= dispatchTransformedGenericPointerEvent(*eventNoHistory, child); // move
                    } else { // Send the move as is.
                        handled |= dispatchTransformedGenericPointerEvent(event, child);
                    }
                }
                if (handled) {
                    break;
                }
            }
            preorderedList.clear();
        }
    }

    // Send exit events to all previously hovered children that are no longer hovered.
    while (firstOldHoverTarget != nullptr) {
        View* child = firstOldHoverTarget->child;

        // Exit the old hovered child.
        if (action == MotionEvent::ACTION_HOVER_EXIT) { // Send the exit as is.
            handled |= dispatchTransformedGenericPointerEvent(event, child); // exit
        } else {
            // Synthesize an exit from a move or enter.
            // Ignore the result because hover focus has moved to a different view.
            if (action == MotionEvent::ACTION_HOVER_MOVE) {
                bool hoverExitPending = event.isHoverExitPending();
                event.setHoverExitPending(true);
                dispatchTransformedGenericPointerEvent(event, child); // move
                event.setHoverExitPending(hoverExitPending);
            }
            eventNoHistory = obtainMotionEventNoHistoryOrSelf(eventNoHistory);
            eventNoHistory->setAction(MotionEvent::ACTION_HOVER_EXIT);
            dispatchTransformedGenericPointerEvent(*eventNoHistory, child); // exit
            eventNoHistory->setAction(action);
        }

        HoverTarget* nextOldHoverTarget = firstOldHoverTarget->next;
        firstOldHoverTarget->recycle();
        firstOldHoverTarget = nextOldHoverTarget;
    }

    // Send events to the view group itself if no children have handled it and the view group
    // itself is not currently being hover-exited.
    const bool newHoveredSelf = !handled && (action != MotionEvent::ACTION_HOVER_EXIT) && !event.isHoverExitPending();
    if (newHoveredSelf == mHoveredSelf) {
        if (newHoveredSelf) {
            // Send event to the view group as before.
            handled |= View::dispatchHoverEvent(event);
        }
    } else {
        if (mHoveredSelf) {
            // Exit the view group.
            if (action == MotionEvent::ACTION_HOVER_EXIT) {
                // Send the exit as is.
                handled |= View::dispatchHoverEvent(event); // exit
            } else {
                // Synthesize an exit from a move or enter.
                // Ignore the result because hover focus is moving to a different view.
                if (action == MotionEvent::ACTION_HOVER_MOVE) {
                    View::dispatchHoverEvent(event); // move
                }
                eventNoHistory = obtainMotionEventNoHistoryOrSelf(eventNoHistory);
                eventNoHistory->setAction(MotionEvent::ACTION_HOVER_EXIT);
                View::dispatchHoverEvent(*eventNoHistory); // exit
                eventNoHistory->setAction(action);
            }
            mHoveredSelf = false;
        }

        if (newHoveredSelf) {
            // Enter the view group.
            if (action == MotionEvent::ACTION_HOVER_ENTER) {
                // Send the enter as is.
                handled |= View::dispatchHoverEvent(event); // enter
                mHoveredSelf = true;
            } else if (action == MotionEvent::ACTION_HOVER_MOVE) {
                // Synthesize an enter from a move.
                eventNoHistory = obtainMotionEventNoHistoryOrSelf(eventNoHistory);
                eventNoHistory->setAction(MotionEvent::ACTION_HOVER_ENTER);
                handled |= View::dispatchHoverEvent(*eventNoHistory); // enter
                eventNoHistory->setAction(action);

                handled |= View::dispatchHoverEvent(*eventNoHistory); // move
                mHoveredSelf = true;
            }
        }
    }
    // Recycle the copy of the event that we made.
    if (eventNoHistory != &event) {
        eventNoHistory->recycle();
    }
    return handled;
}

bool ViewGroup::onInterceptHoverEvent(MotionEvent& event) {
    if (event.isFromSource(InputDevice::SOURCE_MOUSE)) {
        const int action = event.getAction();
        const float x = event.getXDispatchLocation(0);
        const float y = event.getYDispatchLocation(0);
        if ((action == MotionEvent::ACTION_HOVER_MOVE
                || action == MotionEvent::ACTION_HOVER_ENTER) && isOnScrollbar(x, y)) {
            return true;
        }
    }
    return false;
}

MotionEvent* ViewGroup::obtainMotionEventNoHistoryOrSelf(MotionEvent* event) {
    if (event->getHistorySize() == 0) {
        return event;
    }
    return MotionEvent::obtainNoHistory(*event);
}
void ViewGroup::dispatchDetachedFromWindow(){
    // If we still have a touch target, we are still in the process of
    // dispatching motion events to a child; we need to get rid of that
    // child to avoid dispatching events to it after the window is torn
    // down. To make sure we keep the child in a consistent state, we
    // first send it an ACTION_CANCEL motion event.
    cancelAndClearTouchTargets(nullptr);

    // Similarly, set ACTION_EXIT to all hover targets and clear them.
    exitHoverTargets();
    exitTooltipHoverTargets();

    // In case view is detached while transition is running
    mLayoutCalledWhileSuppressed = false;

    // Tear down our drag tracking
    mChildrenInterestedInDrag.clear();// = null;
    mIsInterestedInDrag = false;
    /*if (mCurrentDragStartEvent != null) {
        mCurrentDragStartEvent.recycle();
        mCurrentDragStartEvent = null;
    }*/

    for (View*child:mChildren) {
        child->dispatchDetachedFromWindow();
    }
    clearDisappearingChildren();
    //final int transientCount = mTransientViews == null ? 0 : mTransientIndices.size();
    for (View*view:mTransientViews){//int i = 0; i < transientCount; ++i) {
        view->dispatchDetachedFromWindow();
    }
    View::dispatchDetachedFromWindow();
}

void ViewGroup::dispatchCancelPendingInputEvents(){
    View::dispatchCancelPendingInputEvents();
    for (View*child:mChildren){
        child->dispatchCancelPendingInputEvents();
    }
}

void ViewGroup::internalSetPadding(int left, int top, int width, int height){
    View::internalSetPadding(left,top,width,height);
    if ((mPaddingLeft | mPaddingTop | mPaddingRight | mPaddingBottom) != 0) {
         mGroupFlags |= FLAG_PADDING_NOT_NULL;
    } else {
         mGroupFlags &= ~FLAG_PADDING_NOT_NULL;
    }
}

void ViewGroup::dispatchSaveInstanceState(SparseArray<Parcelable*>& container){
    View::dispatchSaveInstanceState(container);
    for (View*c:mChildren){
        if ((c->mViewFlags & PARENT_SAVE_DISABLED_MASK) != PARENT_SAVE_DISABLED) {
            c->dispatchSaveInstanceState(container);
        }
    }
}

void ViewGroup::dispatchFreezeSelfOnly(SparseArray<Parcelable*>& container){
    View::dispatchSaveInstanceState(container);
}

void ViewGroup::dispatchRestoreInstanceState(SparseArray<Parcelable*>& container){
    View::dispatchRestoreInstanceState(container);
    for (View*c:mChildren){
        if ((c->mViewFlags & PARENT_SAVE_DISABLED_MASK) != PARENT_SAVE_DISABLED) {
            c->dispatchRestoreInstanceState(container);
        }
    }
}

void ViewGroup::dispatchThawSelfOnly(SparseArray<Parcelable*>& container){
    View::dispatchRestoreInstanceState(container);
}

bool ViewGroup::dispatchVisibilityAggregated(bool isVisible) {
    isVisible = View::dispatchVisibilityAggregated(isVisible);
    for (View*child:mChildren){
        // Only dispatch to visible children. Not visible children and their subtrees already
        // know that they aren't visible and that's not going to change as a result of
        // whatever triggered this dispatch.
        if (child->getVisibility() == VISIBLE) {
            child->dispatchVisibilityAggregated(isVisible);
        }
    }
    return isVisible;
}

void ViewGroup::recomputeViewAttributes(View* child) {
    if (mAttachInfo && !mAttachInfo->mRecomputeGlobalAttributes) {
        ViewGroup* parent = mParent;
        if (parent != nullptr) parent->recomputeViewAttributes(this);
    }
}

void ViewGroup::bringChildToFront(View* child) {
    const int index = indexOfChild(child);
    if (index >= 0) {
        removeFromArray(index);
        addInArray(child, mChildren.size());
        child->mParent = this;
        requestLayout();
        invalidate();
    }
}

bool ViewGroup::requestLayoutDuringLayout(View* view){
    return true;
}

bool ViewGroup::requestFocus(int direction,Rect*previouslyFocusedRect){
    const int descendantFocusability = getDescendantFocusability();

    bool result,took;
    switch (descendantFocusability) {
    case FOCUS_BLOCK_DESCENDANTS:
        result = View::requestFocus(direction, previouslyFocusedRect);
        break;
    case FOCUS_BEFORE_DESCENDANTS:
        took = View::requestFocus(direction, previouslyFocusedRect);
        result = took ? took : onRequestFocusInDescendants(direction,
                       previouslyFocusedRect);
        break;
    case FOCUS_AFTER_DESCENDANTS:
        took = onRequestFocusInDescendants(direction, previouslyFocusedRect);
        result = took ? took : View::requestFocus(direction, previouslyFocusedRect);
        break;
    default:
        LOGW("descendant focusability must be one of FOCUS_BEFORE_DESCENDANTS,"
           " FOCUS_AFTER_DESCENDANTS, FOCUS_BLOCK_DESCENDANTS but is %x",descendantFocusability);
    }
    if (result && !isLayoutValid() && ((mPrivateFlags & PFLAG_WANTS_FOCUS) == 0)) {
        mPrivateFlags |= PFLAG_WANTS_FOCUS;
    }
    return result;
}

bool ViewGroup::restoreFocusNotInCluster(){
    if (mFocusedInCluster != nullptr) {
        // since clusters don't nest; we can assume that a non-null mFocusedInCluster
        // will refer to a view not-in a cluster.
        return restoreFocusInCluster(View::FOCUS_DOWN);
    }
    if (isKeyboardNavigationCluster() || (mViewFlags & VISIBILITY_MASK) != VISIBLE) {
        return false;
    }
    int descendentFocusability = getDescendantFocusability();
    if (descendentFocusability == FOCUS_BLOCK_DESCENDANTS) {
        return View::requestFocus(FOCUS_DOWN, nullptr);
    }
    if (descendentFocusability == FOCUS_BEFORE_DESCENDANTS
        && View::requestFocus(FOCUS_DOWN, nullptr)) {
        return true;
    }
    for (auto child:mChildren){
        if (!child->isKeyboardNavigationCluster()
                && child->restoreFocusNotInCluster()) {
            return true;
        }
    }
    if (descendentFocusability == FOCUS_AFTER_DESCENDANTS && !hasFocusableChild(false)) {
        return View::requestFocus(FOCUS_DOWN, nullptr);
    }
    return false;
}

View* ViewGroup::keyboardNavigationClusterSearch(View* currentCluster,int direction) {
    //checkThread();
    return FocusFinder::getInstance().findNextKeyboardNavigationCluster(
                getRootView()/*this*/, currentCluster, direction);
}

bool ViewGroup::performKeyboardGroupNavigation(int direction){
    View* focused = findFocus();
    if (focused == nullptr && restoreDefaultFocus()) {
        return true;
    }
    View*old=focused;
    View* cluster = focused == nullptr ? keyboardNavigationClusterSearch(nullptr, direction)
                    : focused->keyboardNavigationClusterSearch(nullptr, direction);

    LOGD("Focus changed %p:%d-->%p:%d",old,old?old->mID:-2,focused,focused?focused->mID:-2);
    // Since requestFocus only takes "real" focus directions (and therefore also
    // restoreFocusInCluster), convert forward/backward focus into FOCUS_DOWN.
    int realDirection = direction;
    if (direction == View::FOCUS_FORWARD || direction == View::FOCUS_BACKWARD) {
        realDirection = View::FOCUS_DOWN;
    }

    if (cluster && cluster->isRootNamespace()) {
        // the default cluster. Try to find a non-clustered view to focus.
        if (cluster->restoreFocusNotInCluster()) {
            playSoundEffect(SoundEffectConstants::getContantForFocusDirection(direction));
            return true;
        }
        // otherwise skip to next actual cluster
        cluster = keyboardNavigationClusterSearch(nullptr, direction);
    }

    if (cluster && cluster->restoreFocusInCluster(realDirection)) {
        playSoundEffect(SoundEffectConstants::getContantForFocusDirection(direction));
        return true;
    }
    return false;
}

static int isExcludedKeys(int key){
   return key==KeyEvent::KEYCODE_MENU||key==KeyEvent::KEYCODE_ESCAPE;
}

void ViewGroup::drawableStateChanged(){
    View::drawableStateChanged();

    if ((mGroupFlags & FLAG_NOTIFY_CHILDREN_ON_DRAWABLE_STATE_CHANGE) != 0) {
        if ((mGroupFlags & FLAG_ADD_STATES_FROM_CHILDREN) != 0) {
            LOGE("addStateFromChildren cannot be enabled if a"
                        " child has duplicateParentState set to true");
        }
        for (auto child:mChildren){
            if ((child->mViewFlags & DUPLICATE_PARENT_STATE) != 0) {
                child->refreshDrawableState();
            }
        }
    }
}

void ViewGroup::jumpDrawablesToCurrentState(){
    View::jumpDrawablesToCurrentState();
    for (auto child:mChildren){
        child->jumpDrawablesToCurrentState();
    }
}

std::vector<int> ViewGroup::onCreateDrawableState(){
    if ((mGroupFlags & FLAG_ADD_STATES_FROM_CHILDREN) == 0) {
        return View::onCreateDrawableState();
    }
    std::vector<int>state = View::onCreateDrawableState();
    const int N = getChildCount();
    for (int i = 0; i < N; i++) {
        std::vector<int> childState = getChildAt(i)->getDrawableState();
        state = mergeDrawableStates(state, childState);
    }
    return state;
}

void ViewGroup::setAddStatesFromChildren(bool addsStates) {
    if (addsStates) {
        mGroupFlags |= FLAG_ADD_STATES_FROM_CHILDREN;
    } else {
        mGroupFlags &= ~FLAG_ADD_STATES_FROM_CHILDREN;
    }
    refreshDrawableState();
}

bool ViewGroup::addStatesFromChildren() {
    return (mGroupFlags & FLAG_ADD_STATES_FROM_CHILDREN) != 0;
}

void ViewGroup::childDrawableStateChanged(View* child) {
    if ((mGroupFlags & FLAG_ADD_STATES_FROM_CHILDREN) != 0) {
        refreshDrawableState();
    }
}

}  // namespace ui
