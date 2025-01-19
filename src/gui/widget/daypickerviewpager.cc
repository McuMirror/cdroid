#include <widget/daypickerviewpager.h>
#include <widget/daypickerpageradapter.h>
namespace cdroid{

DayPickerViewPager::DayPickerViewPager(Context* context, const AttributeSet& attrs)
    :ViewPager(context,attrs){
}

void DayPickerViewPager::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    populate();

    // Everything below is mostly copied from FrameLayout.
    int count = getChildCount();

    bool measureMatchParentChildren =
           MeasureSpec::getMode(widthMeasureSpec) != MeasureSpec::EXACTLY ||
           MeasureSpec::getMode(heightMeasureSpec) != MeasureSpec::EXACTLY;

    int maxHeight = 0;
    int maxWidth = 0;
    int childState = 0;

    for (int i = 0; i < count; i++) {
        View* child = getChildAt(i);
        if (child->getVisibility() != GONE) {
            measureChild(child, widthMeasureSpec, heightMeasureSpec);
            LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
            maxWidth = std::max(maxWidth, child->getMeasuredWidth());
            maxHeight = std::max(maxHeight, child->getMeasuredHeight());
            childState = combineMeasuredStates(childState, child->getMeasuredState());
            if (measureMatchParentChildren) {
                if (lp->width == LayoutParams::MATCH_PARENT ||
                    lp->height == LayoutParams::MATCH_PARENT) {
                    mMatchParentChildren.push_back(child);
                }
            }
        }
    }

    // Account for padding too
    maxWidth += getPaddingLeft() + getPaddingRight();
    maxHeight += getPaddingTop() + getPaddingBottom();

    // Check against our minimum height and width
    maxHeight = std::max(maxHeight, getSuggestedMinimumHeight());
    maxWidth  = std::max(maxWidth, getSuggestedMinimumWidth());

    // Check against our foreground's minimum height and width
    Drawable* drawable = getForeground();
    if (drawable != nullptr) {
        maxHeight = std::max(maxHeight, drawable->getMinimumHeight());
        maxWidth = std::max(maxWidth, drawable->getMinimumWidth());
    }

    setMeasuredDimension(resolveSizeAndState(maxWidth, widthMeasureSpec, childState),
            resolveSizeAndState(maxHeight, heightMeasureSpec,
                    childState << MEASURED_HEIGHT_STATE_SHIFT));

    count = mMatchParentChildren.size();
    if (count > 1) {
        for (int i = 0; i < count; i++) {
            View* child = mMatchParentChildren.at(i);

            const LayoutParams* lp = (LayoutParams*) child->getLayoutParams();
            int childWidthMeasureSpec;
            int childHeightMeasureSpec;

            if (lp->width == LayoutParams::MATCH_PARENT) {
                childWidthMeasureSpec = MeasureSpec::makeMeasureSpec(
                    getMeasuredWidth() - getPaddingLeft() - getPaddingRight(),
                    MeasureSpec::EXACTLY);
            } else {
                childWidthMeasureSpec = getChildMeasureSpec(widthMeasureSpec,
                        getPaddingLeft() + getPaddingRight(),
                        lp->width);
            }

            if (lp->height == LayoutParams::MATCH_PARENT) {
                childHeightMeasureSpec = MeasureSpec::makeMeasureSpec(
                    getMeasuredHeight() - getPaddingTop() - getPaddingBottom(),
                    MeasureSpec::EXACTLY);
            } else {
                childHeightMeasureSpec = getChildMeasureSpec(heightMeasureSpec,
                        getPaddingTop() + getPaddingBottom(),
                        lp->height);
            }

            child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
        }
    }

    mMatchParentChildren.clear();
}

View*DayPickerViewPager::findViewByPredicateTraversal(std::function<bool(View*)>predicate,View* childToSkip){
    if (predicate(this)){//.test(this)) {
        return (View*)this;
    }

    // Always try the selected view first.
    DayPickerPagerAdapter* adapter = (DayPickerPagerAdapter*)getAdapter();
    SimpleMonthView* current = (SimpleMonthView*)adapter->getView(getCurrent());
    if (current != childToSkip && current != nullptr) {
         View* v = current->findViewByPredicate(predicate);
        if (v != nullptr) {
            return v;
        }
    }

    int len = getChildCount();
    for (int i = 0; i < len; i++) {
        View* child = getChildAt(i);

        if (child != childToSkip && child != current) {
            View* v = child->findViewByPredicate(predicate);

            if (v != nullptr) {
                return v;
            }
        }
    }

    return nullptr;
}


}//endof namespace
