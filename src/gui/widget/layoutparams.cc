#include <widget/layoutparams.h>
#include <widget/view.h>
#include <cdlog.h>
namespace cdroid{

LayoutParams::LayoutParams(){
}

LayoutParams::LayoutParams(Context* c,const AttributeSet& attrs){
    width =attrs.getLayoutDimension("layout_width" ,WRAP_CONTENT);
    height=attrs.getLayoutDimension("layout_height",WRAP_CONTENT);
}

LayoutParams::LayoutParams(int w, int h):width(w),height(h){
}

LayoutParams::LayoutParams(const LayoutParams& source){
    width=source.width;
	height=source.height;
}

void LayoutParams::setBaseAttributes(const AttributeSet& a, int widthAttr, int heightAttr){
}

void LayoutParams::resolveLayoutDirection(int layoutDirection){
}

void LayoutParams::onDebugDraw(View&view, Canvas&canvas){
}

const std::string LayoutParams::sizeToString(int size) {
    switch(size){
	case WRAP_CONTENT: return "wrap-content";
    case MATCH_PARENT: return "match-parent";
    default:return std::to_string(size);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
MarginLayoutParams::MarginLayoutParams(Context*c,const AttributeSet& attrs)
   :LayoutParams(c,attrs){
    int margin=attrs.getDimensionPixelSize("margin",-1);
    if(margin>0){
        leftMargin=topMargin=rightMargin=bottomMargin=margin;
    }else{
        int horzMargin = attrs.getDimensionPixelSize("marginHorizontal",-1);
        int vertMargin = attrs.getDimensionPixelSize("marginVertical",-1);
        if(horzMargin>=0){
            leftMargin= rightMargin =horzMargin;
        }else{
            leftMargin = attrs.getDimensionPixelSize("leftMargin", UNDEFINED_MARGIN);
            rightMargin= attrs.getDimensionPixelSize("rightMargin",UNDEFINED_MARGIN);
            if(leftMargin==UNDEFINED_MARGIN){
                mMarginFlags |= LEFT_MARGIN_UNDEFINED_MASK;
                leftMargin = DEFAULT_MARGIN_RESOLVED;
            }
            if (rightMargin == UNDEFINED_MARGIN) {
                mMarginFlags |= RIGHT_MARGIN_UNDEFINED_MASK;
                rightMargin = DEFAULT_MARGIN_RESOLVED;
            }
        }
        startMargin = attrs.getDimensionPixelSize("startMargin",DEFAULT_MARGIN_RELATIVE);
        endMargin   = attrs.getDimensionPixelSize("endMargin",DEFAULT_MARGIN_RELATIVE);
        if(vertMargin>=0){
            topMargin = bottomMargin = vertMargin;
        }else{
            topMargin   = attrs.getDimensionPixelSize("topMargin",DEFAULT_MARGIN_RESOLVED);
            bottomMargin= attrs.getDimensionPixelSize("bottomMargin",DEFAULT_MARGIN_RESOLVED);
        }
        if (isMarginRelative()) {
            mMarginFlags |= NEED_RESOLUTION_MASK;
        }
    }
    mMarginFlags |= View::LAYOUT_DIRECTION_LTR;
}

MarginLayoutParams::MarginLayoutParams(int width, int height)
	:LayoutParams(width, height){
	mMarginFlags=0;
    leftMargin = topMargin=0;
    rightMargin= bottomMargin=0;
    startMargin=endMargin=0;
    mMarginFlags |= LEFT_MARGIN_UNDEFINED_MASK;
    mMarginFlags |= RIGHT_MARGIN_UNDEFINED_MASK;
    mMarginFlags &= ~NEED_RESOLUTION_MASK;
    mMarginFlags &= ~RTL_COMPATIBILITY_MODE_MASK;
}

MarginLayoutParams::MarginLayoutParams(const LayoutParams& source)
	:LayoutParams(source){
    mMarginFlags=0;
    leftMargin = topMargin=0;
    rightMargin= bottomMargin=0;
    startMargin=endMargin=0;

    mMarginFlags |= LEFT_MARGIN_UNDEFINED_MASK;
    mMarginFlags |= RIGHT_MARGIN_UNDEFINED_MASK;

    mMarginFlags &= ~NEED_RESOLUTION_MASK;
    mMarginFlags &= ~RTL_COMPATIBILITY_MODE_MASK;
}

MarginLayoutParams::MarginLayoutParams(const MarginLayoutParams& source){
    width = source.width;
    height = source.height;

    leftMargin = source.leftMargin;
    topMargin = source.topMargin;
    rightMargin = source.rightMargin;
    bottomMargin = source.bottomMargin;
    startMargin = source.startMargin;
    endMargin = source.endMargin;

    mMarginFlags = source.mMarginFlags;
}

void MarginLayoutParams::copyMarginsFrom(const MarginLayoutParams& source){
    width = source.width;
    height = source.height;
    leftMargin = source.leftMargin;
    topMargin = source.topMargin;
    rightMargin = source.rightMargin;
    bottomMargin = source.bottomMargin;
    startMargin = source.startMargin;
    endMargin = source.endMargin;

    mMarginFlags = source.mMarginFlags;
}

void MarginLayoutParams::setMargins(int left, int top, int right, int bottom){
    leftMargin = left;
    topMargin = top;
    rightMargin = right;
    bottomMargin = bottom;
    mMarginFlags &= ~LEFT_MARGIN_UNDEFINED_MASK;
    mMarginFlags &= ~RIGHT_MARGIN_UNDEFINED_MASK;
    if (isMarginRelative()) {
        mMarginFlags |= NEED_RESOLUTION_MASK;
    } else {
        mMarginFlags &= ~NEED_RESOLUTION_MASK;
    }
}

void MarginLayoutParams::setMarginsRelative(int start, int top, int end, int bottom){
    startMargin = start;
    topMargin = top;
    endMargin = end;
    bottomMargin = bottom;
    mMarginFlags |= NEED_RESOLUTION_MASK;
}

void MarginLayoutParams::setMarginStart(int start){
    startMargin = start;
    mMarginFlags |= NEED_RESOLUTION_MASK;
}

int MarginLayoutParams::getMarginStart(){
    if (startMargin != DEFAULT_MARGIN_RELATIVE) return startMargin;
    if ((mMarginFlags & NEED_RESOLUTION_MASK) == NEED_RESOLUTION_MASK) {
       doResolveMargins();
    }
    switch(mMarginFlags & LAYOUT_DIRECTION_MASK) {
	case View::LAYOUT_DIRECTION_RTL:return rightMargin;
	case View::LAYOUT_DIRECTION_LTR:
    default:return leftMargin;
    }
}

void MarginLayoutParams::setMarginEnd(int end){
    endMargin = end;
    mMarginFlags |= NEED_RESOLUTION_MASK;
}

int MarginLayoutParams::getMarginEnd(){
    if (endMargin != DEFAULT_MARGIN_RELATIVE) return endMargin;
    if ((mMarginFlags & NEED_RESOLUTION_MASK) == NEED_RESOLUTION_MASK) {
        doResolveMargins();
    }
    switch(mMarginFlags & LAYOUT_DIRECTION_MASK) {
	case View::LAYOUT_DIRECTION_RTL:return leftMargin;
	case View::LAYOUT_DIRECTION_LTR:
    default:return rightMargin;
    }
}

bool MarginLayoutParams::isMarginRelative()const{
    return (startMargin != DEFAULT_MARGIN_RELATIVE || endMargin != DEFAULT_MARGIN_RELATIVE);
}

void MarginLayoutParams::setLayoutDirection(int layoutDirection){
    if (layoutDirection != View::LAYOUT_DIRECTION_LTR &&
          layoutDirection != View::LAYOUT_DIRECTION_RTL) return;
    if (layoutDirection != (mMarginFlags & LAYOUT_DIRECTION_MASK)) {
        mMarginFlags &= ~LAYOUT_DIRECTION_MASK;
        mMarginFlags |= (layoutDirection & LAYOUT_DIRECTION_MASK);
        if (isMarginRelative()) {
            mMarginFlags |= NEED_RESOLUTION_MASK;
        } else {
            mMarginFlags &= ~NEED_RESOLUTION_MASK;
        }
    }
}

int MarginLayoutParams::getLayoutDirection()const{
    return (mMarginFlags & LAYOUT_DIRECTION_MASK);
}

void MarginLayoutParams::resolveLayoutDirection(int layoutDirection){
    setLayoutDirection(layoutDirection);

    // No relative margin or pre JB-MR1 case or no need to resolve, just dont do anything
    // Will use the left and right margins if no relative margin is defined.
    if (!isMarginRelative() || (mMarginFlags & NEED_RESOLUTION_MASK) != NEED_RESOLUTION_MASK) return;

    // Proceed with resolution
    doResolveMargins();
}

bool MarginLayoutParams::isLayoutRtl()const{
    return ((mMarginFlags & LAYOUT_DIRECTION_MASK) == View::LAYOUT_DIRECTION_RTL);
}

void MarginLayoutParams::onDebugDraw(View&view, Canvas&canvas){

}

void MarginLayoutParams::doResolveMargins(){
    if ((mMarginFlags & RTL_COMPATIBILITY_MODE_MASK) == RTL_COMPATIBILITY_MODE_MASK) {
        // if left or right margins are not defined and if we have some start or end margin
        // defined then use those start and end margins.
        if ((mMarginFlags & LEFT_MARGIN_UNDEFINED_MASK) == LEFT_MARGIN_UNDEFINED_MASK
               && startMargin > DEFAULT_MARGIN_RELATIVE) {
            leftMargin = startMargin;
        }
        if ((mMarginFlags & RIGHT_MARGIN_UNDEFINED_MASK) == RIGHT_MARGIN_UNDEFINED_MASK
               && endMargin > DEFAULT_MARGIN_RELATIVE) {
            rightMargin = endMargin;
        }
    } else {
        // We have some relative margins (either the start one or the end one or both). So use
        // them and override what has been defined for left and right margins. If either start
        // or end margin is not defined, just set it to default "0".
        switch(mMarginFlags & LAYOUT_DIRECTION_MASK) {
		case View::LAYOUT_DIRECTION_RTL:
             leftMargin = (endMargin > DEFAULT_MARGIN_RELATIVE) ?
                     endMargin : DEFAULT_MARGIN_RESOLVED;
             rightMargin = (startMargin > DEFAULT_MARGIN_RELATIVE) ?
                     startMargin : DEFAULT_MARGIN_RESOLVED;
             break;
		case View::LAYOUT_DIRECTION_LTR:
        default:
             leftMargin = (startMargin > DEFAULT_MARGIN_RELATIVE) ?
                     startMargin : DEFAULT_MARGIN_RESOLVED;
             rightMargin = (endMargin > DEFAULT_MARGIN_RELATIVE) ?
                     endMargin : DEFAULT_MARGIN_RESOLVED;
             break;
        }
    }
    mMarginFlags &= ~NEED_RESOLUTION_MASK;
}

}//namespace

