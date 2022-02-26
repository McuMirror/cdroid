#ifndef __DRAWER_LAYOUT_H__
#define __DRAWER_LAYOUT_H__
#include <widget/viewgroup.h>
#include <widget/viewdraghelper.h>
namespace cdroid{

class DrawerLayout:public ViewGroup{
public:
    static constexpr int STATE_IDLE = ViewDragHelper::STATE_IDLE;
    static constexpr int STATE_DRAGGING = ViewDragHelper::STATE_DRAGGING;
    static constexpr int STATE_SETTLING = ViewDragHelper::STATE_SETTLING;
    static constexpr int LOCK_MODE_UNLOCKED = 0;

    /**The drawer is locked closed. The user may not open it, though
     * the app may open it programmatically. */
    static constexpr int LOCK_MODE_LOCKED_CLOSED = 1;

    /**The drawer is locked open. The user may not close it, though the app
     * may close it programmatically.*/
    static constexpr int LOCK_MODE_LOCKED_OPEN = 2;

    /**The drawer's lock state is reset to default.*/
    static constexpr int LOCK_MODE_UNDEFINED = 3;

    class DrawerListener{
    public:
        
    };
    class LayoutParams:public ViewGroup::MarginLayoutParams{
    public:
        static constexpr int FLAG_IS_OPENED = 0x1;
        static constexpr int FLAG_IS_OPENING = 0x2;
        static constexpr int FLAG_IS_CLOSING = 0x4;

        int gravity = Gravity::NO_GRAVITY;
        float onScreen;
        bool isPeeking;
        int openState;
        LayoutParams(Context* c,const AttributeSet& attrs);
        LayoutParams(int width, int height);
        LayoutParams(int width, int height, int gravity);
        LayoutParams(const LayoutParams& source);
        LayoutParams(const ViewGroup::LayoutParams& source);
        LayoutParams(const ViewGroup::MarginLayoutParams& source);
    };
private:
    static constexpr int MIN_DRAWER_MARGIN = 64; // dp
    static constexpr int DRAWER_ELEVATION = 10; //dp

    static constexpr int DEFAULT_SCRIM_COLOR = 0x99000000;

    /**Length of time to delay before peeking the drawer. */
    static constexpr int PEEK_DELAY = 160; // ms

    /**Minimum velocity that will be detected as a fling */
    static constexpr int MIN_FLING_VELOCITY = 400; // dips per second

    /* Experimental feature.*/
    static constexpr bool ALLOW_EDGE_LOCK = false;
    static constexpr bool CHILDREN_DISALLOW_INTERCEPT = true;
    static constexpr float TOUCH_SLOP_SENSITIVITY = 1.f;
    static constexpr bool CAN_HIDE_DESCENDANTS=true;
    static constexpr bool SET_DRAWER_SHADOW_FROM_ELEVATION=false;

    class ViewDragCallback:public ViewDragHelper::Callback{
    private:
        int mAbsGravity;
        DrawerLayout*mDL;
        ViewDragHelper*mDragger;
        Runnable mPeekRunnable;
        void closeOtherDrawer();
    public:
        ViewDragCallback(DrawerLayout*dl,int gravity);
        void setDragger(ViewDragHelper* dragger);
        void removeCallbacks();
        bool tryCaptureView(View& child, int pointerId)override;
        void onViewDragStateChanged(int state)override;
        void onViewPositionChanged(View& changedView, int left, int top, int dx, int dy)override;
        void onViewCaptured(View& capturedChild, int activePointerId)override;
        void onViewReleased(View& releasedChild, float xvel, float yvel)override;
        void onEdgeTouched(int edgeFlags, int pointerId);
        void peekDrawer();
        bool onEdgeLock(int edgeFlags)override;
        void onEdgeDragStarted(int edgeFlags, int pointerId)override;
        int getViewHorizontalDragRange(View& child)override;
        int clampViewPositionHorizontal(View& child, int left, int dx)override;
        int clampViewPositionVertical(View& child, int top, int dy)override;
    };
private:
    float mDrawerElevation;

    int mMinDrawerMargin;

    int mScrimColor = DEFAULT_SCRIM_COLOR;
    float mScrimOpacity;

    ViewDragHelper*   mLeftDragger;
    ViewDragHelper*   mRightDragger;
    ViewDragHelper*   mTopDragger;
    ViewDragHelper*   mBottomDragger;
    ViewDragCallback* mLeftCallback;
    ViewDragCallback* mRightCallback;
    ViewDragCallback* mTopCallback;
    ViewDragCallback* mBottomCallback;
    int mDrawerState;
    bool mInLayout;
    bool mFirstLayout = true;

    int mLockModeLeft  = LOCK_MODE_UNDEFINED;
    int mLockModeRight = LOCK_MODE_UNDEFINED;
    int mLockModeStart = LOCK_MODE_UNDEFINED;
    int mLockModeEnd   = LOCK_MODE_UNDEFINED;

    bool mDisallowInterceptRequested;
    bool mChildrenCanceledTouch;

    //DrawerListener mListener;
    std::vector<DrawerListener> mListeners;

    float mInitialMotionX;
    float mInitialMotionY;

    Drawable* mStatusBarBackground;
    Drawable* mShadowLeftResolved;
    Drawable* mShadowRightResolved;

    std::string mTitleLeft;
    std::string mTitleRight;

    //Object mLastInsets;
    bool mDrawStatusBarBackground;

    /** Shadow drawables for different gravity */
    Drawable* mShadowStart = nullptr;
    Drawable* mShadowEnd = nullptr;
    Drawable* mShadowLeft = nullptr;
    Drawable* mShadowRight = nullptr;

    std::vector<View*> mNonDrawerViews;
    Rect mChildHitRect;
    Matrix mChildInvertedMatrix;
private:
    void initView();
    static const std::string gravityToString(int gravity);
    bool isInBoundsOfChild(float x, float y, View* child);
    bool dispatchTransformedGenericPointerEvent(MotionEvent& event, View* child);
    MotionEvent getTransformedMotionEvent(MotionEvent& event, View* child);
    void updateChildrenImportantForAccessibility(View* drawerView, bool isDrawerOpen);
    void resolveShadowDrawables();
    Drawable* resolveLeftShadow();
    Drawable* resolveRightShadow();
    bool mirror(Drawable* drawable, int layoutDirection);
    static bool hasOpaqueBackground(View* v);
    bool hasPeekingDrawer();
    bool hasVisibleDrawer();
protected:
    void updateDrawerState(int forGravity,int activeState, View* activeDrawer);
    void dispatchOnDrawerClosed(View* drawerView);
    void dispatchOnDrawerOpened(View* drawerView);
    void dispatchOnDrawerSlide(View* drawerView, float slideOffset);
    void setDrawerViewOffset(View* drawerView, float slideOffset);
    float getDrawerViewOffset(View* drawerView);
    int getDrawerViewAbsoluteGravity(View* drawerView);
    bool checkDrawerViewAbsoluteGravity(View* drawerView, int checkFor);
    View* findOpenDrawer();
    void moveDrawerToOffset(View* drawerView, float slideOffset);
    View* findDrawerWithGravity(int gravity);
    bool isContentView(View* child)const;
    bool isDrawerView (View* child)const;
    bool isDrawerViewTopBottom(View*child)const;
    View* findVisibleDrawer();
    void cancelChildViewTouch();

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec)override;
    void onLayout(bool changed, int l, int t, int r, int b)override;
    bool drawChild(Canvas& canvas, View* child, long drawingTime)override;
    ViewGroup::LayoutParams* generateDefaultLayoutParams()const override;
    ViewGroup::LayoutParams* generateLayoutParams(const ViewGroup::LayoutParams* p)const override;
    bool checkLayoutParams(const ViewGroup::LayoutParams* p)const override;
    ViewGroup::LayoutParams* generateLayoutParams(const AttributeSet& attrs)const override;
    ~DrawerLayout()override;
public:
    DrawerLayout(int w,int h);
    DrawerLayout(Context*ctx,const AttributeSet&atts); 
    void setDrawerElevation(float elevation);
    float getDrawerElevation()const;
    void setDrawerShadow(Drawable* shadowDrawable,int gravity);
    void setDrawerShadow(const std::string& resId,int gravity);
    void setScrimColor(int color);
    void addDrawerListener(DrawerListener listener);
    void removeDrawerListener(DrawerListener listener);
    void setDrawerLockMode(int lockMode);
    void setDrawerLockMode(int lockMode,int edgeGravity);
    void setDrawerLockMode(int lockMode,View* drawerView);
    int getDrawerLockMode(int edgeGravity);
    int getDrawerLockMode(View* drawerView);
    void setDrawerTitle(int edgeGravity,const std::string&title);
    const std::string getDrawerTitle(int edgeGravity);
    void requestLayout()override;
    void computeScroll()override;
    void setStatusBarBackground(Drawable* bg);
    Drawable* getStatusBarBackgroundDrawable() ;
    void setStatusBarBackground(const std::string& resId);
    void setStatusBarBackgroundColor(int color);
    void onRtlPropertiesChanged(int layoutDirection);
    void onDraw(Canvas& c)override;
    bool onInterceptTouchEvent(MotionEvent& ev)override;
    bool dispatchGenericMotionEvent(MotionEvent& event);//override;
    bool onTouchEvent(MotionEvent& ev)override;
    void requestDisallowInterceptTouchEvent(bool disallowIntercept)override;
    void closeDrawers();
    void closeDrawers(bool peekingOnly);
    void openDrawer(View* drawerView);
    void openDrawer(View* drawerView, bool animate);
    void openDrawer(int gravity);
    void openDrawer(int gravity, bool animate);
    void closeDrawer(View* drawerView);
    void closeDrawer(View* drawerView, bool animate);
    void closeDrawer(int gravity);
    void closeDrawer(int gravity, bool animate);
    bool isDrawerOpen(View* drawer);
    bool isDrawerOpen(int drawerGravity);
    bool isDrawerVisible(View* drawer);
    bool isDrawerVisible(int drawerGravity);
    void addFocusables(std::vector<View*>&views, int direction, int focusableMode)override;
    bool onKeyDown(int keyCode, KeyEvent& event)override;
    bool onKeyUp(int keyCode, KeyEvent& event)override;
    View& addView(View* child, int index, ViewGroup::LayoutParams* params)override;
};

}//endof namespace
#endif

