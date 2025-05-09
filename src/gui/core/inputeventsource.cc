#include <core/inputeventsource.h>
#include <core/windowmanager.h>
#include <core/systemclock.h>
#include <porting/cdlog.h>
#include <unordered_map>
#include <thread>
#include <chrono>

namespace cdroid{
InputEventSource::InputEventSource(){
    InputInit();
    mScreenSaveTimeOut = -1;
    mRunning = true;
    mIsPlayback = false;
    mIsScreenSaveActived = false;
    mLastPlaybackEventTime = SystemClock::uptimeMillis();
    mLastInputEventTime = mLastPlaybackEventTime;
    auto func = std::bind(&InputEventSource::doEventsConsume,this);
    std::thread th(func);
    th.detach();
    LOGD("%p",this);
}

void InputEventSource::doEventsConsume(){
    INPUTEVENT es[128];
    while(mRunning){
        const int count = InputGetEvents(es,sizeof(es)/sizeof(INPUTEVENT),20);
        std::lock_guard<std::mutex> lock(mtxEvents);
        if(count)mLastInputEventTime = SystemClock::uptimeMillis();
        LOGV_IF(count,"rcv %d rawEvents",count);
        for(int i = 0 ; i < count ; i ++){
            const INPUTEVENT*e = es+i;
            auto it = mDevices.find(e->device);
            if(es[i].type >= EV_ADD){
                onDeviceChanged(es+i);
                continue;
            }
            if(it==mDevices.end()){
                getDevice(es->device)->putEvent(e->tv_sec,e->tv_usec,e->type,e->code,e->value);
                continue;
            }
            it->second->putEvent(e->tv_sec,e->tv_usec,e->type,e->code,e->value);
        }
    }
}

InputEventSource::~InputEventSource(){
    mRunning = false;
    Looper::getMainLooper()->removeEventHandler(this);
    if(frecord.is_open())
       frecord.close();
    LOGD("%p Destroied",this);
}

void InputEventSource::onDeviceChanged(const INPUTEVENT*es){
    auto itr = mDevices.find(es->device);
    std::shared_ptr<InputDevice>dev = nullptr;
    switch(es->type){
    case EV_ADD:/*noting todo*/
        dev = getDevice(es->device);
        LOGI("device %s %d is added",(dev?dev->getName().c_str():""),es->device);
        break;
    case EV_REMOVE:
        if(itr!=mDevices.end())dev = itr->second;
        LOGI_IF(dev,"device %s:%d/%d is removed", dev->getName().c_str(), es->device,dev->getId());
        LOGI_IF(dev==nullptr,"remove unknwon dev %d",es->device);
        if(itr!=mDevices.end())mDevices.erase(itr);
        break;
    default:
        LOGI("dev %d unknown event %d",es->device,es->type);
        break;
    }
}

std::unique_ptr<InputEventSource>InputEventSource::mInst;
InputEventSource& InputEventSource::getInstance(){
    if(mInst==nullptr)
        mInst = std::unique_ptr<InputEventSource>(new InputEventSource());
    return *mInst;
}

void InputEventSource::setScreenSaver(ScreenSaver func,int timeout){
    if(mScreenSaver)mScreenSaver(false);
    mScreenSaver = func;
    mScreenSaveTimeOut = timeout;
}

std::shared_ptr<InputDevice>InputEventSource::getDevice(int fd){
    std::shared_ptr<InputDevice>dev;
    auto itr = mDevices.find(fd);
    if(itr == mDevices.end()){
        InputDevice tmpdev(fd);
        if(tmpdev.getClasses()&(INPUT_DEVICE_CLASS_TOUCH|INPUT_DEVICE_CLASS_TOUCH_MT)){
            dev.reset(new MouseDevice(fd));
        }else if(tmpdev.getClasses()&(INPUT_DEVICE_CLASS_KEYBOARD)){
            dev.reset(new KeyDevice(fd));
        }else dev.reset(new InputDevice(fd));
        mDevices.emplace(fd,dev);
        return dev;
    }
    return itr->second;
}

bool InputEventSource::needCancel(InputDevice*dev){
    int32_t action;
    nsecs_t etime;
    const nsecs_t now = SystemClock::uptimeMillis();
    dev->getLastEvent(action,etime);
    if( (action!=MotionEvent::ACTION_CANCEL) && (now - etime>2000) ){
        MotionEvent*e = MotionEvent::obtain(now, now, MotionEvent::ACTION_CANCEL, 0, 0, 0);
        dev->pushEvent(e);
    }
    return false;
}

int InputEventSource::checkEvents(){
    std::lock_guard<std::mutex> lock(mtxEvents);
    const nsecs_t now = SystemClock::uptimeMillis();
    int count = 0;
    for(auto dev:mDevices){
        const int devEvents= dev.second->getEventCount();
        count += devEvents;
        if(devEvents==0)
            needCancel(dev.second.get());
    }
    if( ((now - mLastInputEventTime) > mScreenSaveTimeOut) && (mScreenSaveTimeOut>0)
            && ( mIsScreenSaveActived == false ) && mScreenSaver){
        mScreenSaver(true);
        mIsScreenSaveActived = true;
    }
    if(count && mIsScreenSaveActived){
        if(mScreenSaver)
            mScreenSaver(false);
        mIsScreenSaveActived= false;
        mLastInputEventTime = now;
    }
    return count;
}

void InputEventSource::openScreenSaver(){
    mIsScreenSaveActived = true;
    mLastInputEventTime = SystemClock::uptimeMillis();
}

void InputEventSource::closeScreenSaver(){
    if(mIsScreenSaveActived){
        mIsScreenSaveActived = false;
        if(mScreenSaver)mScreenSaver(false);
    }
    mLastInputEventTime = SystemClock::uptimeMillis();
}

bool InputEventSource::isScreenSaverActived()const{
    return mIsScreenSaveActived;
}

int InputEventSource::handleEvents(){
    std::lock_guard<std::mutex> lock(mtxEvents);
    int ret = 0;
    for(auto it:mDevices){
        auto dev = it.second;
        if(dev->getEventCount()==0)continue;
        ret += dev->getEventCount();
        if(dev->getClasses()&(INPUT_DEVICE_CLASS_TOUCH|INPUT_DEVICE_CLASS_TOUCH_MT)){
            while(dev->getEventCount()){
               MotionEvent*e = (MotionEvent*)dev->popEvent();
               WindowManager::getInstance().processEvent(*e);
               e->recycle();
            }
        }else if(dev->getClasses()&INPUT_DEVICE_CLASS_KEYBOARD){
            while(dev->getEventCount()){
                KeyEvent*e = (KeyEvent*)dev->popEvent();
                WindowManager::getInstance().processEvent(*e);
                e->recycle();
            }
        }else{
            while(dev->getEventCount()){
                InputEvent*e=dev->popEvent();
                e->recycle();
            }
        }
    }
    return ret;
}

void InputEventSource::sendEvent(InputEvent&event){
    WindowManager::getInstance().processEvent(event);
}

void InputEventSource::recordEvent(InputEvent&inputEvent){
    std::ostringstream oss;
    const int type = inputEvent.getType();
    //MotionEvent::ACTION_DOWN==KeyEvent::ACTION_DOWN,MotionEvent::ACTION_UP==KeyEvent::ACTION_UP,
    //So MotionEvent::ACTION_DOWN/UP must be mapped from 0,1 ... -> 4,5 ... to make them diffrent
    if(type==InputEvent::INPUT_EVENT_TYPE_MOTION){
        MotionEvent& m = (MotionEvent&)inputEvent;
        if(m.getPointerCount())
            oss << m.getActionIndex()<<",";
        oss<<(4 + m.getAction())<<","<<m.getX(0)<<","<<m.getY()<<","<<m.getEventTime()<<std::endl;
    }else if(type==InputEvent::INPUT_EVENT_TYPE_KEY){
        KeyEvent& k = (KeyEvent&)inputEvent;
        oss<<k.getAction()<<","<<k.getLabel()<<","<<k.getEventTime()<<std::endl;
    }
}

InputEvent*InputEventSource::parseEvent(const char*line){
    const char*tokens[16]={0};
    const char*p = line;
    int tokenCount = 0;
    while( p && ( tokenCount < sizeof(tokens)/sizeof(tokens[0]) )){
        tokens[tokenCount++] = p;
        p = strchr(p+1,',');
    }
    if(tokenCount<4){/*KeyEvent*/
        const int keyCode = atoi(tokens[1]);
        const nsecs_t etime = atoi(tokens[2]);
        const int action = atoi(line);
        KeyEvent* k = KeyEvent::obtain(etime,etime,action,keyCode,
            0/*repeat*/, 0/*metaState*/, 0/*deviceId*/, keyCode/*scancode*/,
            0/*flags*/, 0/*source*/,0/*displayid*/);
        return k;
    }else {/*(tokenCount>=4)MotionEvent*/
        int tkIdx = tokenCount>4?1:0;
        const int pointer =(tokenCount>4) ? atoi(tokens[0]) : 0;
        const int action = atoi(tokens[tkIdx]);
        const int x = atoi(tokens[tkIdx+1]);
        const int y = atoi(tokens[tkIdx+2]);
        const nsecs_t etime = atoll(tokens[tkIdx+3]);
        MotionEvent*m = MotionEvent::obtain(etime,etime,
            action|(pointer<<MotionEvent::ACTION_POINTER_INDEX_SHIFT),
            float(x),float(y), 0/*pressure*/, 0/*size*/, 0/*metaState*/,
            0,0/*x/yPrecision*/,0/*deviceId*/,0/*edgeFlags*/);
        return m;
    }
    return nullptr;
}

void InputEventSource::record(const std::string&fname){
    frecord.open(fname);
}

void InputEventSource::playback(const std::string&fname){
    auto func = [this](const std::string&fname){
         std::fstream in(fname);
         std::this_thread::sleep_for(std::chrono::milliseconds(10000));
         LOGD_IF(in.good(),"play key from %s",fname.c_str());
         if(!in.good())return ;
         mIsPlayback = true;
         while(1){
             char line[256] = { 0 };
             in.getline(line,255);
             InputEvent*e = parseEvent(line);
             if(e){
                 e->recycle();
             }
             if(in.gcount() == 0){
                 in.close();
                 in.open(fname);
             }
         }
    };
    if(!fname.empty()){
        std::thread th(func,fname);
        th.detach();
    }
}
}//end namespace

