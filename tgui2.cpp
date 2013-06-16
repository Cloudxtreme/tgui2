#include "tgui2.hpp"

#include <allegro5/allegro_primitives.h>

#include <cstdio>
#include <cmath>

namespace tgui {

	struct TGUI {
		std::vector<TGUIWidget*> widgets;
	};

}

static void drawRect(tgui::TGUI *gui, int x1, int y1, int x2, int y2);

namespace tgui {

static TGUIWidget *getWidgetInDirection(TGUIWidget *widget, int xdir, int ydir);

static ALLEGRO_DISPLAY *display;

static std::vector<TGUI*> stack;
static std::vector<TGUIWidget *> stackFocus;
static double lastUpdate;
static TGUIWidget* currentParent = 0;

static int screenWidth = 0;
static int screenHeight = 0;

static float x_scale = 1;
static float y_scale = 1;
static float x_offset = 0;
static float y_offset = 0;

static std::vector<TGUIWidget *> focusOrderList;
static bool focusWrap = false;
static TGUIWidget *focussedWidget;

static ALLEGRO_FONT *font;

static bool clipSet = false;

static std::vector<TGUIWidget *> preDrawWidgets;
static std::vector<TGUIWidget *> postDrawWidgets;

struct TGUIEventSource {
	ALLEGRO_EVENT_SOURCE event_source;
	TGUIEventType type;
	TGUIWidget *widget;
};

static TGUIEventSource eventSource;

static bool keyState[ALLEGRO_KEY_MAX] = { 0, };

static int screenSizeOverrideX = -1;
static int screenSizeOverrideY = -1;

inline bool checkBoxCollision(int x1, int y1, int x2, int y2, int x3, int y3,
	int x4, int y4)
{
	if ((y2 < y3) || (y1 > y4) || (x2 < x3) || (x1 > x4))
		return false;
	return true;
}

void setFont(ALLEGRO_FONT *f)
{
	font = f;
}

ALLEGRO_FONT *getFont(void)
{
	return font;
}

long currentTimeMillis()
{
#ifndef ALLEGRO4
	return (long)(al_get_time() * 1000);
#else
#ifndef ALLEGRO_WINDOWS
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#else
	return timeGetTime();
#endif
#endif
}

static void deleteGUI(TGUI *gui)
{
	for (size_t i = 0; i < gui->widgets.size(); i++) {
		delete gui->widgets[i];
	}
	delete gui;
}
	
static void deletestack(void)
{
	while (stack.size() > 0) {
		deleteGUI(stack[0]);
		stack.erase(stack.begin());
	}
	stackFocus.clear();
}

void init(ALLEGRO_DISPLAY *d)
{
	display = d;

	deletestack();

	TGUI *gui = new TGUI;

	stack.push_back(gui);
	stackFocus.push_back(NULL);

	lastUpdate = currentTimeMillis();

	getScreenSize(&screenWidth, &screenHeight);

	al_init_user_event_source((ALLEGRO_EVENT_SOURCE *)&eventSource);
}

void shutdown()
{
	deletestack();
	preDrawWidgets.clear();
	postDrawWidgets.clear();
}

void setFocus(TGUIWidget *widget)
{
	if (widget == NULL || widget->acceptsFocus()) {
		focussedWidget = widget;
	}
}

TGUIWidget *getFocussedWidget(void)
{
	return focussedWidget;
}

void setFocusOrder(std::vector<TGUIWidget *> list)
{
	focusOrderList = list;
}

void focusPrevious(void)
{
	TGUIWidget *focussed = getFocussedWidget();

	if (focussed == NULL)
		return;

	for (size_t i = 0; i < focusOrderList.size(); i++) {
		if (focusOrderList[i] == focussed) {
			if (i > 0) {
				setFocus(focusOrderList[i-1]);
			}
			else {
				return;
			}
		}
	}
}

void focusNext(void)
{
	TGUIWidget *focussed = getFocussedWidget();

	if (focussed == NULL)
		return;

	for (size_t i = 0; i < focusOrderList.size(); i++) {
		if (focusOrderList[i] == focussed) {
			if (i < focusOrderList.size()-1) {
				setFocus(focusOrderList[i+1]);
			}
			else {
				return;
			}
		}
	}
}

void translateAll(int x, int y)
{
	TGUI *gui = stack[0];

	for (size_t i = 0; i < gui->widgets.size(); i++) {
		TGUIWidget *w = gui->widgets[i];
		w->translate(x, y);
	}
}

void addWidget(TGUIWidget* widget)
{
	if (!widget->getParent())
		widget->setParent(currentParent);
	if (currentParent) {
		currentParent->setChild(widget);
	}
	stack[0]->widgets.push_back(widget);
}


TGUIWidget *update()
{
	long currTime = currentTimeMillis();
	long elapsed = currTime - lastUpdate;
	if (elapsed > 50) {
		elapsed = 50;
	}
	lastUpdate = currTime;

	for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
		TGUIWidget *widget = stack[0]->widgets[i];
		TGUIWidget *retVal = widget->update();
		if (retVal) {
			return retVal;
		}
	}

	return NULL;
}

std::vector<TGUIWidget *> updateAll(void)
{
	std::vector<TGUIWidget *> retVect;
	long currTime = currentTimeMillis();
	long elapsed = currTime - lastUpdate;

	if (elapsed > 50) {
		elapsed = 50;
	}

	lastUpdate = currTime;

	for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
		TGUIWidget *widget = stack[0]->widgets[i];
		TGUIWidget *retVal = widget->update();
		if (retVal) {
			retVect.push_back(retVal);
		}
	}

	return retVect;
}

void draw()
{
	int abs_x, abs_y;

	for (size_t i = 0; i < preDrawWidgets.size(); i++) {
		determineAbsolutePosition(preDrawWidgets[i], &abs_x, &abs_y);
		preDrawWidgets[i]->preDraw(abs_x, abs_y);
	}

	drawRect(0, 0, screenWidth, screenHeight);

	// Draw focus
	if (focussedWidget && focussedWidget->getDrawFocus()) {
		int x = focussedWidget->getX();
		int y = focussedWidget->getY();
		int w = focussedWidget->getWidth();
		int h = focussedWidget->getHeight();
		drawFocusRectangle(x, y, w, h);
	}
	
	for (size_t i = 0; i < postDrawWidgets.size(); i++) {
		determineAbsolutePosition(postDrawWidgets[i], &abs_x, &abs_y);
		postDrawWidgets[i]->postDraw(abs_x, abs_y);
	}
}

void drawRect(int x1, int y1, int x2, int y2)
{
	for (int i = stack.size()-1; i >= 0; i--) {
		::drawRect(stack[i], x1, y1, x2, y2);
	}
}

void push()
{
	TGUI *gui = new TGUI;

	stack.insert(stack.begin(), gui);
	stackFocus.insert(stackFocus.begin(), getFocussedWidget());

	setFocus(NULL);
}

bool pop()
{
	if (stack.size() <= 0)
		return false;

	deleteGUI(stack[0]);
	stack.erase(stack.begin());

	setFocus(stackFocus[0]);
	stackFocus.erase(stackFocus.begin());

	return true;
}

void setNewWidgetParent(TGUIWidget* parent)
{
	currentParent = parent;
}

TGUIWidget *getNewWidgetParent(void)
{
	return currentParent;
}

void centerWidget(TGUIWidget* widget, int x, int y)
{
	if (x >= 0) {
		widget->setX(x - (int)(widget->getWidth() / 2));
	}
	if (y >= 0) {
		widget->setY(y - (int)(widget->getHeight() / 2));
	}
}

void setScale(float xscale, float yscale)
{
	x_scale = xscale;
	y_scale = yscale;
}

void setOffset(float xoffset, float yoffset)
{
	x_offset = xoffset;
	y_offset = yoffset;
}

void convertMousePosition(int *x, int *y)
{
	*x -= x_offset;
	*y -= y_offset;

	*x = *x / x_scale;
	*y = *y / y_scale;
}

void bufferToScreenPos(int *x, int *y, int bw, int bh)
{
	*x = *x * x_scale + x_offset;
	*y = *y * y_scale + y_offset;
}

void setFocusWrap(bool wrap)
{
	focusWrap = wrap;
}

void maybe_make_mouse_event(ALLEGRO_EVENT *event)
{
	if (event->type == ALLEGRO_EVENT_TOUCH_BEGIN ||
			event->type == ALLEGRO_EVENT_TOUCH_END ||
			event->type == ALLEGRO_EVENT_TOUCH_MOVE) {
		if (event->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
			event->type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
		}
		else if (event->type == ALLEGRO_EVENT_TOUCH_END) {
			event->type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;
		}
		else if (event->type == ALLEGRO_EVENT_TOUCH_MOVE) {
			event->type = ALLEGRO_EVENT_MOUSE_AXES;
		}
		event->mouse.x = event->touch.x;
		event->mouse.y = event->touch.y;
		event->mouse.z = 0;
		event->mouse.w = 0;
		event->mouse.button = event->touch.id;
	}
}

void handleEvent_pretransformed(void *allegro_event)
{
	ALLEGRO_EVENT *event = (ALLEGRO_EVENT *)allegro_event;
	
	maybe_make_mouse_event(event);

	switch (event->type) {
		case ALLEGRO_EVENT_MOUSE_AXES: {
			int mx = event->mouse.x;
			int my = event->mouse.y;
			int mz = event->mouse.z;
			int mw = event->mouse.w;
			TGUIWidget *w = determineTopLevelOwner(mx, my);
			if (w) {
				int rel_x;
				int rel_y;
				int abs_x, abs_y;
				determineAbsolutePosition(w, &abs_x, &abs_y);
				rel_x = mx - abs_x;
				rel_y = my - abs_y;
				TGUIWidget *leftOut = w->chainMouseMove(rel_x, rel_y, mx, my, mz, mw);
				for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
					TGUIWidget *w2 = stack[0]->widgets[i];
					if (w2->getParent() == NULL) {
						w2->mouseMoveAll(leftOut, mx, my);
					}
				}
			}
			else {
				for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
					stack[0]->widgets[i]->mouseMove(-1, -1, mx, my);
				}
			}
			break;
		}
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
		case ALLEGRO_EVENT_MOUSE_BUTTON_UP: {
			bool down = event->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
			int mx = event->mouse.x;
			int my = event->mouse.y;
			TGUIWidget *w = determineTopLevelOwner(mx, my);
			if (w) {
				int rel_x;
				int rel_y;
				int abs_x, abs_y;
				determineAbsolutePosition(w, &abs_x, &abs_y);
				rel_x = mx - abs_x;
				rel_y = my - abs_y;
				if (down) {
					TGUIWidget *leftOut = w->chainMouseDown(rel_x, rel_y, mx, my, event->mouse.button);
					setFocus(leftOut);
					for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
						TGUIWidget *w2 = stack[0]->widgets[i];
						if (w2->getParent() == NULL) {
							w2->mouseDownAll(leftOut, mx, my, event->mouse.button);
						}
					}
				}
				else {
					TGUIWidget *leftOut = w->chainMouseUp(rel_x, rel_y, mx, my, event->mouse.button);
					for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
						TGUIWidget *w2 = stack[0]->widgets[i];
						if (w2->getParent() == NULL) {
							w2->mouseUpAll(leftOut, mx, my, event->mouse.button);
						}
					}
				}
			}
			else {
				for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
					if (down)
						stack[0]->widgets[i]->mouseDown(-1, -1, mx, my, event->mouse.button);
					else
						stack[0]->widgets[i]->mouseUp(-1, -1, mx, my, event->mouse.button);
				}
			}
			break;
		}
		case ALLEGRO_EVENT_KEY_DOWN: {
			keyState[event->keyboard.keycode] = true;
			for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
				if (stack[0]->widgets[i]->getParent() == NULL) {
					stack[0]->widgets[i]->chainKeyDown(event->keyboard.keycode);
				}
			}
			break;
		}
		case ALLEGRO_EVENT_KEY_UP: {
			keyState[event->keyboard.keycode] = false;
			for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
				if (stack[0]->widgets[i]->getParent() == NULL) {
					stack[0]->widgets[i]->chainKeyUp(event->keyboard.keycode);
				}
			}
			break;
		}
		case ALLEGRO_EVENT_KEY_CHAR: {
			bool used = false;
			for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
				if (stack[0]->widgets[i]->getParent() == NULL) {
					used = used || stack[0]->widgets[i]->chainKeyChar(event->keyboard.keycode, event->keyboard.unichar);
				}
			}
			if (!used) {
				if (focussedWidget && event->keyboard.keycode == ALLEGRO_KEY_LEFT) {
					TGUIWidget *w = getWidgetInDirection(focussedWidget, -1, 0);
					if (w) {
						setFocus(w);
					}
				}
				else if (focussedWidget && event->keyboard.keycode == ALLEGRO_KEY_RIGHT) {
					TGUIWidget *w = getWidgetInDirection(focussedWidget, 1, 0);
					if (w) {
						setFocus(w);
					}
				}
				else if (focussedWidget && event->keyboard.keycode == ALLEGRO_KEY_UP) {
					TGUIWidget *w = getWidgetInDirection(focussedWidget, 0, -1);
					if (w) {
						setFocus(w);
					}
				}
				else if (focussedWidget && event->keyboard.keycode == ALLEGRO_KEY_DOWN) {
					TGUIWidget *w = getWidgetInDirection(focussedWidget, 0, 1);
					if (w) {
						setFocus(w);
					}
				}
			}
			break;
		}
		case ALLEGRO_EVENT_JOYSTICK_AXIS: {
			int stick = event->joystick.stick;
			int axis = event->joystick.axis;
			float value = event->joystick.pos;
			bool used = false;
			for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
				if (stack[0]->widgets[i]->getParent() == NULL) {
					used = used || stack[0]->widgets[i]->chainJoyAxis(stick, axis, value);
				}
			}
			if (!used) {
				if (axis == 0) {
					if (value < -0.5) {
						TGUIWidget *w = getWidgetInDirection(focussedWidget, -1, 0);
						if (w) {
							setFocus(w);
						}
					}
					else if (value > 0.5) {
						TGUIWidget *w = getWidgetInDirection(focussedWidget, 1, 0);
						if (w) {
							setFocus(w);
						}
					}
				}
				else {
					if (value < -0.5) {
						TGUIWidget *w = getWidgetInDirection(focussedWidget, 0, -1);
						if (w) {
							setFocus(w);
						}
					}
					else if (value > 0.5) {
						TGUIWidget *w = getWidgetInDirection(focussedWidget, 0, 1);
						if (w) {
							setFocus(w);
						}
					}
				}
			}
			break;
		}
		case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN: {
			for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
				if (stack[0]->widgets[i]->getParent() == NULL) {
					stack[0]->widgets[i]->chainJoyButtonDown(event->joystick.button);
				}
			}
			break;
		}
		case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP: {
			for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
				if (stack[0]->widgets[i]->getParent() == NULL) {
					stack[0]->widgets[i]->chainJoyButtonUp(event->joystick.button);
				}
			}
			break;
		}
	}
}

void handleEvent(void *allegro_event)
{
	ALLEGRO_EVENT *ev = (ALLEGRO_EVENT *)allegro_event;
	ALLEGRO_EVENT event = *ev;

#ifdef ALLEGRO_IPHONE
	maybe_make_mouse_event(&event);
#endif

	if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP ||
	    event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN ||
	    event.type == ALLEGRO_EVENT_MOUSE_AXES) {
		ALLEGRO_EVENT ev;
		memcpy(&ev, &event, sizeof(ALLEGRO_EVENT));
		convertMousePosition(&ev.mouse.x, &ev.mouse.y);
		handleEvent_pretransformed(&ev);
	}
	else {
		handleEvent_pretransformed(&event);
	}
}

TGUIWidget *getTopLevelParent(TGUIWidget *widget)
{
	TGUIWidget *p = widget;
	while (p->getParent()) {
		p = p->getParent();
		if (p->getParent() == NULL) {
			return p;
		}
	}

	return NULL;
}

TGUIWidget *determineTopLevelOwner(int x, int y)
{
	if (stack[0]->widgets.size() <= 0)
		return NULL;

	for (int i = (int)stack[0]->widgets.size()-1; i >= 0; i--) {
		TGUIWidget *widget = stack[0]->widgets[i];
		if (pointOnWidget(widget, x, y)) {
			TGUIWidget *p = widget;
			while (p->getParent()) {
				p = p->getParent();
			}
			return p;
		}
	}
	return NULL;
}

void determineAbsolutePosition(TGUIWidget *widget, int *x, int *y)
{
	int wx = widget->getX();
	int wy = widget->getY();
	TGUIWidget *parent = widget->getParent();
	while (parent) {
		wx += parent->getX();
		wy += parent->getY();
		parent = parent->getParent();
	}
	*x = wx;
	*y = wy;
}

bool pointOnWidget(TGUIWidget *widget, int x, int y)
{
	int wx, wy;
	determineAbsolutePosition(widget, &wx, &wy);
	if (x >= wx && y >= wy && x < wx+widget->getWidth() && y < wy+widget->getHeight()) {
		return true;
	}
	return false;
}

TGUIWidget *TGUIWidget::chainMouseMove(int rel_x, int rel_y, int abs_x, int abs_y, int z, int w)
{
	bool used = false;
	TGUIWidget *ret = NULL;

	if (child) {
		// pass it on to the child
		if (pointOnWidget(child, abs_x, abs_y)) {
			int wx, wy;
			determineAbsolutePosition(child, &wx, &wy);
			rel_x = abs_x - wx;
			rel_y = abs_y - wy;
			ret = child->chainMouseMove(
				rel_x,
				rel_y,
				abs_x,
				abs_y,
				z,
				w
			);
			used = true;
		}
	}

	if (!used) {
		// handle it within ourself
		if (pointOnWidget(this, abs_x, abs_y)) {
			mouseMove(rel_x, rel_y, abs_x, abs_y);
			mouseScroll(z, w);
			ret = this;
		}
	}

	return ret;
}

TGUIWidget *TGUIWidget::chainMouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb)
{
	bool used = false;
	TGUIWidget *ret = NULL;

	if (child) {
		// pass it on to the child
		if (pointOnWidget(child, abs_x, abs_y)) {
			int wx, wy;
			determineAbsolutePosition(child, &wx, &wy);
			rel_x = abs_x - wx;
			rel_y = abs_y - wy;
			ret = child->chainMouseDown(
				rel_x,
				rel_y,
				abs_x,
				abs_y,
				mb
			);
			used = true;
		}
	}

	if (!used) {
		// handle it within outself
		if (pointOnWidget(this, abs_x, abs_y)) {
			mouseDown(rel_x, rel_y, abs_x, abs_y, mb);
			ret = this;
		}
	}

	return ret;
}

TGUIWidget *TGUIWidget::chainMouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int mb)
{
	bool used = false;
	TGUIWidget *ret = NULL;

	if (child) {
		// pass it on to the child
		if (pointOnWidget(child, abs_x, abs_y)) {
			int wx, wy;
			determineAbsolutePosition(child, &wx, &wy);
			rel_x = abs_x - wx;
			rel_y = abs_y - wy;
			ret = child->chainMouseUp(
				rel_x,
				rel_y,
				abs_x,
				abs_y,
				mb
			);
			used = true;
		}
	}

	if (!used) {
		// handle it within outself
		if (pointOnWidget(this, abs_x, abs_y)) {
			mouseUp(rel_x, rel_y, abs_x, abs_y, mb);
			ret = this;
		}
	}

	return ret;
}

void TGUIWidget::chainKeyDown(int keycode)
{
	// handle it within outself
	keyDown(keycode);

	// pass it on to the child
	if (child) {
		child->chainKeyDown(
			keycode
		);
	}
}

void TGUIWidget::chainKeyUp(int keycode)
{
	// handle it within outself
	keyUp(keycode);

	// pass it on to the child
	if (child) {
		child->chainKeyUp(
			keycode
		);
	}
}

bool TGUIWidget::chainKeyChar(int keycode, int unichar)
{
	// handle it within outself
	bool used = keyChar(keycode, unichar);

	// pass it on to the child
	if (child) {
		used = used || child->chainKeyChar(keycode, unichar);
	}

	return used;
}

void TGUIWidget::chainJoyButtonDown(int button)
{
	// handle it within outself
	joyButtonDown(button);

	// pass it on to the child
	if (child) {
		child->chainJoyButtonDown(
			button
		);
	}
}

void TGUIWidget::chainJoyButtonUp(int button)
{
	// handle it within outself
	joyButtonUp(button);

	// pass it on to the child
	if (child) {
		child->chainJoyButtonUp(
			button
		);
	}
}

bool TGUIWidget::chainJoyAxis(int stick, int axis, float value)
{
	// handle it within outself
	bool used = joyAxis(stick, axis, value);

	// pass it on to the child
	if (child) {
		used = used || child->chainJoyAxis(stick, axis, value);
	}

	return used;
}

void TGUIWidget::chainDraw(void)
{
	int abs_x, abs_y;
	determineAbsolutePosition(this, &abs_x, &abs_y);
	draw(abs_x, abs_y);

	if (child) {
		child->chainDraw();
	}
}

void setScreenSize(int w, int h)
{
	screenSizeOverrideX = w;
	screenSizeOverrideY = h;
}

void getScreenSize(int *w, int *h)
{
	if (screenSizeOverrideX > 0) {
		*w = screenSizeOverrideX;
		*h = screenSizeOverrideY;
		return;
	}

	if (al_get_current_display()) {
		ALLEGRO_BITMAP *bb = al_get_backbuffer(al_get_current_display());

		screenWidth = al_get_bitmap_width(bb);
		screenHeight = al_get_bitmap_height(bb);
	}
	else {
		screenWidth = -1;
		screenHeight = -1;
	}
	if (w) {
		*w = screenWidth;
	}
	if (h) {
		*h = screenHeight;
	}
}

void resize(TGUIWidget *parent)
{
	getScreenSize(NULL, NULL);
	clearClip();

	if (parent) {
		parent->resize();
	}
	else {
		for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
			if (parent == stack[0]->widgets[i]->getParent()) {
				stack[0]->widgets[i]->resize();
			}
		}
	}
}

bool isClipSet(void)
{
	return clipSet;
}

void setClip(int x, int y, int width, int height)
{
	const ALLEGRO_TRANSFORM *t = al_get_current_transform();
	float tx = t->m[3][0];
	float ty = t->m[3][1];

	x = x * x_scale;
	y = y * y_scale;

	x += tx;
	y += ty;

	al_set_clipping_rectangle(x, y, ceil(width*x_scale), ceil(height*y_scale));
	clipSet = true;
}

void setClippedClip(int x, int y, int width, int height)
{
	int curr_x, curr_y, curr_w, curr_h;
	al_get_clipping_rectangle(&curr_x, &curr_y, &curr_w, &curr_h);
	if (
		x >= curr_x+curr_w || x+width <= curr_x ||
		y >= curr_y+curr_h || y+height <= curr_y
	) {
		x = curr_x;
		y = curr_y;
		width = curr_w;
		height = curr_h;
	}
	else {
		if (curr_x > x && curr_x < x+width) {
			int d = curr_x-x;
			x += d;
			width -= d;
		}
		if (curr_y > y && curr_y < y+height) {
			int d = curr_y-y;
			y += d;
			height -= d;
		}
		if (curr_x+curr_w > x && curr_x+curr_w < x+width) {
			width -= (x+width)-(curr_x+curr_w);
		}
		if (curr_y+curr_h > y && curr_y+curr_h < y+height) {
			height -= (y+height)-(curr_y+curr_h);
		}
	}
	setClip(x, y, width, height);
}

void clearClip(void)
{
	clipSet = false;
	al_set_clipping_rectangle(0, 0, screenWidth, screenHeight);
}

void getClip(int *x, int *y, int *w, int *h)
{
	al_get_clipping_rectangle(x, y, w, h);
}

bool isDeepChild(TGUIWidget *child, TGUIWidget *parent)
{
	TGUIWidget *p = parent;
	while (p->getChild()) {
		if (p->getChild() == child)
			return true;
		p = p->getChild();
	}
	return false;
}

std::vector<TGUIWidget *> findChildren(TGUIWidget *widget) {
	std::vector<TGUIWidget *> found;
	std::vector<TGUIWidget *>::iterator it;
	for (it = stack[0]->widgets.begin(); it != stack[0]->widgets.end(); it++) {
		if (isDeepChild(*it, widget)) {
			found.push_back(*it);
		}
	}

	return found;
}

void TGUIWidget::raise(void) {
	// Remove and place parent at top
	std::vector<TGUIWidget *>::iterator it = std::find(stack[0]->widgets.begin(), stack[0]->widgets.end(), this);
	if (it != stack[0]->widgets.end()) {
		stack[0]->widgets.erase(it);
		stack[0]->widgets.push_back(this);
	}

	// Do the same with all the children of this widget
	std::vector<TGUIWidget *> toRaise = findChildren(this);
	for (size_t i = 0; i < toRaise.size(); i++) {
		toRaise[i]->raise();
	}
}

void TGUIWidget::lower(void) {
	// Remove and place parent at top
	std::vector<TGUIWidget *>::iterator it = std::find(stack[0]->widgets.begin(), stack[0]->widgets.end(), this);
	if (it != stack[0]->widgets.end()) {
		stack[0]->widgets.erase(it);
		stack[0]->widgets.insert(stack[0]->widgets.begin(), this);
	}

	// Do the same with all the children of this widget
	std::vector<TGUIWidget *> toLower = findChildren(this);
	for (size_t i = 0; i < toLower.size(); i++) {
		toLower[i]->lower();
	}
}

void addPreDrawWidget(TGUIWidget *widget)
{
	preDrawWidgets.push_back(widget);
}

void addPostDrawWidget(TGUIWidget *widget)
{
	postDrawWidgets.push_back(widget);
}

void TGUIWidget::remove(void) {
	std::vector<TGUIWidget *>::iterator it = std::find(stack[0]->widgets.begin(), stack[0]->widgets.end(), this);
	if (it != stack[0]->widgets.end()) {
		stack[0]->widgets.erase(it);
	}
	it = std::find(preDrawWidgets.begin(), preDrawWidgets.end(), this);
	if (it != preDrawWidgets.end()) {
		preDrawWidgets.erase(it);
	}
	it = std::find(postDrawWidgets.begin(), postDrawWidgets.end(), this);
	if (it != postDrawWidgets.end()) {
		postDrawWidgets.erase(it);
	}
	
	if (child) {
		child->remove();
	}

	if (this == focussedWidget) {
		setFocus(NULL);
	}
}

static void destroyEvent(ALLEGRO_USER_EVENT *u)
{
	delete u;
}

void pushEvent(TGUIEventType type, void *data) {
	ALLEGRO_EVENT_SOURCE *es = (ALLEGRO_EVENT_SOURCE *)&eventSource;

	ALLEGRO_USER_EVENT *event = new ALLEGRO_USER_EVENT;
	((ALLEGRO_EVENT *)event)->type = ALLEGRO_GET_EVENT_TYPE('T', 'G', 'U', 'I');
	event->source = NULL;
	event->data1 = (intptr_t)type;
	event->data2 = (intptr_t)data;

	al_emit_user_event(es, (ALLEGRO_EVENT *)event, destroyEvent);
};

ALLEGRO_EVENT_SOURCE *getEventSource(void) {
	return (ALLEGRO_EVENT_SOURCE *)&eventSource;
}

bool isKeyDown(int keycode) {
	return keyState[keycode];
}

ALLEGRO_DISPLAY *getDisplay(void)
{
	return display;
}

static ALLEGRO_BITMAP *clone_target()
{
#if 1
	ALLEGRO_BITMAP *target = al_get_target_bitmap();
	int w = al_get_bitmap_width(target);
	int h = al_get_bitmap_height(target);

	ALLEGRO_BITMAP *b = al_create_bitmap(w, h);

	ALLEGRO_LOCKED_REGION *lr1 = al_lock_bitmap(b, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
	ALLEGRO_LOCKED_REGION *lr2 = al_lock_bitmap_region(
		target,
		0, 0, w, h,
		ALLEGRO_PIXEL_FORMAT_ANY,
		ALLEGRO_LOCK_READONLY
	);
	int pixel_size = al_get_pixel_size(al_get_bitmap_format(target));
	for (int y = 0; y < h; y++) {
		uint8_t *d1 = (uint8_t *)lr1->data + lr1->pitch * y;
		uint8_t *d2 = (uint8_t *)lr2->data + lr2->pitch * y;
		memcpy(d1, d2, pixel_size*w);
	}
	al_unlock_bitmap(b);
	al_unlock_bitmap(target);

	return b;
#else
	ALLEGRO_BITMAP *b;
	ALLEGRO_BITMAP *target = al_get_target_bitmap();
	int w = al_get_bitmap_width(target);
	int h = al_get_bitmap_height(target);
	b = al_create_bitmap(w, h);
	al_set_target_bitmap(b);
	al_draw_bitmap(target, 0, 0, 0);
	al_set_target_bitmap(target);
	return b;
#endif
}

void doModal(
	ALLEGRO_EVENT_QUEUE *queue,
	ALLEGRO_BITMAP *background,
	bool (*callback)(TGUIWidget *widget),
	void (*before_flip_callback)(),
	void (*resize_callback)()
	)
{
	ALLEGRO_BITMAP *back;
	if (background) {
		back = background;
	}
	else {
		back = clone_target();
	}

	int redraw = 0;
	ALLEGRO_TIMER *logic_timer = al_create_timer(1.0/60.0);
	al_register_event_source(queue, al_get_timer_event_source(logic_timer));
	al_start_timer(logic_timer);

	while (1) {
		ALLEGRO_EVENT event;

		while (!al_event_queue_is_empty(queue)) {
			al_wait_for_event(queue, &event);

			if (event.type == ALLEGRO_EVENT_TIMER && event.timer.source != logic_timer) {
				continue;
			}

			if (event.type == ALLEGRO_EVENT_TIMER) {
				redraw++;
			}

			if (resize_callback && event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
				al_acknowledge_resize(display);
				resize_callback();
			}

			handleEvent(&event);

			TGUIWidget *w = update();

			if (callback(w)) {
				goto done;
			}
		}

		if (redraw) {
			redraw = 0;

			al_clear_to_color(al_map_rgb_f(0.0f, 0.0f, 0.0f));
			ALLEGRO_TRANSFORM t, backup;
			al_copy_transform(&backup, al_get_current_transform());
			al_identity_transform(&t);
			al_use_transform(&t);
			al_draw_tinted_bitmap(back, al_map_rgb_f(0.5f, 0.5f, 0.5f), 0, 0, 0);
			al_use_transform(&backup);
		
			int abs_x, abs_y;
			for (size_t i = 0; i < preDrawWidgets.size(); i++) {
				determineAbsolutePosition(preDrawWidgets[i], &abs_x, &abs_y);
				preDrawWidgets[i]->preDraw(abs_x, abs_y);
			}
			::drawRect(stack[0], 0, 0, screenWidth, screenHeight);

			// Draw focus
			if (focussedWidget && focussedWidget->getDrawFocus()) {
				int x = focussedWidget->getX();
				int y = focussedWidget->getY();
				int w = focussedWidget->getWidth();
				int h = focussedWidget->getHeight();
				drawFocusRectangle(x, y, w, h);
			}

			for (size_t i = 0; i < postDrawWidgets.size(); i++) {
				determineAbsolutePosition(postDrawWidgets[i], &abs_x, &abs_y);
				postDrawWidgets[i]->postDraw(abs_x, abs_y);
			}

			if (before_flip_callback) {
				before_flip_callback();
			}

			al_flip_display();
		}
	}

done:
	if (!background) {
		al_destroy_bitmap(back);
	}

	al_destroy_timer(logic_timer);
}

static TGUIWidget *getWidgetInDirection(TGUIWidget *widget, int xdir, int ydir)
{
	int x1, y1, x2, y2;
	int measuring_point;

	if (xdir < 0) {
		x1 = 0;
		x2 = widget->getX();
		y1 = widget->getY();
		y2 = widget->getY() + widget->getHeight();
		measuring_point = widget->getX();
	}
	else if (xdir > 0) {
		x1 = widget->getX() + widget->getWidth();
		y1 = widget->getY();
		x2 = screenWidth;
		y2 = widget->getY() + widget->getHeight();
		measuring_point = widget->getX() + widget->getWidth();
	}
	else if (ydir < 0) {
		x1 = widget->getX();
		x2 = widget->getX() + widget->getWidth();
		y1 = 0;
		y2 = widget->getY();
		measuring_point = widget->getY();
	}
	else { // ydir > 0
		x1 = widget->getX();
		x2 = widget->getX() + widget->getWidth();
		y1 = widget->getY() + widget->getHeight();
		y2 = screenHeight;
		measuring_point = widget->getY() + widget->getHeight();
	}

	std::vector<TGUIWidget *> colliding;

	for (size_t i = 0; i < stack[0]->widgets.size(); i++) {
		TGUIWidget* w = stack[0]->widgets[i];
		if (w == widget || !w->acceptsFocus()) {
			continue;
		}
		int _x1, _y1, _x2, _y2;
		_x1 = w->getX();
		_x2 = w->getX() + w->getWidth();
		_y1 = w->getY();
		_y2 = w->getY() + w->getHeight();
		if (x1 >= _x2 || y1 >= _y2 || x2 < _x1 || y2 < _y1) {
			continue;
		}
		colliding.push_back(w);
	}

	// FIXME: certain conditions here: (If there isn't one, go to the next on in own group -- if none in group, go to next group. failing that, do nothing) 
	if (colliding.size() == 0) {
		return NULL;
	}

	int closest = INT_MAX;
	TGUIWidget *closest_widget = NULL;

	for (size_t i = 0; i < colliding.size(); i++) {
		TGUIWidget *w = colliding[i];
		int measuring_point2;
		if (xdir < 0) {
			measuring_point2 = w->getX() + w->getWidth();
		}
		else if (xdir > 0) {
			measuring_point2 = w->getX();
		}
		else if (ydir < 0) {
			measuring_point2 = w->getY() + w->getHeight();
		}
		else if (ydir > 0) {
			measuring_point2 = w->getY();
		}
		int dist = abs(measuring_point-measuring_point2);
		if (dist < closest) {
			closest = dist;
			closest_widget = w;
		}
	}

	return closest_widget;
}

void drawFocusRectangle(int x, int y, int w, int h)
{
	float f = fmod(al_get_time(), 2);
	if (f > 1) f = 2 - f;
	al_draw_rectangle(x+0.5f, y+0.5f, x+w-0.5f, y+h-0.5f, al_map_rgb_f(f, f, 0), 1);
}

} // end namespace tgui

static void drawRect(tgui::TGUI *gui, int x1, int y1, int x2, int y2)
{
	for (size_t i = 0; i < gui->widgets.size(); i++) {
		tgui::TGUIWidget* widget = gui->widgets[i];
		if (widget->getParent() == NULL) {
			widget->chainDraw();
		}
	}
}

