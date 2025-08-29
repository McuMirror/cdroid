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
#include <widget/R.h>
#include <widget/menupopupwindow.h>
#include <menu/menuadapter.h>
#include <menu/menupopuphelper.h>
#include <menu/submenubuilder.h>
#include <menu/standardmenupopup.h>
namespace cdroid{
//private static final int ITEM_LAYOUT = com.android.internal.R.layout.popup_menu_item_layout;
static constexpr const char* ITEM_LAYOUT_MATERIAL ="cdroid:layout/popup_menu_item_layout_material";

void StandardMenuPopup::onGlobalLayout() {
    // Only move the popup if it's showing and non-modal. We don't want
    // to be moving around the only interactive window, since there's a
    // good chance the user is interacting with it.
    if (isShowing() && !mPopup->isModal()) {
        View* anchor = mShownAnchorView;
        if ((anchor == nullptr) || !anchor->isShown()) {
            dismiss();
        } else {
            // Recompute window size and position
            mPopup->show();
        }
    }
}

void StandardMenuPopup::onViewDetachedFromWindow(View& v) {
    if (mTreeObserver != nullptr) {
        if (!mTreeObserver->isAlive()) mTreeObserver = v.getViewTreeObserver();
        mTreeObserver->removeGlobalOnLayoutListener(mGlobalLayoutListener);
    }
    v.removeOnAttachStateChangeListener(mAttachStateChangeListener);//this);
}

StandardMenuPopup::StandardMenuPopup(Context* context, MenuBuilder* menu, View* anchorView,
        const std::string& popupStyleAttr,const std::string& popupStyleRes, bool overflowOnly) {
    mContext = context;//Objects.requireNonNull(context);
    mMenu = menu;
    mOverflowOnly = overflowOnly;
    LayoutInflater* inflater = LayoutInflater::from(context);
    mAdapter = new MenuAdapter(menu, inflater, mOverflowOnly, ITEM_LAYOUT_MATERIAL);
    mPopupStyleAttr= popupStyleAttr;
    mPopupStyleRes = popupStyleRes;

    mAttachStateChangeListener.onViewDetachedFromWindow=[this](View&v){
        onViewDetachedFromWindow(v);
    };
    mGlobalLayoutListener=[this](){
        onGlobalLayout();
    };
    mPopupMaxWidth = std::max(context->getDisplayMetrics().widthPixels / 2,context->getDimensionPixelSize("cdroid:dimen/config_prefDialogWidth"));
    mAnchorView = anchorView;
    mPopup = new MenuPopupWindow(mContext,AttributeSet(mContext,"cdroid"), mPopupStyleAttr, mPopupStyleRes);

    // Present the menu using our context, not the menu builder's context.
    menu->addMenuPresenter(this, context);
}

void StandardMenuPopup::setForceShowIcon(bool forceShow) {
    mAdapter->setForceShowIcon(forceShow);
}

void StandardMenuPopup::setGravity(int gravity) {
    mDropDownGravity = gravity;
}

bool StandardMenuPopup::tryShow() {
    if (isShowing()) {
        return true;
    }

    if (mWasDismissed || (mAnchorView == nullptr)) {
        return false;
    }

    mShownAnchorView = mAnchorView;

    mPopup->setOnDismissListener([this](){onDismiss();});
    mPopup->setOnItemClickListener([this](AdapterView&parent, View& view, int position, long id){
        onItemClick(parent,view,position,id);
    });
    mPopup->setAdapter(mAdapter);
    mPopup->setModal(true);

    View* anchor = mShownAnchorView;
    const bool addGlobalListener = (mTreeObserver == nullptr);
    mTreeObserver = anchor->getViewTreeObserver(); // Refresh to latest
    if (addGlobalListener) {
        mTreeObserver->addOnGlobalLayoutListener(mGlobalLayoutListener);
    }
    anchor->addOnAttachStateChangeListener(mAttachStateChangeListener);
    mPopup->setAnchorView(anchor);
    mPopup->setDropDownGravity(mDropDownGravity);

    if (!mHasContentWidth) {
        mContentWidth = measureIndividualMenuWidth(mAdapter, nullptr, mContext, mPopupMaxWidth);
        mHasContentWidth = true;
    }

    mPopup->setContentWidth(mContentWidth);
    mPopup->setInputMethodMode(PopupWindow::INPUT_METHOD_NOT_NEEDED);
    mPopup->setEpicenterBounds(getEpicenterBounds());
    mPopup->show();

    ListView* listView = mPopup->getListView();
    listView->setOnKeyListener([this](View& v, int keyCode, KeyEvent&event){
        return onKey(v,keyCode,event);
    });

    if (mShowTitle && mMenu->getHeaderTitle().size()){// != null) {
        FrameLayout* titleItemView =(FrameLayout*) LayoutInflater::from(mContext)->inflate(
                        "cdroid:layout/popup_menu_header_item_layout",listView, false);
        TextView* titleView = (TextView*) titleItemView->findViewById(cdroid::R::id::title);
        if (titleView != nullptr) {
            titleView->setText(mMenu->getHeaderTitle());
        }
        titleItemView->setEnabled(false);
        listView->addHeaderView(titleItemView, nullptr, false);

        // Update to show the title.
        mPopup->show();
    }
    return true;
}

void StandardMenuPopup::show() {
    if (!tryShow()) {
        throw std::runtime_error("StandardMenuPopup cannot be used without an anchor");
    }
}

void StandardMenuPopup::dismiss() {
    if (isShowing()) {
        mPopup->dismiss();
    }
}

void StandardMenuPopup::addMenu(MenuBuilder* menu) {
    // No-op: standard implementation has only one menu which is set in the constructor.
}

bool StandardMenuPopup::isShowing() {
    return !mWasDismissed && mPopup->isShowing();
}

void StandardMenuPopup::onDismiss() {
    mWasDismissed = true;
    mMenu->close();

    if (mTreeObserver != nullptr) {
        if (!mTreeObserver->isAlive()) mTreeObserver = mShownAnchorView->getViewTreeObserver();
        mTreeObserver->removeGlobalOnLayoutListener(mGlobalLayoutListener);
        mTreeObserver = nullptr;
    }
    mShownAnchorView->removeOnAttachStateChangeListener(mAttachStateChangeListener);

    if (mOnDismissListener != nullptr) {
        mOnDismissListener/*.onDismiss*/();
    }
}

void StandardMenuPopup::updateMenuView(bool cleared) {
    mHasContentWidth = false;

    if (mAdapter != nullptr) {
        mAdapter->notifyDataSetChanged();
    }
}

void StandardMenuPopup::setCallback(const Callback& cb) {
    mPresenterCallback = cb;
}

bool StandardMenuPopup::onSubMenuSelected(SubMenuBuilder* subMenu) {
    if (subMenu->hasVisibleItems()) {
        MenuPopupHelper* subPopup = new MenuPopupHelper(mContext, subMenu,
                mShownAnchorView, mOverflowOnly, mPopupStyleAttr, mPopupStyleRes);
        subPopup->setPresenterCallback(mPresenterCallback);
        subPopup->setForceShowIcon(MenuPopup::shouldPreserveIconSpacing(subMenu));

        // Pass responsibility for handling onDismiss to the submenu.
        subPopup->setOnDismissListener(mOnDismissListener);
        mOnDismissListener = nullptr;

        // Close this menu popup to make room for the submenu popup.
        mMenu->close(false /* closeAllMenus */);

        // Show the new sub-menu popup at the same location as this popup.
        int horizontalOffset = mPopup->getHorizontalOffset();
        const int verticalOffset = mPopup->getVerticalOffset();

        // As xOffset of parent menu popup is subtracted with Anchor width for Gravity.RIGHT,
        // So, again to display sub-menu popup in same xOffset, add the Anchor width.
        const int hgrav = Gravity::getAbsoluteGravity(mDropDownGravity,
            mAnchorView->getLayoutDirection()) & Gravity::HORIZONTAL_GRAVITY_MASK;
        if (hgrav == Gravity::RIGHT) {
          horizontalOffset += mAnchorView->getWidth();
        }

        if (subPopup->tryShow(horizontalOffset, verticalOffset)) {
            if (mPresenterCallback.onOpenSubMenu != nullptr) {
                mPresenterCallback.onOpenSubMenu(*subMenu);
            }
            return true;
        }
    }
    return false;
}

void StandardMenuPopup::onCloseMenu(MenuBuilder* menu, bool allMenusAreClosing) {
    // Only care about the (sub)menu we're presenting.
    if (menu != mMenu) return;

    dismiss();
    if (mPresenterCallback.onCloseMenu != nullptr) {
        mPresenterCallback.onCloseMenu(*menu, allMenusAreClosing);
    }
}

bool StandardMenuPopup::flagActionItems() {
    return false;
}

Parcelable* StandardMenuPopup::onSaveInstanceState() {
    return nullptr;
}

void StandardMenuPopup::onRestoreInstanceState(Parcelable& state) {
}

void StandardMenuPopup::setAnchorView(View* anchor) {
    mAnchorView = anchor;
}

bool StandardMenuPopup::onKey(View& v, int keyCode, KeyEvent& event) {
    if (event.getAction() == KeyEvent::ACTION_UP && keyCode == KeyEvent::KEYCODE_MENU) {
        dismiss();
        return true;
    }
    return false;
}

void StandardMenuPopup::setOnDismissListener(const PopupWindow::OnDismissListener& listener) {
    mOnDismissListener = listener;
}

ListView* StandardMenuPopup::getListView() const{
    return mPopup->getListView();
}


void StandardMenuPopup::setHorizontalOffset(int x) {
    mPopup->setHorizontalOffset(x);
}

void StandardMenuPopup::setVerticalOffset(int y) {
    mPopup->setVerticalOffset(y);
}

void StandardMenuPopup::setShowTitle(bool showTitle) {
    mShowTitle = showTitle;
}
}/*endof namespace*/
