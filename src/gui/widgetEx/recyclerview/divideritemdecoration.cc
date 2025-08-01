/*********************************************************************************
 * Copyright (C) [2019] [houzh@msn.com]
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *********************************************************************************/
#include <widgetEx/recyclerview/divideritemdecoration.h>
namespace cdroid{

DividerItemDecoration::DividerItemDecoration(Context* context, int orientation) {
    //final TypedArray a = context.obtainStyledAttributes(ATTRS);
    //mDivider = a.getDrawable(0);
    AttributeSet attr = context->obtainStyledAttributes("cdroid:attr/listDivider");
    mDivider = attr.getDrawable("listDivider");
    LOGW_IF(mDivider == nullptr,"@android:attr/listDivider was not set in the theme used for this "
               "DividerItemDecoration. Please set that attribute all call setDrawable()");
    setOrientation(orientation);
}

DividerItemDecoration::~DividerItemDecoration(){
    delete mDivider;
}

void DividerItemDecoration::setOrientation(int orientation) {
    if (orientation != HORIZONTAL && orientation != VERTICAL) {
        FATAL("Invalid orientation. It should be either HORIZONTAL or VERTICAL");
    }
    mOrientation = orientation;
}

void DividerItemDecoration::setDrawable(Drawable* drawable) {
    FATAL_IF(drawable==nullptr,"Drawable cannot be null.");
    delete mDivider;
    mDivider = drawable;
}

Drawable*DividerItemDecoration::getDrawable()const{
    return mDivider;
}

void DividerItemDecoration::onDraw(Canvas& c, RecyclerView& parent, RecyclerView::State& state) {
    if (parent.getLayoutManager() == nullptr || mDivider == nullptr) {
        return;
    }
    if (mOrientation == VERTICAL) {
        drawVertical(c, parent);
    } else {
        drawHorizontal(c, parent);
    }
}

void DividerItemDecoration::drawVertical(Canvas& canvas, RecyclerView& parent) {
    canvas.save();
    int left, right;
    //noinspection AndroidLintNewApi - NewApi lint fails to handle overrides.
    if (parent.getClipToPadding()) {
        left = parent.getPaddingLeft();
        right = parent.getWidth() - parent.getPaddingRight();
        canvas.rectangle(left, parent.getPaddingTop(), right-left,
            parent.getHeight() -parent.getPaddingTop()- parent.getPaddingBottom());
        canvas.clip();
    } else {
        left = 0;
        right = parent.getWidth();
    }

    const int childCount = parent.getChildCount();
    for (int i = 0; i < childCount; i++) {
        View* child = parent.getChildAt(i);
        parent.getDecoratedBoundsWithMargins(child, mBounds);
        const int bottom = mBounds.bottom() + std::round(child->getTranslationY());
        const int top = bottom - mDivider->getIntrinsicHeight();
        mDivider->setBounds(left, top, right-left, bottom-top);
        mDivider->draw(canvas);
    }
    canvas.restore();
}

void DividerItemDecoration::drawHorizontal(Canvas& canvas, RecyclerView& parent) {
    canvas.save();
    int top,bottom;
    //noinspection AndroidLintNewApi - NewApi lint fails to handle overrides.
    if (parent.getClipToPadding()) {
        top = parent.getPaddingTop();
        bottom = parent.getHeight() - parent.getPaddingBottom();
        canvas.rectangle(parent.getPaddingLeft(), top,
            parent.getWidth() - parent.getPaddingLeft() - parent.getPaddingRight(), bottom-top);
        canvas.clip();
    } else {
        top = 0;
        bottom = parent.getHeight();
    }

    const int childCount = parent.getChildCount();
    for (int i = 0; i < childCount; i++) {
        View* child = parent.getChildAt(i);
        parent.getLayoutManager()->getDecoratedBoundsWithMargins(child, mBounds);
        const int right = mBounds.right() + std::round(child->getTranslationX());
        const int left = right - mDivider->getIntrinsicWidth();
        mDivider->setBounds(left, top, right-left, bottom-top);
        mDivider->draw(canvas);
    }
    canvas.restore();
}

void DividerItemDecoration::getItemOffsets(Rect& outRect, View& view, RecyclerView& parent,
        RecyclerView::State& state) {
    if (mDivider == nullptr) {
        outRect.set(0, 0, 0, 0);
        return;
    }
    if (mOrientation == VERTICAL) {
        outRect.set(0, 0, 0, mDivider->getIntrinsicHeight());
    } else {
        outRect.set(0, 0, mDivider->getIntrinsicWidth(), 0);
    }
}

}
