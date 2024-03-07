#include <widget/nestedscrollview.h>
#include <widget/nestedscrollinghelper.h>
#include <view/focusfinder.h>

#define TYPE_TOUCH 0
#define TYPE_NON_TOUCH 1

namespace cdroid{

DECLARE_WIDGET2(NestedScrollView,"cdroid:attr/scrollViewStyle")

NestedScrollView::NestedScrollView(int w,int h):FrameLayout(w,h){
    initScrollView();
    LOGD("NestedScrollView cant scroll,do not use it");
}

NestedScrollView::NestedScrollView(Context* context,const AttributeSet&attrs):FrameLayout(context,attrs){
    initScrollView();
    LOGE("NestedScrollView cant scroll,do not use it");
}

NestedScrollView::~NestedScrollView(){
    delete mScroller;
    delete mParentHelper;
    delete mChildHelper;
    delete mEdgeGlowTop;
    delete mEdgeGlowBottom;
}

bool NestedScrollView::startNestedScroll(int axes, int type){
    return mChildHelper->startNestedScroll(axes, type);
}

void NestedScrollView::stopNestedScroll(int type){
    mChildHelper->stopNestedScroll(type);
}

bool NestedScrollView::hasNestedScrollingParent(int type) {
    return mChildHelper->hasNestedScrollingParent(type);
}

bool NestedScrollView::dispatchNestedScroll(int dxConsumed, int dyConsumed, int dxUnconsumed,
        int dyUnconsumed, int offsetInWindow[], int type){
    return mChildHelper->dispatchNestedScroll(dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed,
            offsetInWindow, type);
}

bool NestedScrollView::dispatchNestedPreScroll(int dx, int dy, int consumed[], int offsetInWindow[], int type) {
    return mChildHelper->dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, type);
}

void NestedScrollView::setNestedScrollingEnabled(bool enabled) {
    mChildHelper->setNestedScrollingEnabled(enabled);
}

bool NestedScrollView::isNestedScrollingEnabled() {
    return mChildHelper->isNestedScrollingEnabled();
}

bool NestedScrollView::startNestedScroll(int axes) {
    return startNestedScroll(axes, TYPE_TOUCH);
}

void NestedScrollView::stopNestedScroll() {
    stopNestedScroll(TYPE_TOUCH);
}

bool NestedScrollView::hasNestedScrollingParent() {
    return hasNestedScrollingParent(TYPE_TOUCH);
}

bool NestedScrollView::dispatchNestedScroll(int dxConsumed, int dyConsumed, int dxUnconsumed,
        int dyUnconsumed, int offsetInWindow[]) {
    return dispatchNestedScroll(dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed,
            offsetInWindow, TYPE_TOUCH);
}

bool NestedScrollView::dispatchNestedPreScroll(int dx, int dy, int consumed[], int offsetInWindow[]) {
    return dispatchNestedPreScroll(dx, dy, consumed, offsetInWindow, TYPE_TOUCH);
}

bool NestedScrollView::dispatchNestedFling(float velocityX, float velocityY, bool consumed) {
    return mChildHelper->dispatchNestedFling(velocityX, velocityY, consumed);
}

bool NestedScrollView::dispatchNestedPreFling(float velocityX, float velocityY) {
    return mChildHelper->dispatchNestedPreFling(velocityX, velocityY);
}

bool NestedScrollView::onStartNestedScroll(View* child,View* target, int axes, int type) {
    return (axes & View::SCROLL_AXIS_VERTICAL) != 0;
}

//NestedScrollingParent2

void NestedScrollView::onNestedScrollAccepted(View* child,View* target, int axes, int type) {
    mParentHelper->onNestedScrollAccepted(child, target, axes, type);
    startNestedScroll(View::SCROLL_AXIS_VERTICAL, type);
}


void NestedScrollView::onStopNestedScroll(View* target, int type) {
    mParentHelper->onStopNestedScroll(target, type);
    stopNestedScroll(type);
}


void NestedScrollView::onNestedScroll(View* target, int dxConsumed, int dyConsumed, int dxUnconsumed,
        int dyUnconsumed, int type) {
    const int oldScrollY = getScrollY();
    scrollBy(0, dyUnconsumed);
    const int myConsumed = getScrollY() - oldScrollY;
    const int myUnconsumed = dyUnconsumed - myConsumed;
    dispatchNestedScroll(0, myConsumed, 0, myUnconsumed, nullptr, type);
}


void NestedScrollView::onNestedPreScroll(View* target, int dx, int dy,int consumed[], int type) {
    dispatchNestedPreScroll(dx, dy, consumed, nullptr, type);
}

// NestedScrollingParent


bool NestedScrollView::onStartNestedScroll(View* child, View* target, int nestedScrollAxes) {
    return onStartNestedScroll(child, target, nestedScrollAxes, TYPE_TOUCH);
}


void NestedScrollView::onNestedScrollAccepted(View* child, View* target, int nestedScrollAxes) {
    onNestedScrollAccepted(child, target, nestedScrollAxes, TYPE_TOUCH);
}


void NestedScrollView::onStopNestedScroll(View* target) {
    onStopNestedScroll(target, TYPE_TOUCH);
}


void NestedScrollView::onNestedScroll(View* target, int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed) {
    onNestedScroll(target, dxConsumed, dyConsumed, dxUnconsumed, dyUnconsumed,TYPE_TOUCH);
}


void NestedScrollView::onNestedPreScroll(View* target, int dx, int dy, int consumed[]) {
    onNestedPreScroll(target, dx, dy, consumed, TYPE_TOUCH);
}


bool NestedScrollView::onNestedFling(View* target, float velocityX, float velocityY, bool consumed) {
    if (!consumed) {
        flingWithNestedDispatch((int) velocityY);
        return true;
    }
    return false;
}


bool NestedScrollView::onNestedPreFling(View* target, float velocityX, float velocityY) {
    return dispatchNestedPreFling(velocityX, velocityY);
}


int NestedScrollView::getNestedScrollAxes() {
    return mParentHelper->getNestedScrollAxes();
}

// ScrollView import


bool NestedScrollView::shouldDelayChildPressedState() {
    return true;
}


float NestedScrollView::getTopFadingEdgeStrength() {
    if (getChildCount() == 0) {
        return 0.0f;
    }

    int length = getVerticalFadingEdgeLength();
    int scrollY = getScrollY();
    if (scrollY < length) {
        return scrollY / (float) length;
    }

    return 1.0f;
}


float NestedScrollView::getBottomFadingEdgeStrength() {
    if (getChildCount() == 0) {
        return 0.0f;
    }

    View* child = getChildAt(0);
    NestedScrollView::LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
    int length = getVerticalFadingEdgeLength();
    int bottomEdge = getHeight() - getPaddingBottom();
    int span = child->getBottom() + lp->bottomMargin - getScrollY() - bottomEdge;
    if (span < length) {
        return span / (float) length;
    }

    return 1.0f;
}

/**
 * @return The maximum amount this scroll view will scroll in response to
 *   an arrow event.
 */
int NestedScrollView::getMaxScrollAmount() {
    return (int) (MAX_SCROLL_FACTOR * getHeight());
}

void NestedScrollView::initScrollView() {
    mScroller = new OverScroller(mContext);
    setFocusable(true);
    mFillViewport  = true;
    mSmoothScrollingEnabled = true;
    mIsBeingDragged= false;
    mIsLayoutDirty = true;
    mIsLaidOut = false;
    mLastMotionY   = 0;
    mLastScrollerY = 0;
    mNestedYOffset = 0;
    mLastScroll    = 0;
    mScrollOffset[0] = 0;
    mScrollOffset[1] = 0;
    mScrollConsumed[0] = 0;
    mScrollConsumed[1] = 0;
    setDescendantFocusability(FOCUS_AFTER_DESCENDANTS);
    setWillNotDraw(false);
    ViewConfiguration& configuration = ViewConfiguration::get(getContext());
    mTouchSlop = configuration.getScaledTouchSlop();
    mVelocityTracker = nullptr;
    mChildToScrollTo = nullptr;
    mMinimumVelocity = configuration.getScaledMinimumFlingVelocity();
    mMaximumVelocity = configuration.getScaledMaximumFlingVelocity();

    mParentHelper= new NestedScrollingParentHelper(this);
    mChildHelper = new NestedScrollingChildHelper(this);
    mEdgeGlowTop = new EdgeEffect(mContext);
    mEdgeGlowBottom = new EdgeEffect(mContext);
    setNestedScrollingEnabled(true); 
}


View& NestedScrollView::addView(View* child) {
    if (getChildCount() > 0) {
        throw "ScrollView can host only one direct child";
    }
    return FrameLayout::addView(child);
}


View& NestedScrollView::addView(View* child, int index) {
    if (getChildCount() > 0) {
        throw "ScrollView can host only one direct child";
    }
    return FrameLayout::addView(child, index);
}


View& NestedScrollView::addView(View* child, ViewGroup::LayoutParams* params) {
    if (getChildCount() > 0) {
        throw "ScrollView can host only one direct child";
    }
    return FrameLayout::addView(child, params);
}


View& NestedScrollView::addView(View* child, int index, ViewGroup::LayoutParams* params) {
    if (getChildCount() > 0) {
        throw "ScrollView can host only one direct child";
    }
    return FrameLayout::addView(child, index, params);
}

/**
 * Register a callback to be invoked when the scroll X or Y positions of
 * this view change.
 * <p>This version of the method works on all versions of Android, back to API v4.</p>
 *
 * @param l The listener to notify when the scroll X or Y position changes.
 * @see android.view.View#getScrollX()
 * @see android.view.View#getScrollY()
 */
void NestedScrollView::setOnScrollChangeListener(NestedScrollView::OnScrollChangeListener l) {
    mOnScrollChangeListener = l;
}

/**
 * @return Returns true this ScrollView can be scrolled
 */
bool NestedScrollView::canScroll() {
    if (getChildCount() ==0)return false;

    View* child = getChildAt(0);
    LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
    const int childSize = child->getHeight() + lp->topMargin + lp->bottomMargin;
    const int parentSpace = getHeight() - getPaddingTop() - getPaddingBottom();
    return childSize > parentSpace;
}

/**
 * Indicates whether this ScrollView's content is stretched to fill the viewport.
 *
 * @return True if the content fills the viewport, false otherwise.
 *
 * @attr name android:fillViewport
 */
bool NestedScrollView::isFillViewport() {
    return mFillViewport;
}

/**
 * Set whether this ScrollView should stretch its content height to fill the viewport or not.
 *
 * @param fillViewport True to stretch the content's height to the viewport's
 *        boundaries, false otherwise.
 *
 * @attr name android:fillViewport
 */
void NestedScrollView::setFillViewport(bool fillViewport) {
    if (fillViewport != mFillViewport) {
        mFillViewport = fillViewport;
        requestLayout();
    }
}

/**
 * @return Whether arrow scrolling will animate its transition.
 */
bool NestedScrollView::isSmoothScrollingEnabled() {
    return mSmoothScrollingEnabled;
}

/**
 * Set whether arrow scrolling will animate its transition.
 * @param smoothScrollingEnabled whether arrow scrolling will animate its transition
 */
void NestedScrollView::setSmoothScrollingEnabled(bool smoothScrollingEnabled) {
    mSmoothScrollingEnabled = smoothScrollingEnabled;
}


void NestedScrollView::onScrollChanged(int l, int t, int oldl, int oldt) {
    FrameLayout::onScrollChanged(l, t, oldl, oldt);

    if (mOnScrollChangeListener != nullptr) {
        mOnScrollChangeListener(*this, l, t, oldl, oldt);
    }
}


void NestedScrollView::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    FrameLayout::onMeasure(widthMeasureSpec, heightMeasureSpec);

    if (!mFillViewport) {
        return;
    }

    int heightMode = MeasureSpec::getMode(heightMeasureSpec);
    if (heightMode == MeasureSpec::UNSPECIFIED) {
        return;
    }

    if (getChildCount() > 0) {
        View* child = getChildAt(0);
	NestedScrollView::LayoutParams* lp = (LayoutParams*) child->getLayoutParams();

        int childSize = child->getMeasuredHeight();
        int parentSpace = getMeasuredHeight() - getPaddingTop()
                - getPaddingBottom() - lp->topMargin  - lp->bottomMargin;

        if (childSize < parentSpace) {
            int childWidthMeasureSpec = getChildMeasureSpec(widthMeasureSpec,
                    getPaddingLeft() + getPaddingRight() + lp->leftMargin + lp->rightMargin,
                    lp->width);
            int childHeightMeasureSpec =
                    MeasureSpec::makeMeasureSpec(parentSpace, MeasureSpec::EXACTLY);
            child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
        }
    }
}


bool NestedScrollView::dispatchKeyEvent(KeyEvent& event) {
    // Let the focused view and/or our descendants get the key first
    return FrameLayout::dispatchKeyEvent(event) || executeKeyEvent(event);
}

/**
 * You can call this function yourself to have the scroll view perform
 * scrolling from a key event, just as if the event had been dispatched to
 * it by the view hierarchy.
 *
 * @param event The key event to execute.
 * @return Return true if the event was handled, else false.
 */
bool NestedScrollView::executeKeyEvent(KeyEvent& event) {
    mTempRect.setEmpty();

    if (!canScroll()) {
        if (isFocused() && event.getKeyCode() != KEY_BACK) {
            View* currentFocused = findFocus();
            if (currentFocused == this) currentFocused = nullptr;
            View* nextFocused = FocusFinder::getInstance().findNextFocus(this,
                    currentFocused, View::FOCUS_DOWN);
            return nextFocused != nullptr && nextFocused != this
                    && nextFocused->requestFocus(View::FOCUS_DOWN);
        }
        return false;
    }

    bool handled = false;
    if (event.getAction() == KeyEvent::ACTION_DOWN) {
        switch (event.getKeyCode()) {
        case KEY_DPAD_UP:
            if (!event.isAltPressed()) {
                handled = arrowScroll(View::FOCUS_UP);
            } else {
                handled = fullScroll(View::FOCUS_UP);
            }
            break;
        case KEY_DPAD_DOWN:
            if (!event.isAltPressed()) {
                handled = arrowScroll(View::FOCUS_DOWN);
            } else {
                handled = fullScroll(View::FOCUS_DOWN);
            }
            break;
        case KEY_SPACE:
            pageScroll(event.isShiftPressed() ? FOCUS_UP : FOCUS_DOWN);
            break;
        }
    }

    return handled;
}

bool NestedScrollView::inChild(int x, int y) {
    if (getChildCount() > 0) {
        int scrollY = getScrollY();
        View* child = getChildAt(0);
        return !(y < child->getTop() - scrollY
                || y >= child->getBottom() - scrollY
                || x < child->getLeft()
                || x >= child->getRight());
    }
    return false;
}

void NestedScrollView::initOrResetVelocityTracker() {
    if (mVelocityTracker == nullptr) {
        mVelocityTracker = VelocityTracker::obtain();
    } else {
        mVelocityTracker->clear();
    }
}

void NestedScrollView::initVelocityTrackerIfNotExists() {
    if (mVelocityTracker == nullptr) {
        mVelocityTracker = VelocityTracker::obtain();
    }
}

void NestedScrollView::recycleVelocityTracker() {
    if (mVelocityTracker != nullptr) {
        mVelocityTracker->recycle();
        mVelocityTracker = nullptr;
    }
}

void NestedScrollView::requestDisallowInterceptTouchEvent(bool disallowIntercept) {
    if (disallowIntercept) {
        recycleVelocityTracker();
    }
    FrameLayout::requestDisallowInterceptTouchEvent(disallowIntercept);
}

bool NestedScrollView::onInterceptTouchEvent(MotionEvent& ev) {
    /*
     * This method JUST determines whether we want to intercept the motion.
     * If we return true, onMotionEvent will be called and we do the actual
     * scrolling there.
     */

    /*
    * Shortcut the most recurring case: the user is in the dragging
    * state and he is moving his finger.  We want to intercept this
    * motion.
    */
    int action = ev.getAction();
    if ((action == MotionEvent::ACTION_MOVE) && (mIsBeingDragged)) {
        return true;
    }

    switch (action & MotionEvent::ACTION_MASK) {
        case MotionEvent::ACTION_MOVE: {
            /*
             * mIsBeingDragged == false, otherwise the shortcut would have caught it. Check
             * whether the user has moved far enough from his original down touch.
             */

            /*
            * Locally do absolute value. mLastMotionY is set to the y value
            * of the down event.
            */
            int activePointerId = mActivePointerId;
            if (activePointerId == INVALID_POINTER) {
                // If we don't have a valid id, the touch down wasn't on content.
                break;
            }

            int pointerIndex = ev.findPointerIndex(activePointerId);
            if (pointerIndex == -1) {
                LOGE("Invalid pointerId=%d in onInterceptTouchEvent",activePointerId);
                break;
            }

            int y = (int) ev.getY(pointerIndex);
            int yDiff = std::abs(y - mLastMotionY);
            if (yDiff > mTouchSlop
                    && (getNestedScrollAxes() & View::SCROLL_AXIS_VERTICAL) == 0) {
                mIsBeingDragged = true;
                mLastMotionY = y;
                initVelocityTrackerIfNotExists();
                mVelocityTracker->addMovement(ev);
                mNestedYOffset = 0;
                ViewGroup* parent = getParent();
                if (parent != nullptr) {
                    parent->requestDisallowInterceptTouchEvent(true);
                }
            }
            break;
        }

        case MotionEvent::ACTION_DOWN: {
            int y = (int) ev.getY();
            if (!inChild((int) ev.getX(), y)) {
                mIsBeingDragged = false;
                recycleVelocityTracker();
                break;
            }

            /*
             * Remember location of down touch.
             * ACTION_DOWN always refers to pointer index 0.
             */
            mLastMotionY = y;
            mActivePointerId = ev.getPointerId(0);

            initOrResetVelocityTracker();
            mVelocityTracker->addMovement(ev);
            /*
             * If being flinged and user touches the screen, initiate drag;
             * otherwise don't. mScroller.isFinished should be false when
             * being flinged. We need to call computeScrollOffset() first so that
             * isFinished() is correct.
            */
            mScroller->computeScrollOffset();
            mIsBeingDragged = !mScroller->isFinished();
            startNestedScroll(View::SCROLL_AXIS_VERTICAL, TYPE_TOUCH);
            break;
        }

        case MotionEvent::ACTION_CANCEL:
        case MotionEvent::ACTION_UP:
            /* Release the drag */
            mIsBeingDragged = false;
            mActivePointerId = INVALID_POINTER;
            recycleVelocityTracker();
            if (mScroller->springBack(getScrollX(), getScrollY(), 0, 0, 0, getScrollRange())) {
                postInvalidateOnAnimation();
            }
            stopNestedScroll(TYPE_TOUCH);
            break;
        case MotionEvent::ACTION_POINTER_UP:
            onSecondaryPointerUp(ev);
            break;
    }

    /*
    * The only time we want to intercept motion events is if we are in the
    * drag mode.
    */
    return mIsBeingDragged;
}

bool NestedScrollView::onTouchEvent(MotionEvent& ev) {
    initVelocityTrackerIfNotExists();

    MotionEvent* vtev = MotionEvent::obtain(ev);

    int actionMasked = ev.getActionMasked();

    if (actionMasked == MotionEvent::ACTION_DOWN) {
        mNestedYOffset = 0;
    }
    vtev->offsetLocation(0, mNestedYOffset);

    switch (actionMasked) {
        case MotionEvent::ACTION_DOWN: {
            if (getChildCount() == 0) {
                return false;
            }
            if ((mIsBeingDragged = !mScroller->isFinished())) {
                ViewGroup* parent = getParent();
                if (parent) {
                    parent->requestDisallowInterceptTouchEvent(true);
                }
            }

            /*
             * If being flinged and user touches, stop the fling. isFinished
             * will be false if being flinged.
             */
            if (!mScroller->isFinished()) {
                mScroller->abortAnimation();
            }

            // Remember where the motion event started
            mLastMotionY = (int) ev.getY();
            mActivePointerId = ev.getPointerId(0);
            startNestedScroll(View::SCROLL_AXIS_VERTICAL, TYPE_TOUCH);
            break;
        }
        case MotionEvent::ACTION_MOVE:{
            int activePointerIndex = ev.findPointerIndex(mActivePointerId);
            if (activePointerIndex == -1) {
                LOGE("Invalid pointerId=%d in onTouchEvent", mActivePointerId);
                break;
            }

            int y = (int) ev.getY(activePointerIndex);
            int deltaY = mLastMotionY - y;
            if (dispatchNestedPreScroll(0, deltaY, mScrollConsumed, mScrollOffset,TYPE_TOUCH)) {
                deltaY -= mScrollConsumed[1];
                vtev->offsetLocation(0, mScrollOffset[1]);
                mNestedYOffset += mScrollOffset[1];
            }
            if (!mIsBeingDragged && std::abs(deltaY) > mTouchSlop) {
                ViewGroup* parent = getParent();
                if (parent) {
                    parent->requestDisallowInterceptTouchEvent(true);
                }
                mIsBeingDragged = true;
                if (deltaY > 0) {
                    deltaY -= mTouchSlop;
                } else {
                    deltaY += mTouchSlop;
                }
            }
            if (mIsBeingDragged) {
                // Scroll to follow the motion event
                mLastMotionY = y - mScrollOffset[1];

                int oldY = getScrollY();
                int range = getScrollRange();
                int overscrollMode = getOverScrollMode();
                bool canOverscroll = overscrollMode == View::OVER_SCROLL_ALWAYS
                        || (overscrollMode == View::OVER_SCROLL_IF_CONTENT_SCROLLS && range > 0);

                // Calling overScrollByCompat will call onOverScrolled, which
                // calls onScrollChanged if applicable.
                if (overScrollByCompat(0, deltaY, 0, getScrollY(), 0, range, 0,
                        0, true) && !hasNestedScrollingParent(TYPE_TOUCH)) {
                    // Break our velocity if we hit a scroll barrier.
                    mVelocityTracker->clear();
                }

                int scrolledDeltaY = getScrollY() - oldY;
                int unconsumedY = deltaY - scrolledDeltaY;
                if (dispatchNestedScroll(0, scrolledDeltaY, 0, unconsumedY, mScrollOffset,TYPE_TOUCH)) {
                    mLastMotionY -= mScrollOffset[1];
                    vtev->offsetLocation(0, mScrollOffset[1]);
                    mNestedYOffset += mScrollOffset[1];
                } else if (canOverscroll) {
                    ensureGlows();
                    int pulledToY = oldY + deltaY;
                    if (pulledToY < 0) {
                        mEdgeGlowTop->onPull( (float) deltaY / getHeight(),
                                ev.getX(activePointerIndex) / getWidth());
                        if (!mEdgeGlowBottom->isFinished()) {
                            mEdgeGlowBottom->onRelease();
                        }
                    } else if (pulledToY > range) {
                        mEdgeGlowBottom->onPull((float) deltaY / getHeight(),
                                1.f - ev.getX(activePointerIndex) / getWidth());
                        if (!mEdgeGlowTop->isFinished()) {
                            mEdgeGlowTop->onRelease();
                        }
                    }
                    if (mEdgeGlowTop != nullptr
                            && (!mEdgeGlowTop->isFinished() || !mEdgeGlowBottom->isFinished())) {
                        this->postInvalidateOnAnimation();
                    }
                }
            }
            }break;
        case MotionEvent::ACTION_UP:{
                VelocityTracker* velocityTracker = mVelocityTracker;
                velocityTracker->computeCurrentVelocity(1000, mMaximumVelocity);
                int initialVelocity = (int) velocityTracker->getYVelocity(mActivePointerId);
                if ((std::abs(initialVelocity) > mMinimumVelocity)) {
                    flingWithNestedDispatch(-initialVelocity);
                } else if (mScroller->springBack(getScrollX(), getScrollY(), 0, 0, 0,
                        getScrollRange())) {
                    this->postInvalidateOnAnimation();
                }
                mActivePointerId = INVALID_POINTER;
                endDrag();
	    }break;
        case MotionEvent::ACTION_CANCEL:
            if (mIsBeingDragged && getChildCount() > 0) {
                if (mScroller->springBack(getScrollX(), getScrollY(), 0, 0, 0,
                        getScrollRange())) {
                    this->postInvalidateOnAnimation();
                }
            }
            mActivePointerId = INVALID_POINTER;
            endDrag();
            break;
        case MotionEvent::ACTION_POINTER_DOWN: {
            int index = ev.getActionIndex();
            mLastMotionY = (int) ev.getY(index);
            mActivePointerId = ev.getPointerId(index);
            break;
        }
        case MotionEvent::ACTION_POINTER_UP:
            onSecondaryPointerUp(ev);
            mLastMotionY = (int) ev.getY(ev.findPointerIndex(mActivePointerId));
            break;
    }

    if (mVelocityTracker != nullptr) {
        mVelocityTracker->addMovement(*vtev);
    }
    vtev->recycle();
    return true;
}

void NestedScrollView::onSecondaryPointerUp(MotionEvent& ev) {
    int pointerIndex = ev.getActionIndex();
    int pointerId = ev.getPointerId(pointerIndex);
    if (pointerId == mActivePointerId) {
        // This was our active pointer going up. Choose a new
        // active pointer and adjust accordingly.
        // TODO: Make this decision more intelligent.
        int newPointerIndex = pointerIndex == 0 ? 1 : 0;
        mLastMotionY = (int) ev.getY(newPointerIndex);
        mActivePointerId = ev.getPointerId(newPointerIndex);
        if (mVelocityTracker != nullptr) {
            mVelocityTracker->clear();
        }
    }
}

bool NestedScrollView::onGenericMotionEvent(MotionEvent& event) {
    if ((event.getSource() & InputDevice::SOURCE_CLASS_POINTER) != 0) {
        switch (event.getAction()) {
        case MotionEvent::ACTION_SCROLL:
            if (!mIsBeingDragged) {
                float vscroll = event.getAxisValue(MotionEvent::AXIS_VSCROLL,0);
                if (vscroll != 0) {
                    int delta = (int) (vscroll * getVerticalScrollFactorCompat());
                    int range = getScrollRange();
                    int oldScrollY = getScrollY();
                    int newScrollY = oldScrollY - delta;
                    if (newScrollY < 0) {
                        newScrollY = 0;
                    } else if (newScrollY > range) {
                        newScrollY = range;
                    }
                    if (newScrollY != oldScrollY) {
                        FrameLayout::scrollTo(getScrollX(), newScrollY);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

float NestedScrollView::getVerticalScrollFactorCompat() {
    if (mVerticalScrollFactor == 0) {
        /*TypedValue outValue = new TypedValue();
        Context* context = getContext();
        if (!context->getTheme().resolveAttribute(android.R.attr.listPreferredItemHeight, outValue, true)) {
            throw "Expected theme to define listPreferredItemHeight.";
        }
        mVerticalScrollFactor = outValue.getDimension(context->getResources().getDisplayMetrics());*/
	mVerticalScrollFactor=1.f;
    }
    return mVerticalScrollFactor;
}

void NestedScrollView::onOverScrolled(int scrollX, int scrollY, bool clampedX, bool clampedY) {
    FrameLayout::scrollTo(scrollX, scrollY);
}

bool NestedScrollView::overScrollByCompat(int deltaX, int deltaY,int scrollX, int scrollY,
        int scrollRangeX, int scrollRangeY, int maxOverScrollX, int maxOverScrollY,  bool isTouchEvent) {
    int overScrollMode = getOverScrollMode();
    bool canScrollHorizontal =
            computeHorizontalScrollRange() > computeHorizontalScrollExtent();
    bool canScrollVertical =
            computeVerticalScrollRange() > computeVerticalScrollExtent();
    bool overScrollHorizontal = overScrollMode == View::OVER_SCROLL_ALWAYS
            || (overScrollMode == View::OVER_SCROLL_IF_CONTENT_SCROLLS && canScrollHorizontal);
    bool overScrollVertical = overScrollMode == View::OVER_SCROLL_ALWAYS
            || (overScrollMode == View::OVER_SCROLL_IF_CONTENT_SCROLLS && canScrollVertical);

    int newScrollX = scrollX + deltaX;
    if (!overScrollHorizontal) {
        maxOverScrollX = 0;
    }

    int newScrollY = scrollY + deltaY;
    if (!overScrollVertical) {
        maxOverScrollY = 0;
    }

    // Clamp values if at the limits and record
    int left = -maxOverScrollX;
    int right = maxOverScrollX + scrollRangeX;
    int top = -maxOverScrollY;
    int bottom = maxOverScrollY + scrollRangeY;

    bool clampedX = false;
    if (newScrollX > right) {
        newScrollX = right;
        clampedX = true;
    } else if (newScrollX < left) {
        newScrollX = left;
        clampedX = true;
    }

    bool clampedY = false;
    if (newScrollY > bottom) {
        newScrollY = bottom;
        clampedY = true;
    } else if (newScrollY < top) {
        newScrollY = top;
        clampedY = true;
    }

    if (clampedY && !hasNestedScrollingParent(TYPE_NON_TOUCH)) {
        mScroller->springBack(newScrollX, newScrollY, 0, 0, 0, getScrollRange());
    }

    onOverScrolled(newScrollX, newScrollY, clampedX, clampedY);

    return clampedX || clampedY;
}

int NestedScrollView::getScrollRange() {
    int scrollRange = 0;
    if (getChildCount() > 0) {
        View* child = getChildAt(0);
        NestedScrollView::LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
        int childSize = child->getHeight() + lp->topMargin + lp->bottomMargin;
        int parentSpace = getHeight() - getPaddingTop() - getPaddingBottom();
        scrollRange = std::max(0, childSize - parentSpace);
    }
    return scrollRange;
}

View* NestedScrollView::findFocusableViewInBounds(bool topFocus, int top, int bottom) {

    std::vector<View*> focusables = getFocusables(View::FOCUS_FORWARD);
    View* focusCandidate = nullptr;

    /*
     * A fully contained focusable is one where its top is below the bound's
     * top, and its bottom is above the bound's bottom. A partially
     * contained focusable is one where some part of it is within the
     * bounds, but it also has some part that is not within bounds.  A fully contained
     * focusable is preferred to a partially contained focusable.
     */
    bool foundFullyContainedFocusable = false;

    int count = focusables.size();
    for (int i = 0; i < count; i++) {
        View* view = focusables.at(i);
        int viewTop = view->getTop();
        int viewBottom = view->getBottom();

        if (top < viewBottom && viewTop < bottom) {
            /*
             * the focusable is in the target area, it is a candidate for
             * focusing
             */

            bool viewIsFullyContained = (top < viewTop) && (viewBottom < bottom);

            if (focusCandidate == nullptr) {
                /* No candidate, take this one */
                focusCandidate = view;
                foundFullyContainedFocusable = viewIsFullyContained;
            } else {
                bool viewIsCloserToBoundary = (topFocus && viewTop < focusCandidate->getTop())
                                || (!topFocus && viewBottom > focusCandidate->getBottom());

                if (foundFullyContainedFocusable) {
                    if (viewIsFullyContained && viewIsCloserToBoundary) {
                        /*
                         * We're dealing with only fully contained views, so
                         * it has to be closer to the boundary to beat our
                         * candidate
                         */
                        focusCandidate = view;
                    }
                } else {
                    if (viewIsFullyContained) {
                        /* Any fully contained view beats a partially contained view */
                        focusCandidate = view;
                        foundFullyContainedFocusable = true;
                    } else if (viewIsCloserToBoundary) {
                        /*
                         * Partially contained view beats another partially
                         * contained view if it's closer
                         */
                        focusCandidate = view;
                    }
                }
            }
        }
    }

    return focusCandidate;
}

bool NestedScrollView::pageScroll(int direction) {
    bool down = direction == View::FOCUS_DOWN;
    int height = getHeight();

    if (down) {
        mTempRect.top = getScrollY() + height;
        int count = getChildCount();
        if (count > 0) {
            View* view = getChildAt(count - 1);
            NestedScrollView::LayoutParams* lp = (LayoutParams*) view->getLayoutParams();
            int bottom = view->getBottom() + lp->bottomMargin + getPaddingBottom();
            if (mTempRect.top + height > bottom) {
                mTempRect.top = bottom - height;
            }
        }
    } else {
        mTempRect.top = getScrollY() - height;
        if (mTempRect.top < 0) {
            mTempRect.top = 0;
        }
    }
    mTempRect.height=height;

    return scrollAndFocus(direction, mTempRect.top, mTempRect.bottom());
}

bool NestedScrollView::fullScroll(int direction) {
    bool down = direction == View::FOCUS_DOWN;
    int height = getHeight();

    mTempRect.top = 0;
    mTempRect.height = height;

    if (down) {
        int count = getChildCount();
        if (count > 0) {
            View* view = getChildAt(count - 1);
            NestedScrollView::LayoutParams* lp = (LayoutParams*) view->getLayoutParams();
            //mTempRect.bottom = view->getBottom() + lp->bottomMargin + getPaddingBottom();
            //mTempRect.top = mTempRect.bottom - height;
	    mTempRect.top= view->getBottom() + lp->bottomMargin + getPaddingBottom() -height;
        }
    }

    return scrollAndFocus(direction, mTempRect.top, mTempRect.bottom());
}

bool NestedScrollView::scrollAndFocus(int direction, int top, int bottom) {
    bool handled = true;

    int height = getHeight();
    int containerTop = getScrollY();
    int containerBottom = containerTop + height;
    bool up = direction == View::FOCUS_UP;

    View* newFocused = findFocusableViewInBounds(up, top, bottom);
    if (newFocused == nullptr) {
        newFocused = this;
    }

    if (top >= containerTop && bottom <= containerBottom) {
        handled = false;
    } else {
        int delta = up ? (top - containerTop) : (bottom - containerBottom);
        doScrollY(delta);
    }

    if (newFocused != findFocus()) newFocused->requestFocus(direction);

    return handled;
}

bool NestedScrollView::arrowScroll(int direction) {
    View* currentFocused = findFocus();
    if (currentFocused == this) currentFocused = nullptr;

    View* nextFocused = FocusFinder::getInstance().findNextFocus(this, currentFocused, direction);

    int maxJump = getMaxScrollAmount();

    if (nextFocused != nullptr && isWithinDeltaOfScreen(nextFocused, maxJump, getHeight())) {
        nextFocused->getDrawingRect(mTempRect);
        offsetDescendantRectToMyCoords(nextFocused, mTempRect);
        int scrollDelta = computeScrollDeltaToGetChildRectOnScreen(mTempRect);
        doScrollY(scrollDelta);
        nextFocused->requestFocus(direction);
    } else {
        // no new focus
        int scrollDelta = maxJump;

        if (direction == View::FOCUS_UP && getScrollY() < scrollDelta) {
            scrollDelta = getScrollY();
        } else if (direction == View::FOCUS_DOWN) {
            if (getChildCount() > 0) {
                View* child = getChildAt(0);
                NestedScrollView::LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
                int daBottom = child->getBottom() + lp->bottomMargin;
                int screenBottom = getScrollY() + getHeight() - getPaddingBottom();
                scrollDelta = std::min(daBottom - screenBottom, maxJump);
            }
        }
        if (scrollDelta == 0) {
            return false;
        }
        doScrollY(direction == View::FOCUS_DOWN ? scrollDelta : -scrollDelta);
    }

    if (currentFocused != nullptr && currentFocused->isFocused()
            && isOffScreen(currentFocused)) {
        // previously focused item still has focus and is off screen, give
        // it up (take it back to ourselves)
        // (also, need to temporarily force FOCUS_BEFORE_DESCENDANTS so we are
        // sure to
        // get it)
        int descendantFocusability = getDescendantFocusability();  // save
        setDescendantFocusability(ViewGroup::FOCUS_BEFORE_DESCENDANTS);
        requestFocus();
        setDescendantFocusability(descendantFocusability);  // restore
    }
    return true;
}

bool NestedScrollView::isOffScreen(View* descendant) {
    return !isWithinDeltaOfScreen(descendant, 0, getHeight());
}

bool NestedScrollView::isWithinDeltaOfScreen(View* descendant, int delta, int height) {
    descendant->getDrawingRect(mTempRect);
    offsetDescendantRectToMyCoords(descendant, mTempRect);

    return (mTempRect.bottom() + delta) >= getScrollY()
            && (mTempRect.top - delta) <= (getScrollY() + height);
}

void NestedScrollView::doScrollY(int delta) {
    if (delta != 0) {
        if (mSmoothScrollingEnabled) {
            smoothScrollBy(0, delta);
        } else {
            scrollBy(0, delta);
        }
    }
}

void NestedScrollView::smoothScrollBy(int dx, int dy) {
    if (getChildCount() == 0) {
        // Nothing to do.
        return;
    }
    long duration = AnimationUtils::currentAnimationTimeMillis() - mLastScroll;
    if (duration > ANIMATED_SCROLL_GAP) {
        View* child = getChildAt(0);
        NestedScrollView::LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
        int childSize = child->getHeight() + lp->topMargin + lp->bottomMargin;
        int parentSpace = getHeight() - getPaddingTop() - getPaddingBottom();
        int scrollY = getScrollY();
        int maxY = std::max(0, childSize - parentSpace);
        dy = std::max(0, std::min(scrollY + dy, maxY)) - scrollY;
        mLastScrollerY = getScrollY();
        mScroller->startScroll(getScrollX(), scrollY, 0, dy);
        this->postInvalidateOnAnimation();
    } else {
        if (!mScroller->isFinished()) {
            mScroller->abortAnimation();
        }
        scrollBy(dx, dy);
    }
    mLastScroll = AnimationUtils::currentAnimationTimeMillis();
}

void NestedScrollView::smoothScrollTo(int x, int y) {
    smoothScrollBy(x - getScrollX(), y - getScrollY());
}

int NestedScrollView::computeVerticalScrollRange() {
    int count = getChildCount();
    int parentSpace = getHeight() - getPaddingBottom() - getPaddingTop();
    if (count == 0) {
        return parentSpace;
    }

    View* child = getChildAt(0);
    NestedScrollView::LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
    int scrollRange = child->getBottom() + lp->bottomMargin;
    int scrollY = getScrollY();
    int overscrollBottom = std::max(0, scrollRange - parentSpace);
    if (scrollY < 0) {
        scrollRange -= scrollY;
    } else if (scrollY > overscrollBottom) {
        scrollRange += scrollY - overscrollBottom;
    }

    return scrollRange;
}

int NestedScrollView::computeVerticalScrollOffset() {
    return std::max(0, FrameLayout::computeVerticalScrollOffset());
}

int NestedScrollView::computeVerticalScrollExtent() {
    return FrameLayout::computeVerticalScrollExtent();
}

int NestedScrollView::computeHorizontalScrollRange() {
    return FrameLayout::computeHorizontalScrollRange();
}

int NestedScrollView::computeHorizontalScrollOffset() {
    return FrameLayout::computeHorizontalScrollOffset();
}

int NestedScrollView::computeHorizontalScrollExtent() {
    return FrameLayout::computeHorizontalScrollExtent();
}

void NestedScrollView::measureChild(View* child, int parentWidthMeasureSpec,  int parentHeightMeasureSpec) {
    ViewGroup::LayoutParams* lp = child->getLayoutParams();

    int childWidthMeasureSpec;
    int childHeightMeasureSpec;

    childWidthMeasureSpec = getChildMeasureSpec(parentWidthMeasureSpec, getPaddingLeft()
            + getPaddingRight(), lp->width);

    childHeightMeasureSpec = MeasureSpec::makeMeasureSpec(0, MeasureSpec::UNSPECIFIED);

    child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
}

void NestedScrollView::measureChildWithMargins(View* child, int parentWidthMeasureSpec, int widthUsed,
        int parentHeightMeasureSpec, int heightUsed) {
    const MarginLayoutParams* lp = (const MarginLayoutParams*) child->getLayoutParams();

    const int usedTotal = mPaddingTop + mPaddingBottom + lp->topMargin + lp->bottomMargin +  heightUsed;
    const int childWidthMeasureSpec = getChildMeasureSpec(parentWidthMeasureSpec,
                               mPaddingLeft + mPaddingRight + lp->leftMargin + lp->rightMargin
                               + widthUsed, lp->width);
    const int childHeightMeasureSpec = MeasureSpec::makeSafeMeasureSpec(
                                std::max(0, MeasureSpec::getSize(parentHeightMeasureSpec) - usedTotal),
                                MeasureSpec::UNSPECIFIED);

    child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
}

void NestedScrollView::computeScroll() {
    if (mScroller->computeScrollOffset()) {
        int x = mScroller->getCurrX();
        int y = mScroller->getCurrY();

        int dy = y - mLastScrollerY;

        // Dispatch up to parent
        if (dispatchNestedPreScroll(0, dy, mScrollConsumed, nullptr, TYPE_NON_TOUCH)) {
            dy -= mScrollConsumed[1];
        }

        if (dy != 0) {
            int range = getScrollRange();
            int oldScrollY = getScrollY();

            overScrollByCompat(0, dy, getScrollX(), oldScrollY, 0, range, 0, 0, false);

            int scrolledDeltaY = getScrollY() - oldScrollY;
            int unconsumedY = dy - scrolledDeltaY;

            if (!dispatchNestedScroll(0, scrolledDeltaY, 0, unconsumedY, nullptr,TYPE_NON_TOUCH)) {
                int mode = getOverScrollMode();
                bool canOverscroll = mode == OVER_SCROLL_ALWAYS
                        || (mode == OVER_SCROLL_IF_CONTENT_SCROLLS && range > 0);
                if (canOverscroll) {
                    ensureGlows();
                    if (y <= 0 && oldScrollY > 0) {
                        mEdgeGlowTop->onAbsorb((int) mScroller->getCurrVelocity());
                    } else if (y >= range && oldScrollY < range) {
                        mEdgeGlowBottom->onAbsorb((int) mScroller->getCurrVelocity());
                    }
                }
            }
        }

        // Finally update the scroll positions and post an invalidation
        mLastScrollerY = y;
        this->postInvalidateOnAnimation();
    } else {
        // We can't scroll any more, so stop any indirect scrolling
        if (hasNestedScrollingParent(TYPE_NON_TOUCH)) {
            stopNestedScroll(TYPE_NON_TOUCH);
        }
        // and reset the scroller y
        mLastScrollerY = 0;
    }
}

void NestedScrollView::scrollToChild(View* child) {
    child->getDrawingRect(mTempRect);

    /* Offset from child's local coordinates to ScrollView coordinates */
    offsetDescendantRectToMyCoords(child, mTempRect);

    int scrollDelta = computeScrollDeltaToGetChildRectOnScreen(mTempRect);

    if (scrollDelta != 0) {
        scrollBy(0, scrollDelta);
    }
}

bool NestedScrollView::scrollToChildRect(const Rect& rect, bool immediate) {
    int delta = computeScrollDeltaToGetChildRectOnScreen(rect);
    bool scroll = delta != 0;
    if (scroll) {
        if (immediate) {
            scrollBy(0, delta);
        } else {
            smoothScrollBy(0, delta);
        }
    }
    return scroll;
}

int NestedScrollView::computeScrollDeltaToGetChildRectOnScreen(Rect rect) {
    if (getChildCount() == 0) return 0;

    int height = getHeight();
    int screenTop = getScrollY();
    int screenBottom = screenTop + height;
    int actualScreenBottom = screenBottom;

    int fadingEdge = getVerticalFadingEdgeLength();

    // TODO: screenTop should be incremented by fadingEdge * getTopFadingEdgeStrength (but for
    // the target scroll distance).
    // leave room for top fading edge as long as rect isn't at very top
    if (rect.top > 0) {
        screenTop += fadingEdge;
    }

    // TODO: screenBottom should be decremented by fadingEdge * getBottomFadingEdgeStrength (but
    // for the target scroll distance).
    // leave room for bottom fading edge as long as rect isn't at very bottom
    View* child = getChildAt(0);
    NestedScrollView::LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
    if (rect.bottom() < child->getHeight() + lp->topMargin + lp->bottomMargin) {
        screenBottom -= fadingEdge;
    }

    int scrollYDelta = 0;

    if (rect.bottom() > screenBottom && rect.top > screenTop) {
        // need to move down to get it in view: move down just enough so
        // that the entire rectangle is in view (or at least the first
        // screen size chunk).

        if (rect.height > height) {
            // just enough to get screen size chunk on
            scrollYDelta += (rect.top - screenTop);
        } else {
            // get entire rect at bottom of screen
            scrollYDelta += (rect.bottom() - screenBottom);
        }

        // make sure we aren't scrolling beyond the end of our content
        int bottom = child->getBottom() + lp->bottomMargin;
        int distanceToBottom = bottom - actualScreenBottom;
        scrollYDelta = std::min(scrollYDelta, distanceToBottom);

    } else if (rect.top < screenTop && rect.bottom() < screenBottom) {
        // need to move up to get it in view: move up just enough so that
        // entire rectangle is in view (or at least the first screen
        // size chunk of it).

        if (rect.height > height) {
            // screen size chunk
            scrollYDelta -= (screenBottom - rect.bottom());
        } else {
            // entire rect at top
            scrollYDelta -= (screenTop - rect.top);
        }

        // make sure we aren't scrolling any further than the top our content
        scrollYDelta = std::max(scrollYDelta, -getScrollY());
    }
    return scrollYDelta;
}

void NestedScrollView::requestChildFocus(View* child, View* focused) {
    if (!mIsLayoutDirty) {
        scrollToChild(focused);
    } else {
        // The child may not be laid out yet, we can't compute the scroll yet
        mChildToScrollTo = focused;
    }
    FrameLayout::requestChildFocus(child, focused);
}

bool NestedScrollView::onRequestFocusInDescendants(int direction,Rect* previouslyFocusedRect) {

    // convert from forward / backward notation to up / down / left / right
    // (ugh).
    if (direction == View::FOCUS_FORWARD) {
        direction = View::FOCUS_DOWN;
    } else if (direction == View::FOCUS_BACKWARD) {
        direction = View::FOCUS_UP;
    }

    View* nextFocus = previouslyFocusedRect == nullptr
            ? FocusFinder::getInstance().findNextFocus(this, nullptr, direction)
            : FocusFinder::getInstance().findNextFocusFromRect(
                    this, previouslyFocusedRect, direction);

    if (nextFocus == nullptr) {
        return false;
    }

    if (isOffScreen(nextFocus)) {
        return false;
    }

    return nextFocus->requestFocus(direction, previouslyFocusedRect);
}

bool NestedScrollView::requestChildRectangleOnScreen(View* child, Rect rectangle, bool immediate) {
    // offset into coordinate space of this scroll view
    rectangle.offset(child->getLeft() - child->getScrollX(),
            child->getTop() - child->getScrollY());

    return scrollToChildRect(rectangle, immediate);
}

void NestedScrollView::requestLayout() {
    mIsLayoutDirty = true;
    FrameLayout::requestLayout();
}

void NestedScrollView::onLayout(bool changed, int l, int t, int width, int height) {
    FrameLayout::onLayout(changed, l, t, width, height);
    mIsLayoutDirty = false;
    // Give a child focus if it needs it
    if (mChildToScrollTo != nullptr && isViewDescendantOf(mChildToScrollTo, this)) {
        scrollToChild(mChildToScrollTo);
    }
    mChildToScrollTo = nullptr;

    if (!mIsLaidOut) {
        // If there is a saved state, scroll to the position saved in that state.
        /*if (mSavedState != nullptr) {
            scrollTo(getScrollX(), mSavedState.scrollPosition);
            mSavedState = nullptr;
        }*/ // mScrollY default value is "0"

        // Make sure current scrollY position falls into the scroll range.  If it doesn't,
        // scroll such that it does.
        int childSize = 0;
        if (getChildCount() > 0) {
            View* child = getChildAt(0);
            NestedScrollView::LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
            childSize = child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin;
        }
        int parentSpace = height - getPaddingTop() - getPaddingBottom();
        int currentScrollY = getScrollY();
        int newScrollY = clamp(currentScrollY, parentSpace, childSize);
        if (newScrollY != currentScrollY) {
            scrollTo(getScrollX(), newScrollY);
        }
    }

    // Calling this with the present values causes it to re-claim them
    scrollTo(getScrollX(), getScrollY());
    mIsLaidOut = true;
}

void NestedScrollView::onAttachedToWindow() {
    FrameLayout::onAttachedToWindow();

    mIsLaidOut = false;
}

void NestedScrollView::onSizeChanged(int w, int h, int oldw, int oldh) {
    FrameLayout::onSizeChanged(w, h, oldw, oldh);

    View* currentFocused = findFocus();
    if (nullptr == currentFocused || this == currentFocused) {
        return;
    }

    // If the currently-focused view was visible on the screen when the
    // screen was at the old height, then scroll the screen to make that
    // view visible with the new screen height.
    if (isWithinDeltaOfScreen(currentFocused, 0, oldh)) {
        currentFocused->getDrawingRect(mTempRect);
        offsetDescendantRectToMyCoords(currentFocused, mTempRect);
        int scrollDelta = computeScrollDeltaToGetChildRectOnScreen(mTempRect);
        doScrollY(scrollDelta);
    }
}

bool NestedScrollView::isViewDescendantOf(View* child, View* parent) {
    if (child == parent) {
        return true;
    }

    ViewGroup* theParent = child->getParent();
    return /*(theParent instanceof ViewGroup) &&*/ isViewDescendantOf((View*) theParent, parent);
}

void NestedScrollView::fling(int velocityY) {
    if (getChildCount() > 0) {
        startNestedScroll(View::SCROLL_AXIS_VERTICAL, TYPE_NON_TOUCH);
        mScroller->fling(getScrollX(), getScrollY(), // start
                0, velocityY, // velocities
                0, 0, // x
                INT_MIN, INT_MAX, // y
                0, 0); // overscroll
        mLastScrollerY = getScrollY();
	LOGD("mLastScrollerY=%d",mLastScrollerY);
        this->postInvalidateOnAnimation();
    }
}

void NestedScrollView::flingWithNestedDispatch(int velocityY) {
    int scrollY = getScrollY();
    bool canFling = (scrollY > 0 || velocityY > 0)
            && (scrollY < getScrollRange() || velocityY < 0);
    if (!dispatchNestedPreFling(0, velocityY)) {
         dispatchNestedFling(0, velocityY, canFling);
         fling(velocityY);
    }
}

void NestedScrollView::endDrag() {
    mIsBeingDragged = false;

    recycleVelocityTracker();
    stopNestedScroll(TYPE_TOUCH);

    if (mEdgeGlowTop != nullptr) {
        mEdgeGlowTop->onRelease();
        mEdgeGlowBottom->onRelease();
    }
}

void NestedScrollView::scrollTo(int x, int y) {
    // we rely on the fact the View.scrollBy calls scrollTo.
    if (getChildCount() > 0) {
        View* child = getChildAt(0);
        NestedScrollView::LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
        int parentSpaceHorizontal = getWidth() - getPaddingLeft() - getPaddingRight();
        int childSizeHorizontal = child->getWidth() + lp->leftMargin + lp->rightMargin;
        int parentSpaceVertical = getHeight() - getPaddingTop() - getPaddingBottom();
        int childSizeVertical = child->getHeight() + lp->topMargin + lp->bottomMargin;
        x = clamp(x, parentSpaceHorizontal, childSizeHorizontal);
        y = clamp(y, parentSpaceVertical, childSizeVertical);
        if (x != getScrollX() || y != getScrollY()) {
            FrameLayout::scrollTo(x, y);
        }
    }
}

void NestedScrollView::ensureGlows() {
    if (getOverScrollMode() != View::OVER_SCROLL_NEVER) {
        if (mEdgeGlowTop == nullptr) {
            Context* context = getContext();
            mEdgeGlowTop = new EdgeEffect(context);
            mEdgeGlowBottom = new EdgeEffect(context);
        }
    } else {
        delete mEdgeGlowTop;
        delete mEdgeGlowBottom;
        mEdgeGlowTop = nullptr;
        mEdgeGlowBottom = nullptr;
    }
}

void NestedScrollView::draw(Canvas& canvas) {
    FrameLayout::draw(canvas);
    if (mEdgeGlowTop != nullptr) {
        int scrollY = getScrollY();
        if (!mEdgeGlowTop->isFinished()) {
            int width = getWidth();
            int height = getHeight();
            int xTranslation = 0;
            int yTranslation = std::min(0, scrollY);
            canvas.save();
            if (/*Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP ||*/ getClipToPadding()) {
                width -= getPaddingLeft() + getPaddingRight();
                xTranslation += getPaddingLeft();
            }
            if (/*Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP &&*/ getClipToPadding()) {
                height -= getPaddingTop() + getPaddingBottom();
                yTranslation += getPaddingTop();
            }
            canvas.translate(xTranslation, yTranslation);
            mEdgeGlowTop->setSize(width, height);
            if (mEdgeGlowTop->draw(canvas)) {
                this->postInvalidateOnAnimation();
            }
            canvas.restore();
        }
        if (!mEdgeGlowBottom->isFinished()) {
            int width = getWidth();
            int height = getHeight();
            int xTranslation = 0;
            int yTranslation = std::max(getScrollRange(), scrollY) + height;
            canvas.save();
            if (/*Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP ||*/ getClipToPadding()) {
                width -= getPaddingLeft() + getPaddingRight();
                xTranslation += getPaddingLeft();
            }
            if (/*Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP && */getClipToPadding()) {
                height -= getPaddingTop() + getPaddingBottom();
                yTranslation -= getPaddingBottom();
            }
            canvas.translate(xTranslation - width, yTranslation);
            //canvas.rotate(180, width, 0);
            mEdgeGlowBottom->setSize(width, height);
            if (mEdgeGlowBottom->draw(canvas)) {
                this->postInvalidateOnAnimation();
            }
            canvas.restore();
        }
    }
}

int NestedScrollView::clamp(int n, int my, int child) {
    if (my >= child || n < 0) {
        /* my >= child is this case:
         *                    |--------------- me ---------------|
         *     |------ child ------|
         * or
         *     |--------------- me ---------------|
         *            |------ child ------|
         * or
         *     |--------------- me ---------------|
         *                                  |------ child ------|
         *
         * n < 0 is this case:
         *     |------ me ------|
         *                    |-------- child --------|
         *     |-- mScrollX --|
         */
        return 0;
    }
    if ((my + n) > child) {
        /* this case:
         *                    |------ me ------|
         *     |------ child ------|
         *     |-- mScrollX --|
         */
        return child - my;
    }
    return n;
}
}/*endof namespace*/
