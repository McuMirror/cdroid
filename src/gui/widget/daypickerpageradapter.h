#ifndef __DAYPICKER_PAGERADAPTER_H__
#define __DAYPICKER_PAGERADAPTER_H__
#include <widget/adapter.h>
#include <widget/simplemonthview.h>

namespace cdroid{

class DayPickerPagerAdapter:public PagerAdapter{
public:
    DECLARE_UIEVENT(void,OnDaySelectedListener,DayPickerPagerAdapter& view,Calendar& day);
private:
    static constexpr int MONTHS_IN_YEAR=12;
    class ViewHolder {
    public:
        int position;
        View* container;
        SimpleMonthView* calendar;
        ViewHolder(int position, View* container, SimpleMonthView* calendar);
    };
    Calendar mMinDate;
    Calendar mMaxDate;
    SparseArray<ViewHolder*,nullptr>mItems;
    LayoutInflater* mInflater;
    std::string mLayoutResId;
    int mCalendarViewId;
    
    Calendar* mSelectedDay;

    std::string mMonthTextAppearance;
    std::string mDayOfWeekTextAppearance;
    std::string mDayTextAppearance;

    ColorStateList* mCalendarTextColor;
    ColorStateList* mDaySelectorColor;
    ColorStateList* mDayHighlightColor;

    SimpleMonthView::OnDayClickListener mOnDayClickListener;
    OnDaySelectedListener mOnDaySelectedListener;

    int mCount;
    int mFirstDayOfWeek;
private:
    int getMonthForPosition(int position);
    int getYearForPosition(int position);
    int getPositionForDay(Calendar* day);
public:
    DayPickerPagerAdapter(Context* context,const std::string&layoutResId,int calendarViewId);
    ~DayPickerPagerAdapter();
    void setRange(Calendar& min,Calendar& max);
    void setFirstDayOfWeek(int weekStart);
    int getFirstDayOfWeek()const;
    bool getBoundsForDate(Calendar& day, Rect& outBounds);
    void setSelectedDay(Calendar* day);
    void setOnDaySelectedListener(OnDaySelectedListener listener);
    void setCalendarTextColor(const ColorStateList* calendarTextColor);
    void setDaySelectorColor(const ColorStateList* selectorColor);

    void setMonthTextAppearance(const std::string& resId);
    void setDayOfWeekTextAppearance(const std::string& resId);
    std::string getDayOfWeekTextAppearance();

    void setDayTextAppearance(const std::string&resId);
    std::string getDayTextAppearance();

    int getCount()override;
    bool isViewFromObject(View* view,void*object)override;
    void* instantiateItem(ViewGroup* container, int position)override;
    void destroyItem(ViewGroup* container, int position,void* object)override;
    int getItemPosition(void* object)override;
    std::string getPageTitle(int position)override;

    SimpleMonthView* getView(void* object);
};
}//namespace
#endif
