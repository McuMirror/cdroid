#ifndef __UIBASE_H__
#define __UIBASE_H__
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    private: TypeName(const TypeName&);    \
    private: TypeName& operator=(const TypeName&)
namespace cdroid{

#define _MIN(a,b) ((a)>(b)?(b):(a))
#define _MAX(a,b) ((a)<(b)?(b):(a))

template<typename T>
struct CPoint{
    T x;
    T y;
    void set(T _x,T _y){x=_x;y=_y;};
    static constexpr CPoint Make(T x,T y){return {x,y};}
};
typedef CPoint< int > Point,POINT;
typedef CPoint<float> PointF;
#define MAKEPOINT(x,y) POINT::Make(x,y)

template<typename T>
struct CSize{
    T x;
    T y;
    void set(T x_,T y_){x=x_;y=y_;}
    T width()const{return x;}
    T height()const{return y;}
    static constexpr CSize Make(T x,T y){return {x,y};}
    static constexpr CSize MakeEmpty(){return {0,0};}
};
typedef CSize< int > Size,SIZE;
typedef CSize<float> SizeF;
#define MAKESIZE(x,y) SIZE::Make(x,y)

template<typename T>
struct CRect{
    T left;
    T top;
    T width;
    T height;

    T bottom()const{return top+height;}
    T right()const{return left+width;}
    T centerX()const{return left+width/2;}
    T centerY()const{return top+height/2;}

    void set(T x,T y,T w,T h){
        left= x;
        top =y;
        width =w;
        height=h;
    }
	
    void inflate(T dx,T dy){
        left-=dx;
        top -=dy;
        width+=(dx+dx);
        height+=(dy+dy);
    }
	
    void inset(T dx,T dy){
        inflate(-dx,-dy);
    }

    bool empty()const{
        return width<=0||height<=0;
    }

    void setEmpty(){set(0,0,0,0);}

    void offset(int dx,int dy){
        left+= dx;
        top += dy;
    }
 
    bool intersect(const CRect&a,const CRect&b){
        T x1 = _MAX (a.left, b.left);
        T y1 = _MAX (a.top , b.top);
        T x2 = _MIN (a.left + a.width, b.left + b.width);
        T y2 = _MIN (a.top +  a.height, b.top + b.height);

        if (x1 >= x2 || y1 >= y2) {
            left = 0;
            top = 0;
            width  = 0;
            height = 0;
            return false;
        } else {
            left = x1;
            top = y1;
            width  = x2 - x1;
            height = y2 - y1;
            return true;
        }
        return true;
    }

    bool intersect(const CRect&b){
        return intersect(*this,b);
    }

    bool intersect(T l,T t,T w,T h){
        return intersect(*this,Make(l,t,w,h));
    }

    bool contains(T x,T y)const{
        return x>=left&&x<right() && y>=top&&y<bottom();
    }

    bool contains(const CRect&a)const{
        if(a.empty()||empty())return false;
        if((a.right()<=this->left)||(this->right()<=a.left)||(a.bottom()<=this->top)||(this->bottom()<a.top))
            return false;
        return true;
    }

    bool operator==(const CRect&b)const{
        return (left==b.left)&&(top==b.top)&&(width==b.width)&&(height==b.height);
    }

    bool operator!=(const CRect&b)const{
        return (left!=b.left)||(top!=b.top)||(width!=b.width)||(height!=b.height);
    }

    void Union(const CRect&b){
        if(empty()){
            set(b.left,b.top,b.width,b.height);
        }else{
            T x1 = std::min (left, b.left);
            T y1 = std::min (top, b.top);
            T newWidth = std::max (b.left + b.width,  left + width) - x1;
            T newHeight = std::max (b.top +  b.height, top + height) - y1;
            left = x1;
            top = y1;
            width  = newWidth;
            height = newHeight;
        }
    }
    void Union(T px,T py){
        const T new_x = std::min(left, px);
        const T new_y = std::min(top, py);
        const T new_w = std::max(left + width, px) - new_x;
        const T new_h = std::max(top + height, py) - new_y;

        left = new_x;
        top = new_y;
        width = new_w;
        height = new_h;
    }
    void Union(T x,T y,T w,T h){
        Union(Make(x,y,w,h));
    }
    static constexpr CRect Make(T x,T y,T w,T h){return CRect{x,y,w,h};}
    static constexpr CRect MakeEmpty(){return CRect{0,0,0,0};}
    static constexpr CRect MakeWH(T w,T h){return CRect{0,0,w,h};}
    static constexpr CRect MakeLTRB(T l,T t,T r,T b){return CRect{l,t,r-l,b-t};}
    static constexpr CRect MakeSize(const CSize<T>&sz){return CRect{0,0,sz.x,sz.y};}
};

typedef CRect<int> Rect,RECT;
typedef CRect<float>RectF;

#define MAKERECT(x,y,w,h) RECT::Make(x,y,w,h)

}
#endif

