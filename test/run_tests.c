#include <unistd.h>
#include <stdio.h>
#include <wchar.h>
#include <LCUI.h>
#include "test.h"
#include "run_tests.h"

int main(void)
{
	int ret = 0;
	char cwd[256];

	getcwd(cwd, 256);
	printf("cwd: %s\n", cwd);
	//Logger_SetLevel(LOGGER_LEVEL_OFF);
	// describe("test charset", test_charset);
	// describe("test linkedlist", test_linkedlist);
	// describe("test string", test_string);
	// describe("test strpool", test_strpool);
	// describe("test object", test_object);
	describe("test font load", test_font_load);
	describe("test image reader", test_image_reader);
	describe("test xml parser", test_xml_parser);
	// describe("test thread", test_thread);
	// describe("test widget event", test_widget_event);
	// describe("test widget opacity", test_widget_opacity);
	// describe("test textview resize", test_textview_resize);
	// describe("test textedit", test_textedit);
	// describe("test scrollbar", test_scrollbar);
	// describe("test mainloop", test_mainloop);
	// describe("test settings", test_settings);
	// describe("test css parser", test_css_parser);
	// describe("test block layout", test_block_layout);
	// describe("test flex layout", test_flex_layout);
	// describe("test widget rect", test_widget_rect);
	return ret - print_test_result();
}
