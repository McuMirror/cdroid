#ifndef __GRID_LAYOUT_MANAGER_H__
#define __GRID_LAYOUT_MANAGER_H__
#include <widgetEx/recyclerview/linearlayoutmanager.h>
namespace cdroid{

class GridLayoutManager:public LinearLayoutManager {
private:
    static constexpr bool _Debug = false;
    static constexpr int INVALID_POSITION = -1;
public:
    static constexpr int DEFAULT_SPAN_COUNT = -1;
    class SpanSizeLookup;
    class DefaultSpanSizeLookup;
    class LayoutParams;
private:
    int  mPositionTargetedByScrollInDirection = INVALID_POSITION;
protected:
    bool mPendingSpanCountChange = false;
    int mSpanCount = DEFAULT_SPAN_COUNT;
    int mRowWithAccessibilityFocus = INVALID_POSITION;
    int mColumnWithAccessibilityFocus = INVALID_POSITION;
    std::vector<int>mCachedBorders;/*int []*/
    std::vector<View*>mSet;//[] mSet;
    SparseIntArray mPreLayoutSpanSizeCache;
    SparseIntArray mPreLayoutSpanIndexCache;;
    SpanSizeLookup* mSpanSizeLookup;// = new DefaultSpanSizeLookup();
    // re-used variable to acquire decor insets from RecyclerView
    Rect mDecorInsets;
private:
    int findScrollTargetPositionOnTheRight(int startingRow, int startingColumn, int startingAdapterPosition);
    int findScrollTargetPositionOnTheLeft(int startingRow, int startingColumn, int startingAdapterPosition);
    int findScrollTargetPositionAbove(int startingRow, int startingColumn, int startingAdapterPosition);
    int findScrollTargetPositionBelow(int startingRow, int startingColumn, int startingAdapterPosition);
    int getRowIndex(int position);
    int getColumnIndex(int position);
    std::set<int> getRowIndices(int position);
    std::set<int> getColumnIndices(int position);
    std::set<int> getRowOrColumnIndices(int rowOrColumnIndex, int position);
    View* findChildWithAccessibilityFocus();
    bool hasAccessibilityFocusChanged(int adapterPosition);
    void clearPreLayoutSpanMappingCache();
    void cachePreLayoutSpanMapping();
    void calculateItemBorders(int totalSpace);
    void updateMeasurements();
    void ensureViewSet();
    void ensureAnchorIsInCorrectSpan(RecyclerView::Recycler& recycler,
            RecyclerView::State& state, AnchorInfo& anchorInfo, int itemDirection);
    int getSpanGroupIndex(RecyclerView::Recycler& recycler, RecyclerView::State& state,int viewPosition);
    int getSpanIndex(RecyclerView::Recycler& recycler, RecyclerView::State& state, int pos);
    int getSpanSize(RecyclerView::Recycler& recycler, RecyclerView::State& state, int pos);
    void measureChild(View* view, int otherDirParentSpecMode, bool alreadyMeasured);
    void guessMeasurement(float maxSizeInOther, int currentOtherDirSize);
    void measureChildWithDecorationsAndMargin(View* child, int widthSpec, int heightSpec,
            bool alreadyMeasured);
    void assignSpans(RecyclerView::Recycler& recycler, RecyclerView::State& state, int count,
            int consumedSpanCount, bool layingOutInPrimaryDirection);

protected:
    int findPositionOfLastItemOnARowAboveForHorizontalGrid(int startingRow);
    int findPositionOfFirstItemOnARowBelowForHorizontalGrid(int startingRow);
    static int* calculateItemBorders(std::vector<int>&cachedBorders, int spanCount, int totalSpace);
    int getSpaceForSpanRange(int startSpan, int spanSize); 

    void onAnchorReady(RecyclerView::Recycler& recycler, RecyclerView::State& state,
                       AnchorInfo& anchorInfo, int itemDirection)override;
    View* findReferenceChild(RecyclerView::Recycler& recycler, RecyclerView::State& state,
                            int start, int end, int itemCount)override;
    void collectPrefetchPositionsForLayoutState(RecyclerView::State& state, LayoutState& layoutState,
            LayoutPrefetchRegistry& layoutPrefetchRegistry)override;
    void layoutChunk(RecyclerView::Recycler& recycler, RecyclerView::State& state,
            LayoutState& layoutState, LayoutChunkResult& result)override;
public:
    GridLayoutManager(Context* context, const AttributeSet& attrs);
    GridLayoutManager(Context* context, int spanCount);
    GridLayoutManager(Context* context, int spanCount,int orientation, bool reverseLayout);
    ~GridLayoutManager()override;
    void setStackFromEnd(bool stackFromEnd)override;
    int getRowCountForAccessibility(RecyclerView::Recycler& recycler, RecyclerView::State& state)override;
    int getColumnCountForAccessibility(RecyclerView::Recycler& recycler, RecyclerView::State& state)override;
    void onInitializeAccessibilityNodeInfoForItem(RecyclerView::Recycler& recycler,
            RecyclerView::State& state, View* host, AccessibilityNodeInfo& info)override;
    bool performAccessibilityAction(int action,Bundle args)override;
    void onLayoutChildren(RecyclerView::Recycler& recycler, RecyclerView::State& state)override;
    void onLayoutCompleted(RecyclerView::State& state)override;

    void onItemsAdded(RecyclerView& recyclerView, int positionStart, int itemCount)override;

    void onItemsChanged(RecyclerView& recyclerView)override;
    void onItemsRemoved(RecyclerView& recyclerView, int positionStart, int itemCount)override;
    void onItemsUpdated(RecyclerView& recyclerView, int positionStart, int itemCount,Object* payload)override;
    void onItemsMoved(RecyclerView& recyclerView, int from, int to, int itemCount)override;
    RecyclerView::LayoutParams* generateDefaultLayoutParams()const override;
    RecyclerView::LayoutParams* generateLayoutParams(Context* c,const AttributeSet& attrs)const override;
    RecyclerView::LayoutParams* generateLayoutParams(const ViewGroup::LayoutParams& lp)const override;

    bool checkLayoutParams(const RecyclerView::LayoutParams* lp)const override;
    void setSpanSizeLookup(SpanSizeLookup* spanSizeLookup);
    SpanSizeLookup* getSpanSizeLookup();
    void setMeasuredDimension(Rect& childrenBounds, int wSpec, int hSpec)override;

    int scrollHorizontallyBy(int dx, RecyclerView::Recycler& recycler,RecyclerView::State& state)override;
    int scrollVerticallyBy(int dy, RecyclerView::Recycler& recycler,RecyclerView::State& state)override;
    int getSpanCount();
    void setSpanCount(int spanCount); 

    View* onFocusSearchFailed(View* focused, int focusDirection,
            RecyclerView::Recycler& recycler, RecyclerView::State& state)override;
    bool supportsPredictiveItemAnimations()override;

};

class GridLayoutManager::SpanSizeLookup {
protected:
    SparseIntArray mSpanIndexCache;// = new SparseIntArray();
private:
    bool mCacheSpanIndices = false;
protected:
    friend class GridLayoutManager;
    int getCachedSpanIndex(int position, int spanCount);
    int findReferenceIndexFromCache(int position);
public:
    virtual int getSpanSize(int position)=0;
    void setSpanIndexCacheEnabled(bool cacheSpanIndices);
    void invalidateSpanIndexCache();
    bool isSpanIndexCacheEnabled();
    virtual int getSpanIndex(int position, int spanCount);
    int getSpanGroupIndex(int adapterPosition, int spanCount);
};

class GridLayoutManager::DefaultSpanSizeLookup:public GridLayoutManager::SpanSizeLookup {
public:
    int getSpanSize(int position)override;
    int getSpanIndex(int position, int spanCount)override;
};

class GridLayoutManager::LayoutParams:public RecyclerView::LayoutParams {
public:
    static constexpr int INVALID_SPAN_ID = -1;
protected:
    int mSpanIndex = INVALID_SPAN_ID;
    int mSpanSize = 0;
    friend GridLayoutManager;
public:
    LayoutParams(Context* c,const AttributeSet& attrs);
    LayoutParams(int width, int height);
    LayoutParams(const ViewGroup::MarginLayoutParams& source);
    LayoutParams(const ViewGroup::LayoutParams& source);
    LayoutParams(const RecyclerView::LayoutParams& source);
    int getSpanIndex()const;
    int getSpanSize()const;
};
}/*endof namespace*/
#endif/*__GRID_LAYOUT_MANAGER_H__*/
