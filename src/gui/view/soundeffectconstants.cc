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
#include <view/soundeffectconstants.h>
#include <view/view.h>
namespace cdroid{

int SoundEffectConstants::sLastNavigationRepeatSoundEffectId = -1;

int SoundEffectConstants::getContantForFocusDirection(int direction) {
    switch (direction) {
    case View::FOCUS_RIGHT:    return SoundEffectConstants::NAVIGATION_RIGHT;
    case View::FOCUS_FORWARD:
    case View::FOCUS_DOWN:     return SoundEffectConstants::NAVIGATION_DOWN;
    case View::FOCUS_LEFT:     return SoundEffectConstants::NAVIGATION_LEFT;
    case View::FOCUS_BACKWARD:
    case View::FOCUS_UP:       return SoundEffectConstants::NAVIGATION_UP;
    default:
    throw "direction must be one of {FOCUS_UP, FOCUS_DOWN, FOCUS_LEFT, FOCUS_RIGHT, FOCUS_FORWARD, FOCUS_BACKWARD}.";
    }
    return SoundEffectConstants::NAVIGATION_DOWN;//NOT REACHED
}

int SoundEffectConstants::getConstantForFocusDirection(int direction, bool repeating){
    if (repeating) {
        switch (direction) {
        case View::FOCUS_RIGHT:
              return SoundEffectConstants::NAVIGATION_REPEAT_RIGHT;
	case View::FOCUS_FORWARD:
	case View::FOCUS_DOWN:
             return SoundEffectConstants::NAVIGATION_REPEAT_DOWN;
	case View::FOCUS_LEFT:
             return SoundEffectConstants::NAVIGATION_REPEAT_LEFT;
	case View::FOCUS_BACKWARD:
	case View::FOCUS_UP:
             return SoundEffectConstants::NAVIGATION_REPEAT_UP;
        }
        LOGE("direction must be one of {FOCUS_UP, FOCUS_DOWN,"
               "FOCUS_LEFT, FOCUS_RIGHT, FOCUS_FORWARD, FOCUS_BACKWARD}.");
    }
    return getContantForFocusDirection(direction);
}

bool SoundEffectConstants::isNavigationRepeat(int effectId){
     return effectId == SoundEffectConstants::NAVIGATION_REPEAT_DOWN
                || effectId == SoundEffectConstants::NAVIGATION_REPEAT_LEFT
                || effectId == SoundEffectConstants::NAVIGATION_REPEAT_RIGHT
                || effectId == SoundEffectConstants::NAVIGATION_REPEAT_UP;
}

int SoundEffectConstants::nextNavigationRepeatSoundEffectId(){
    int next = 0;//NAVIGATION_REPEAT_RANDOMIZER.nextInt(AudioManager.NUM_NAVIGATION_REPEAT_SOUND_EFFECTS - 1);
    if (next >= sLastNavigationRepeatSoundEffectId) {
        next++;
    }
    sLastNavigationRepeatSoundEffectId = next;
    return next;//AudioManager.getNthNavigationRepeatSoundEffect(next);
}
}//namespace

