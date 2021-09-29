#ifndef __LAYOUT_TRANSITION_H__
#define __LAYOUT_TRANSITION_H__
#include <map>
#include <functional>
#include <widget/view.h>
#include <animation/animator.h>

namespace cdroid{
class ViewGroup;
class LayoutTransition{
public:
    struct TransitionListener{
       CallbackBase<void,LayoutTransition&, ViewGroup*/*container*/,View* /*view*/, int/*transitionType*/> startTransition;
       CallbackBase<void,LayoutTransition&, ViewGroup*/*container*/,View* /*view*/, int/*transitionType*/> endTransition;
    };
private:
    static constexpr int FLAG_APPEARING             = 0x01;
    static constexpr int FLAG_DISAPPEARING          = 0x02;
    static constexpr int FLAG_CHANGE_APPEARING      = 0x04;
    static constexpr int FLAG_CHANGE_DISAPPEARING   = 0x08;
    static constexpr int FLAG_CHANGING              = 0x10;
    static constexpr long DEFAULT_DURATION          = 300;

    long mChangingAppearingDuration   = DEFAULT_DURATION;
    long mChangingDisappearingDuration= DEFAULT_DURATION;
    long mChangingDuration            = DEFAULT_DURATION;
    long mAppearingDuration           = DEFAULT_DURATION;
    long mDisappearingDuration        = DEFAULT_DURATION;

    long mAppearingDelay              = DEFAULT_DURATION;
    long mDisappearingDelay           = 0;
    long mChangingAppearingDelay      = 0;
    long mChangingDisappearingDelay   = DEFAULT_DURATION;
    long mChangingDelay               = 0;
    long mChangingAppearingStagger    = 0;
    long mChangingDisappearingStagger = 0;
    long mChangingStagger             = 0;
    long staggerDelay;
    int  mTransitionTypes = FLAG_CHANGE_APPEARING | FLAG_CHANGE_DISAPPEARING | FLAG_APPEARING | FLAG_DISAPPEARING;
    bool mAnimateParentHierarchy      = true;
    std::vector<TransitionListener>mListeners;

    static TimeInterpolator* ACCEL_DECEL_INTERPOLATOR;
    static TimeInterpolator* DECEL_INTERPOLATOR      ;
    static TimeInterpolator* sAppearingInterpolator  ;
    static TimeInterpolator* sDisappearingInterpolator;
    static TimeInterpolator* sChangingAppearingInterpolator;
    static TimeInterpolator* sChangingDisappearingInterpolator;
    static TimeInterpolator* sChangingInterpolator;

    TimeInterpolator* mAppearingInterpolator            = sAppearingInterpolator;
    TimeInterpolator* mDisappearingInterpolator         = sDisappearingInterpolator;
    TimeInterpolator* mChangingAppearingInterpolator    = sChangingAppearingInterpolator;
    TimeInterpolator* mChangingDisappearingInterpolator = sChangingDisappearingInterpolator;
    TimeInterpolator* mChangingInterpolator             = sChangingInterpolator;

    static Animator* defaultChange;
    static Animator* defaultChangeIn;
    static Animator* defaultChangeOut;
    static Animator* defaultFadeIn;
    static Animator* defaultFadeOut;
    Animator* mDisappearingAnim = nullptr;
    Animator* mAppearingAnim    = nullptr;
    Animator* mChangingAppearingAnim = nullptr;
    Animator* mChangingDisappearingAnim = nullptr;
    Animator* mChangingAnim = nullptr;

    std::map<View*, Animator*> pendingAnimations;
    std::map<View*, Animator*> currentChangingAnimations;
    std::map<View*, Animator*> currentAppearingAnimations;
    std::map<View*, Animator*> currentDisappearingAnimations;
    std::map<View*, View::OnLayoutChangeListener> layoutChangeListenerMap;
public :
    /**
     * A flag indicating the animation that runs on those items that are changing
     * due to a new item appearing in the container. */
    static constexpr int CHANGE_APPEARING = 0;

    /**
     * A flag indicating the animation that runs on those items that are changing
     * due to an item disappearing from the container.*/
    static constexpr int CHANGE_DISAPPEARING = 1;

    /**
     * A flag indicating the animation that runs on those items that are appearing
     * in the container.*/
    static constexpr int APPEARING = 2;

    /**
     * A flag indicating the animation that runs on those items that are disappearing
     * from the container. */
    static constexpr int DISAPPEARING = 3;

    /**
     * A flag indicating the animation that runs on those items that are changing
     * due to a layout change not caused by items being added to or removed
     * from the container. This transition type is not enabled by default; it can be
     * enabled via {@link #enableTransitionType(int)}. */
    static constexpr int CHANGING = 4;
private:
    bool hasListeners()const;
    void runChangeTransition(ViewGroup* parent, View* newView, int changeReason);
    void setupChangeAnimation(ViewGroup* parent, int changeReason, Animator* baseAnimator,long duration, View* child);
    void runAppearingTransition(ViewGroup* parent,View* child);
    void runDisappearingTransition(ViewGroup* parent,View* child);
    void addChild(ViewGroup* parent, View* child, bool changesLayout);
    void removeChild(ViewGroup* parent, View* child, bool changesLayout);
public:
    void setDuration(long duration);
    void setDuration(int transitionType, long duration);
    long getDuration(int transitionType);
    void enableTransitionType(int transitionType);
    void disableTransitionType(int transitionType);
    bool isTransitionTypeEnabled(int transitionType);
    void setStartDelay(int transitionType, long delay);
    long getStartDelay(int transitionType);
    void setStagger(int transitionType, long duration);
    long getStagger(int transitionType);

    void setInterpolator(int transitionType, TimeInterpolator* interpolator);
    TimeInterpolator* getInterpolator(int transitionType);

    void setAnimator(int transitionType, Animator* anim);
    Animator* getAnimator(int transitionType);
    void startChangingAnimations();
    void endChangingAnimations();
    bool isChangingLayout();
    bool isRunning();
    void cancel();
    void cancel(int transitionType);
    void addChild(ViewGroup* parent, View* child);
    void removeChild(ViewGroup* parent, View* child);
    void layoutChange(ViewGroup* parent);
    void hideChild(ViewGroup* parent, View* child);
    void hideChild(ViewGroup* parent, View* child, int newVisibility);
    void showChild(ViewGroup* parent, View* child, int oldVisibility);
    void addTransitionListener(TransitionListener& listener);
    void removeTransitionListener(TransitionListener& listener);
};

}


#endif
