#include <widget/numberpicker.h>
#include <widget/R.h>
#include <color.h>
#include <textutils.h>
#include <cdlog.h>
#include <core/neverdestroyed.h>

//https://gitee.com/awang/WheelView/blob/master/src/com/wangjie/wheelview/WheelView.java

namespace cdroid{

DECLARE_WIDGET2(NumberPicker,"cdroid:attr/numberPickerStyle")
const std::string DEFAULT_LAYOUT_VERT="cdroid:layout/number_picker";
const std::string DEFAULT_LAYOUT_HORZ="cdroid:layout/number_picker_horz";

NumberPicker::NumberPicker(int w,int h):LinearLayout(w,h){
    initView();
    setOrientation(h>w?VERTICAL:HORIZONTAL);

    const std::string layoutres = (getOrientation()==VERTICAL)?DEFAULT_LAYOUT_VERT:DEFAULT_LAYOUT_HORZ;
    LayoutInflater::from(mContext)->inflate(layoutres,this,true);
 
    mSelectedText =(EditText*)findViewById(R::id::numberpicker_input);
    if(mSelectedText){
        mSelectedText->setTextAlignment(View::TEXT_ALIGNMENT_CENTER);
        mSelectedTextSize = mSelectedText->getTextSize();
        mTextSize = mSelectedTextSize;
        mSelectorElementSize = mSelectedTextSize;
    }
    setWidthAndHeight();
    mComputeMaxWidth = (mMaxWidth == SIZE_UNSPECIFIED);
    measure(MeasureSpec::makeMeasureSpec(w,MeasureSpec::EXACTLY),MeasureSpec::makeMeasureSpec(h,MeasureSpec::EXACTLY));
    layout(0,0,getMeasuredWidth(),getMeasuredHeight());
    updateInputTextView();
    setFocusable(int(View::FOCUSABLE));
    setFocusableInTouchMode(true);
}

NumberPicker::NumberPicker(Context* context,const AttributeSet& atts)
  :LinearLayout(context,atts){
    initView();
    mHideWheelUntilFocused = atts.getBoolean("hideWheelUntilFocused",false);
    mWrapSelectorWheelPreferred= atts.getBoolean("wrapSelectorWheel",mWrapSelectorWheelPreferred);
    mDividerDrawable = atts.getDrawable("selectionDivider");
    mSelectedTextSize = atts.getDimensionPixelSize("selectedTextSize",mSelectedTextSize);
    if (mDividerDrawable) {
        mDividerDrawable->setCallback(this);
        mDividerDrawable->setLayoutDirection(getLayoutDirection());
        if (mDividerDrawable->isStateful()) {
            mDividerDrawable->setState(getDrawableState());
        }
    }else{
        setDividerColor(atts.getColor("dividerColor",mDividerColor));
    }
    mDividerDistance = atts.getDimensionPixelSize("dividerDistance",UNSCALED_DEFAULT_SELECTION_DIVIDERS_DISTANCE);
    mDividerLength = atts.getDimensionPixelSize("dividerLength",0);
    mOrder = ASCENDING;
    mSelectionDividerHeight = atts.getDimensionPixelSize("selectionDividerHeight",UNSCALED_DEFAULT_SELECTION_DIVIDER_HEIGHT);
    mSelectionDividersDistance=atts.getDimensionPixelSize("selectionDividersDistance",UNSCALED_DEFAULT_SELECTION_DIVIDERS_DISTANCE);
    mMinHeight = atts.getDimensionPixelSize("internalMinHeight",SIZE_UNSPECIFIED);
    mMaxHeight = atts.getDimensionPixelSize("internalMaxHeight",SIZE_UNSPECIFIED);
    
    mMinWidth = atts.getDimensionPixelSize("internalMinWidth", SIZE_UNSPECIFIED);
    mMaxWidth = atts.getDimensionPixelSize("internalMaxWidth", SIZE_UNSPECIFIED);

    if (mMinWidth != SIZE_UNSPECIFIED && mMaxWidth != SIZE_UNSPECIFIED
                && mMinWidth > mMaxWidth) {
        throw "minWidth  > maxWidth";
    }

    const std::string layoutres=atts.getString("internalLayout",(getOrientation()==LinearLayout::VERTICAL?DEFAULT_LAYOUT_VERT:DEFAULT_LAYOUT_HORZ));
    LayoutInflater::from(mContext)->inflate(layoutres,this);
    setWidthAndHeight();
    mComputeMaxWidth = (mMaxWidth == SIZE_UNSPECIFIED);
    setWillNotDraw(false);

    mSelectedText =(EditText*)findViewById(cdroid::R::id::numberpicker_input);
    mSelectedText->setEnabled(false);
    mSelectedText->setFocusable(false);
    mTextAlign = mSelectedText->getGravity();
    mSelectedTextSize = mSelectedText->getTextSize();
    mTypeface = Typeface::create(atts.getString("fontFamily"),Typeface::NORMAL);
    mSelectedTypeface = Typeface::create(atts.getString("selectedfontFamily"),Typeface::NORMAL);
    //ViewConfiguration configuration = ViewConfiguration::get(context);
    setTextSize(atts.getDimensionPixelSize("textSize",mTextSize));
    mTextSize2 = atts.getDimensionPixelSize("textSize2",mTextSize);
    if(atts.hasAttribute("selectedTextSize"))
        mSelectedTextSize = atts.getDimensionPixelSize("selectedTextSize");
    else if(!atts.hasAttribute("internalLayout"))
        mSelectedTextSize =std::max(mSelectedTextSize,mTextSize);
    setSelectedTextSize(mSelectedTextSize);
    setTextColor(atts.getColor("textColor"));
    setTextColor(mTextColor,atts.getColor("textColor2",mTextColor));
    setSelectedTextColor(atts.getColor("selectedTextColor"));
    const ColorStateList*colors = mSelectedText->getTextColors();
    if(colors->isStateful())
        setSelectedTextColor(colors->getColorForState(StateSet::get(StateSet::VIEW_STATE_ENABLED),mSelectedTextColor));
    updateInputTextView();

    setWheelItemCount(atts.getInt("wheelItemCount",mWheelItemCount));
    setValue(atts.getInt("value",0));
    setMinValue(atts.getInt("min",0));
    setMaxValue(atts.getInt("max",0));

    updateWrapSelectorWheel();
    LOGV("%p:%d textSize=%d,%d",this,mID,mSelectedTextSize,mTextSize);
    if(getFocusable()==View::FOCUSABLE_AUTO){
        setFocusable(int(View::FOCUSABLE));
        setFocusableInTouchMode(true);
    }
}

NumberPicker::~NumberPicker(){
    delete mDividerDrawable;
    delete mFlingScroller;
    delete mAdjustScroller;
    for(auto d:mDisplayedDrawables)delete d;
}

bool NumberPicker::isHorizontalMode()const{
    return getOrientation() == HORIZONTAL;
}

bool NumberPicker::isAscendingOrder()const{
    return mOrder == ASCENDING;
}

static float dpToPx(float dp){
    return dp;
}

void NumberPicker::setWidthAndHeight() {
    if (isHorizontalMode()) {
        mMinHeight = SIZE_UNSPECIFIED;
        mMaxHeight = (int) dpToPx(DEFAULT_MIN_WIDTH);
        mMinWidth = (int) dpToPx(DEFAULT_MAX_HEIGHT);
        mMaxWidth = SIZE_UNSPECIFIED;
    } else {
        mMinHeight = SIZE_UNSPECIFIED;
        mMaxHeight = (int) dpToPx(DEFAULT_MAX_HEIGHT);
        mMinWidth = (int) dpToPx(DEFAULT_MIN_WIDTH);
        mMaxWidth = SIZE_UNSPECIFIED;
    }
}

void NumberPicker::setOrientation(int orientation) {
    //if(orientation!=getOrientation())
    LinearLayout::setOrientation(orientation);
    setWidthAndHeight();
    requestLayout();
}

void NumberPicker::setWheelItemCount(int count) {
    LOGE_IF(count<1,"Wheel item count must be >= 1");
    mRealWheelItemCount = count;
    mWheelItemCount = std::max(count, (int)DEFAULT_WHEEL_ITEM_COUNT);
    mWheelMiddleItemIndex = mWheelItemCount / 2;
    mSelectorIndices.resize(mWheelItemCount);
}

const NeverDestroyed<DecelerateInterpolator>gDecelerateInterpolator(2.5);

void NumberPicker::initView(){
    ViewConfiguration&config= ViewConfiguration::get(mContext);
    mDisplayedDrawableCount = 0;
    mDisplayedDrawableSize = 0;
    mSelectedText = nullptr;
    mOnValueChangeListener = nullptr;
    mFormatter = nullptr;
    mOnScrollListener.onScrollStateChange = nullptr;
    mScrollState = OnScrollListener::SCROLL_STATE_IDLE;
    mTextSize   = 24;
    mItemSpacing= 0;
    mTopSelectionDividerTop = 0;
    mSelectedTextSize = 24;
    mSelectedTextColor = 0xFFFFFFFF;
    mSelectedTypeface = nullptr;
    mTypeface = nullptr;
    mDividerColor =DEFAULT_DIVIDER_COLOR;
    mWheelMiddleItemIndex = 0;
    mDividerDrawable  = nullptr;
    mDividerLength = 2;
    mDividerThickness =2;
    mBottomSelectionDividerBottom = 0;
    mDividerType = SIDE_LINES;
    mLastHandledDownDpadKeyCode = -1;
    mWrapSelectorWheel= false;
    mWrapSelectorWheelPreferred = true;
    mIncrementVirtualButtonPressed = false;
    mDecrementVirtualButtonPressed = false;
    mSelectionDividerHeight = UNSCALED_DEFAULT_SELECTION_DIVIDER_HEIGHT;
    mPreviousScrollerY   = 0;
    mCurrentScrollOffset = 0;
    mInitialScrollOffset = 0;
    mLongPressUpdateInterval = DEFAULT_LONG_PRESS_UPDATE_INTERVAL;
    mMinHeight = SIZE_UNSPECIFIED;
    mMaxHeight = SIZE_UNSPECIFIED;
    mMinWidth  = SIZE_UNSPECIFIED;
    mMaxWidth  = SIZE_UNSPECIFIED;
    mValue    = 0;
    mMinValue = 0;
    mMaxValue = 0;
    mSelectorTextGapHeight = 0;
    mSelectorElementSize = 1;//avoid divide by zero.
    mSelectionDividersDistance =UNSCALED_DEFAULT_SELECTION_DIVIDERS_DISTANCE;
    mVelocityTracker = nullptr;

    mTouchSlop = config.getScaledTouchSlop();
    mMinimumFlingVelocity = config.getScaledMinimumFlingVelocity();
    mMaximumFlingVelocity = config.getScaledMaximumFlingVelocity()/ SELECTOR_MAX_FLING_VELOCITY_ADJUSTMENT;
    mFlingScroller  = new Scroller(getContext(), nullptr, true);
    mAdjustScroller = new Scroller(getContext(), gDecelerateInterpolator.get());
    mComputeMaxWidth = (mMaxWidth == SIZE_UNSPECIFIED);
    mHideWheelUntilFocused = false;
    mWheelItemCount = DEFAULT_WHEEL_ITEM_COUNT;
    mRealWheelItemCount= DEFAULT_WHEEL_ITEM_COUNT;
    mSelectorIndices.resize(mWheelItemCount);
}
void NumberPicker::onLayout(bool changed, int left, int top, int width, int height){
    const int msrdWdth = getMeasuredWidth();
    const int msrdHght = getMeasuredHeight();

    // Input text centered horizontally.
    const int inptTxtMsrdWdth = isHorizontalMode()?mSelectorElementSize:mSelectedText->getMeasuredWidth();
    const int inptTxtMsrdHght = mSelectedText->getMeasuredHeight();
    const int inptTxtLeft= (msrdWdth - inptTxtMsrdWdth) /2;
    const int inptTxtTop = (msrdHght - inptTxtMsrdHght)/2;
    //const int inptTxtRight= inptTxtLeft + inptTxtMsrdWdth;
    //const int inptTxtBottom=inptTxtTop + inptTxtMsrdHght;
    mSelectedText->layout(inptTxtLeft, inptTxtTop, inptTxtMsrdWdth, inptTxtMsrdHght);
    mSelectedTextCenter = (isHorizontalMode()?getWidth():getHeight())/2;
    if (changed) { // need to do all this when we know our size
        initializeSelectorWheel();
        initializeFadingEdges();
        const int dividerDistence = 2*mDividerThickness + mDividerDistance;
        if(isHorizontalMode()){
            mLeftDividerLeft = (getWidth()-mDividerDistance)/2 - mDividerThickness;
            mRightDividerRight= mLeftDividerLeft + dividerDistence;
            mBottomDividerBottom = getHeight();
            mSelectedText->layout(inptTxtLeft, inptTxtTop,std::max(inptTxtMsrdWdth,mSelectorElementSize), inptTxtMsrdHght);
        }else{
            mTopDividerTop = (getHeight() - mDividerDistance)/2 - mDividerThickness;
            mBottomDividerBottom = mTopDividerTop + dividerDistence;
        }
    }
}


void NumberPicker::onMeasure(int widthMeasureSpec, int heightMeasureSpec){
    // Try greedily to fit the max width and height.
    const int newWidthMeasureSpec = makeMeasureSpec(widthMeasureSpec, mMaxWidth);
    const int newHeightMeasureSpec = makeMeasureSpec(heightMeasureSpec, mMaxHeight);
    LinearLayout::onMeasure(newWidthMeasureSpec, newHeightMeasureSpec);
    // Flag if we are measured with width or height less than the respective min.
    const int widthSize = resolveSizeAndStateRespectingMinSize(mMinWidth, getMeasuredWidth(),
                widthMeasureSpec);
    const int heightSize = resolveSizeAndStateRespectingMinSize(mMinHeight, getMeasuredHeight(),
                heightMeasureSpec);
    setMeasuredDimension(widthSize, heightSize);
}


bool NumberPicker::moveToFinalScrollerPosition(Scroller* scroller) {
    scroller->forceFinished(true);
    if(isHorizontalMode()){
        int amountToScroll = scroller->getFinalX() - scroller->getCurrX();
        int futureScrollOffset = (mCurrentScrollOffset + amountToScroll) % mSelectorElementSize;
        int overshootAdjustment = mInitialScrollOffset - futureScrollOffset;
        if (overshootAdjustment != 0) {
            if (std::abs(overshootAdjustment) > mSelectorElementSize / 2) {
                if (overshootAdjustment > 0) {
                    overshootAdjustment -= mSelectorElementSize;
                } else {
                    overshootAdjustment += mSelectorElementSize;
                }
            }
            amountToScroll += overshootAdjustment;
            scrollBy(amountToScroll, 0);
            return true;
        }
    }else{
        int amountToScroll = scroller->getFinalY() - scroller->getCurrY();
        int futureScrollOffset = (mCurrentScrollOffset + amountToScroll) % mSelectorElementSize;
        int overshootAdjustment = mInitialScrollOffset - futureScrollOffset;
        if (overshootAdjustment != 0) {
            if (std::abs(overshootAdjustment) > mSelectorElementSize / 2) {
                if (overshootAdjustment > 0) {
                    overshootAdjustment -= mSelectorElementSize;
                } else {
                    overshootAdjustment += mSelectorElementSize;
                }
            }
            amountToScroll += overshootAdjustment;
            scrollBy(0, amountToScroll);
            return true;
        }
    }
    return false;
}
bool NumberPicker::onInterceptTouchEvent(MotionEvent& event){
    const int action = event.getActionMasked();
    if (!isEnabled() ||(action!=MotionEvent::ACTION_DOWN)) {
        return false;
    }
    if(isHorizontalMode()){
        mLastDownOrMoveEventX = mLastDownEventX = event.getX();
        if (!mFlingScroller->isFinished()) {
            mFlingScroller->forceFinished(true);
            mAdjustScroller->forceFinished(true);
            onScrollerFinished(mFlingScroller);
            onScrollStateChange(OnScrollListener::SCROLL_STATE_IDLE);
        } else if (!mAdjustScroller->isFinished()) {
            mFlingScroller->forceFinished(true);
            mAdjustScroller->forceFinished(true);
            onScrollerFinished(mAdjustScroller);
        } else if (mLastDownEventX >= mLeftDividerLeft
                && mLastDownEventX <= mRightDividerRight) {
            if (mOnClickListener) {
                mOnClickListener(*this);
            }
        } else if (mLastDownEventX < mLeftDividerLeft) {
            postChangeCurrentByOneFromLongPress(false);
        } else if (mLastDownEventX > mRightDividerRight) {
            postChangeCurrentByOneFromLongPress(true);
        }
    }else{
        mLastDownOrMoveEventY = mLastDownEventY = event.getY();
        if (!mFlingScroller->isFinished()) {
            mFlingScroller->forceFinished(true);
            mAdjustScroller->forceFinished(true);
            onScrollStateChange(OnScrollListener::SCROLL_STATE_IDLE);
        } else if (!mAdjustScroller->isFinished()) {
            mFlingScroller->forceFinished(true);
            mAdjustScroller->forceFinished(true);
        } else if (mLastDownEventY >= mTopDividerTop
                && mLastDownEventY <= mBottomDividerBottom) {
            if (mOnClickListener) {
                mOnClickListener(*this);
            }
        } else if (mLastDownEventY < mTopDividerTop) {
            postChangeCurrentByOneFromLongPress(false);
        } else if (mLastDownEventY > mBottomDividerBottom) {
            postChangeCurrentByOneFromLongPress(true);
        }        
    }//endif isHorizontalMode
    return true;
}

bool NumberPicker::onTouchEvent(MotionEvent& event){
    if (!isEnabled()) {
        return false;
    }
    if (mVelocityTracker == nullptr) mVelocityTracker = VelocityTracker::obtain();

    mVelocityTracker->addMovement(event);
    int action = event.getActionMasked();
    switch (action) {
    case MotionEvent::ACTION_CANCEL:LOGD("ACTION_CANCEL");break;
    case MotionEvent::ACTION_MOVE:
        if (isHorizontalMode()) {
            float currentMoveX = event.getX();
            if (mScrollState != OnScrollListener::SCROLL_STATE_TOUCH_SCROLL) {
                int deltaDownX = (int) std::abs(currentMoveX - mLastDownEventX);
                if (deltaDownX > mTouchSlop) {
                    removeAllCallbacks();
                    onScrollStateChange(OnScrollListener::SCROLL_STATE_TOUCH_SCROLL);
                }
            } else {
                int deltaMoveX = (int) ((currentMoveX - mLastDownOrMoveEventX));
                scrollBy(deltaMoveX, 0);
                invalidate();
            }
            mLastDownOrMoveEventX = currentMoveX;
        }else{
            const float currentMoveY = event.getY();
            if (mScrollState != OnScrollListener::SCROLL_STATE_TOUCH_SCROLL) {
                int deltaDownY = (int) std::abs(currentMoveY - mLastDownEventY);
                if (deltaDownY > mTouchSlop) {
                    removeAllCallbacks();
                    onScrollStateChange(OnScrollListener::SCROLL_STATE_TOUCH_SCROLL);
                }
            } else {
                int deltaMoveY = (int) ((currentMoveY - mLastDownOrMoveEventY));
                scrollBy(0, deltaMoveY);
                invalidate();
            }
            mLastDownOrMoveEventY = currentMoveY;
        }
        break;
    case MotionEvent::ACTION_UP:
        removeBeginSoftInputCommand();
        removeChangeCurrentByOneFromLongPress();
        pshCancel();
        mVelocityTracker->computeCurrentVelocity(1000, mMaximumFlingVelocity);
        if(isHorizontalMode()){
            int initialVelocity = (int) mVelocityTracker->getXVelocity();
            if (std::abs(initialVelocity) > mMinimumFlingVelocity) {
                fling(initialVelocity);
                onScrollStateChange(OnScrollListener::SCROLL_STATE_FLING);
            } else {
                const int eventX = (int) event.getX();
                const int deltaMoveX = (int) std::abs(eventX - mLastDownEventX);
                if (deltaMoveX <= mTouchSlop) {
                    int selectorIndexOffset = (eventX / mSelectorElementSize)
                            - mWheelMiddleItemIndex;
                    if (selectorIndexOffset > 0) {
                        changeValueByOne(true);
                    } else if (selectorIndexOffset < 0) {
                        changeValueByOne(false);
                    } else {
                        ensureScrollWheelAdjusted();
                    }
                } else {
                    ensureScrollWheelAdjusted();
                }
                onScrollStateChange(OnScrollListener::SCROLL_STATE_IDLE);
            }
         }else{
            const int initialVelocity = (int) mVelocityTracker->getYVelocity();
            if (std::abs(initialVelocity) > mMinimumFlingVelocity) {
                fling(initialVelocity);
                onScrollStateChange(OnScrollListener::SCROLL_STATE_FLING);
            } else {
                const int eventY = (int) event.getY();
                const int deltaMoveY = (int) std::abs(eventY - mLastDownEventY);
                if (deltaMoveY <= mTouchSlop){
                    int selectorIndexOffset = (eventY / mSelectorElementSize) - mWheelMiddleItemIndex;
                    if (selectorIndexOffset > 0) {
                        changeValueByOne(true);
                        pshButtonTapped(R::id::increment);
                    } else if (selectorIndexOffset < 0) {
                        changeValueByOne(false);
                        pshButtonTapped(R::id::decrement);
                    }else{
                        ensureScrollWheelAdjusted();
                    }
                }else{
                    ensureScrollWheelAdjusted();
                }
                onScrollStateChange(OnScrollListener::SCROLL_STATE_IDLE);
            }
        }
        mVelocityTracker->recycle();
        mVelocityTracker = nullptr;
        break;
    }//end switch
    return true;
}

bool NumberPicker::dispatchTouchEvent(MotionEvent& event){
    int action = event.getActionMasked();
    switch (action) {
    case MotionEvent::ACTION_CANCEL:LOGD("ACTION_CANCEL");
    case MotionEvent::ACTION_UP:
        removeAllCallbacks();
        break;
    }
    return LinearLayout::dispatchTouchEvent(event);
}

bool NumberPicker::dispatchKeyEvent(KeyEvent& event){
    int keyCode = event.getKeyCode();
    switch (keyCode) {
    case KeyEvent::KEYCODE_DPAD_CENTER:
    case KeyEvent::KEYCODE_ENTER:
        removeAllCallbacks();
        break;
    case KeyEvent::KEYCODE_DPAD_DOWN:
    case KeyEvent::KEYCODE_DPAD_UP:
        switch (event.getAction()) {
        case KeyEvent::ACTION_DOWN:
            if (mWrapSelectorWheel || ((keyCode == KeyEvent::KEYCODE_DPAD_DOWN)
                    ? getValue() < getMaxValue() : getValue() > getMinValue())) {
            requestFocus();
            mLastHandledDownDpadKeyCode = keyCode;
            removeAllCallbacks();
            changeValueByOne(keyCode == KeyEvent::KEYCODE_DPAD_DOWN);
            return true;
        }break;
        case KeyEvent::ACTION_UP:
            if (mLastHandledDownDpadKeyCode == keyCode) {
                mLastHandledDownDpadKeyCode = -1;
                return true;
            }break;
        }
    }
    return LinearLayout::dispatchKeyEvent(event);
}

void NumberPicker::computeScroll() {
    Scroller* scroller = mFlingScroller;
    if (scroller->isFinished()) {
        scroller = mAdjustScroller;
        if (scroller->isFinished()) {
            return;
        }
    }
    scroller->computeScrollOffset();
    if(isHorizontalMode()){
        int currentScrollerX = scroller->getCurrX();
        if (mPreviousScrollerX == 0) {
            mPreviousScrollerX = scroller->getStartX();
        }
        scrollBy(currentScrollerX - mPreviousScrollerX, 0);
        mPreviousScrollerX = currentScrollerX;
    }else{
        int currentScrollerY = scroller->getCurrY();
        if (mPreviousScrollerY == 0) {
            mPreviousScrollerY = scroller->getStartY();
        }
        scrollBy(0, currentScrollerY - mPreviousScrollerY);
        mPreviousScrollerY = currentScrollerY;
    }
    if (scroller->isFinished()) {
        onScrollerFinished(scroller);
    } else {
        postInvalidate();
    }
}

View& NumberPicker::setEnabled(bool enabled) {
    ViewGroup::setEnabled(enabled);
    mSelectedText->setEnabled(enabled);
    return *this;
}

void NumberPicker::scrollBy(int x, int y){
    std::vector<int>&selectorIndices = mSelectorIndices;
    const int startScrollOffset = mCurrentScrollOffset;
    const int gap = mSelectorElementSize/2;
    const int xy = isHorizontalMode() ? x : y;
    if (isAscendingOrder()) {
        if (!mWrapSelectorWheel && xy > 0
                && selectorIndices[mWheelMiddleItemIndex] <= mMinValue) {//changed from <=--><,make items wrapable
            mCurrentScrollOffset = mInitialScrollOffset;
            return;
        }
        if (!mWrapSelectorWheel && xy < 0
                && selectorIndices[mWheelMiddleItemIndex] >= mMaxValue) {
            mCurrentScrollOffset = mInitialScrollOffset;
            return;
        }
    } else {
        if (!mWrapSelectorWheel && xy > 0
                && selectorIndices[mWheelMiddleItemIndex] >= mMaxValue) {//changed from >=-->>,make items wrapable
            mCurrentScrollOffset = mInitialScrollOffset;
            return;
        }
        if (!mWrapSelectorWheel && xy < 0
                && selectorIndices[mWheelMiddleItemIndex] <= mMinValue) {
            mCurrentScrollOffset = mInitialScrollOffset;
            return;
        }
    }
    mCurrentScrollOffset += xy;

    while (mCurrentScrollOffset - mInitialScrollOffset >  gap) {
        mCurrentScrollOffset -= mSelectorElementSize;
        if(isAscendingOrder())
            decrementSelectorIndices(selectorIndices);
        else
            incrementSelectorIndices(selectorIndices);
        setValueInternal(selectorIndices[mWheelMiddleItemIndex], true);
        if (!mWrapSelectorWheel && selectorIndices[mWheelMiddleItemIndex] <= mMinValue) {
            mCurrentScrollOffset = mInitialScrollOffset;
        }
    }

    while (mCurrentScrollOffset - mInitialScrollOffset < - gap) {
        mCurrentScrollOffset += mSelectorElementSize;
        if(isAscendingOrder())
            incrementSelectorIndices(selectorIndices);
        else
            decrementSelectorIndices(selectorIndices);
        setValueInternal(selectorIndices[mWheelMiddleItemIndex], true);
        if (!mWrapSelectorWheel && selectorIndices[mWheelMiddleItemIndex] >= mMaxValue) {
            mCurrentScrollOffset = mInitialScrollOffset;
        }
    }
    if (startScrollOffset != mCurrentScrollOffset) {
        if(isHorizontalMode())
            onScrollChanged(mCurrentScrollOffset,0,startScrollOffset,0);
        else
            onScrollChanged(0, mCurrentScrollOffset, 0, startScrollOffset);
    }
}

int NumberPicker::computeHorizontalScrollOffset(){
    return isHorizontalMode()?mCurrentScrollOffset:0;
}
int NumberPicker::computeHorizontalScrollRange(){
    return isHorizontalMode()?(mMaxValue - mMinValue +1)*mSelectorElementSize:0;
}
int NumberPicker::computeHorizontalScrollExtent(){
    return isHorizontalMode()?getWidth():0;
}
int NumberPicker::computeVerticalScrollOffset() {
    return isHorizontalMode()?0:mCurrentScrollOffset;
}

int NumberPicker::computeVerticalScrollRange() {
    //return std::min(mMaxValue - mMinValue + 1,mMaxSelectorIndices) * mSelectorElementSize;
    return isHorizontalMode()?0:(mMaxValue - mMinValue + 1) * mSelectorElementSize;
}

int NumberPicker::computeVerticalScrollExtent() {
    return isHorizontalMode()?0:getHeight();
}

void NumberPicker::setOnClickListener(OnClickListener onClickListener){
    mOnClickListener = onClickListener;
}
void NumberPicker::setOnValueChangedListener(OnValueChangeListener onValueChangedListener){
    mOnValueChangeListener=onValueChangedListener;
}

void NumberPicker::setOnScrollListener(const OnScrollListener& onScrollListener){
    mOnScrollListener = onScrollListener;
}

void NumberPicker::setFormatter(Formatter formatter){
    mFormatter = formatter;
    initializeSelectorWheelIndices();
    updateInputTextView();
}

void NumberPicker::setValue(int value) {
    setValueInternal(value, false);
}

float NumberPicker::getMaxTextSize()const {
    return std::max(std::max(mTextSize,mDisplayedDrawableSize), mSelectedTextSize);
}

bool NumberPicker::performClick() {
    if (true/*!mHasSelectorWheel*/) {
        return ViewGroup::performClick();
    } else if (!ViewGroup::performClick()) {
        showSoftInput();
    }
    return true;
}

bool NumberPicker::performLongClick() {
    if (true/*!mHasSelectorWheel*/) {
        return ViewGroup::performLongClick();
    } else if (!ViewGroup::performLongClick()) {
        showSoftInput();
    }
    return true;
}

void NumberPicker::showSoftInput(){
    //if(mHasSelectorWheel)
	mSelectedText->setVisibility(View::VISIBLE);
}

void NumberPicker::hideSoftInput(){
    if (mSelectedText->getInputType() != EditText::TYPE_NONE) {
        mSelectedText->setVisibility(View::INVISIBLE);
    }
}

void NumberPicker::tryComputeMaxWidth(){
    if (!mComputeMaxWidth) {
        return;
    }
    int maxTextWidth = 0;
    Layout l(mTextSize,-1);
    l.setTypeface(mSelectedText->getTypeface());
    if (mDisplayedValues.size() == 0) {
        float maxDigitWidth = 0;
        for (int i = 0; i <= 9; i++) {
            l.setText(std::to_string(i));
            l.relayout();
            const float digitWidth = l.getMaxLineWidth();
            if (digitWidth > maxDigitWidth) {
                maxDigitWidth = digitWidth;
            }
        }
        int numberOfDigits = 0;
        int current = mMaxValue;
        while (current > 0) {
            numberOfDigits++;
            current = current / 10;
        }
        maxTextWidth = (int) (numberOfDigits * maxDigitWidth);
    } else {
        const int valueCount = mDisplayedValues.size();
        for (int i = 0; i < valueCount; i++) {
            l.setText(mDisplayedValues[i]);
            l.relayout();
            const float textWidth = l.getMaxLineWidth();
            if (textWidth > maxTextWidth) {
                maxTextWidth = (int) textWidth;
            }
        }
    }
    maxTextWidth += mSelectedText->getPaddingLeft() + mSelectedText->getPaddingRight();
    if (mMaxWidth != maxTextWidth) {
        mMaxWidth = std::max(mMinWidth,maxTextWidth);
        invalidate();
    }
}

bool NumberPicker::getWrapSelectorWheel()const{
    return mWrapSelectorWheel;
}

void NumberPicker::setWrapSelectorWheel(bool wrapSelectorWheel) {
    mWrapSelectorWheelPreferred = wrapSelectorWheel;
    updateWrapSelectorWheel();
}

void NumberPicker::updateWrapSelectorWheel() {
    const bool wrappingAllowed = (mMaxValue - mMinValue + 1) >= mSelectorIndices.size();
    mWrapSelectorWheel = wrappingAllowed && mWrapSelectorWheelPreferred;
}

void NumberPicker::setOnLongPressUpdateInterval(long intervalMillis) {
    mLongPressUpdateInterval = intervalMillis;
}

EditText*NumberPicker::getSelectedText()const{
    return mSelectedText;
}

Drawable*NumberPicker::getDivider()const{
    return mDividerDrawable;
}

void NumberPicker::setDivider(Drawable*d){
    delete mDividerDrawable;
    mDividerDrawable = d;
    if (mDividerDrawable) {
        mDividerDrawable->setCallback(this);
        mDividerDrawable->setLayoutDirection(getLayoutDirection());
        if (mDividerDrawable->isStateful()) {
            mDividerDrawable->setState(getDrawableState());
        }
    }
    invalidate();
}

void NumberPicker::setSelectionDivider(Drawable*d){
    setDivider(d);
}

Drawable*NumberPicker::getSelectionDivider()const{
    return mDividerDrawable;
}

int  NumberPicker::getDividerColor()const{
    return mDividerColor;
}

void NumberPicker::setDividerColor(int color){
    mDividerColor = color;
    mDividerDrawable = new ColorDrawable(color);
}

int NumberPicker::getDividerType()const{
    return mDividerType;
}

void NumberPicker::setDividerType(int type){
    mDividerType = type;
    invalidate();
}

int NumberPicker::getDividerThickness()const{
    return mDividerThickness;
}

void NumberPicker::setDividerThickness(int thickness) {
    mDividerThickness = thickness;
    invalidate();
}

int NumberPicker::getOrder()const{
    return mOrder;
}

void NumberPicker::setOrder(int order) {
    mOrder = order;
    invalidate();
}

int NumberPicker::getValue()const{
    return mValue;
}

int NumberPicker::getMinValue()const{
    return mMinValue;
}

void NumberPicker::setMinValue(int minValue){
    mMinValue = minValue;
    if (mMinValue > mValue) mValue = mMinValue;
    updateWrapSelectorWheel();
    initializeSelectorWheelIndices();
    updateInputTextView();
    tryComputeMaxWidth();
    invalidate();
}

int NumberPicker::getMaxValue()const{
    return mMaxValue;
}

void NumberPicker::setMaxValue(int maxValue) {
    mMaxValue = maxValue;
    if (mMaxValue < mValue) mValue = mMaxValue;
    updateWrapSelectorWheel();
    initializeSelectorWheelIndices();
    updateInputTextView();
    tryComputeMaxWidth();
    invalidate();
}

void NumberPicker::setMinHeight(int h){
    mMinHeight = h;
}

void NumberPicker::setMaxHeight(int h){
    mMaxHeight = h;
}

int NumberPicker::getMinHeight()const{
    return mMinHeight;
}

int NumberPicker::getMaxHeight()const{
    return mMaxHeight;
}

std::vector<std::string>  NumberPicker::getDisplayedValues()const{
    return mDisplayedValues;
}

void  NumberPicker::setDisplayedValues(const std::vector<std::string>&displayedValues){
    mDisplayedValues = displayedValues;
    mSelectorIndexToStringCache.clear();
    updateInputTextView();
    initializeSelectorWheelIndices();
    tryComputeMaxWidth();
    for(auto d:mDisplayedDrawables)delete d;
    mDisplayedDrawables.clear();
    mDisplayedDrawableCount = 0;
    mDisplayedDrawableSize = 0;
    int drsize=0;
    for(auto s:mDisplayedValues){
        Drawable*dr = mContext->getDrawable(s);
        mDisplayedDrawables.push_back(dr);
        if(dr){
            drsize += (isHorizontalMode()?dr->getIntrinsicWidth():dr->getIntrinsicHeight());
            mDisplayedDrawableCount++;
        }
    }
    if(mDisplayedDrawableCount==mDisplayedValues.size())
        mSelectedText->setVisibility(View::INVISIBLE); 
    if(mDisplayedDrawableCount)
        mDisplayedDrawableSize = drsize/mDisplayedDrawableCount;
}


void NumberPicker::drawableStateChanged() {
    ViewGroup::drawableStateChanged();

    if (mDividerDrawable!=nullptr  && mDividerDrawable->isStateful()
        && mDividerDrawable->setState(getDrawableState())) {
        invalidateDrawable(*mDividerDrawable);
    }
}

void NumberPicker::jumpDrawablesToCurrentState() {
    ViewGroup::jumpDrawablesToCurrentState();

    if (mDividerDrawable != nullptr) {
        mDividerDrawable->jumpToCurrentState();
    }
}

void NumberPicker::onResolveDrawables(int layoutDirection){
    ViewGroup::onResolveDrawables(layoutDirection);
    if (mDividerDrawable) {
        mDividerDrawable->setLayoutDirection(layoutDirection);
    }
}

void NumberPicker::setTextColor(int color){
    mTextColor = color;
    mTextColor2= color;
    invalidate();
}
void NumberPicker::setTextColor(int color,int color2){
    mTextColor  = color;
    mTextColor2 = color2;
    invalidate();
}

int  NumberPicker::getTextColor()const{
    return mTextColor;
}

void NumberPicker::setTextSize(int size){
    mTextSize  = size;
    mTextSize2 = size;
    invalidate();
}
void NumberPicker::setTextSize(int size,int size2){
    mTextSize  = size;
    mTextSize2 = size2;
    requestLayout();
    invalidate();
}

int  NumberPicker::getTextSize()const{
    return mTextSize;
}

int  NumberPicker::getSelectedTextColor()const{
    return mSelectedTextColor;
}

void NumberPicker::setSelectedTextColor(int textColor){
    mSelectedTextColor = textColor;
    mSelectedText->setTextColor(textColor);
}

int  NumberPicker::getSelectedTextSize()const{
    return mSelectedTextSize;
}

void NumberPicker::setSelectedTextSize(int textSize) {
    mSelectedTextSize = textSize;
    mSelectedText->setTextSize(textSize);
    requestLayout();
    invalidate();
}

void NumberPicker::setSelectedTypeface(Typeface* typeface){
    mSelectedTypeface = typeface;
    if (mSelectedTypeface != nullptr) {
        //do nothing mSelectorWheelPaint.setTypeface(mSelectedTypeface);
    } else if (mTypeface != nullptr) {
        mSelectedTypeface = mTypeface;
    } else {
        mSelectedTypeface = Typeface::MONOSPACE;
    }
}

void NumberPicker::setSelectedTypeface(const std::string& string, int style){
    setSelectedTypeface(Typeface::create(string, style));
}

Typeface* NumberPicker::getSelectedTypeface()const{
    return mSelectedTypeface;
}

void NumberPicker::setTypeface(Typeface* typeface){
    mTypeface = typeface;
    if (mTypeface != nullptr) {
        mSelectedText->setTypeface(mTypeface);
        setSelectedTypeface(mSelectedTypeface);
    } else {
        mSelectedText->setTypeface(Typeface::MONOSPACE);
    }
}

void NumberPicker::setTypeface(const std::string& string, int style){
    setTypeface(Typeface::create(string, style));
}

Typeface* NumberPicker::getTypeface()const{
    return mTypeface;
}

void NumberPicker::onDraw(Canvas&canvas){
    const bool showSelectorWheel = !mHideWheelUntilFocused || hasFocus();
    float x=0, y=0;
    Rect recText;
    const int textGravity = mSelectedText?mSelectedText->getGravity():Gravity::CENTER;
    canvas.save();
    if (isHorizontalMode()) {
        x = mCurrentScrollOffset - mSelectorElementSize/2;
        recText = Rect::Make(x,y,mSelectorElementSize,getHeight());
        if (mRealWheelItemCount < DEFAULT_WHEEL_ITEM_COUNT) {
            canvas.rectangle(mLeftDividerLeft, 0, mRightDividerRight-mLeftDividerLeft, getHeight());
            canvas.clip();
        }
    } else {
        y = mCurrentScrollOffset - mSelectorElementSize/2;
        recText = Rect::Make(0,y,getWidth(),mSelectorElementSize);
        if (mRealWheelItemCount < DEFAULT_WHEEL_ITEM_COUNT) {
            canvas.rectangle(0, mTopDividerTop, getWidth(), mBottomDividerBottom-mTopDividerTop);
            canvas.clip();
        }
    }
    if( mTextColor != mTextColor2 ){
        if( mPat == nullptr ) {
            Color c1(mTextColor), c2(mTextColor2);
            CycleInterpolator ci(0.5f);
            if(isHorizontalMode())
                mPat = Cairo::LinearGradient::create( 0 , 0 , getWidth() , 0);
            else
                mPat = Cairo::LinearGradient::create( 0 , 0 , 0, getHeight());
            const int cStops = mSelectorIndices.size()*3;
            for(int i = 0; i < cStops ;i++){
                const float offset = float(i)/cStops;
                const float fraction = ci.getInterpolation(offset);
                mPat->add_color_stop_rgba(offset,lerp(c2.red(),c1.red(),fraction),lerp(c2.green(),c1.green(),fraction),
                                 lerp(c2.blue(),c1.blue(),fraction), std::abs(lerp(c2.alpha(),c1.alpha(),fraction)));
            }
        }
        canvas.set_source(mPat);
    }else{
        canvas.set_color(mTextColor);
    }
    canvas.set_font_face(mSelectedText->getTypeface()->getFontFace()->get_font_face());
    canvas.set_font_size(mTextSize);
    // draw the selector wheel
    for (int i = 0; i < mSelectorIndices.size(); i++) {
        float font_size  = mTextSize;
        int selectedSize = mSelectorElementSize;
        if(mTextSize != mTextSize2){
            const float harfSize = (isHorizontalMode()?getWidth():getHeight())/2.f;
            const float fraction = std::abs( (isHorizontalMode()?x:y) - harfSize + mSelectorElementSize/2)/harfSize;
            font_size = lerp(mTextSize,mTextSize2,fraction);
            canvas.set_font_size(font_size);
        }

        int selectorIndex = mSelectorIndices[isAscendingOrder() ? i : mSelectorIndices.size() - i - 1];
        std::string scrollSelectorValue = mSelectorIndexToStringCache.at(selectorIndex);
        if (scrollSelectorValue.empty()) {
            if(isHorizontalMode()){
                x += selectedSize;
                recText.offset(selectedSize,0);
            }else{
                y += selectedSize;
                recText.offset(0,selectedSize);
            }
            continue;
        }
        if(mSelectedText->getVisibility()==View::VISIBLE){
            if(isHorizontalMode()==false){
                if(i==mWheelMiddleItemIndex)
                    selectedSize = std::max(mSelectorElementSize,mSelectedText->getHeight());
                recText.height = selectedSize;
            }else{
                if(i==mWheelMiddleItemIndex)
                    selectedSize = std::max(mSelectorElementSize,mSelectedText->getWidth());
                recText.width = selectedSize;
            }
        }
        // Do not draw the middle item if input is visible since the input
        // is shown only if the wheel is static and it covers the middle item.
        // Otherwise, if the user starts editing the text via the IME he may 
        // see a dimmed version of the old value intermixed with the new one.
        if ((showSelectorWheel && i != mWheelMiddleItemIndex) || (i == mWheelMiddleItemIndex && mSelectedText->getVisibility() != VISIBLE)) {
            Drawable*dr = nullptr;
            if(selectorIndex<mDisplayedDrawables.size() && (dr = mDisplayedDrawables.at(selectorIndex))){
                Rect outRect;
                const ColorStateList*cl = getForegroundTintList();
                Gravity::apply(textGravity,dr->getIntrinsicWidth(),dr->getIntrinsicHeight(),recText,outRect,getLayoutDirection());
                dr->setBounds(outRect);
                if(cl){
                    const int color =cl->getColorForState((i != mWheelMiddleItemIndex?StateSet::NOTHING:StateSet::SELECTED_STATE_SET),0xFFFFFFFF);
                    dr->setTint(color);
                }
                dr->draw(canvas);
            }else{
                canvas.draw_text(recText,scrollSelectorValue,textGravity);
            }
        }
        if (isHorizontalMode()) {
            x += selectedSize;
            recText.offset(selectedSize,0);
        } else {
            y += selectedSize;
            recText.offset(0,selectedSize);
        }
    }
    canvas.restore();

    // draw the dividers
    if (showSelectorWheel && mDividerDrawable) {
        if (isHorizontalMode())
            drawHorizontalDividers(canvas);
        else
            drawVerticalDividers(canvas);
    }
}

void NumberPicker::drawHorizontalDividers(Canvas& canvas) {
    int top,bottom,left,right;
    int leftOfLeftDivider/*,rightOfLeftDivider*/;
    int /*bottomOfUnderlineDivider,*/topOfUnderlineDivider;
    int rightOfRightDivider,leftOfRightDivider;

    switch (mDividerType) {
    case SIDE_LINES:
        if (mDividerLength > 0 && mDividerLength <= mMaxHeight) {
            top = (mMaxHeight - mDividerLength) / 2;
            bottom = top + mDividerLength;
        } else {
            top = 0;
            bottom = getBottom();
        }
        // draw the left divider
        leftOfLeftDivider = mLeftDividerLeft;
        //rightOfLeftDivider = leftOfLeftDivider + mDividerThickness;
        mDividerDrawable->setBounds(leftOfLeftDivider, top, mDividerThickness, bottom-top);
        mDividerDrawable->draw(canvas);
        // draw the right divider
        rightOfRightDivider = mRightDividerRight;
        leftOfRightDivider = rightOfRightDivider - mDividerThickness;
        mDividerDrawable->setBounds(leftOfRightDivider, top, mDividerThickness, bottom-top);
        mDividerDrawable->draw(canvas);
        break;
    case UNDERLINE:
        if (mDividerLength > 0 && mDividerLength <= mMaxWidth) {
            left = (mMaxWidth - mDividerLength) / 2;
            right = left + mDividerLength;
        } else {
            left = mLeftDividerLeft;
            right = mRightDividerRight;
        }
        //bottomOfUnderlineDivider = mBottomDividerBottom;
        mDividerDrawable->setBounds(left,topOfUnderlineDivider,right - left,mDividerThickness);
        mDividerDrawable->draw(canvas);
        break;
   }
}

void NumberPicker::drawVerticalDividers(Canvas& canvas) {
    int left, right;
    int topOfTopDivider/*,bottomOfTopDivider*/;
    int bottomOfUnderlineDivider,topOfUnderlineDivider;
    int topOfBottomDivider,bottomOfBottomDivider;
    if (mDividerLength > 0 && mDividerLength <= mMaxWidth) {
        left = (mMaxWidth - mDividerLength) / 2;
        right = left + mDividerLength;
    } else {
        left = 0;
        right = getRight();
    }
    switch (mDividerType) {
    case SIDE_LINES:
        // draw the top divider
        topOfTopDivider = mTopDividerTop;
        //bottomOfTopDivider = topOfTopDivider + mDividerThickness;
        mDividerDrawable->setBounds(left, topOfTopDivider, right-left, mDividerThickness);
        mDividerDrawable->draw(canvas);
        // draw the bottom divider
        bottomOfBottomDivider = mBottomDividerBottom;
        topOfBottomDivider = bottomOfBottomDivider - mDividerThickness;
        mDividerDrawable->setBounds(left,topOfBottomDivider,right-left, mDividerThickness);
        mDividerDrawable->draw(canvas);
        break;
    case UNDERLINE:
        bottomOfUnderlineDivider = mBottomDividerBottom;
        topOfUnderlineDivider = bottomOfUnderlineDivider - mDividerThickness;
        mDividerDrawable->setBounds(left,topOfUnderlineDivider,right-left, mDividerThickness);
        mDividerDrawable->draw(canvas);
        break;
    }
}

void NumberPicker::drawText(const std::string& text, float x, float y,Canvas& canvas) {
    /*if (text.contains("\n")) {
        std::string[] lines = text.split("\n");
        float height = Math.abs(paint.descent() + paint.ascent())
                * mLineSpacingMultiplier;
        float diff = (lines.length - 1) * height / 2;
        y -= diff;
        for (String line : lines) {
            canvas.drawText(line, x, y, paint);
            y += height;
        }
    } else */{
        canvas.move_to(x,y);
        canvas.show_text(text);
    }
}

int NumberPicker::makeMeasureSpec(int measureSpec, int maxSize){
    if (maxSize == SIZE_UNSPECIFIED) {
        return measureSpec;
    }
    int size = MeasureSpec::getSize(measureSpec);
    int mode = MeasureSpec::getMode(measureSpec);
    switch (mode) {
    case MeasureSpec::EXACTLY:     return measureSpec;
    case MeasureSpec::AT_MOST:     return MeasureSpec::makeMeasureSpec(std::min(size, maxSize), MeasureSpec::EXACTLY);
    case MeasureSpec::UNSPECIFIED: return MeasureSpec::makeMeasureSpec(maxSize, MeasureSpec::EXACTLY);
    default:        throw std::string("Unknown measure mode: ")+std::to_string(mode);
    }
}

int NumberPicker::resolveSizeAndStateRespectingMinSize(int minSize, int measuredSize, int measureSpec) {
    if (minSize != SIZE_UNSPECIFIED) {
        int desiredWidth = std::max(minSize, measuredSize);
        return resolveSizeAndState(desiredWidth, measureSpec, 0);
    } else {
        return measuredSize;
    }
}

void NumberPicker::initializeSelectorWheelIndices(){
    mSelectorIndexToStringCache.clear();
    const int current = getValue();
    //const int count = (std::abs(mMaxValue - mMinValue) + 1);
    for (int i = 0; i < mSelectorIndices.size(); i++) {
        int selectorIndex = current + (i - mWheelMiddleItemIndex);
        if (mWrapSelectorWheel) {
            selectorIndex = getWrappedSelectorIndex(selectorIndex);
        }
        //if(mSelectorIndices.size() > count)
        //    selectorIndex = (selectorIndex + count)%count;
        mSelectorIndices[i] = selectorIndex;
        ensureCachedScrollSelectorValue(mSelectorIndices[i]);
    }
}

void NumberPicker::setValueInternal(int current, bool notifyChng){
    if (mValue == current) {
        return;
    }
    // Wrap around the values if we go past the start or end
    if (mWrapSelectorWheel) {
        current = getWrappedSelectorIndex(current);
    } else {
        current = std::max(current, mMinValue);
        current = std::min(current, mMaxValue);
    }
    int previous = mValue;
    mValue = current;
    // If we're flinging, we'll update the text view at the end when it becomes visible
    if (mScrollState != OnScrollListener::SCROLL_STATE_FLING)
        updateInputTextView();
    if (notifyChng)
        notifyChange(previous, current);

    initializeSelectorWheelIndices();
    invalidate();
}

void NumberPicker::changeValueByOne(bool increment){
    if (!moveToFinalScrollerPosition(mFlingScroller)) {
        moveToFinalScrollerPosition(mAdjustScroller);
    }
    if(mFlingScroller->isFinished()){
        smoothScroll(increment,1);
    }else{
        setValueInternal(mValue+(increment?1:-1),true);
    }
}

void NumberPicker::smoothScrollToPosition(int position) {
    const int currentPosition = mSelectorIndices[mWheelMiddleItemIndex];
    if (currentPosition == position) {
        return;
    }
    smoothScroll(position > currentPosition, std::abs(position - currentPosition));
}

void NumberPicker::smoothScroll(bool increment, int steps) {
    const int diffSteps = (increment ? -mSelectorElementSize : mSelectorElementSize) * steps;
    if (isHorizontalMode()) {
        mPreviousScrollerX = 0;
        mFlingScroller->startScroll(0, 0, diffSteps, 0, SNAP_SCROLL_DURATION);
    } else {
        mPreviousScrollerY = 0;
        mFlingScroller->startScroll(0, 0, 0, diffSteps, SNAP_SCROLL_DURATION);
    }
    invalidate();
}

void NumberPicker::initializeSelectorWheel(){
    initializeSelectorWheelIndices();
    std::vector<int>& selectorIndices = mSelectorIndices;
    const float textGapCount = selectorIndices.size();
    const int inputEdit_Size = isHorizontalMode()?mSelectedText->getWidth():mSelectedText->getHeight();
    if (isHorizontalMode()) {
        int selectedWidth= (mSelectedText->getVisibility()==View::VISIBLE)?std::max(mSelectedTextSize,inputEdit_Size):mSelectedTextSize;
        const int totalTextSize = int ((selectorIndices.size() - 1) * mTextSize + selectedWidth);
        const float totalTextGapWidth = getWidth() - totalTextSize;
        mSelectorTextGapWidth = (int) (totalTextGapWidth / textGapCount);
        mSelectorElementSize = std::min(getMaxTextSize() + mSelectorTextGapWidth,getWidth()/textGapCount);
        if(mSelectedText->getVisibility()==View::INVISIBLE)
            selectedWidth = mSelectorElementSize;
        mInitialScrollOffset = (int) (mSelectedTextCenter - mSelectorElementSize * mWheelMiddleItemIndex-(selectedWidth-mSelectorElementSize)/2);
    } else {
        int selectedHeight= std::max(mSelectorElementSize,inputEdit_Size);//:mSelectedTextSize;
        const int totalTextSize = int ((selectorIndices.size() - 1) * mTextSize + selectedHeight);
        const float totalTextGapHeight= getHeight() - totalTextSize;
        mSelectorTextGapHeight = (int) (totalTextGapHeight / textGapCount);
        mSelectorElementSize = std::min(getMaxTextSize() + mSelectorTextGapHeight,getHeight()/textGapCount);
        if(mSelectedText->getVisibility()==View::INVISIBLE ||(selectedHeight-mSelectorElementSize<0))
            selectedHeight = mSelectorElementSize;
        mInitialScrollOffset = (int) (mSelectedTextCenter - mSelectorElementSize * mWheelMiddleItemIndex-(selectedHeight-mSelectorElementSize)/2);
    }
    LOGV("mInitialScrollOffset=%d %d/%d textsize=%d,%d",mInitialScrollOffset,mSelectorElementSize,mSelectedText->getHeight(),mSelectedTextSize,mTextSize);
    mCurrentScrollOffset = mInitialScrollOffset;
    updateInputTextView();
}

void NumberPicker::initializeFadingEdges(){
    if(mTextColor!=mTextColor2)
	    return;
    const int size = isHorizontalMode()?getWidth():getHeight();
    setFadingEdgeLength((size - mTextSize)/2);
}

void NumberPicker::onScrollerFinished(Scroller* scroller) {
    if (scroller == mFlingScroller) {
        ensureScrollWheelAdjusted();
        updateInputTextView();
        onScrollStateChange(OnScrollListener::SCROLL_STATE_IDLE);
    } else {
        if (mScrollState != OnScrollListener::SCROLL_STATE_TOUCH_SCROLL) {
            updateInputTextView();
        }
    }
}

void NumberPicker::onScrollStateChange(int scrollState) {
    if (mScrollState == scrollState)  return;
    mScrollState = scrollState;
    if (mOnScrollListener.onScrollStateChange) mOnScrollListener.onScrollStateChange(*this, scrollState);
}

void NumberPicker::fling(int velocity) {
    if (isHorizontalMode()){
        mPreviousScrollerX = 0;
        if (velocity > 0) {
            mFlingScroller->fling(0, 0, velocity, 0, 0, INT_MAX, 0, 0);
        } else {
            mFlingScroller->fling(INT_MAX, 0, velocity, 0, 0, INT_MAX, 0, 0);
        }
    }else{
        mPreviousScrollerY = 0;
        if (velocity > 0) {
            mFlingScroller->fling(0, 0, 0, velocity, 0, 0, 0,INT_MAX);
        } else {
            mFlingScroller->fling(0, INT_MAX, 0, velocity, 0, 0, 0,INT_MAX);
        }
    }
}

int NumberPicker::getWrappedSelectorIndex(int selectorIndex){
   if (selectorIndex > mMaxValue) {
        return mMinValue + (selectorIndex - mMaxValue) % (mMaxValue - mMinValue) - 1;
    } else if (selectorIndex < mMinValue) {
        return mMaxValue - (mMinValue - selectorIndex) % (mMaxValue - mMinValue) + 1;
    }
    return selectorIndex;
}

void NumberPicker::incrementSelectorIndices(std::vector<int>&selectorIndices){
    for (int i = 0; i < selectorIndices.size() - 1; i++) {
        selectorIndices[i] = selectorIndices[i + 1];
    }
    int nextScrollSelectorIndex = selectorIndices[selectorIndices.size() - 2] + 1;
    if (mWrapSelectorWheel && nextScrollSelectorIndex > mMaxValue) {
        nextScrollSelectorIndex = mMinValue;
    }
    selectorIndices[selectorIndices.size() - 1] = nextScrollSelectorIndex;
    ensureCachedScrollSelectorValue(nextScrollSelectorIndex);
}

void NumberPicker::decrementSelectorIndices(std::vector<int>&selectorIndices) {
    for (int i = selectorIndices.size() - 1; i > 0; i--) {
        selectorIndices[i] = selectorIndices[i - 1];
    }
    int nextScrollSelectorIndex = selectorIndices[1] - 1;
    if (mWrapSelectorWheel && nextScrollSelectorIndex < mMinValue) {
        nextScrollSelectorIndex = mMaxValue;
    }
    selectorIndices[0] = nextScrollSelectorIndex;
    ensureCachedScrollSelectorValue(nextScrollSelectorIndex);
}

void NumberPicker::ensureCachedScrollSelectorValue(int selectorIndex) {
    std::string scrollSelectorValue;
    std::map<int,std::string>& cache = mSelectorIndexToStringCache;
    auto itr= cache.find(selectorIndex);

    if (cache.size()&&(itr != cache.end())) return;

    if ((selectorIndex < mMinValue)||(selectorIndex > mMaxValue)||(mMinValue==mMaxValue)) {
        scrollSelectorValue = "";
    } else {
        if (mDisplayedValues.size()){
            const int displayedValueIndex = selectorIndex - mMinValue;
            /*if(cache.size()&&(displayedValueIndex >=mDisplayedValues.size())){
                cache.erase(itr);
                return;
            }*/
            if((displayedValueIndex>=0)&&(displayedValueIndex<mDisplayedValues.size()))
                scrollSelectorValue = mDisplayedValues[displayedValueIndex];
        } else {
            scrollSelectorValue = formatNumber(selectorIndex);
        }
    }
    cache[selectorIndex] = scrollSelectorValue;
}

std::string NumberPicker::formatNumber(int value){
    return (mFormatter != nullptr) ? mFormatter(value):std::to_string(value);
}

void NumberPicker::validateInputTextView(View* v){
    std::string str =((TextView*)v)->getText();// String.valueOf(((TextView*) v)->getText());
    if (str.empty()){ // Restore to the old value as we don't allow empty values
        updateInputTextView();
    } else {  // Check the new value and ensure it's in range
        int current = getSelectedPos(str);//.toString());
        setValueInternal(current, true);
    }
}

bool NumberPicker::updateInputTextView(){
    std::string text = (mDisplayedValues.size() == 0) ? formatNumber(mValue) : mDisplayedValues[mValue - mMinValue];
    if (!text.empty() ){
        std::string beforeText = mSelectedText->getText();
        if (text != beforeText){//!text.equals(beforeText.toString())) {
            mSelectedText->setText(text);
            return true;
        }
    }

    return false;
}

void NumberPicker::notifyChange(int previous, int current){
    if (mOnValueChangeListener) {
        mOnValueChangeListener(*this, previous, mValue);
    }
}

void NumberPicker::postChangeCurrentByOneFromLongPress(bool increment, long delayMillis){
    if(mChangeCurrentByOneFromLongPressCommand!=nullptr)
        removeCallbacks(mChangeCurrentByOneFromLongPressCommand);

        mChangeCurrentByOneFromLongPressCommand=[this,increment](){
        changeValueByOne(increment);
        postDelayed(mChangeCurrentByOneFromLongPressCommand, mLongPressUpdateInterval);
    };
    postDelayed(mChangeCurrentByOneFromLongPressCommand, delayMillis);
}

void NumberPicker::postChangeCurrentByOneFromLongPress(bool increment){
    postChangeCurrentByOneFromLongPress(increment, ViewConfiguration::getLongPressTimeout());
}
void NumberPicker::removeChangeCurrentByOneFromLongPress(){
    if (mChangeCurrentByOneFromLongPressCommand != nullptr) {
        removeCallbacks(mChangeCurrentByOneFromLongPressCommand);
        mChangeCurrentByOneFromLongPressCommand =nullptr;
    }
}

void NumberPicker::removeBeginSoftInputCommand(){
    if(mBeginSoftInputOnLongPressCommand!=nullptr){
        removeCallbacks(mBeginSoftInputOnLongPressCommand);
        mBeginSoftInputOnLongPressCommand=nullptr;
    }
}

void NumberPicker::postBeginSoftInputOnLongPressCommand(){
    if(mBeginSoftInputOnLongPressCommand!=nullptr)
        removeCallbacks(mBeginSoftInputOnLongPressCommand);
    mBeginSoftInputOnLongPressCommand=[this](){
        performLongClick();
    };
    postDelayed(mBeginSoftInputOnLongPressCommand,ViewConfiguration::getLongPressTimeout());
}


void NumberPicker::removeAllCallbacks(){
    if (mChangeCurrentByOneFromLongPressCommand != nullptr) {
        removeCallbacks(mChangeCurrentByOneFromLongPressCommand);
        mChangeCurrentByOneFromLongPressCommand =nullptr;
    }
    removeBeginSoftInputCommand();
    pshCancel();
}

int NumberPicker::getSelectedPos(const std::string& value){
    if (mDisplayedValues.size()==0){
        return std::strtol(value.c_str(),nullptr,10);
    } else {
        for (int i = 0; i < mDisplayedValues.size(); i++) {
            // Don't force the user to type in jan when ja will do
            //value = value.toLowerCase();
            //if (mDisplayedValues[i].toLowerCase().startsWith(value)) return mMinValue + i;
            if( TextUtils::startWith(mDisplayedValues[i],value))return mMinValue+i;
        }
        /* The user might have typed in a number into the month field i.e.
        * 10 instead of OCT so support that too.*/
        return std::strtol(value.c_str(),nullptr,10);//Integer.parseInt(value);
    }
    return mMinValue;
}

void  NumberPicker::ensureScrollWheelAdjusted() {
    // adjust to the closest value
    int delta = mInitialScrollOffset - mCurrentScrollOffset;
    if (delta == 0) return;

    if(std::abs(delta)>mSelectorElementSize/2){
        delta += (delta > 0) ? -mSelectorElementSize : mSelectorElementSize;
    }
    if(isHorizontalMode()){
        mPreviousScrollerX = 0;
        mAdjustScroller->startScroll(0, 0,delta,0, SELECTOR_ADJUSTMENT_DURATION_MILLIS);
    }else{
        mPreviousScrollerY = 0;
        mAdjustScroller->startScroll(0, 0, 0, delta, SELECTOR_ADJUSTMENT_DURATION_MILLIS);
        LOGV("delta=%d scrollOffset=%d/%d ",delta,mInitialScrollOffset,mCurrentScrollOffset);
    }
    invalidate();
}

void NumberPicker::pshCancel(){
    mPSHMode = 0; 
    mPSHManagedButton = 0;
    if(mPressedStateHelpers != nullptr){
        removeCallbacks(mPressedStateHelpers);
        invalidate(0, mBottomSelectionDividerBottom, mRight-mLeft, mBottom-mTop);
    }
    mPressedStateHelpers = [this](){
        pshRun();
    };
    mDecrementVirtualButtonPressed = false;
    if(mDecrementVirtualButtonPressed)
        invalidate(0,0,mRight-mLeft,mTopSelectionDividerTop);
}

void NumberPicker::pshButtonPressDelayed(int button){
    pshCancel();
    mPSHMode = MODE_PRESS;
    mPSHManagedButton = button;
    
    postDelayed(mPressedStateHelpers,ViewConfiguration::getTapTimeout());
}

void NumberPicker::pshButtonTapped(int button){
    pshCancel();
    mPSHMode = MODE_TAPPED;
    mPSHManagedButton = button;
    post(mPressedStateHelpers);
}

void NumberPicker::pshRun(){
    switch (mPSHMode) {
    case MODE_PRESS:
        switch (mPSHManagedButton) {
        case R::id::increment:
             mIncrementVirtualButtonPressed = true;
             invalidate(0, mBottomSelectionDividerBottom, mRight, mBottom);
             break;
        case R::id::decrement:
             mDecrementVirtualButtonPressed = true;
             invalidate(0, 0, mRight, mTopSelectionDividerTop);
             break;
        }
        break;
    case MODE_TAPPED:
        switch (mPSHManagedButton) {
        case R::id::increment:
            if (!mIncrementVirtualButtonPressed) {
                postDelayed(mPressedStateHelpers,ViewConfiguration::getPressedStateDuration());
            }
            mIncrementVirtualButtonPressed ^= true;
            invalidate(0, mBottomSelectionDividerBottom, mRight, mBottom);
             break;
        case R::id::decrement:
            if (!mDecrementVirtualButtonPressed) {
                postDelayed(mPressedStateHelpers,ViewConfiguration::getPressedStateDuration());
            }
            mDecrementVirtualButtonPressed ^= true;
            invalidate(0, 0, mRight, mTopSelectionDividerTop);
        }//endof switch (mManagedButton)
        break;/*endof case MODE_TAPPED*/
    }
}

}//namespace
