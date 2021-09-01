#ifndef __GRADIENT_DRAWABLE_H__
#define __GRADIENT_DRAWABLE_H__
#include <drawables/drawable.h>
namespace cdroid{

class GradientDrawable:public Drawable{
public:
    enum Orientation {
        /** draw the gradient from the top to the bottom */
        TOP_BOTTOM,
        /** draw the gradient from the top-right to the bottom-left */
        TR_BL,
        /** draw the gradient from the right to the left */
        RIGHT_LEFT,
        /** draw the gradient from the bottom-right to the top-left */
        BR_TL,
        /** draw the gradient from the bottom to the top */
        BOTTOM_TOP,
        /** draw the gradient from the bottom-left to the top-right */
        BL_TR,
        /** draw the gradient from the left to the right */
        LEFT_RIGHT,
        /** draw the gradient from the top-left to the bottom-right */
        TL_BR,
    };
    enum Shape{
        RECTANGLE   =0,
        OVAL        =1,
        LINE        =2,
        RING        =3
    };
    enum GradientType{
        LINEAR_GRADIENT=0,
        RADIAL_GRADIENT=1,
        SWEEP_GRADIENT=2
    };
    enum RadiusType{
        RADIUS_TYPE_PIXELS  =0,
        RADIUS_TYPE_FRACTION=1,
        RADIUS_TYPE_FRACTION_PARENT=2
    };
private:
    class GradientState:public std::enable_shared_from_this<GradientState>,public ConstantState{
    public:
       int mChangingConfigurations;
       int mShape;
       int mGradient;// = LINEAR_GRADIENT
       int mAngle;
       Orientation mOrientation;
       ColorStateList* mSolidColors;
       ColorStateList* mStrokeColors;
       std::vector<int>mGradientColors;
       std::vector<float>mPositions;
       int mStrokeWidth;
       float mStrokeDashWidth = 0.0f;
       float mStrokeDashGap = 0.0f;
       float mRadius = 0.0f;
       std::vector<float>mRadiusArray;/**/
       Rect mPadding;
       int mWidth=-1;
       int mHeight=-1;
       float mInnerRadiusRatio;
       float mThicknessRatio;
       int mInnerRadius=-1;
       int mThickness;
       bool mDither = false;
       Rect mOpticalInsets;
       float mCenterX,mCenterY;
       float mGradientRadius = 0.5f;
       int mGradientRadiusType;
       GradientType mGradientType;
       bool mUseLevel;
       bool mUseLevelForShape;
       bool mOpaqueOverBounds;
       bool mOpaqueOverShape;
       ColorStateList*mTint;
       int mDensity;
       std::vector<int>mAttrSize;
       std::vector<int>mAttrGradient;
       std::vector<int>mAttrSolid;
       std::vector<int>mAttrStroke;
       std::vector<int>mAttrCorners;
       std::vector<int>mAttrPadding;

       GradientState();
       GradientState(Orientation orientation, const std::vector<int>&gradientColors);
       GradientState(const GradientState& orig);
       void setDensity(int targetDensity);
       void applyDensityScaling(int sourceDensity, int targetDensity);
       Drawable* newDrawable()override;
	   int getChangingConfigurations()const;
       void setShape(int shape);
       void setGradientType(int gradient);
       void setGradientCenter(float x, float y);
       void setGradientColors(const std::vector<int>&colors);
       void setSolidColors(ColorStateList* colors);
       void setStroke(int width,ColorStateList* colors,float dashWidth,float dashGap);
       void setCornerRadius(float radius);
       void setCornerRadii(const std::vector<float>& radii);
       void setSize(int width, int height);
       void setGradientRadius(float gradientRadius,int type);
       void computeOpacity();
    };
    int mAlpha;
    Rect mRect;
    Rect mPadding;
    bool mPathIsDirty;
    bool mGradientIsDirty;
    bool mMutated;
    float mGradientRadius;
    std::shared_ptr<GradientState>mGradientState;
    RefPtr<Gradient>pat;
    bool ensureValidRect();
    void buildPathIfDirty();
    bool isOpaqueForState()const;
    int modulateAlpha(int alpha);
    void setStrokeInternal(int width, int color, float dashWidth, float dashGap);
    GradientDrawable(std::shared_ptr<GradientState>state);
    void updateLocalState();
protected:
    void onBoundsChange(const Rect& r)override;
    bool onLevelChange(int level)override;
    bool onStateChange(const std::vector<int>& stateSet);
public:
    GradientDrawable();
    GradientDrawable(Orientation orientation,const std::vector<int>&colors);
    static bool isOpaque(int color){return ((color>>24)&0xFF)==0xFF;}
    bool getPadding(Rect& padding);
    void setCornerRadii(const std::vector<float>& radii);
    const std::vector<float>&getCornerRadii()const;
    void setCornerRadius(float radius);
    float getCornerRadius()const;
    void setStroke(int width,int color);
    void setStroke(int width, ColorStateList* colorStateList);
    void setStroke(int width,int color, float dashWidth, float dashGap);
    void setStroke(int width, ColorStateList* colorStateList, float dashWidth, float dashGap);
    void setShape(int shape);
    int getShape()const;
    int getIntrinsicWidth()const;
    int getIntrinsicHeight()const;
    void setSize(int width, int height);
    void setGradientType(int gradient);
    int getGradientType()const;
    void setGradientCenter(float x, float y);
    float getGradientCenterX()const;
    float getGradientCenterY()const;
    void setGradientRadius(float gradientRadius);
    float getGradientRadius();
    void setUseLevel(bool useLevel);
    bool getUseLevel()const;
    Orientation getOrientation()const;
    void setOrientation(Orientation orientation);
    void setColors(const std::vector<int>& colors);
    const std::vector<int>&getColors()const;
    void setColor(int argb);
    void setColor(ColorStateList* colorStateList);
    ColorStateList* getColor();
    bool isStateful()const override;
    bool hasFocusStateSpecified()const override;
    int getChangingConfigurations()const override;
    void setAlpha(int a)override;
    int getAlpha()const override;
    void setDither(bool dither);
    int getOpacity()const;
    void getGradientCenter(float&x,float&y)const;
    std::shared_ptr<ConstantState>getConstantState()override;
    Drawable*mutate()override;
    void clearMutated()override;
    void draw(Canvas&canvas)override;
	static Drawable*inflate(Context*ctx,const AttributeSet&atts);
};

}
#endif
