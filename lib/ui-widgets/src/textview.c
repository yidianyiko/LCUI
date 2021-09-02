﻿/*
 * textview.c -- TextView widget for display text.
 *
 * Copyright (c) 2018-2020, Liu chao <lc-soft@live.cn> All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of LCUI nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <LCUI.h>
#include "../../ui/include/ui.h"
#include <LCUI/font.h>
#include <LCUI/gui/css_parser.h>
#include <LCUI/gui/css_fontstyle.h>
#include <LCUI/gui/widget/textview.h>

#define GetData(W) ui_widget_get_data(W, self.prototype)
#define ComputeActual ui_compute_actual

typedef struct LCUI_TextViewTaskRec_ {
	wchar_t *content;
	LCUI_BOOL update_content;
} LCUI_TextViewTaskRec, *LCUI_TextViewTask;

typedef struct LCUI_TextViewRec_ {
	float available_width;
	wchar_t *content;
	LCUI_BOOL trimming;
	ui_widget_t* widget;
	LCUI_TextLayer layer;
	LCUI_CSSFontStyleRec style;
	LCUI_TextViewTaskRec task;
	LinkedListNode node;
} LCUI_TextViewRec, *LCUI_TextView;

static struct LCUI_TextViewModule {
	int key_word_break;
	LinkedList list;
	ui_widget_prototype_t prototype;
} self;

static LCUI_BOOL ParseBoolean(const char *str)
{
	if (strcmp(str, "on") == 0 && strcmp(str, "true") == 0 &&
	    strcmp(str, "yes") == 0 && strcmp(str, "1") == 0) {
		return TRUE;
	}
	return FALSE;
}

static int OnParseWordBreak(LCUI_CSSParserStyleContext ctx, const char *value)
{
	char *str = strdup2(value);
	LCUI_Style s = &ctx->sheet->sheet[self.key_word_break];
	if (s->is_valid && s->string) {
		free(s->string);
	}
	s->type = LCUI_STYPE_STRING;
	s->is_valid = TRUE;
	s->string = str;
	return 0;
}

static LCUI_WordBreakMode ComputeWordBreakMode(LCUI_StyleSheet sheet)
{
	LCUI_Style s = &sheet->sheet[self.key_word_break];
	if (s->is_valid && s->type == LCUI_STYPE_STRING && s->string) {
		if (strcmp(s->string, "break-all") == 0) {
			return LCUI_WORD_BREAK_BREAK_ALL;
		}
	}
	return LCUI_WORD_BREAK_NORMAL;
}

static void TextView_OnParseAttr(ui_widget_t* w, const char *name,
				 const char *value)
{
	LCUI_TextView txt = GetData(w);
	if (strcmp(name, "trimming") == 0) {
		if (ParseBoolean(value) != txt->trimming) {
			txt->trimming = !txt->trimming;
			TextView_SetTextW(w, txt->content);
		}
		return;
	}
	if (strcmp(name, "multiline") == 0) {
		LCUI_BOOL enable = ParseBoolean(value);
		if (enable != txt->layer->enable_mulitiline) {
			ui_textview_set_multiline(w, enable);
		}
	}
}

static void TextView_OnParseText(ui_widget_t* w, const char *text)
{
	TextView_SetText(w, text);
}

static void TextView_Update(ui_widget_t* w)
{
	float scale = ui_get_scale();

	LCUI_RectF rect;
	LCUI_TextView txt = GetData(w);

	LinkedList rects;
	LinkedListNode *node;

	LinkedList_Init(&rects);
	TextLayer_Update(txt->layer, &rects);
	TextLayer_ClearInvalidRect(txt->layer);
	for (LinkedList_Each(node, &rects)) {
		LCUIRect_ToRectF(node->data, &rect, 1.0f / scale);
		Widget_InvalidateArea(w, &rect, SV_CONTENT_BOX);
	}
	RectList_Clear(&rects);
	Widget_AddTask(w, UI_WIDGET_TASK_REFLOW);
}

static void TextView_UpdateStyle(ui_widget_t* w)
{
	LCUI_CSSFontStyleRec style;
	LCUI_TextStyleRec text_style;
	LCUI_TextView txt = GetData(w);

	CSSFontStyle_Init(&style);
	CSSFontStyle_Compute(&style, w->style);
	if (CSSFontStyle_IsEquals(&style, &txt->style)) {
		CSSFontStyle_Destroy(&style);
		return;
	}
	CSSFontStyle_GetTextStyle(&style, &text_style);
	TextLayer_SetTextAlign(txt->layer, style.text_align);
	TextLayer_SetLineHeight(txt->layer, style.line_height);
	TextLayer_SetAutoWrap(txt->layer, style.white_space != SV_NOWRAP);
	TextLayer_SetWordBreak(txt->layer, ComputeWordBreakMode(w->style));
	TextLayer_SetTextStyle(txt->layer, &text_style);
	if (style.content) {
		TextView_SetTextW(w, style.content);
	} else if (txt->style.content) {
		TextView_SetTextW(w, NULL);
	}
	CSSFontStyle_Destroy(&txt->style);
	TextStyle_Destroy(&text_style);
	txt->style = style;
	TextView_Update(w);
}

static void TextView_OnInit(ui_widget_t* w)
{
	LCUI_TextView txt;

	txt = ui_widget_add_data(w, self.prototype, sizeof(LCUI_TextViewRec));
	txt->widget = w;
	txt->available_width = 0;
	txt->task.update_content = FALSE;
	txt->task.content = NULL;
	txt->content = NULL;
	txt->trimming = TRUE;
	txt->layer = TextLayer_New();
	TextLayer_SetAutoWrap(txt->layer, TRUE);
	TextLayer_SetMultiline(txt->layer, TRUE);
	TextLayer_EnableStyleTag(txt->layer, TRUE);
	CSSFontStyle_Init(&txt->style);
	txt->node.data = txt;
	txt->node.prev = txt->node.next = NULL;
	LinkedList_AppendNode(&self.list, &txt->node);
}

static void TextView_OnDestroy(ui_widget_t* w)
{
	LCUI_TextView txt = GetData(w);

	LinkedList_Unlink(&self.list, &txt->node);
	CSSFontStyle_Destroy(&txt->style);
	TextLayer_Destroy(txt->layer);
	free(txt->content);
	if (txt->task.content) {
		free(txt->task.content);
		txt->task.content = NULL;
	}
}

static void TextView_OnAutoSize(ui_widget_t* w, float *width, float *height,
				ui_layout_rule_t rule)
{
	int max_width, max_height;
	float scale = ui_get_scale();

	LCUI_TextView txt = GetData(w);

	LinkedList rects;

	if (w->parent &&
	    w->parent->computed_style.width_sizing == UI_SIZING_RULE_FIXED) {
		txt->available_width = w->parent->box.content.width;
		max_width = (int)(scale * txt->available_width - Widget_PaddingX(w) -
				  Widget_BorderX(w));
	} else {
		txt->available_width = 0;
		max_width = 0;
	}
	switch (rule) {
	case UI_LAYOUT_RULE_FIXED_WIDTH:
		max_width = (int)(scale * w->box.content.width);
		max_height = 0;
		break;
	case UI_LAYOUT_RULE_FIXED_HEIGHT:
		max_height = (int)(scale * w->box.content.height);
		break;
	case UI_LAYOUT_RULE_FIXED:
		max_width = (int)(scale * w->box.content.width);
		max_height = (int)(scale * w->box.content.height);
		break;
	default:
		max_height = 0;
		break;
	}
	LinkedList_Init(&rects);
	TextLayer_SetFixedSize(txt->layer, 0, 0);
	TextLayer_SetMaxSize(txt->layer, max_width, max_height);
	TextLayer_Update(txt->layer, &rects);
	TextLayer_ClearInvalidRect(txt->layer);
	*width = TextLayer_GetWidth(txt->layer) / scale;
	*height = TextLayer_GetHeight(txt->layer) / scale;
	RectList_Clear(&rects);
}

static void TextView_OnResize(ui_widget_t* w, float width, float height)
{
	float scale = ui_get_scale();
	int fixed_width = (int)(width * scale);
	int fixed_height = (int)(height * scale);

	LCUI_RectF rect;
	LCUI_TextView txt = GetData(w);

	LinkedList rects;
	LinkedListNode *node;

	LinkedList_Init(&rects);
	TextLayer_SetFixedSize(txt->layer, fixed_width, fixed_height);
	TextLayer_SetMaxSize(txt->layer, fixed_width, fixed_height);
	TextLayer_Update(txt->layer, &rects);
	TextLayer_ClearInvalidRect(txt->layer);
	for (LinkedList_Each(node, &rects)) {
		LCUIRect_ToRectF(node->data, &rect, 1.0f / scale);
		Widget_InvalidateArea(w, &rect, SV_CONTENT_BOX);
	}
	RectList_Clear(&rects);
}

static void TextView_OnPaint(ui_widget_t* w, LCUI_PaintContext paint,
			     ui_widget_actual_style_t* style)
{
	LCUI_Pos pos;
	LCUI_Graph canvas;
	LCUI_Rect content_rect, rect;
	LCUI_TextView txt = GetData(w);

	content_rect.width = style->content_box.width;
	content_rect.height = style->content_box.height;
	content_rect.x = style->content_box.x - style->canvas_box.x;
	content_rect.y = style->content_box.y - style->canvas_box.y;
	if (!LCUIRect_GetOverlayRect(&content_rect, &paint->rect, &rect)) {
		return;
	}
	pos.x = content_rect.x - rect.x;
	pos.y = content_rect.y - rect.y;
	rect.x -= paint->rect.x;
	rect.y -= paint->rect.y;
	Graph_Quote(&canvas, &paint->canvas, &rect);
	rect = paint->rect;
	rect.x -= content_rect.x;
	rect.y -= content_rect.y;
	TextLayer_RenderTo(txt->layer, rect, pos, &canvas);
}

int TextView_SetTextW(ui_widget_t* w, const wchar_t *text)
{
	LCUI_TextView txt = GetData(w);
	wchar_t *newtext = malloc(wcssize(text));

	if (!newtext) {
		return -ENOMEM;
	}
	if (txt->content) {
		free(txt->content);
	}
	txt->content = malloc(wcssize(text));
	if (!txt->content) {
		free(newtext);
		return -ENOMEM;
	}
	do {
		if (!text) {
			newtext[0] = 0;
			txt->content[0] = 0;
			break;
		}
		wcscpy(txt->content, text);
		if (!txt->trimming) {
			wcscpy(newtext, text);
			break;
		}
		wcstrim(newtext, text, NULL);
	} while (0);
	if (txt->task.content) {
		free(txt->task.content);
	}
	txt->task.update_content = TRUE;
	txt->task.content = newtext;
	Widget_AddTask(w, UI_WIDGET_TASK_USER);
	return 0;
}

int TextView_SetText(ui_widget_t* w, const char *utf8_text)
{
	int ret;
	wchar_t *wstr;
	size_t len = strlen(utf8_text) + 1;

	wstr = malloc(sizeof(wchar_t) * len);
	LCUI_DecodeString(wstr, utf8_text, len, ENCODING_UTF8);
	ret = TextView_SetTextW(w, wstr);
	if (wstr) {
		free(wstr);
	}
	return ret;
}

void TextView_SetLineHeight(ui_widget_t* w, int height)
{
	Widget_SetFontStyle(w, key_line_height, (float)height, px);
}

void TextView_SetTextAlign(ui_widget_t* w, int align)
{
	Widget_SetFontStyle(w, key_text_align, align, style);
}

void TextView_SetColor(ui_widget_t* w, LCUI_Color color)
{
	Widget_SetFontStyle(w, key_color, color, color);
}

void TextView_SetAutoWrap(ui_widget_t* w, LCUI_BOOL enable)
{
	if (enable) {
		Widget_SetFontStyle(w, key_white_space, SV_AUTO, style);
	} else {
		Widget_SetFontStyle(w, key_white_space, SV_NOWRAP, style);
	}
}



void ui_textview_set_multiline(ui_widget_t* w, LCUI_BOOL enable)
{
	LCUI_TextView txt = GetData(w);

	TextLayer_SetMultiline(txt->layer, enable);
	Widget_AddTask(w, UI_WIDGET_TASK_USER);
}

static void textview_on_font_face_load(ui_widget_t *w, ui_event_t *e, void *arg)
{
	size_t count = 0;
	LCUI_TextView txt;
	LinkedListNode *node;

	for (LinkedList_Each(node, &self.list)) {
		txt = node->data;
		if (txt->widget->state != LCUI_WSTATE_DELETED) {
			Widget_UpdateStyle(txt->widget, TRUE);
		}
		count += 1;
	}
	return count;
}

static void TextVIew_OnTask(ui_widget_t* w, int task)
{
	LCUI_TextView txt;

	txt = GetData(w);
	if (txt->task.update_content) {
		TextLayer_SetTextW(txt->layer, txt->task.content, NULL);
		TextView_Update(w);
		free(txt->task.content);
		txt->task.content = NULL;
		txt->task.update_content = FALSE;
	}
	if (task != UI_WIDGET_TASK_RESIZE ||
	    w->computed_style.width_sizing != UI_SIZING_RULE_FIT_CONTENT) {
		return;
	}
	if (w->parent && w->parent->box.content.width != txt->available_width) {
		txt->available_width = w->parent->box.content.width;
		Widget_AddTask(w, UI_WIDGET_TASK_REFLOW);
	}
}

void LCUIWidget_AddTextView(void)
{
	LCUI_CSSPropertyParserRec parser = { 0, "word-break",
					     OnParseWordBreak };
	self.key_word_break = LCUI_AddCSSPropertyName("word-break");
	self.prototype = ui_create_widget_prototype("textview", NULL);
	self.prototype->init = TextView_OnInit;
	self.prototype->paint = TextView_OnPaint;
	self.prototype->destroy = TextView_OnDestroy;
	self.prototype->autosize = TextView_OnAutoSize;
	self.prototype->resize = TextView_OnResize;
	self.prototype->update = TextView_UpdateStyle;
	self.prototype->settext = TextView_OnParseText;
	self.prototype->setattr = TextView_OnParseAttr;
	self.prototype->runtask = TextVIew_OnTask;
	LCUI_AddCSSPropertyParser(&parser);
	LinkedList_Init(&self.list);
	ui_on_event("font_face_load", textview_on_font_face_load, NULL, NULL);
}

void LCUIWidget_FreeTextView(void)
{
	LinkedList_ClearData(&self.list, NULL);
	ui_off_event("font_face_load", textview_on_font_face_load, NULL, NULL);
}
