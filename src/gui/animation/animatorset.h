#ifndef __AIMATORSET_H__
#define __AIMATORSET_H__
#include <vector>
#include <animation/animator.h>
#include <animation/animationhandler.h>
namespace cdroid{

class AnimatorSet:public Animator,public AnimationHandler::AnimationFrameCallback{
private:
    class Node;
    class SeekState;
    class AnimationEvent;
private:
    std::vector<Node*> mPlayingSet;
    std::map<Animator*, Node*> mNodeMap;
    std::vector<AnimationEvent*> mEvents;
    std::vector<Node*> mNodes;
    AnimatorListener mDummyListener;
    bool mDependencyDirty = false;

    /* Indicates whether an AnimatorSet has been start()'d, whether or
     * not there is a nonzero startDelay. */
    bool mStarted = false;
    bool mShouldIgnoreEndWithoutStart;
    bool mShouldResetValuesAtStart;
    bool mEndCanBeCalled;
    bool mReversing = false;
    bool mSelfPulse = true;
    bool mChildrenInitialized = false;

    // The amount of time in ms to delay starting the animation after start() is called
    long mStartDelay = 0;

    // Animator used for a nonzero startDelay
    class ValueAnimator* mDelayAnim;// = ValueAnimator.ofFloat(0f, 1f).setDuration(0);

    Node* mRootNode;// = new Node(mDelayAnim);

    TimeInterpolator* mInterpolator = nullptr;

    // The total duration of finishing all the Animators in the set.
    long mTotalDuration = 0;
    long mDuration = -1;

    int mLastEventId = -1;
    int64_t mLastFrameTime = -1;
    int64_t mFirstFrame = -1;
    int64_t mPauseTime = -1;
    // Indicates whether the animation is reversing.
    SeekState* mSeekState;
private:
    void forceToEnd();
    void initAnimation();
    void start(bool inReverse, bool selfPulse);
    static bool isEmptySet(AnimatorSet* set);
    static bool AnimationEventCompare(AnimationEvent* e1,AnimationEvent* e2);
    void updateAnimatorsDuration();
    void skipToStartValue(bool inReverse);
    void initChildren();
    void handleAnimationEvents(int startId, int latestId, int64_t playTime);
    void pulseFrame(Node* node, int64_t animPlayTime);
    int64_t getPlayTimeForNode(int64_t overallPlayTime, Node* node) ;
    int64_t getPlayTimeForNode(int64_t overallPlayTime, Node* node, bool inReverse);
    void startAnimation();
    void addDummyListener();
    void removeDummyListener();
     int findLatestEventIdForTime(int64_t currentPlayTime);
    void endAnimation();
    void removeAnimationCallback();
    void addAnimationCallback(long delay);
    void createDependencyGraph();
    void sortAnimationEvents();
    void updatePlayTime(Node* parent,std::vector<Node*>& visited);
    void findSiblings(Node* node,std::vector<Node*>& siblings) ;
    Node* getNodeForAnimation(Animator* anim);
protected:
    void skipToEndValue(bool inReverse)override;
    void animateBasedOnPlayTime(int64_t currentPlayTime, int64_t lastPlayTime, bool inReverse);
    bool isInitialized()override;
    void startWithoutPulsing(bool)override;
public:
    class Builder;
    AnimatorSet();
    ~AnimatorSet();
    void playTogether(const std::vector<Animator*>&);
    void playSequentially(const std::vector<Animator*>&);
    std::vector<Animator*> getChildAnimations()const;
    void setTarget(void* target);
    int getChangingConfigurations()override;
    void setInterpolator(TimeInterpolator*)override;
    TimeInterpolator*getInterpolator()override;
    Builder* play(Animator* anim);
    void cancel()override;
    void end()override;
    bool isRunning()override;
    bool isStarted()override;
    long getStartDelay()override;
    void setStartDelay(long startDelay)override;
    long getDuration();
    Animator& setDuration(long)override;
    void setupStartValues()override;
    void setupEndValues()override;
    void pause()override;
    void resume()override;
    void start()override;
    void setCurrentPlayTime(int64_t playTime);
    int64_t getCurrentPlayTime();
    bool doAnimationFrame(int64_t frameTime)override;
    void commitAnimationFrame(int64_t frameTime)override;
    bool pulseAnimationFrame(int64_t frameTime)override;
    Animator* clone()const override;
    bool canReverse()override;
    void reverse()override;
    bool shouldPlayTogether();
    long getTotalDuration()override;
};

class AnimatorSet::Builder{
private:
    AnimatorSet*mAnimSet;
    Node*mCurrentNode;
public:
    Builder(AnimatorSet*set,Animator* anim);
    Builder& with(Animator* anim);
    Builder& before(Animator* anim);
    Builder& after(Animator* anim);
    Builder& after(long delay);
};

class AnimatorSet::Node{
public:
    Animator* mAnimation;
    std::vector<Node*>mChildNodes;
    bool mEnded = false;
    bool mParentsAdded = false;

    /**Nodes with animations that are defined to play simultaneously with the animation
     * associated with this current node. */
    std::vector<Node*> mSiblings;

    /**Parent nodes are the nodes with animations preceding current node's animation. Parent
     * nodes here are derived from user defined animation sequence. */
    std::vector<Node*> mParents;
    /**Latest parent is the parent node associated with a animation that finishes after all
     * the other parents' animations. */
    Node* mLatestParent = nullptr;

    int64_t mStartTime = 0;
    int64_t mEndTime = 0;
    long mTotalDuration = 0;
public:
    Node(Animator* animation);
    Node* clone();
    void addChild(Node* node);
    void addSibling(Node* node);
    void addParent(Node* node);
    void addParents(const std::vector<Node*>& parents);
};

class AnimatorSet::AnimationEvent{
public:
    static constexpr int ANIMATION_START = 0;
    static constexpr int ANIMATION_DELAY_ENDED = 1;
    static constexpr int ANIMATION_END = 2;
    Node* mNode;
    int mEvent;
public:
    AnimationEvent(Node* node, int event);
    int64_t getTime()const;
};

class AnimatorSet::SeekState{
private:
    AnimatorSet*mAnimSet;
    int64_t mPlayTime;
    bool mSeekingInReverse;
public:
    SeekState(AnimatorSet*set);
    void reset();
    void setPlayTime(int64_t playTime, bool inReverse);
    void updateSeekDirection(bool inReverse);
    int64_t getPlayTime()const;
    int64_t getPlayTimeNormalized()const;
    bool isActive()const;
};
}/*endof namespace*/
#endif//__AIMATORSET_H__
