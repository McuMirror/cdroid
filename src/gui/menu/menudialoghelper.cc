#include <menu/menubuilder.h>
#include <menu/menudialoghelper.h>
#include <menu/listmenupresenter.h>
namespace cdroid{

MenuDialogHelper::MenuDialogHelper(MenuBuilder* menu) {
    mMenu = menu;
}

void MenuDialogHelper::show() {
    // Many references to mMenu, create local reference
    MenuBuilder* menu = mMenu;

    // Get the builder for the dialog
    AlertDialog::Builder* builder = new AlertDialog::Builder(menu->getContext());

    mPresenter = new ListMenuPresenter(builder->getContext(),"android:layout/list_menu_item_layout");

    MenuPresenter::Callback mpc;
    mpc.onCloseMenu=std::bind(&MenuDialogHelper::onCloseMenu,this,std::placeholders::_1,std::placeholders::_2);
    mpc.onOpenSubMenu=std::bind(&MenuDialogHelper::onOpenSubMenu,this,std::placeholders::_1);
    mPresenter->setCallback(mpc);
    mMenu->addMenuPresenter(mPresenter);
    DialogInterface::OnClickListener onClick=std::bind(&MenuDialogHelper::onClick,this,std::placeholders::_1,std::placeholders::_2);
    builder->setAdapter(mPresenter->getAdapter(), onClick);

    // Set the title
    View* headerView = menu->getHeaderView();
    if (headerView != nullptr) {
        // Menu's client has given a custom header view, use it
        builder->setCustomTitle(headerView);
    } else {
        // Otherwise use the (text) title and icon
        builder->setIcon(menu->getHeaderIcon()).setTitle(menu->getHeaderTitle());
    }

    // Set the key listener
    DialogInterface::OnKeyListener onKey=std::bind(&MenuDialogHelper::onKey,this,
            std::placeholders::_1, std::placeholders::_2,std::placeholders::_3);
    builder->setOnKeyListener(onKey);

    // Show the menu
    mDialog = builder->create();
    DialogInterface::OnDismissListener onDismiss = std::bind(&MenuDialogHelper::onDismiss,this,
            std::placeholders::_1);
    mDialog->setOnDismissListener(onDismiss);
#if 0
    WindowManager.LayoutParams lp = mDialog.getWindow().getAttributes();
    lp.type = WindowManager.LayoutParams.TYPE_APPLICATION_ATTACHED_DIALOG;
    if (windowToken != null) {
        lp.token = windowToken;
    }
    lp.flags |= WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM;
#endif

    mDialog->show();
}

bool MenuDialogHelper::onKey(DialogInterface& dialog, int keyCode, KeyEvent& event) {
    if (keyCode == KeyEvent::KEYCODE_MENU || keyCode == KeyEvent::KEYCODE_BACK) {
        if (event.getAction() == KeyEvent::ACTION_DOWN
                && event.getRepeatCount() == 0) {
            Window* win = mDialog->getWindow();
            if (win != nullptr) {
                View* decor = win->getRootView();//win->getDecorView();
                if (decor != nullptr) {
                    KeyEvent::DispatcherState* ds = decor->getKeyDispatcherState();
                    if (ds != nullptr) {
                        ds->startTracking(event, this);
                        return true;
                    }
                }
            }
        } else if (event.getAction() == KeyEvent::ACTION_UP && !event.isCanceled()) {
            Window* win = mDialog->getWindow();
            if (win != nullptr) {
                View* decor = win->getRootView();//win->getDecorView();
                if (decor != nullptr) {
                    KeyEvent::DispatcherState* ds = decor->getKeyDispatcherState();
                    if (ds != nullptr && ds->isTracking(event)) {
                        mMenu->close(true /* closeAllMenus */);
                        dialog.dismiss();
                        return true;
                    }
                }
            }
        }
    }

    // Menu shortcut matching
    return mMenu->performShortcut(keyCode, event, 0);

}

void MenuDialogHelper::setPresenterCallback(const MenuPresenter::Callback& cb) {
    mPresenterCallback = cb;
}

void MenuDialogHelper::dismiss() {
    if (mDialog != nullptr) {
        mDialog->dismiss();
    }
}

void MenuDialogHelper::onDismiss(DialogInterface& dialog) {
    mPresenter->onCloseMenu(mMenu, true);
}

void MenuDialogHelper::onCloseMenu(MenuBuilder& menu, bool allMenusAreClosing) {
    if (allMenusAreClosing || &menu == mMenu) {
        dismiss();
    }
    if (mPresenterCallback.onCloseMenu != nullptr) {
        mPresenterCallback.onCloseMenu(menu, allMenusAreClosing);
    }
}

bool MenuDialogHelper::onOpenSubMenu(MenuBuilder& subMenu) {
    if (mPresenterCallback.onOpenSubMenu != nullptr) {
        return mPresenterCallback.onOpenSubMenu(subMenu);
    }
    return false;
}

void MenuDialogHelper::onClick(DialogInterface& dialog, int which) {
    mMenu->performItemAction((MenuItem*) mPresenter->getAdapter()->getItem(which), 0);
}
}
