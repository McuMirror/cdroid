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
#include <drawable/colorfilters.h>
#include <porting/cdlog.h>
#include <core/color.h>
namespace cdroid{

ColorMatrixColorFilter::ColorMatrixColorFilter(const float(&v)[20]){
    mCM.set(v);
}

void ColorMatrixColorFilter::apply(Canvas&canvas,const Rect&rect){
    Cairo::ImageSurface*img=dynamic_cast<Cairo::ImageSurface*>(canvas.get_target().get());
    uint8_t *data=img->get_data();
    /*RGB��Alpha����ֵ���㷽�����£�
    Redͨ����ֵ= a[0] * srcR + a[1] * srcG + a[2] * srcB + a[3] * srcA + a[4]
    Greenͨ����ֵ= a[5] * srcR + a[6] * srcG + a[7] * srcB + a[8] * srcA + a[9]
    Blueͨ����ֵ= a[10] * srcR + a[11] * srcG + a[12] * srcB + a[13] * srcA + a[14]
    Alphaͨ����ֵ= a[15]*srcR+a[16]*srcG + a[17] * srcB + a[18] * srcA + a[19]*/
}

PorterDuffColorFilter::PorterDuffColorFilter(int color,int mode){
    mColor=color;
    mMode=mode;
}

void PorterDuffColorFilter::apply(Canvas&canvas,const Rect&rect){
    canvas.set_operator((Cairo::Context::Operator)PorterDuff::toOperator(mMode));//2,5(6,7),8,9
    canvas.set_color(mColor);
    canvas.paint();
}
void PorterDuffColorFilter::setColor(int c){
    mColor=c;
}

int PorterDuffColorFilter::getColor()const{
    return mColor;
}

void PorterDuffColorFilter::setMode(int m){
    mMode=m;
}

int PorterDuffColorFilter::getMode()const{
    return mMode;
}

LightingColorFilter::LightingColorFilter(int mul,int add){
    mMul=mul;
    mAdd=add;
}

void LightingColorFilter::apply(Canvas&canvas,const Rect&rect){
    // 在这里，假设 cr->source() 已经是你通过 push_group 和 pop_group_to_source 设置好的图像源
    Cairo::RefPtr<Cairo::Pattern> pattern = canvas.get_source();
    if(pattern->get_type() != Cairo::Pattern::Type::SURFACE){
        LOGE("LightingColorFilter only supports surface pattern as source");
        return;
    }
    Cairo::RefPtr<Cairo::SurfacePattern> surface_pattern = std::dynamic_pointer_cast<Cairo::SurfacePattern>(pattern);
    Cairo::RefPtr<Cairo::ImageSurface> surface = std::dynamic_pointer_cast<Cairo::ImageSurface>(surface_pattern->get_surface());
    auto temp_mul_surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, surface->get_width(), surface->get_height());
    auto temp_mul_cr = Cairo::Context::create(temp_mul_surface);

    // 使用 mMul 颜色填充临时表面
    Color cmul(mMul);
    temp_mul_cr->set_source_rgba(cmul.red(), cmul.green(), cmul.blue(),cmul.alpha());
    temp_mul_cr->paint(); // Fill entire temporary surface with mMul color

    // 创建临时表面的图案模式
    auto mul_pattern = Cairo::SurfacePattern::create(temp_mul_surface);

    // 创建临时表面用于加法层
    auto temp_add_surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, surface->get_width(), surface->get_height());
    auto temp_add_cr = Cairo::Context::create(temp_add_surface);

    // 使用 mAdd 颜色填充临时表面
    Color cadd(mAdd);
    temp_add_cr->set_source_rgba(cadd.red(), cadd.green(), cadd.blue(),cadd.alpha());
    temp_add_cr->paint(); // Fill entire temporary surface with mAdd color

    // 创建临时表面的图案模式
    auto add_pattern = Cairo::SurfacePattern::create(temp_add_surface);

    // 应用乘法层。
    // Cairo::Context::Operator 枚举中没有 MULTIPLY，使用 PorterDuff helper 或者
    // 直接 cast CAIRO_OPERATOR_MULTIPLY
    canvas.save();
    canvas.set_operator((Cairo::Context::Operator)PorterDuff::toOperator(PorterDuff::MULTIPLY));
    canvas.set_source(mul_pattern);
    canvas.paint();

    // 应用加法层。
    // Operator::ADD 存在，但我们还是通过 PorterDuff 统一转换，以便于以后扩展。
    canvas.set_operator((Cairo::Context::Operator)PorterDuff::toOperator(PorterDuff::ADD));
    canvas.set_source(add_pattern);
    canvas.paint();

    canvas.restore();
}

}

