#ifndef __TABLE_ROW_H__
#define __TABLE_ROW_H__
#include <widget/linearlayout.h>
#include <core/sparsearray.h>

namespace cdroid{

class TableRow:public LinearLayout{
public:
    class LayoutParams;
private:
    friend class TableLayout;
    int mNumColumns;
    std::vector<int>mColumnWidths;
    std::vector<int>mConstrainedColumnWidths;
	SparseIntArray mColumnToChildIndex;
    OnHierarchyChangeListener mChildrenTracker;
    OnHierarchyChangeListener mExtHCL;
    void initTableRow();
    void mapIndexAndColumns();
    void onChildViewAdded(View& parent, View* child);
    void onChildViewRemoved(View& parent, View* child);
protected:
    void onLayout(bool changed, int l, int t, int w, int h)override;
    int measureNullChild(int childIndex)override;
    void measureChildBeforeLayout(View* child, int childIndex,int widthMeasureSpec, 
            int totalWidth,int heightMeasureSpec, int totalHeight)override;
    int getChildrenSkipCount(View* child, int index)override;
    int getLocationOffset(View* child)override;
    int getNextLocationOffset(View* child)override;
    
    std::vector<int>getColumnsWidths(int widthMeasureSpec, int heightMeasureSpec);
    void setColumnsWidthConstraints(const std::vector<int>& columnWidths);

    ViewGroup::LayoutParams* generateDefaultLayoutParams() const override;
    bool checkLayoutParams(const ViewGroup::LayoutParams* p)const override;
    ViewGroup::LayoutParams* generateLayoutParams(const ViewGroup::LayoutParams* p)const override;
public:
    TableRow(int w,int h);
    TableRow(Context* context,const AttributeSet&attrs);
    void setOnHierarchyChangeListener(const OnHierarchyChangeListener& listener);
    void setColumnCollapsed(int columnIndex, bool collapsed);
    void onMeasure(int widthMeasureSpec, int heightMeasureSpec)override;
    View* getVirtualChildAt(int i) override;
    int getVirtualChildCount()override;
    std::string getAccessibilityClassName()const override;
    ViewGroup::LayoutParams* generateLayoutParams(const AttributeSet& attrs)const override;
};

class TableRow::LayoutParams:public LinearLayout::LayoutParams{
public:
    int span;
    int column;
    int mOffset [2];
    enum{
	    LOCATION=0,
	    LOCATION_NEXT=1
    };
    LayoutParams();
    LayoutParams(int column);
    LayoutParams(Context* c,const AttributeSet&attrs);
    LayoutParams(int w, int h);
    LayoutParams(int w, int h, float initWeight);
    LayoutParams(const ViewGroup::LayoutParams& p);
    LayoutParams(const MarginLayoutParams& source);
};
}
#endif
