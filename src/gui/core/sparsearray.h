/*********************************************************************************
 * Copyright (C) [2019] [houzh@msn.com]
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *********************************************************************************/
#ifndef __SPARSE_ARRAY_H__
#define __SPARSE_ARRAY_H__
#include <algorithm>
#include <vector>

namespace cdroid{

template<typename K,typename T>
class SparseArrayImpl{
private:
    std::vector<K>mKeys;
    std::vector<T>mValues;
public:
    SparseArrayImpl(){}
    size_t size()const{
        return mKeys.size();
    }
    void clear(){
        mKeys.clear();
        mValues.clear();
    }
    void put(int key, T value){
        auto itr=std::find(mKeys.begin(),mKeys.end(),key);
        if(itr!=mKeys.end()){
            mValues[itr-mKeys.begin()]=value;
	        return;
	    }
        mKeys.push_back(key);
        mValues.push_back(value);	
    }
    T get( int key,T def)const{
        auto itr=std::find(mKeys.begin(),mKeys.end(),key);
        if(itr!=mKeys.end())
            return mValues[itr-mKeys.begin()];
        return def;
    }
    T get( int key)const{
        return get(key,static_cast<T>(0));
    }
    int indexOfKey(int key)const{
        auto itr=std::find(mKeys.begin(),mKeys.end(),key);
        return itr!=mKeys.end()?int(itr-mKeys.begin()):-1;
    }
    int indexOfValue(T value)const{
        auto itr=std::find(mValues.begin(),mValues.end(),value);
        return itr!=mValues.end()?int(itr-mValues.begin()):-1;
    }
    int keyAt(size_t index)const{
        return mKeys[index];
    }

    T valueAt(size_t index)const{
        return mValues[index];
    }
    void remove(int key){
        int idx=indexOfKey(key);
        if(idx>=0){
            mKeys.erase(mKeys.begin()+idx);
	        mValues.erase(mValues.begin()+idx);
        }	
    }
    void removeAt(size_t idx){
        mKeys.erase(mKeys.begin()+idx);
        mValues.erase(mValues.begin()+idx);
    }
    void append(int key, T value) {
        if (size() && (key <= mKeys.back())) {
            put(key, value);
            return;
        }
        mKeys.push_back(key);// = GrowingArrayUtils.append(mKeys, mSize, key);
        mValues.push_back(value);// = GrowingArrayUtils.append(mValues, mSize, value);
    }
};
template<typename T>
using SparseArray = SparseArrayImpl<int,T>;

using SparseBooleanArray = SparseArrayImpl<int, bool>;
using SparseIntArray = SparseArrayImpl<int,int>;
using SparseLongArray= SparseArrayImpl<int,int64_t>;

template<typename T>
using LongSparseArray= SparseArrayImpl<int64_t,T>;

using LongSparseLongArray=LongSparseArray<long>;
}
#endif
