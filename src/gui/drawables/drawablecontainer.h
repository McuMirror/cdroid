#ifndef __DRAWABLE_CONTAINER_H__
#define __DRAWABLE_CONTAINER_H__
#include <drawables/drawable.h>
namespace cdroid{

class DrawableContainer:public Drawable,Drawable::Callback{
protected:
    class DrawableContainerState:public std::enable_shared_from_this<DrawableContainerState>,public ConstantState{
    private:
        Drawable*prepareDrawable(Drawable* child);
    public:
        DrawableContainer*mOwner;
        int mDensity;
        int mChangingConfigurations;
        int mChildrenChangingConfigurations;
        bool mVariablePadding;
        bool mCheckedPadding;
        RECT mConstantPadding;
        bool mConstantSize;
        bool mCheckedConstantSize;
        int mConstantWidth,mConstantHeight;
        int mConstantMinimumWidth,mConstantMinimumHeight;

        bool mCheckedOpacity;
        int mOpacity;

        bool mCheckedConstantState;
        bool mCanConstantState;
        bool mCheckedStateful;
        bool mStateful;

        bool mDither;
        bool mMutated;
        int mLayoutDirection;
        int mEnterFadeDuration,mExitFadeDuration;
        bool mAutoMirrored;
        ColorStateList*mTintList;
        std::vector<Drawable* >mDrawables;
        std::map<int,std::shared_ptr<ConstantState> >mDrawableFutures;
        DrawableContainerState(const DrawableContainerState*orig,DrawableContainer*own);
        ~DrawableContainerState();
        Drawable*newDrawable()override{return nullptr;}//must be overrided by inherited
        int addChild(Drawable* dr);
        int getChildCount()const;
        Drawable*getChild(int index);
        void invalidateCache();
        void mutate();
        void clearMutated();
        void createAllFutures();
        bool isStateful();
        bool canConstantState();
        int getChangingConfigurations()const override;
        bool setLayoutDirection(int layoutDirection, int currentIndex);
        bool getConstantPadding(RECT&rect);
        void setConstantSize(bool constant){mConstantSize=constant;}
        bool isConstantSize()const {return mConstantSize;}
        void setVariablePadding(bool variable){mVariablePadding=variable;}
        int getConstantWidth();
        int getConstantHeight();
        int getConstantMinimumWidth();
        int getConstantMinimumHeight();
        void computeConstantSize();
        int getEnterFadeDuration()const {return mEnterFadeDuration; }
        void setEnterFadeDuration(int duration) {mEnterFadeDuration = duration; }
        int getExitFadeDuration()const {return mExitFadeDuration; }
        void setExitFadeDuration(int duration) {mExitFadeDuration = duration;}
    };
    class BlockInvalidateCallback*mBlockInvalidateCallback;
    void initializeDrawableForDisplay(Drawable*d);
    std::shared_ptr<DrawableContainerState>mDrawableContainerState;
protected:
    int mCurIndex;
    int mLastIndex;
    bool mMutated;
    Drawable* mCurrDrawable;
    Drawable* mLastDrawable;
    bool needsMirroring();
    virtual std::shared_ptr<DrawableContainerState> cloneConstantState();
    virtual void setConstantState(std::shared_ptr<DrawableContainerState>state);
    void onBoundsChange(const RECT&bounds)override;
    bool onStateChange(const std::vector<int>&state)override;
    bool onLevelChange(int level)override;
public:
    DrawableContainer();
    ~DrawableContainer();
    int getCurrentIndex()const;
    virtual void setCurrentIndex(int index);
    virtual bool selectDrawable(int index);
    int addChild(Drawable*);
    int getChildCount()const;
    Drawable*getChild(int index);
    bool getPadding(RECT&padding)override;
    int getChangingConfigurations()const override;

    int getIntrinsicWidth() const override;
    int getIntrinsicHeight()const override;
    int getMinimumWidth() const override;
    int getMinimumHeight()const override;

    void invalidateDrawable(Drawable& who)override;
    void scheduleDrawable(Drawable&who,Runnable what, long when)override;
    void unscheduleDrawable(Drawable& who,Runnable what)override;
    std::shared_ptr<ConstantState>getConstantState()override;
    Drawable*mutate()override;
    void clearMutated()override;
    void draw(Canvas&canvas)override;
};
}
#endif
