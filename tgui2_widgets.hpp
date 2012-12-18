#ifndef _WIDGETS_H
#define _WIDGETS_H

#include "tgui2.hpp"

#include <string>
#include <vector>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

enum TGUI_Direction
{
	TGUI_HORIZONTAL = 0,
	TGUI_VERTICAL
};

class TGUI_Extended_Widget : public tgui::TGUIWidget
{
public:
	TGUI_Extended_Widget(void);
	virtual ~TGUI_Extended_Widget(void) {}

protected:
	bool resizable;
};

class TGUI_Icon : public TGUI_Extended_Widget
{
public:
	bool acceptsFocus(void);
	void draw(int abs_x, int abs_y);
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	tgui::TGUIWidget *update(void);

	void setClearColor(ALLEGRO_COLOR c);

	// image gets destroyed in destructor
	TGUI_Icon(ALLEGRO_BITMAP *image, int x, int y, int flags);
	virtual ~TGUI_Icon(void);

protected:
	int flags;
	ALLEGRO_BITMAP *image;
	bool clicked;
	ALLEGRO_COLOR clear_color;
};

class TGUI_IconButton : public TGUI_Icon
{
public:
	void draw(int abs_x, int abs_y);

	// image gets destroyed in destructor
	TGUI_IconButton(ALLEGRO_BITMAP *image, int x, int y, 
		int width, int height, int ico_ofs_x, int ico_ofs_y, int flags);
	virtual ~TGUI_IconButton(void);

protected:
	int ico_ofs_x, ico_ofs_y;
};

class TGUI_Splitter : public TGUI_Extended_Widget
{
public:
	void draw(int abs_x, int abs_y);
	void keyDown(int keycode);
	void keyUp(int keycode);
	void keyChar(int keycode, int unichar);
	void mouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void mouseDownAll(TGUIWidget *leftOut, int abs_x, int abs_y, int mb);
	void mouseMoveAll(tgui::TGUIWidget *leftOut, int abs_x, int abs_y);
	void mouseMove(int rel_x, int rel_y, int abs_x, int abs_y);
	tgui::TGUIWidget *update(void);

	void set_resizable(int split, bool value);
	int get_size(int index);
	void set_size(int index, int size);
	void set_widget(int index, TGUIWidget *widget);
	void setClearColor(ALLEGRO_COLOR c);
	void layout(void);
	std::vector<tgui::TGUIWidget *> &getWidgets(void);
	void setDrawLines(bool drawLines);
	void setWidth(int w);
	void setHeight(int h);

	TGUI_Splitter(
		int x, int y,
		int w, int h,
		TGUI_Direction dir,
		bool can_resize,
		std::vector<tgui::TGUIWidget *> widgets
	);

	virtual ~TGUI_Splitter(void);

protected:
	class TGUI_DummyWidget : public TGUI_Extended_Widget
	{
	public:
		TGUI_DummyWidget(int x, int y) {
			this->x = x;
			this->y = y;
		}
		virtual ~TGUI_DummyWidget(void) {}
	};

	TGUI_Direction direction;
	bool weighted_resize;

	std::vector<tgui::TGUIWidget *> widgets;
	std::vector<TGUI_DummyWidget *> dummies;
	std::vector<int> sizes;
	std::vector<float> section_resize_weight;
	std::vector<bool> section_is_resizable; // 1 less then widgets.size()
	ALLEGRO_COLOR clear_color;
	int resizing;
	int last_resize_x, last_resize_y;
	bool drawLines;
};

class TGUI_MenuBar;

class TGUI_TextMenuItem : public TGUI_Extended_Widget
{
public:
	static const int HEIGHT = 16;

	virtual void close(void) {};
	void setMenuBar(TGUI_MenuBar *menuBar);
	int getShortcutKeycode(void);

	virtual void draw(int abs_x, int abs_y);
	virtual tgui::TGUIWidget *update(void);
	virtual void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	virtual void mouseMove(int rel_x, int rel_y, int abs_x, int abs_y);
	virtual void mouseMoveAll(tgui::TGUIWidget *leftOut, int abs_x, int abs_y);

	TGUI_TextMenuItem(std::string name, int shortcut_keycode);
	virtual ~TGUI_TextMenuItem(void) {}

protected:
	std::string name;
	int shortcut_keycode;
	bool clicked;
	bool hover;
	TGUI_MenuBar *menuBar;
};

class TGUI_CheckMenuItem : public TGUI_TextMenuItem
{
public:
	void draw(int abs_x, int abs_y);
	tgui::TGUIWidget *update(void);

	bool isChecked(void);
	void setChecked(bool checked);
	
	TGUI_CheckMenuItem(std::string name, int shortcut_keycode, bool checked);
	virtual ~TGUI_CheckMenuItem(void) {}

protected:
	bool checked;
};

class TGUI_RadioMenuItem : public TGUI_TextMenuItem
{
public:
	struct RadioGroup {
		int selected;
	};

	void draw(int abs_x, int abs_y);
	tgui::TGUIWidget *update(void);

	bool isSelected(void);
	void setSelected(void);

	TGUI_RadioMenuItem(std::string name, int shortcut_keycode, RadioGroup *group, int id);
	virtual ~TGUI_RadioMenuItem(void) {}

protected:
	RadioGroup *group;
	int id;
};

class TGUI_SubMenuItem : public TGUI_TextMenuItem
{
public:
	void close(void);

	void draw(int abs_x, int abs_y);
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void mouseMove(int rel_x, int rel_y, int abs_x, int abs_y);

	bool isOpen(void);
	TGUI_Splitter *getSubMenu(void);
	void setSubMenu(TGUI_Splitter *sub);
	void setParentSplitter(TGUI_Splitter *splitter);

	TGUI_SubMenuItem(std::string name, TGUI_Splitter *sub_menu);
	virtual ~TGUI_SubMenuItem(void) {}

protected:
	bool is_open;
	TGUI_Splitter *sub_menu;
	TGUI_Splitter *parentSplitter;
};

class TGUI_MenuBar : public TGUI_Extended_Widget
{
public:
	static const int PADDING = 10;
	static const int HEIGHT = 16;
	
	void close(void);

	void draw(int abs_x, int abs_y);
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void keyDown(int keycode);
	tgui::TGUIWidget *update(void);

	TGUI_MenuBar(
		int x, int y, int w, int h,
		std::vector<std::string> menu_names,
		std::vector<TGUI_Splitter *> menus
	);
protected:
	void setSubMenuSplitters(TGUI_Splitter *root);
	void checkKeys(int keycode, TGUI_Splitter *splitter);

	std::vector<std::string> menu_names;
	std::vector<TGUI_Splitter *> menus;
	TGUI_Splitter *open_menu;
	bool close_menu;
	tgui::TGUIWidget *itemToReturn;
};

class TGUI_ScrollPane : public TGUI_Extended_Widget
{
public:
	static const int SCROLLBAR_THICKNESS = 16;
	static const int MIN_SCROLLBAR_SIZE = 16;
	
	void draw(int abs_x, int abs_y);
	void keyDown(int keycode);
	void keyUp(int keycode);
	void keyChar(int keycode, int unichar);
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void mouseMove(int rel_x, int rel_y, int abs_x, int abs_y);
	void mouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void mouseMoveAll(TGUIWidget *leftOut, int abs_x, int abs_y);
	void chainDraw(void);
	TGUIWidget *chainMouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);

	void get_values(float *ox, float *oy);
	void setValues(float ox, float oy);
	void get_pixel_offsets(int *xx, int *yy);

	TGUI_ScrollPane(tgui::TGUIWidget *child);

protected:
	enum Down_Type {
		DOWN_V,
		DOWN_H,
		DOWN_NONE
	};

	int get_scrollbar_size(int content_size, int scrollbar_size);
	int get_scrollbar_range(int content_size, int scrollbar_size);
	void get_vtab_details(int *x1, int *y1, int *x2, int *y2);
	void get_htab_details(int *x1, int *y1, int *x2, int *y2);

	float ox, oy;
	
	Down_Type down;
	int down_x, down_y;
	float down_ox, down_oy;
};

class TGUI_Slider : public TGUI_Extended_Widget
{
public:
	static const int TAB_SIZE = 8;

	virtual void draw(int abs_x, int abs_y);
	virtual void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	virtual void mouseMoveAll(tgui::TGUIWidget *leftOut, int abs_x, int abs_y);
	virtual void mouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int mb);

	float getPosition(void);
	void setPosition(float pos);

	TGUI_Slider(int x, int y, int size, TGUI_Direction direction);

protected:
	int size;
	TGUI_Direction direction;
	float pos;
	bool dragging;
};

class TGUI_Button : public TGUI_Icon
{
public:
	void draw(int abs_x, int abs_y);

	TGUI_Button(std::string text, int x, int y, int w, int h);

protected:
	std::string text;
};

class TGUI_TextField : public TGUI_Extended_Widget
{
public:
	static const int PADDING = 3;

	bool acceptsFocus(void);
	void draw(int abs_x, int abs_y);
	void keyChar(int keycode, int unichar);

	void setValidator(bool (*validate)(const std::string str));
	bool isValid(void);
	std::string getText(void);
	void setText(std::string s);

	TGUI_TextField(std::string startStr, int x, int y, int width);
	virtual ~TGUI_TextField(void);

protected:
	void findOffset(void);

	std::string str;
	int cursorPos;
	int offset;
	bool (*validate)(const std::string str);
};

// only need modal frame right now so this one won't be draggable (yet)
class TGUI_Frame : public TGUI_Extended_Widget
{
public:
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void mouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	void mouseMove(int rel_x, int rel_y, int abs_x, int abs_y);
	void draw(int abs_x, int abs_y);

	int barHeight(void);

	TGUI_Frame(std::string title, int x, int y, int width, int height);
	virtual ~TGUI_Frame(void);

protected:

	std::string title;
	bool dragging;
	int drag_x, drag_y;
};

class TGUI_Label : public TGUI_Extended_Widget
{
public:
	void draw(int abs_x, int abs_y);

	TGUI_Label(std::string text, ALLEGRO_COLOR color, int x, int y, int flags);
	virtual ~TGUI_Label(void);

protected:

	std::string text;
	ALLEGRO_COLOR color;
	int flags;
};

class TGUI_List : public TGUI_Extended_Widget
{
public:
	void draw(int abs_x, int abs_y);

	const std::vector<std::string> &getLabels();
	void setLabels(const std::vector<std::string> &labels);
	void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);

	int getSelected(void) { return selected; }
	void setSelected(int selected) { this->selected = selected; }

	TGUI_List(int x, int y, int width);
	virtual ~TGUI_List();

protected:

	std::vector<std::string> labels;
	int selected;
};

#endif // _WIDGETS_H
