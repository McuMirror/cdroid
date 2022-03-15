#include <widget/adapter.h>
#include <cdlog.h>

namespace cdroid{

void Adapter::registerDataSetObserver(DataSetObserver observer) {
    auto it=std::find(mObservers.begin(),mObservers.end(),observer);
    if(it==mObservers.end()&&observer.onChanged)
        mObservers.push_back(observer);
}

bool operator==(const DataSetObserver&o1,const DataSetObserver&o2){
    return o1.onChanged==o2.onChanged;
}

void Adapter::unregisterDataSetObserver(DataSetObserver observer) {
    auto it=std::find(mObservers.begin(),mObservers.end(),observer);
    if(it!=mObservers.end())mObservers.erase(it);
}

void Adapter::notifyDataSetChanged(){
    int sz=mObservers.size()-1;
    for(int i=sz;i>=0;i--){
        DataSetObserver&obs=mObservers[i];
        if(obs.onChanged)
            obs.onChanged();
    }
}

void Adapter::notifyDataSetInvalidated(){
    int sz=mObservers.size()-1;
    for(int i=sz;i>=0;i--){
        DataSetObserver&obs=mObservers[i];
        if(obs.onInvalidated)
            obs.onInvalidated();
    }
}

long Adapter::getItemId(int position)const{
    return position;
}

bool Adapter::hasStableIds()const{
    return true;
}

View*Adapter::getDropDownView(int position, View* convertView, ViewGroup* parent){
    return nullptr;
}

bool Adapter::areAllItemsEnabled()const{
    return true;
}

bool Adapter::isEnabled(int position)const{
    return true;
}

int Adapter::getItemViewType(int position)const{
    return 0;
}

int Adapter::getViewTypeCount()const{
    return 1;
}

bool Adapter::isEmpty()const{
    return getCount() == 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void PagerAdapter::startUpdate(ViewGroup* container){
    startUpdate((View*)container);
}

void PagerAdapter::startUpdate(View*container){
}

void*PagerAdapter::instantiateItem(ViewGroup* container, int position){
    return instantiateItem((View*)container,position);
}

void* PagerAdapter::instantiateItem(View* container, int position){
    throw "Required method instantiateItem was not overridden";
    return nullptr;
}

void PagerAdapter::destroyItem(ViewGroup* container, int position, void* object){
    destroyItem((View*)container,position,object);
}

void PagerAdapter::destroyItem(View* container, int position,void*object){
    throw "Required method destroyItem was not overridden";
}

void PagerAdapter::finishUpdate(ViewGroup* container){
    finishUpdate((View*)container);
}
void PagerAdapter::finishUpdate(View* container) {
}

void PagerAdapter::setPrimaryItem(ViewGroup* container, int position,void*object) {
    setPrimaryItem((View*)container,position,object);
}

void PagerAdapter::setPrimaryItem(View* container, int position,void*object) {
}

void PagerAdapter::notifyDataSetChanged(){
    int sz=mObservers.size()-1;
    for(int i=sz;i>=0;i--){
        DataSetObserver& obs=mObservers[i];
        if(obs.onChanged)
            obs.onChanged();
    }
}

void PagerAdapter::registerDataSetObserver(DataSetObserver observer){
    auto it=std::find(mObservers.begin(),mObservers.end(),observer);
    if(it==mObservers.end())
        mObservers.push_back(observer);
};

void PagerAdapter::unregisterDataSetObserver(DataSetObserver observer){
    auto it=std::find(mObservers.begin(),mObservers.end(),observer);
    if(it!=mObservers.end())mObservers.erase(it);
}

void PagerAdapter::setViewPagerObserver(DataSetObserver observer){
    mViewPagerObserver = observer;
}
}//namespace
