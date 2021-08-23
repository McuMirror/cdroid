#include <windows.h>
#include <cdlog.h>

class MyAdapter:public ArrayAdapter<std::string>{
private:
    int itemType;
public:
    MyAdapter(int type=0):ArrayAdapter(){
        itemType=type;
    }
    View*getView(int position, View* convertView, ViewGroup* parent)override{

        TextView*tv=(TextView*)convertView;
        if(convertView==nullptr){
            if(itemType==0) tv=new TextView("",600,20);
            else tv=new CheckBox("",600,20);
            tv->setPadding(20,0,0,0);
            tv->setFocusable(false);
        }
        if(itemType==1)tv->setLayoutDirection(position<10?View::LAYOUT_DIRECTION_RTL:View::LAYOUT_DIRECTION_LTR);
        tv->setId(position);
        tv->setText("position :"+std::to_string(position));
        tv->setTextColor(0xFFFFFFFF);
        tv->setBackgroundColor(0x80002222);
        tv->setTextSize(40);
        return tv;
    }
};

int main(int argc,const char*argv[]){
    App app;
    Window*w=new Window(100,50,1200,620);
    MyAdapter*adapter=new MyAdapter(0);

    ListView*lv=(ListView*)&w->addView(new ListView(460,500));
    lv->setPos(10,10);
    for(int i=0;i<56;i++){
        adapter->add("");
    }
    lv->setAdapter(adapter);
    adapter->notifyDataSetChanged();
    lv->setVerticalScrollBarEnabled(true);    
    lv->setOverScrollMode(View::OVER_SCROLL_ALWAYS);
    lv->setSmoothScrollbarEnabled(true);
    lv->setSelector(new ColorDrawable(0x8800FF00));
    //lv->setSelection(2);
    lv->setDivider(new ColorDrawable(0x80224422));
    lv->setDividerHeight(1);
    lv->setOnItemClickListener([](AdapterView&lv,View&v,int pos,long id){
        LOGD("clicked %d",pos);
    });
////////////////////////////////////////////////////////////////////////////////////////
    MyAdapter*adapter2=new MyAdapter(1);
    ListView*lv2=(ListView*)&w->addView(new ListView(500,500));
    ToggleButton *toggle=new ToggleButton(300,40);
    w->addView(toggle).setPos(500,520);
    lv2->setPos(500,10);
    lv2->setAdapter(adapter2);
    for(int i=0;i<56;i++){
        adapter2->add("");
    }
    lv2->setDivider(new ColorDrawable(0x80224422));
    lv2->setDividerHeight(1);
    lv2->setVerticalScrollBarEnabled(true);
    lv2->setSmoothScrollbarEnabled(true);
    lv2->setOverScrollMode(View::OVER_SCROLL_ALWAYS);
    lv2->setAdapter(adapter2);
    lv2->setSelector(new ColorDrawable(0x88FF0000));
    lv2->setChoiceMode(ListView::CHOICE_MODE_SINGLE);//MULTIPLE);
    adapter2->notifyDataSetChanged();
    lv2->setOnItemClickListener([&](AdapterView&lv,View&v,int pos,long id){
        LOGD("clicked %d",pos);
        lv2->setItemChecked(pos,((CheckBox&)v).isChecked());//lv2->isItemChecked(pos));
    });
    lv2->setMultiChoiceModeListener([&](int position, long id, bool checked){
        LOGD("multichoice %d checked=%d",position,checked);
        //lv2->setItemChecked(position,checked);
    });
    toggle->setTextOn("SingleChoice");
    toggle->setTextOff("MultiChoice");
    toggle->setBackgroundResource("cdroid:drawable/btn_toggle_bg.xml");
    toggle->setOnCheckedChangeListener([&](CompoundButton&view,bool check){
        lv2->setChoiceMode(check?ListView::CHOICE_MODE_SINGLE:ListView::CHOICE_MODE_MULTIPLE);
    });
    app.exec();
    return 0;
};
