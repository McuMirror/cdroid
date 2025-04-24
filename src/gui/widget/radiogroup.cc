#include <widget/radiogroup.h>
#include <widget/radiobutton.h>
#include <cdlog.h>


namespace cdroid{

DECLARE_WIDGET(RadioGroup)

RadioGroup::RadioGroup(int w,int h):LinearLayout(w,h){
    init();
    setOrientation(VERTICAL);
}

RadioGroup::RadioGroup(Context* context,const AttributeSet& attrs)
    :LinearLayout(context,attrs){
    init();
    mCheckedId = attrs.getResourceId("id",View::NO_ID);
    mInitialCheckedId = (mCheckedId!=View::NO_ID);
}

LayoutParams* RadioGroup::generateLayoutParams(const AttributeSet& attrs)const {
    return new LayoutParams(getContext(), attrs);
}

bool RadioGroup::checkLayoutParams(const ViewGroup::LayoutParams* p)const {
    return dynamic_cast<const LayoutParams*>(p);
}

ViewGroup::LayoutParams* RadioGroup::generateDefaultLayoutParams()const {
    return new LayoutParams(LayoutParams::WRAP_CONTENT, LayoutParams::WRAP_CONTENT);
}

void RadioGroup::onRadioChecked(CompoundButton&c,bool checked){
    if(mProtectFromCheckedChange)return;
    mProtectFromCheckedChange =true;
    if (mCheckedId != -1) {
        setCheckedStateForView(mCheckedId, false);
    }
    LOGD("onRadioChecked %d",c.getId());
    mProtectFromCheckedChange = false;
    setCheckedId(c.getId());
}

void RadioGroup::onChildViewAdded(ViewGroup& parent, View* child){
    if((&parent==this)&&dynamic_cast<RadioButton*>(child)){
	int id = child->getId();
	if(id==View::NO_ID){
	    id = child->generateViewId();
	    child->setId(id);
	}
	((RadioButton*)child)->setOnCheckedChangeWidgetListener(mChildOnCheckedChangeListener);
        if(mOnHierarchyChangeListener.onChildViewAdded)
            mOnHierarchyChangeListener.onChildViewAdded(parent, child);
    }
}

void RadioGroup::onChildViewRemoved(ViewGroup& parent, View* child){
    if((&parent==this)&&dynamic_cast<RadioButton*>(child)){
	    ((RadioButton*)child)->setOnCheckedChangeWidgetListener(nullptr);
    }
    if(mOnHierarchyChangeListener.onChildViewRemoved)
        mOnHierarchyChangeListener.onChildViewRemoved(parent, child);
}

void RadioGroup::init(){
    ViewGroup::OnHierarchyChangeListener lhs;
    mCheckedId = View::NO_ID;
    mInitialCheckedId = false;
    mChildOnCheckedChangeListener=std::bind(&RadioGroup::onRadioChecked,this,std::placeholders::_1,std::placeholders::_2);
    lhs.onChildViewAdded  = std::bind(&RadioGroup::onChildViewAdded,this,std::placeholders::_1,std::placeholders::_2);
    lhs.onChildViewRemoved= std::bind(&RadioGroup::onChildViewRemoved,this,std::placeholders::_1,std::placeholders::_2);
    LinearLayout::setOnHierarchyChangeListener(lhs);
}

void RadioGroup::setOnHierarchyChangeListener(const ViewGroup::OnHierarchyChangeListener& listener) {
        // the user listener is delegated to our pass-through listener
     mOnHierarchyChangeListener = listener;
}

void RadioGroup::setOnCheckedChangeListener(CompoundButton::OnCheckedChangeListener listener){
    mOnCheckedChangeListener = listener;
}

int RadioGroup::getCheckedRadioButtonId()const{
    return mCheckedId;
}

std::string RadioGroup::getAccessibilityClassName()const{
    return "RadioGroup";
}

void RadioGroup::setCheckedId(int id){
    mCheckedId = id;
    if (mOnCheckedChangeListener != nullptr) {
        mOnCheckedChangeListener((CompoundButton&)*this, mCheckedId);
    }
}

void RadioGroup::setCheckedStateForView(int viewId, bool checked){
    View* checkedView = findViewById(viewId);
    if (checkedView != nullptr && dynamic_cast<RadioButton*>(checkedView)) {
        ((RadioButton*) checkedView)->setChecked(checked);
    }
}

void RadioGroup::check(int id){
    if ((id != -1) && (id == mCheckedId)) {
        return;
    }

    if (mCheckedId != -1) {
        setCheckedStateForView(mCheckedId, false);
    }

    if (id != -1) {
        setCheckedStateForView(id, true);
    }
    setCheckedId(id);
}

void RadioGroup::clearCheck(){
    check(-1);
}

void RadioGroup::onFinishInflate() {
    LinearLayout::onFinishInflate();

    // checks the appropriate radio button as requested in the XML file
    if (mCheckedId != -1) {
        mProtectFromCheckedChange = true;
        setCheckedStateForView(mCheckedId, true);
        mProtectFromCheckedChange = false;
        setCheckedId(mCheckedId);
    }
}

View& RadioGroup::addView(View* child, int index,ViewGroup::LayoutParams* params){
    if (dynamic_cast<RadioButton*>(child)) {
        RadioButton* button = (RadioButton*) child;
        if (button->isChecked()) {
            mProtectFromCheckedChange = true;
            if (mCheckedId != -1) {
                setCheckedStateForView(mCheckedId, false);
            }
            mProtectFromCheckedChange = false;
            setCheckedId(button->getId());
        }
    }
    return LinearLayout::addView(child, index, params);
}

//////////////////////////////////////////////////////////////////////////////////
RadioGroup::LayoutParams::LayoutParams(Context*c,const AttributeSet&attrs)
:LinearLayout::LayoutParams(c,attrs){
}

RadioGroup::LayoutParams::LayoutParams(int w,int h)
:LinearLayout::LayoutParams(w,h){
}

RadioGroup::LayoutParams::LayoutParams(int w, int h, float initWeight)
    :LinearLayout::LayoutParams(w,h,initWeight){
}

RadioGroup::LayoutParams::LayoutParams(const ViewGroup::LayoutParams& p)
    :LinearLayout::LayoutParams(p){
}

RadioGroup::LayoutParams::LayoutParams(const MarginLayoutParams& source)
    :LinearLayout::LayoutParams(source){
}

}
