#ifndef __MENU_VIEW_H__
#define __MENU_VIEW_H__
namespace cdroid{
class Drawable;
class MenuBuilder;
class MenuItemImpl;
class MenuView {
public:
    class ItemView;
    /**
     * Initializes the menu to the given menu. This should be called after the
     * view is inflated.
     *
     * @param menu The menu that this MenuView should display.
     */
    virtual void initialize(MenuBuilder* menu)=0;

    /**
     * Returns the default animations to be used for this menu when entering/exiting.
     * @return A resource ID for the default animations to be used for this menu.
     */
    virtual int getWindowAnimations()=0;
};//
/**
 * Minimal interface for a menu item view.  {@link #initialize(MenuItemImpl, int)} must be called
 * for the item to be functional.
 */
class MenuView::ItemView {
public:
    /**
     * Initializes with the provided MenuItemData.  This should be called after the view is
     * inflated.
     * @param itemData The item that this ItemView should display.
     * @param menuType The type of this menu, one of
     *            {@link MenuBuilder#TYPE_ICON}, {@link MenuBuilder#TYPE_EXPANDED},
     *            {@link MenuBuilder#TYPE_DIALOG}).
     */
    virtual void initialize(MenuItemImpl* itemData, int menuType)=0;

    /**
     * Gets the item data that this view is displaying.
     * @return the item data, or null if there is not one
     */
    virtual MenuItemImpl* getItemData()=0;

    /**
     * Sets the title of the item view.
     * @param title The title to set.
     */
    virtual void setTitle(const std::string&title)=0;

    /**
     * Sets the enabled state of the item view.
     * @param enabled Whether the item view should be enabled.
     */
    virtual void setEnabled(bool enabled){};

    /**
     * Displays the checkbox for the item view.  This does not ensure the item view will be
     * checked, for that use {@link #setChecked}.
     * @param checkable Whether to display the checkbox or to hide it
     */
    virtual void setCheckable(bool checkable)=0;

    /**
     * Checks the checkbox for the item view.  If the checkbox is hidden, it will NOT be
     * made visible, call {@link #setCheckable(boolean)} for that.
     * @param checked Whether the checkbox should be checked
     */
    virtual void setChecked(bool checked)=0;

    /**
     * Sets the shortcut for the item.
     * @param showShortcut Whether a shortcut should be shown(if false, the value of
     * shortcutKey should be ignored).
     * @param shortcutKey The shortcut key that should be shown on the ItemView.
     */
    virtual void setShortcut(bool showShortcut, char shortcutKey)=0;

    /**
     * Set the icon of this item view.
     * @param icon The icon of this item. null to hide the icon.
     */
    virtual void setIcon(Drawable* icon)=0;

    /**
     * Whether this item view prefers displaying the condensed title rather
     * than the normal title. If a condensed title is not available, the
     * normal title will be used.
     *
     * @return Whether this item view prefers displaying the condensed
     *         title.
     */
    virtual bool prefersCondensedTitle()=0;

    /**
     * Whether this item view shows an icon.
     *
     * @return Whether this item view shows an icon.
     */
    virtual bool showsIcon()=0;
};
}/*endof namespace*/
#endif/*__MENU_VIEW_H__*/
