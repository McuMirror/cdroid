#include <view/menuitem.h>
#include <view/view.h>
namespace cdroid{

MenuItem& MenuItem::setIconTintList(const ColorStateList* tint){
    return *this;
}

const ColorStateList* MenuItem::getIconTintList() {
    return nullptr;
}

MenuItem& MenuItem::setIconTintMode(int tintMode) {
    return *this;
}

int MenuItem::getIconTintMode() {
    return 0;
}

MenuItem& MenuItem::setIntent(Intent* intent){
    return *this;
}

Intent* MenuItem::getIntent(){
    return nullptr;
}

MenuItem& MenuItem::setShortcut(int numericChar, int alphaChar){
    return *this;
}

MenuItem& MenuItem::setShortcut(int numericChar, int alphaChar, int numericModifiers, int alphaModifiers) {
    if ((alphaModifiers & Menu::SUPPORTED_MODIFIERS_MASK) == KeyEvent::META_CTRL_ON
            && (numericModifiers & Menu::SUPPORTED_MODIFIERS_MASK) == KeyEvent::META_CTRL_ON) {
        return setShortcut(numericChar, alphaChar);
    } else {
        return *this;
    }
}

MenuItem& MenuItem::setNumericShortcut(int numericChar){
    return *this;
}

MenuItem& MenuItem::setNumericShortcut(int numericChar, int numericModifiers) {
    if ((numericModifiers & Menu::SUPPORTED_MODIFIERS_MASK) == KeyEvent::META_CTRL_ON) {
        return setNumericShortcut(numericChar);
    } else {
        return *this;
    }
}

int MenuItem::getNumericShortcut() const{
    return 0;
}

int MenuItem::getNumericModifiers() const{
    return KeyEvent::META_CTRL_ON;
}

MenuItem& MenuItem::setAlphabeticShortcut(int alphaChar){
    return *this;
}

MenuItem& MenuItem::setAlphabeticShortcut(int alphaChar, int alphaModifiers) {
    if ((alphaModifiers & Menu::SUPPORTED_MODIFIERS_MASK) == KeyEvent::META_CTRL_ON) {
        return setAlphabeticShortcut(alphaChar);
    } else {
        return *this;
    }
}

int MenuItem::getAlphabeticShortcut(){
    return 0;
}

int MenuItem::getAlphabeticModifiers() {
    return KeyEvent::META_CTRL_ON;
}

MenuItem& MenuItem::setCheckable(bool checkable){
    return *this;
}

bool MenuItem::isCheckable()const{
    return false;
}
MenuItem& MenuItem::setChecked(bool checked){
    return *this;
}
bool MenuItem::isChecked()const{
    return false;
}

MenuItem& MenuItem::setVisible(bool visible){
    return *this;
}
bool MenuItem::isVisible()const{
    return true;
}

MenuItem& MenuItem::setEnabled(bool enabled){
    return *this;
}
bool MenuItem::isEnabled()const{
    return true;
}

bool MenuItem::hasSubMenu(){
    return false;
}

SubMenu* MenuItem::getSubMenu(){
    return nullptr;
};

MenuItem& MenuItem::setOnMenuItemClickListener(const OnMenuItemClickListener& menuItemClickListener){
    return *this;
}

ContextMenuInfo* MenuItem::getMenuInfo(){
    return nullptr;
}

void MenuItem::setShowAsAction(int actionEnum){
}

MenuItem& MenuItem::setShowAsActionFlags(int actionEnum){
    return *this;
}

MenuItem& MenuItem::setActionView(View* view){
    return *this;
}
MenuItem& MenuItem::setActionView(const std::string& resId){
    return *this;
}
View* MenuItem::getActionView(){
    return nullptr;
}

MenuItem& MenuItem::setActionProvider(ActionProvider* actionProvider){
   return *this;
}
ActionProvider* MenuItem::getActionProvider(){
   return nullptr;
}

bool MenuItem::expandActionView(){
    return false;
}
bool MenuItem::collapseActionView(){
    return false;
}
bool MenuItem::isActionViewExpanded(){
    return false;
}

MenuItem& MenuItem::setOnActionExpandListener(OnActionExpandListener listener){
    return *this;
}

MenuItem& MenuItem::setContentDescription(const std::string& contentDescription) {
    return *this;
}

std::string MenuItem::getContentDescription() {
    return std::string();
}

MenuItem& MenuItem::setTooltipText(const std::string& tooltipText) {
    return *this;
}

std::string MenuItem::getTooltipText() {
    return std::string();
}

bool MenuItem::requiresActionButton() {
    return false;
}

bool MenuItem::requiresOverflow() {
    return true;
}
}/*endof namespace*/

