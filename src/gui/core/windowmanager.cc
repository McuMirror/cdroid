/*
 * Copyright (C) 2015 UI project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <windowmanager.h>
#include <ngl_msgq.h>
#include <ngl_os.h>
#include <cdlog.h>
#include <ngl_timer.h>
#include <graphdevice.h>
#include <uieventsource.h>

using namespace Cairo;
namespace cdroid {
// Initialize the instance of the singleton to nullptr
WindowManager* WindowManager::mInst = nullptr;

WindowManager::WindowManager(){
    mActiveWindow = nullptr;
    mDisplayRotation =0;
}

WindowManager&WindowManager::getInstance(){
    if(nullptr==mInst){
        mInst = new WindowManager();
    }
    return *mInst;
};

WindowManager::~WindowManager() {
    for(Window*w:mWindows){
        View::AttachInfo*info = w->mAttachInfo;
        w->dispatchDetachedFromWindow();
        delete info;
        delete w;
    };
    mWindows.clear();
    LOGD("%p Destroied",this);
}

void WindowManager::setDisplayRotation(int rot){
    mDisplayRotation = rot;
    LOGD("rotation = %d",rot);
}

int WindowManager::getDisplayRotation()const{
    return mDisplayRotation;
}

Display& WindowManager::getDefaultDisplay(){
    if(mDisplays.size()==0){
	size_t dc = GFXGetDisplayCount();
	for(size_t i = 0;i < dc ;i++){
	    DisplayInfo info;
	    info.rotation = mDisplayRotation;
	    GFXGetDisplaySize(i,(UINT*)&info.logicalWidth,(UINT*)&info.logicalHeight);
	    Display d(i,info);
	    mDisplays.push_back(d);
	}
    }
    return mDisplays.at(Display::DEFAULT_DISPLAY); 
}

void WindowManager::addWindow(Window*win){
    mWindows.push_back(win);
    win->mLayer = (win->window_type<<16)|0x7FFF;
    std::sort(mWindows.begin(),mWindows.end(),[](Window*w1,Window*w2){
        return (w2->mLayer - w1->mLayer)>0;
    });

    for(int idx = 0 ;idx < mWindows.size();idx++){
        Window *w = mWindows.at(idx);
        w->mLayer = (w->window_type<<16)|(idx+1);
        LOGV("%p window %p[%s] type=%d layer=%d",win,w,w->getText().c_str(),w->window_type,w->mLayer);
    }
    if(mActiveWindow){
        Window*deactWin = mActiveWindow;
        mActiveWindow->post([deactWin](){deactWin->onDeactive();});
        mActiveWindow->mAttachInfo->mTreeObserver->dispatchOnWindowFocusChange(false);
    }

    View::AttachInfo*info = new View::AttachInfo(win->getContext());
    info->mContentInsets.setEmpty();
    info->mRootView = win;
    info->mEventSource=win->mUIEventHandler;
    win->dispatchAttachedToWindow(info,win->getVisibility());
#if USE_UIEVENTHANDLER    
    Looper::getMainLooper()->addHandler(win->mUIEventHandler);
#else
    Looper::getMainLooper()->addEventHandler(win->mUIEventHandler);
#endif
    win->post([win](){win->onCreate();});
    //the first create only call onCreate,no onActive 
    win->post([info](){
        info->mTreeObserver->dispatchOnWindowFocusChange(true);
    });
    mActiveWindow = win;
    win->invalidate();
    LOGV("win=%p Handler=%p windows.size=%d",win,win->mUIEventHandler,mWindows.size());
}

void WindowManager::removeWindow(Window*w){
    if(w == mActiveWindow){
        mActiveWindow = nullptr;
        w->mAttachInfo->mTreeObserver->dispatchOnWindowFocusChange(false);
    }
    if(w->hasFlag(View::FOCUSABLE)){
        w->dispatchWindowFocusChanged(false);
        w->onDeactive();
    }
    auto itw = std::find(mWindows.begin(),mWindows.end(),w);
    const Rect wrect = w->getBound();
    mWindows.erase(itw);
    for(auto itr=mWindows.begin();itr!=mWindows.end();itr++){
        Window*w1 = (*itr);
        Rect rc = w1->getBound();
        rc.intersect(wrect);
        rc.offset(-w1->getLeft(),-w1->getTop());
        w1->mPendingRgn->do_union({rc.left,rc.top,rc.width,rc.height});
    }
#if USE_UIEVENTHANDLER
    Looper::getMainLooper()->removeHandler(w->mUIEventHandler);
#else
    Looper::getMainLooper()->removeEventHandler(w->mUIEventHandler);
#endif
    View::AttachInfo*info = w->mAttachInfo;
    w->dispatchDetachedFromWindow();
    delete info;
    delete w;
    for(auto it=mWindows.rbegin();it!=mWindows.rend();it++){
        if((*it)->hasFlag(View::FOCUSABLE)&&(*it)->getVisibility()==View::VISIBLE){
            if((*it)!=mActiveWindow){
                 (*it)->dispatchWindowFocusChanged(true);
                 (*it)->onActive();
            }
            mActiveWindow = (*it);
            break;
        } 
    }
    GraphDevice::getInstance().invalidate(wrect);
    GraphDevice::getInstance().flip();
    LOGI("w=%p windows.size=%d",w,mWindows.size());
}

void WindowManager::removeWindows(const std::vector<Window*>&ws){
    Cairo::RefPtr<Cairo::Region>rgn=Cairo::Region::create();
    for(auto w:ws){
        if(w == mActiveWindow){
            mActiveWindow = nullptr;
            w->mAttachInfo->mTreeObserver->dispatchOnWindowFocusChange(false);
        }
        if(w->hasFlag(View::FOCUSABLE)){
            w->dispatchWindowFocusChanged(false);
            w->onDeactive();
        }
        auto itw = std::find(mWindows.begin(),mWindows.end(),w);
        const Rect rw = w->getBound();
        mWindows.erase(itw);
        rgn->do_union({rw.left,rw.top,rw.width,rw.height});
        for(auto itr=mWindows.begin();itr!=mWindows.end();itr++){
            Window*w1 = (*itr);
            Rect rc = w1->getBound();
            rc.intersect(rw);
            rc.offset(-w1->getLeft(),-w1->getTop());
            w1->invalidate(&rc);
            w1->mPendingRgn->do_union({rc.left,rc.top,rc.width,rc.height});
        }
    #if USE_UIEVENTHANDLER
        Looper::getMainLooper()->removeHandler(w->mUIEventHandler);
    #else
        Looper::getMainLooper()->removeEventHandler(w->mUIEventHandler);
    #endif
        View::AttachInfo*info = w->mAttachInfo;
        w->dispatchDetachedFromWindow();
        delete info;
        delete w;
    }
    for(auto it=mWindows.rbegin();it!=mWindows.rend();it++){
        if((*it)->hasFlag(View::FOCUSABLE)&&(*it)->getVisibility()==View::VISIBLE){
            if((*it)!=mActiveWindow){
                (*it)->dispatchWindowFocusChanged(true);
                (*it)->onActive();
            }
            mActiveWindow = (*it);
            break;
        }
    }
    const Cairo::RectangleInt re = rgn->get_extents();
    GraphDevice::getInstance().invalidate({re.x,re.y,re.width,re.height});
    GraphDevice::getInstance().flip();
}

void WindowManager::moveWindow(Window*w,int x,int y){
    Rect rcw = w->getBound();
    Rect rcw2 =rcw;
    rcw2.left = x;
    rcw2.top = y;
    w->setFrame(x,y,rcw.width,rcw.height);
    const auto itw = std::find(mWindows.begin(),mWindows.end(),w);
    if( w->isAttachedToWindow() && (w->getVisibility()==View::VISIBLE)){
        GraphDevice::getInstance().invalidate(rcw);
        GraphDevice::getInstance().invalidate(rcw2);
        for(auto it = mWindows.begin();it<itw;it++){
           Rect rc = w->getBound();
           RefPtr<Region>newrgn = Region::create((RectangleInt&)rc);
           for( auto it2 = it+1 ; it2 < itw ; it2++){
               Rect r = (*it)->getBound();
               newrgn->subtract((const RectangleInt&)r);
           }
           newrgn->translate(-rcw.left,-rcw.top);
           (*it)->mPendingRgn->do_union((RectangleInt&)rcw);
           (*it)->mPendingRgn->subtract((RectangleInt&)rcw2);
        }
        GraphDevice::getInstance().flip();
    }
}

Window*WindowManager::getActiveWindow()const{
    return mActiveWindow;
}

void WindowManager::sendToBack(Window*win){
    win->mLayer = (win->window_type<<16);
    if(mActiveWindow==win) return;
    std::sort(mWindows.begin(),mWindows.end(),[](Window*w1,Window*w2){
        return (w2->mLayer - w1->mLayer)>0;
    });

    for(int idx = 0 ; idx < mWindows.size();idx++){
        Window *w = mWindows.at(idx);
        w->mLayer = (w->window_type<<16)|(idx+1);
    }
    mActiveWindow = mWindows.back();
    win->mPendingRgn->do_union({0,0,win->getWidth(),win->getHeight()});
    win->post([win](){win->onDeactive();});
    Window*newActWin = mActiveWindow;
    mActiveWindow->post([newActWin](){
        newActWin->onActive();
    });
    GraphDevice::getInstance().flip();
}

void WindowManager::bringToFront(Window*win){
    win->mLayer = (win->window_type<<16)|0x7FFF;
    if(mActiveWindow==win) return;
    std::sort(mWindows.begin(),mWindows.end(),[](Window*w1,Window*w2){
        return (w2->mLayer - w1->mLayer)>0;
    });

    for(int idx = 0 ; idx < mWindows.size();idx++){
        Window *w = mWindows.at(idx);
        w->mLayer = (w->window_type<<16)|(idx+1);
    }
    win->post([win](){win->onActive();});

    Window*deactWin= mActiveWindow;
    mActiveWindow->post([deactWin](){
        deactWin->onDeactive();
    });
    mActiveWindow = win;
    win->mPendingRgn->do_union({0,0,win->getWidth(),win->getHeight()});
    GraphDevice::getInstance().flip();
}

int WindowManager::enumWindows(WNDENUMPROC cbk){
    int rc = 0;
    for(auto& w:mWindows)
       rc+=cbk(w);
    return rc;
}

int WindowManager::getWindows(std::vector<Window*>&wins){
    wins = mWindows;
    return mWindows.size();
}

int WindowManager::getVisibleWindows(std::vector<Window*>&wins){
    for(auto& w:mWindows){
        if(w->getVisibility()==View::VISIBLE)
           wins.push_back(w);
    }
    return wins.size();
}

void WindowManager::processEvent(InputEvent&e){
   switch(e.getType()){
   case InputEvent::INPUT_EVENT_TYPE_KEY: onKeyEvent((KeyEvent&)e); break;
   case InputEvent::INPUT_EVENT_TYPE_MOTION: onMotion((MotionEvent&)e);break;
   default:break;
   }
}


void WindowManager::onMotion(MotionEvent&event) {
   // Notify the focused child
   const int x = event.getX();
   const int y = event.getY();
   const int action = event.getActionMasked();
   for (auto itr = mWindows.rbegin();itr != mWindows.rend();itr++) {
       auto w = (*itr);
       ViewTreeObserver*obv = w->getViewTreeObserver();
       if(action == MotionEvent::ACTION_DOWN){
           w->mAttachInfo->mInTouchMode=true;
           obv->dispatchOnTouchModeChanged(true);
       }else if(action == MotionEvent::ACTION_UP){
           w->mAttachInfo->mInTouchMode=false;
           obv->dispatchOnTouchModeChanged(false);
       }
       LOGV_IF(action!=MotionEvent::ACTION_MOVE,"%s at(%d,%d)",MotionEvent::actionToString(action).c_str(),x,y);
       if ((w->getVisibility()==View::VISIBLE) && w->getBound().contains(x,y)) {
           event.offsetLocation(-w->getLeft(),-w->getTop());
           w->dispatchTouchEvent(event);
           event.offsetLocation(w->getLeft(),w->getTop());
           break;
       }
   }
}

void WindowManager::onKeyEvent(KeyEvent&event) {
    // Notify the focused child
    for (auto itr = mWindows.rbegin() ;itr != mWindows.rend();itr++) {
        Window*win = (*itr);
        if ( win->hasFlag(View::FOCUSABLE) && (win->getVisibility()==View::VISIBLE) ) {
            int keyCode = event.getKeyCode();
            LOGV("Window:%p Key:%s[%x] action=%d",win,event.getLabel(),keyCode,event.getAction());
            win->processKeyEvent(event);
            //dispatchKeyEvent(event);
            return;
        }
  }
}

void WindowManager::clip(Window*win){
    RECT rcw = win->getBound();
    for (auto wind = mWindows.rbegin() ;wind != mWindows.rend();wind++){
        if( (*wind)==win )break;
        if( (*wind)->getVisibility()!=View::VISIBLE)continue;
        Rect rc = rcw;
        rc.intersect((*wind)->getBound());
        if(rc.empty())continue;
        rc.offset(-win->getX(),-win->getY());
        win->mInvalidRgn->subtract((const RectangleInt&)rc); 
    }
}

}  // namespace ui
