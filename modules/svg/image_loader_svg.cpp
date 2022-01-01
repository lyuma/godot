/*************************************************************************/
/*  image_loader_svg.cpp                                                 */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "image_loader_svg.h"

#include "core/error/error_macros.h"
#include "core/io/image.h"
#include "core/io/image_loader.h"
#include "core/string/string_builder.h"
#include "core/templates/local_vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <memory>

#include "core/variant/variant.h"
#include "thirdparty/thorvg/inc/thorvg.h"
#include "thirdparty/thorvg/src/lib/tvgIteratorAccessor.h"

void ImageLoaderSVG::create_image_from_string(Ref<::Image> p_image, String p_string, float p_scale, bool p_upsample, bool p_convert_color) {
	ERR_FAIL_COND(Math::is_zero_approx(p_scale));
	uint32_t bgColor = 0xffffffff;
	std::unique_ptr<tvg::Picture> picture = tvg::Picture::gen();
	float fw, fh;
	if (p_convert_color) {
		::Array color_keys = replace_colors.keys();
		for (int32_t color_i = 0; color_i < color_keys.size(); color_i++) {
			Variant elem = color_keys[color_i];
			if (elem.get_type() != Variant::COLOR) {
				continue;
			}
			::Color old_color = elem;
			if (replace_colors[old_color].get_type() != Variant::COLOR) {
				continue;
			}
			::Color new_color = replace_colors[old_color];
			const String old_color_string = String("#") + old_color.to_html(false);
			const String new_color_string = String("#") + new_color.to_html(false);
			const String old_stop_color = vformat("stop-color=\"%s\"", old_color_string);
			const String new_stop_color = vformat("stop-color=\"%s\"", new_color_string);
			p_string = p_string.replace(old_stop_color, new_stop_color);
			const String old_fill_color = vformat("fill=\"%s\"", old_color_string);
			const String new_fill_color = vformat("fill=\"%s\"", new_color_string);
			p_string = p_string.replace(old_fill_color, new_fill_color);
			const String old_stroke_color = vformat("stroke=\"%s\"", old_color_string);
			const String new_stroke_color = vformat("stroke=\"%s\"", new_color_string);
			p_string = p_string.replace(old_stroke_color, new_stroke_color);
		}
	}
	PackedByteArray bytes = p_string.to_utf8_buffer();
	tvg::Result result = picture->load((const char *)bytes.ptr(), bytes.size(), "svg", true);
	if (result != tvg::Result::Success) {
		return;
	}
	picture->viewbox(nullptr, nullptr, &fw, &fh);

	uint32_t width = MIN(fw * p_scale, 16 * 1024);
	uint32_t height = MIN(fh * p_scale, 16 * 1024);
	picture->size(width, height);

	std::unique_ptr<tvg::SwCanvas> sw_canvas = tvg::SwCanvas::gen();
	uint32_t *buffer = (uint32_t *)malloc(sizeof(uint32_t) * width * height);
	tvg::Result res = sw_canvas->target(buffer, width, width, height, tvg::SwCanvas::ARGB8888_STRAIGHT);
	if (res != tvg::Result::Success) {
		ERR_FAIL_MSG("ImageLoaderSVG can't create image.");
	}

	if (bgColor != 0xffffffff) {
		uint8_t bgColorR = (uint8_t)((bgColor & 0xff0000) >> 16);
		uint8_t bgColorG = (uint8_t)((bgColor & 0x00ff00) >> 8);
		uint8_t bgColorB = (uint8_t)((bgColor & 0x0000ff));

		std::unique_ptr<tvg::Shape> shape = tvg::Shape::gen();
		shape->appendRect(0, 0, width, height, 0, 0); //x, y, w, h, rx, ry
		shape->fill(bgColorR, bgColorG, bgColorB, 255); //r, g, b, a

		if (sw_canvas->push(move(shape)) != tvg::Result::Success) {
			ERR_FAIL_MSG("ImageLoaderSVG can't create image.");
		}
	}
	res = sw_canvas->push(move(picture));
	if (res != tvg::Result::Success) {
		ERR_FAIL_MSG("ImageLoaderSVG can't create image.");
	}
	res = sw_canvas->draw();
	if (res != tvg::Result::Success) {
		ERR_FAIL_MSG("ImageLoaderSVG can't create image.");
	}
	res = sw_canvas->sync();
	if (res != tvg::Result::Success) {
		ERR_FAIL_MSG("ImageLoaderSVG can't create image.");
	}
	Vector<uint8_t> image;
	image.resize(width * height * sizeof(uint32_t));

	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			uint32_t n = buffer[y * width + x];
			image.write[sizeof(uint32_t) * width * y + sizeof(uint32_t) * x + 0] = (n >> 16) & 0xff;
			image.write[sizeof(uint32_t) * width * y + sizeof(uint32_t) * x + 1] = (n >> 8) & 0xff;
			image.write[sizeof(uint32_t) * width * y + sizeof(uint32_t) * x + 2] = n & 0xff;
			image.write[sizeof(uint32_t) * width * y + sizeof(uint32_t) * x + 3] = (n >> 24) & 0xff;
		}
	}
	res = sw_canvas->clear(true);
	free(buffer);
	p_image->create(width, height, false, Image::FORMAT_RGBA8, image);
}

void ImageLoaderSVG::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("svg");
}

Error ImageLoaderSVG::load_image(Ref<::Image> p_image, FileAccess *p_fileaccess, bool p_force_linear, float p_scale) {
	String svg = p_fileaccess->get_as_utf8_string();
	create_image_from_string(p_image, svg, p_scale, false, false);
	ERR_FAIL_COND_V(p_image->is_empty(), FAILED);
	if (p_force_linear) {
		p_image->srgb_to_linear();
	}
	return OK;
}
