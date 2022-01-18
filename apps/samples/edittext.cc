#include<cdroid.h>
#include<cdlog.h>
struct TestString{
    const char*text;
    bool singleline;
    int ellipsis;
    int txtalignment;
    int gravity;
};

TestString testStrings[]={
   { 
      "Single line textView (setted by setSingleLine(true) ",
      true, 
      Layout::ELLIPSIS_NONE/*0*/,
      View::TEXT_ALIGNMENT_INHERIT/*0*/,
      Gravity::NO_GRAVITY/*0*/,
   },
   {
      "single line aliment start",
      true,
      Layout::ELLIPSIS_NONE,
      View::TEXT_ALIGNMENT_GRAVITY,
      Gravity::START
   },
   {
      "single line aliment center",
      true,
      Layout::ELLIPSIS_NONE,
      View::TEXT_ALIGNMENT_CENTER,
      Gravity::CENTER
   },
   {
      "single line aliment end",
      true,
      Layout::ELLIPSIS_NONE,
      View::TEXT_ALIGNMENT_TEXT_END,
      Gravity::END,
   },
   { 
     "Multiple lines (setted by setSingleLine(false),word break is supported by default",
      false, 
      Layout::ELLIPSIS_NONE/*0*/,
      View::TEXT_ALIGNMENT_INHERIT/*0*/,
      Gravity::NO_GRAVITY/*0*/,
   },

   { 
      "Ellipsis test ,Text with ellipsis at line start/middle/end,line must be very long,otherwise ellipsis cant be showd",
      true,
      Layout::ELLIPSIS_START,
      View::TEXT_ALIGNMENT_INHERIT/*0*/,
      Gravity::NO_GRAVITY/*0*/,
   },

   {
      "Ellipsis test ,Text with ellipsis at line start/middle/end,line must be very long,otherwise ellipsis cant be showd",
      true,
      Layout::ELLIPSIS_MIDDLE,
      View::TEXT_ALIGNMENT_INHERIT/*0*/,
      Gravity::NO_GRAVITY/*0*/,
   },

   {
      "Ellipsis test ,Text with ellipsis at line start/middle/end,line must be very long,otherwise ellipsis cant be showd",
      true,
      Layout::ELLIPSIS_END,
      View::TEXT_ALIGNMENT_INHERIT/*0*/,
      Gravity::NO_GRAVITY/*0*/,
   }
};

int main(int argc,const char*argv[]){
    App app(argc,argv);
    Window*w=new Window(100,100,800,600);

    LinearLayout*layout=new LinearLayout(LayoutParams::MATCH_PARENT,LayoutParams::MATCH_PARENT);
    layout->setOrientation(LinearLayout::VERTICAL);
    layout->setBackgroundColor(0xFF111111);
    w->addView(layout).setSize(800,600);//we recommand set size for top container(layout)

    for(int i=0;i<sizeof(testStrings)/sizeof(testStrings[0]);i++){
        TestString*ts=testStrings+i;
        LinearLayout::LayoutParams*layoutParams=new LinearLayout::LayoutParams(LayoutParams::MATCH_PARENT,LayoutParams::WRAP_CONTENT);
        layoutParams->setMargins(0,1,0,1); 
        TextView*tv=new EditText(ts->text,0,0);
        tv->setTextColor(0xFFFFFFFF);
        tv->setSingleLine(ts->singleline);
        tv->setEllipsize(ts->ellipsis);
        tv->setTextAlignment(ts->txtalignment);
        tv->setGravity(ts->gravity);
        if(ts->ellipsis==Layout::ELLIPSIS_MARQUEE)
            tv->setSelected(true);//onle focused or selected ot checked state can be marqueed
        tv->setGravity(Gravity::LEFT|Gravity::CENTER_VERTICAL);
        int cc=i*10+8;
        tv->setBackgroundColor(0xFF000000|(cc<<16)|(cc<<8)|cc);
        tv->setTextSize(22+i);
        layout->addView(tv,layoutParams);
    }

    TextView*tv=new EditText("textview with background drawable",0,0);
    tv->setBackgroundResource("cdroid:drawable/btn_default.xml");
    layout->addView(tv);

    tv=new EditText("textview with leftdrawable",0,0);
    tv->setBackgroundResource("cdroid:drawable/progress_horizontal.xml");
    tv->setCompoundDrawablesWithIntrinsicBounds("cdroid:drawable/progress_large.xml","","cdroid:drawable/progress_small.xml","");
    layout->addView(tv);
    int level=0;
    Runnable run;
    run=[tv,&level,&w,&run](){
       Drawable*dr=tv->getBackground();
       dr->setLevel(level);
       level=(level+1000)%10000;
       w->postDelayed(run,200);
    };
    w->postDelayed(run,500);

    w->requestLayout();//addView by code must call requestLayout ,auto call only used by Window::inflate.
    return app.exec();
}
