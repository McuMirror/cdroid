#include <widget/overscroller.h>
#include <systemclock.h>
#include <scroller.h>
#include <cdlog.h>


namespace cdroid{
template <typename T> int signum(T val) {
    return (T(0) < val) - (val < T(0));
}

float OverScroller::SplineOverScroller::SPLINE_POSITION[NB_SAMPLES + 1];
float OverScroller::SplineOverScroller::SPLINE_TIME[NB_SAMPLES+1];

void OverScroller::SplineOverScroller::sInit(){
    float x_min = 0.0f;
    float y_min = 0.0f;
    for (int i = 0; i < NB_SAMPLES; i++) {
        float alpha = (float) i / NB_SAMPLES;

        float x_max = 1.0f;
        float x, tx, coef;
        while (true) {
            x = x_min + (x_max - x_min) / 2.0f;
            coef = 3.0f * x * (1.0f - x);
            tx = coef * ((1.0f - x) * P1 + x * P2) + x * x * x;
            if (std::abs(tx - alpha) < 1E-5) break;
                if (tx > alpha) x_max = x;
                else x_min = x;
            }
            SPLINE_POSITION[i] = coef * ((1.0f - x) * START_TENSION + x) + x * x * x;

            float y_max = 1.0f;
            float y, dy;
            while (true) {
                y = y_min + (y_max - y_min) / 2.0f;
                coef = 3.0f * y * (1.0f - y);
                dy = coef * ((1.0f - y) * START_TENSION + y) + y * y * y;
                if (std::abs(dy - alpha) < 1E-5) break;
                if (dy > alpha) y_max = y;
                else y_min = y;
            }
            SPLINE_TIME[i] = coef * ((1.0f - y) * P1 + y * P2) + y * y * y;
        }
    SPLINE_POSITION[NB_SAMPLES] = SPLINE_TIME[NB_SAMPLES] = 1.0f;
}
OverScroller::SplineOverScroller::SplineOverScroller(Context* context) {
    if(SPLINE_POSITION[NB_SAMPLES] !=1.0f || SPLINE_TIME[NB_SAMPLES]!=1.0f)
        sInit();
    mFinished = true;
    const float ppi = 2.65f*160.f;//context.getResources().getDisplayMetrics().density * 160.0f;
    mPhysicalCoeff = 9.80665f//SensorManager.GRAVITY_EARTH // g (m/s^2)
            * 39.37f // inch/meter
            * ppi
            * 0.84f; // look and feel tuning
}

void OverScroller::SplineOverScroller::startScroll(int start, int distance, int duration) {
    mFinished = false;

    mCurrentPosition = mStart = start;
    mFinal = start + distance;

    mStartTime = SystemClock::uptimeMillis();
    mDuration = duration;

    // Unused
    mDeceleration = 0.0f;
    mVelocity = 0;
}

float OverScroller::SplineOverScroller::getDeceleration(int velocity) {
    return velocity > 0 ? -GRAVITY : GRAVITY;
}

double OverScroller::SplineOverScroller::getSplineDeceleration(int velocity) {
     return std::log(INFLEXION * std::abs(velocity) / (mFlingFriction * mPhysicalCoeff));
}

void OverScroller::SplineOverScroller::setFriction(float friction) {
    mFlingFriction = friction;
}
void OverScroller::SplineOverScroller::updateScroll(float q) {
    mCurrentPosition = mStart + std::round(q * (mFinal - mStart));
}

void OverScroller::SplineOverScroller::finish() {
    mCurrentPosition = mFinal;
    // Not reset since WebView relies on this value for fast fling.
    // TODO: restore when WebView uses the fast fling implemented in this class.
    // mCurrVelocity = 0.0f;
    mFinished = true;
}

void OverScroller::SplineOverScroller::setFinalPosition(int position) {
    mFinal = position;
    mFinished = false;
}

double OverScroller::SplineOverScroller::getSplineFlingDistance(int velocity) {
    double l = getSplineDeceleration(velocity);
    double decelMinusOne = DECELERATION_RATE - 1.0;
    return mFlingFriction * mPhysicalCoeff * std::exp(DECELERATION_RATE / decelMinusOne * l);
}
int OverScroller::SplineOverScroller::getSplineFlingDuration(int velocity) {
    double l = getSplineDeceleration(velocity);
    double decelMinusOne = DECELERATION_RATE - 1.0;
    return (int) (1000.0 * std::exp(l / decelMinusOne));
}

void OverScroller::SplineOverScroller::adjustDuration(int start, int oldFinal, int newFinal) {
    int oldDistance = oldFinal - start;
    int newDistance = newFinal - start;
    float x = std::abs((float) newDistance / oldDistance);
    int index = (int) (NB_SAMPLES * x);
    if (index < NB_SAMPLES) {
       float x_inf = (float) index / NB_SAMPLES;
       float x_sup = (float) (index + 1) / NB_SAMPLES;
       float t_inf = SPLINE_TIME[index];
       float t_sup = SPLINE_TIME[index + 1];
       float timeCoef = t_inf + (x - x_inf) / (x_sup - x_inf) * (t_sup - t_inf);
       mDuration *= timeCoef;
    }
}

void OverScroller::SplineOverScroller::extendDuration(int extend) {
    long time = SystemClock::uptimeMillis();
    int elapsedTime = (int) (time - mStartTime);
    mDuration = elapsedTime + extend;
    mFinished = false;
}

bool OverScroller::SplineOverScroller::springback(int start, int min, int max) {
    mFinished = true;

    mCurrentPosition = mStart = mFinal = start;
    mVelocity = 0;

    mStartTime = SystemClock::uptimeMillis();
    mDuration = 0;

    if (start < min) {
        startSpringback(start, min, 0);
    } else if (start > max) {
        startSpringback(start, max, 0);
    }

    return !mFinished;
}

void OverScroller::SplineOverScroller::startSpringback(int start, int end, int velocity){
    // mStartTime has been set
    mFinished = false;
    mState = CUBIC;
    mCurrentPosition = mStart = start;
    mFinal = end;
    int delta = start - end;
    mDeceleration = getDeceleration(delta);
    // TODO take velocity into account
    mVelocity = -delta; // only sign is used
    mOver = std::abs(delta);
    mDuration = (int) (1000.0 * std::sqrt(-2.0 * delta / mDeceleration));
}

void OverScroller::SplineOverScroller::fling(int start, int velocity, int min, int max, int over){
    mOver = over;
    mFinished = false;
    mCurrVelocity = mVelocity = velocity;
    mDuration = mSplineDuration = 0;
    mStartTime = SystemClock::uptimeMillis();
    mCurrentPosition = mStart = start;

    if (start > max || start < min) {
        startAfterEdge(start, min, max, velocity);
        return;
    }

    mState = SPLINE;
    double totalDistance = 0.0;

    if (velocity != 0) {
        mDuration = mSplineDuration = getSplineFlingDuration(velocity);
        totalDistance = getSplineFlingDistance(velocity);
    }

    mSplineDistance = (int) (totalDistance * signum(velocity));
    mFinal = start + mSplineDistance;

    // Clamp to a valid final position
    if (mFinal < min) {
        adjustDuration(mStart, mFinal, min);
        mFinal = min;
    }

    if (mFinal > max) {
        adjustDuration(mStart, mFinal, max);
        mFinal = max;
    }
}
void OverScroller::SplineOverScroller::fitOnBounceCurve(int start, int end, int velocity) {
    // Simulate a bounce that started from edge
    float durationToApex = - velocity / mDeceleration;
    // The float cast below is necessary to avoid integer overflow.
    float velocitySquared = (float) velocity * velocity;
    float distanceToApex = velocitySquared / 2.0f / std::abs(mDeceleration);
    float distanceToEdge = std::abs(end - start);
    float totalDuration = (float) std::sqrt(2.0 * (distanceToApex + distanceToEdge) / std::abs(mDeceleration));
    mStartTime -= (int) (1000.0f * (totalDuration - durationToApex));
    mCurrentPosition = mStart = end;
    mVelocity = (int) (- mDeceleration * totalDuration);
}

void OverScroller::SplineOverScroller::startBounceAfterEdge(int start, int end, int velocity) {
    mDeceleration = getDeceleration(velocity == 0 ? start - end : velocity);
    fitOnBounceCurve(start, end, velocity);
    onEdgeReached();
}

void OverScroller::SplineOverScroller::startAfterEdge(int start, int min, int max, int velocity) {
    if (start > min && start < max) {
        LOGE("startAfterEdge called from a valid position");
        mFinished = true;
        return;
    }
    bool positive = start > max;
    int edge = positive ? max : min;
    int overDistance = start - edge;
    bool keepIncreasing = overDistance * velocity >= 0;
    if (keepIncreasing) {
        // Will result in a bounce or a to_boundary depending on velocity.
        startBounceAfterEdge(start, edge, velocity);
    } else {
        double totalDistance = getSplineFlingDistance(velocity);
        if (totalDistance > std::abs(overDistance)) {
            fling(start, velocity, positive ? min : start, positive ? start : max, mOver);
        } else {
            startSpringback(start, edge, velocity);
        }
    }
}

void OverScroller::SplineOverScroller::notifyEdgeReached(int start, int end, int over) {
    // mState is used to detect successive notifications 
    if (mState == SPLINE) {
        mOver = over;
        mStartTime = SystemClock::uptimeMillis();
        // We were in fling/scroll mode before: current velocity is such that distance to
        // edge is increasing. This ensures that startAfterEdge will not start a new fling.
        startAfterEdge(start, end, end, (int) mCurrVelocity);
    }
}

void OverScroller::SplineOverScroller::onEdgeReached(){
    float velocitySquared = (float) mVelocity * mVelocity;
    float distance = velocitySquared / (2.0f * std::abs(mDeceleration));
    float sign = signum(mVelocity);

    if (distance > mOver) {
        // Default deceleration is not sufficient to slow us down before boundary
        mDeceleration = - sign * velocitySquared / (2.0f * mOver);
        distance = mOver;
    }

    mOver = (int) distance;
    mState = BALLISTIC;
    mFinal = mStart + (int) (mVelocity > 0 ? distance : -distance);
    mDuration = - (int) (1000.0f * mVelocity / mDeceleration);
}


bool OverScroller::SplineOverScroller::continueWhenFinished() {
    switch (mState) {
    case SPLINE:
        // Duration from start to null velocity
        if (mDuration < mSplineDuration) {
             // If the animation was clamped, we reached the edge
             mCurrentPosition = mStart = mFinal;
             // TODO Better compute speed when edge was reached
             mVelocity = (int) mCurrVelocity;
             mDeceleration = getDeceleration(mVelocity);
             mStartTime += mDuration;
             onEdgeReached();
        } else {
             // Normal stop, no need to continue
             return false;
        }
        break;
    case BALLISTIC:
        mStartTime += mDuration;
        startSpringback(mFinal, mStart, 0);
        break;
    case CUBIC:
        return false;
    }

    update();
    return true;
}

bool OverScroller::SplineOverScroller::update(){
    long time = SystemClock::uptimeMillis();
    long currentTime = time - mStartTime;

    if (currentTime == 0) {
        // Skip work but report that we're still going if we have a nonzero duration.
        return mDuration > 0;
    }
    if (currentTime > mDuration) {
        return false;
    }

    double distance = 0.0;
    switch (mState) {
    case SPLINE: {
         float t = (float) currentTime / mSplineDuration;
         int index = (int) (NB_SAMPLES * t);
         float distanceCoef = 1.f;
         float velocityCoef = 0.f;
         if (index < NB_SAMPLES) {
            float t_inf = (float) index / NB_SAMPLES;
            float t_sup = (float) (index + 1) / NB_SAMPLES;
            float d_inf = SPLINE_POSITION[index];
            float d_sup = SPLINE_POSITION[index + 1];
            velocityCoef = (d_sup - d_inf) / (t_sup - t_inf);
            distanceCoef = d_inf + (t - t_inf) * velocityCoef;
         }

         distance = distanceCoef * mSplineDistance;
         mCurrVelocity = velocityCoef * mSplineDistance / mSplineDuration * 1000.0f;
         break;
         }

    case BALLISTIC: {
         float t = currentTime / 1000.0f;
         mCurrVelocity = mVelocity + mDeceleration * t;
         distance = mVelocity * t + mDeceleration * t * t / 2.0f;
         break;
         }

    case CUBIC: {
         float t = (float) (currentTime) / mDuration;
         float t2 = t * t;
         float sign = signum(mVelocity);
         distance = sign * mOver * (3.0f * t2 - 2.0f * t * t2); 
         mCurrVelocity = sign * mOver * 6.0f * (- t + t2); 
         break;
         }
    }

    mCurrentPosition = mStart + (int) std::round(distance);
    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OverScroller::OverScroller(Context* context):OverScroller(context,nullptr,true){
}

OverScroller::OverScroller(Context* context, Interpolator* interpolator, bool flywheel) {
    if (interpolator == nullptr) {
        mInterpolator = new Scroller::ViscousFluidInterpolator();
    } else {
        mInterpolator = interpolator;
    }
    mFlywheel = flywheel;
    mScrollerX = new SplineOverScroller(context);
    mScrollerY = new SplineOverScroller(context);
}

OverScroller::~OverScroller(){
    delete mScrollerX;
    delete mScrollerY;
    delete mInterpolator;
}

void OverScroller::setInterpolator(Interpolator* interpolator) {
    if (interpolator == nullptr) {
        mInterpolator = new Scroller::ViscousFluidInterpolator();
    } else {
        mInterpolator = interpolator;
    }
}

void OverScroller::setFriction(float friction) {
    mScrollerX->setFriction(friction);
    mScrollerY->setFriction(friction);
}

bool OverScroller::isFinished()const{
    return mScrollerX->mFinished && mScrollerY->mFinished;
}

void OverScroller::forceFinished(bool finished) {
    mScrollerX->mFinished = mScrollerY->mFinished = finished;
}

int OverScroller::getCurrX()const{
    return mScrollerX->mCurrentPosition;
}

int OverScroller::getCurrY()const{
    return mScrollerY->mCurrentPosition;
}

float OverScroller::getCurrVelocity()const{
    return (float) std::hypot(mScrollerX->mCurrVelocity, mScrollerY->mCurrVelocity);
}

int OverScroller::getStartX()const{
    return mScrollerX->mStart;
}

int OverScroller::getStartY()const{
    return mScrollerY->mStart;
}

int OverScroller::getFinalX()const {
    return mScrollerX->mFinal;
}
int OverScroller::getFinalY()const{
    return mScrollerY->mFinal;
}

bool OverScroller::computeScrollOffset() {
    if (isFinished()) {
        return false;
    }

    switch (mMode) {
    case SCROLL_MODE:{
           long time = SystemClock::uptimeMillis();
           // Any scroller can be used for time, since they were started
           // together in scroll mode. We use X here.
           long elapsedTime = time - mScrollerX->mStartTime;

           int duration = mScrollerX->mDuration;
           if (elapsedTime < duration) {
               float q = mInterpolator->getInterpolation(elapsedTime / (float) duration);
               mScrollerX->updateScroll(q);
               mScrollerY->updateScroll(q);
           } else {
               abortAnimation();
           }
        }break;
    case FLING_MODE:
        if (!mScrollerX->mFinished) {
            if (!mScrollerX->update()) {
               if (!mScrollerX->continueWhenFinished()) {
                   mScrollerX->finish();
               }
            }
        }
        if (!mScrollerY->mFinished) {
            if (!mScrollerY->update()) {
                if (!mScrollerY->continueWhenFinished()) {
                    mScrollerY->finish();
                }
            }
        }
        break;
    }
    return true;
}

void OverScroller::startScroll(int startX, int startY, int dx, int dy) {
    startScroll(startX, startY, dx, dy, DEFAULT_DURATION);
}

void OverScroller::startScroll(int startX, int startY, int dx, int dy, int duration) {
    mMode = SCROLL_MODE;
    mScrollerX->startScroll(startX, dx, duration);
    mScrollerY->startScroll(startY, dy, duration);
}

bool OverScroller::springBack(int startX, int startY, int minX, int maxX, int minY, int maxY) {
    mMode = FLING_MODE;

    // Make sure both methods are called.
    bool spingbackX = mScrollerX->springback(startX, minX, maxX);
    bool spingbackY = mScrollerY->springback(startY, minY, maxY);
    return spingbackX || spingbackY;
}

void OverScroller::fling(int startX, int startY, int velocityX, int velocityY,
            int minX, int maxX, int minY, int maxY) {
    fling(startX, startY, velocityX, velocityY, minX, maxX, minY, maxY, 0, 0);
}

void OverScroller::fling(int startX, int startY, int velocityX, int velocityY,
            int minX, int maxX, int minY, int maxY, int overX, int overY) {
        // Continue a scroll or fling in progress
    if (mFlywheel && !isFinished()) {
       float oldVelocityX = mScrollerX->mCurrVelocity;
       float oldVelocityY = mScrollerY->mCurrVelocity;
       if (signum(velocityX) == signum(oldVelocityX) &&
           signum(velocityY) == signum(oldVelocityY)) {
           velocityX += oldVelocityX;
           velocityY += oldVelocityY;
       }
    }

    mMode = FLING_MODE;
    mScrollerX->fling(startX, velocityX, minX, maxX, overX);
    mScrollerY->fling(startY, velocityY, minY, maxY, overY);
}

void OverScroller::notifyHorizontalEdgeReached(int startX, int finalX, int overX) {
    mScrollerX->notifyEdgeReached(startX, finalX, overX);
}

void OverScroller::notifyVerticalEdgeReached(int startY, int finalY, int overY) {
    mScrollerY->notifyEdgeReached(startY, finalY, overY);
}

bool OverScroller::isOverScrolled()const {
    return ((!mScrollerX->mFinished && mScrollerX->mState != SplineOverScroller::SPLINE) ||
            (!mScrollerY->mFinished && mScrollerY->mState != SplineOverScroller::SPLINE));
}

void OverScroller::abortAnimation() {
    mScrollerX->finish();
    mScrollerY->finish();
}

int OverScroller::timePassed()const{
   long time = SystemClock::uptimeMillis();
   long startTime = std::min(mScrollerX->mStartTime, mScrollerY->mStartTime);
   return (int) (time - startTime);
}

bool OverScroller::isScrollingInDirection(float xvel, float yvel)const{
    int dx = mScrollerX->mFinal - mScrollerX->mStart;
    int dy = mScrollerY->mFinal - mScrollerY->mStart;
    return !isFinished() && signum(xvel) == signum(dx) && signum(yvel) == signum(dy);
}
}//namespace
