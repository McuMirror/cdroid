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
#ifndef __VIEW_OVERLAY_H__
#define __VIEW_OVERLAY_H__
#include <view/view.h>
#include <view/viewgroup.h>

namespace cdroid{
class ViewOverlay{
public:
    class OverlayViewGroup;
protected:
    OverlayViewGroup* mOverlayViewGroup;
public:
    ViewOverlay(Context* context, View* hostView);
    virtual ~ViewOverlay();
    ViewGroup* getOverlayView()const;
    void add(Drawable* drawable);
    void remove(Drawable* drawable);
    void clear();
    bool isEmpty()const;
};

}/*endof namespace*/
#endif/*__VIEW_OVERLAY_H__*/
