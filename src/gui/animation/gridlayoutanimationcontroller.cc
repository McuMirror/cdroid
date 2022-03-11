#include <stdlib.h>
#include <view/viewgroup.h>
#include <animation/gridlayoutanimationcontroller.h>

namespace cdroid{

GridLayoutAnimationController::GridLayoutAnimationController(Context* context,const AttributeSet& attrs)
  :LayoutAnimationController(context,attrs){
    mColumnDelay = attrs.getFloat("columnDelay");
    mRowDelay    = attrs.getFloat("rowDelay");
    mDirection   = attrs.getInt("direction");
    mDirectionPriority = attrs.getInt("directionPriority");
}

GridLayoutAnimationController::GridLayoutAnimationController(Animation* animation)
 :GridLayoutAnimationController(animation,.5f,.5f){
}

GridLayoutAnimationController::GridLayoutAnimationController(Animation* animation, float columnDelay, float rowDelay)
:LayoutAnimationController(animation){
    mColumnDelay = columnDelay;
    mRowDelay = rowDelay;
}

float GridLayoutAnimationController::getColumnDelay(){
    return mColumnDelay;
}

void GridLayoutAnimationController::setColumnDelay(float columnDelay){
    mColumnDelay = columnDelay;
}

float GridLayoutAnimationController::getRowDelay(){
    return mRowDelay;
}

void GridLayoutAnimationController::setRowDelay(float rowDelay){
    mRowDelay = rowDelay;
}

int GridLayoutAnimationController::getDirection(){
    return mDirection;
}

void GridLayoutAnimationController::setDirection(int direction){
    mDirection = direction;
}

int GridLayoutAnimationController::getDirectionPriority(){
    return mDirectionPriority;
}

void GridLayoutAnimationController::setDirectionPriority(int directionPriority){
    mDirectionPriority = directionPriority;
}

bool GridLayoutAnimationController::willOverlap(){
    return mColumnDelay < 1.0f || mRowDelay < 1.0f;
}

long GridLayoutAnimationController::getDelayForView(View* view){
    ViewGroup::LayoutParams* lp = view->getLayoutParams();
    AnimationParameters* params = (AnimationParameters*) lp->layoutAnimationParameters;

    if (params == nullptr)  return 0;
     

    const int column = getTransformedColumnIndex(params);
    const int row = getTransformedRowIndex(params);

    const int rowsCount = params->rowsCount;
    const int columnsCount = params->columnsCount;

    const long duration = mAnimation->getDuration();
    const float columnDelay = mColumnDelay * duration;
    const float rowDelay = mRowDelay * duration;

    float totalDelay;
    long viewDelay;

    if (mInterpolator == nullptr) {
        mInterpolator = new LinearInterpolator();
    }

    switch (mDirectionPriority) {
    case PRIORITY_COLUMN:
        viewDelay = (long) (row * rowDelay + column * rowsCount * rowDelay);
        totalDelay = rowsCount * rowDelay + columnsCount * rowsCount * rowDelay;
        break;
    case PRIORITY_ROW:
        viewDelay = (long) (column * columnDelay + row * columnsCount * columnDelay);
        totalDelay = columnsCount * columnDelay + rowsCount * columnsCount * columnDelay;
        break;
    case PRIORITY_NONE:
    default:
        viewDelay = (long) (column * columnDelay + row * rowDelay);
        totalDelay = columnsCount * columnDelay + rowsCount * rowDelay;
        break;
    }

    float normalizedDelay = viewDelay / totalDelay;
    normalizedDelay = mInterpolator->getInterpolation(normalizedDelay);

    return (long) (normalizedDelay * totalDelay);
}

int GridLayoutAnimationController::getTransformedColumnIndex(const AnimationParameters* params){
    int index;
    switch (getOrder()) {
    case ORDER_REVERSE:
        index = params->columnsCount - 1 - params->column;
        break;
    case ORDER_RANDOM:
        //if (mRandomizer == null) mRandomizer = new Random();
        index = (int) (params->columnsCount * drand48());//mRandomizer.nextFloat());
        break;
    case ORDER_NORMAL:
    default:
        index = params->column;
        break;
    }

    int direction = mDirection & DIRECTION_HORIZONTAL_MASK;
    if (direction == DIRECTION_RIGHT_TO_LEFT) {
        index = params->columnsCount - 1 - index;
    }
    return index;
}

int GridLayoutAnimationController::getTransformedRowIndex(const AnimationParameters* params){
    int index;
    switch (getOrder()) {
    case ORDER_REVERSE:
        index = params->rowsCount - 1 - params->row;
        break;
    case ORDER_RANDOM:
        //if (mRandomizer == null) mRandomizer = new Random();
        index = (int) (params->rowsCount * drand48());//mRandomizer.nextFloat());
        break;
    case ORDER_NORMAL:
    default:
        index = params->row;
        break;
    }

    int direction = mDirection & DIRECTION_VERTICAL_MASK;
    if (direction == DIRECTION_BOTTOM_TO_TOP) {
        index = params->rowsCount - 1 - index;
    }
    return index;
}

}
