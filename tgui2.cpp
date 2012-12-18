#include "tgui2.hpp"

#include <cstdio>

namespace tgui {

	struct TGUI {
		std::vector<TGUIWidget*> widgets;
	};

}

static void drawRect(tgui::TGUI *gui, int x1, int y1, int x2, int y2);

namespace tgui {

static ALLEGRO_DISPLAY *display;

static std::vector<TGUI*> stack;
static double lastUpdate;
static TGUIWidget* currentParent = 0;

static int screenWidth = 0;
static int screenHeight = 0;

static float x_scale = 1;
static float y_scale = 1;

static int rotation = 0;

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
	for (unsigned int i = 0; i < gui->widgets.size(); i++) {
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
}

void init(ALLEGRO_DISPLAY *d)
{
	display = d;

	deletestack();

	TGUI *gui = new TGUI;

	stack.push_back(gui);

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
	focussedWidget = widget;
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

	for (unsigned int i = 0; i < focusOrderList.size(); i++) {
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

	for (unsigned int i = 0; i < focusOrderList.size(); i++) {
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

	for (unsigned int i = 0; i < gui->widgets.size(); i++) {
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


TGUIWidget *update(void)
{
	long currTime = currentTimeMillis();
	long elapsed = currTime - lastUpdate;
	if (elapsed > 50) {
		elapsed = 50;
	}
	lastUpdate = currTime;

	for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
		TGUIWidget *widget = stack[0]->widgets[i];
		TGUIWidget *retVal = widget->update();
		if (retVal) {
			return retVal;
		}
	}

	return NULL;
}

void draw()
{
	int abs_x, abs_y;

	for (unsigned int i = 0; i < preDrawWidgets.size(); i++) {
		determineAbsolutePosition(preDrawWidgets[i], &abs_x, &abs_y);
		preDrawWidgets[i]->preDraw(abs_x, abs_y);
	}

	drawRect(0, 0, screenWidth, screenHeight);
	
	for (unsigned int i = 0; i < postDrawWidgets.size(); i++) {
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
}

bool pop()
{
	if (stack.size() <= 0)
		return false;

	deleteGUI(stack[0]);
	stack.erase(stack.begin());

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
	widget->setX(x - (widget->getWidth() / 2));
	widget->setY(y - (widget->getHeight() / 2));
}

void setScale(float xscale, float yscale)
{
	x_scale = xscale;
	y_scale = yscale;
}

// Only accepts 0, 90, 180, 270
void setRotation(int angle_in_degrees)
{
	rotation = angle_in_degrees;
}

void convertMousePosition(int *x, int *y)
{
	int in_x = *x;
	int in_y = *y;

	switch (rotation) {
		case 0:
			break;
		case 90:
			*x = (screenHeight-1) - in_y;
			*y = in_x;
			break;
		case 180:
			*x = (screenWidth-1) - in_x;
			*y = (screenHeight-1) - in_y;
			break;
		case 270:
			*x = in_y;
			*y = (screenWidth-1) - in_x;
			break;
	}

	*x = *x / x_scale;
	*y = *y / y_scale;
}

void bufferToScreenPos(int *x, int *y, int bw, int bh)
{
	int in_x = *x;
	int in_y = *y;

	switch (rotation) {
		case 0:
			break;
		case 90:
			*y = (bw-1) - in_x;
			*x = in_y;
			break;
		case 180:
			*y = (bh-1) - in_y;
			*x = (bw-1) - in_x;
			break;
		case 270:
			*y = in_x;
			*x = (bh-1) - in_y;
			break;
	}
	
	*x = *x * x_scale;
	*y = *y * y_scale;
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
				for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
					TGUIWidget *w2 = stack[0]->widgets[i];
					if (w2->getParent() == NULL) {
						w2->mouseMoveAll(leftOut, mx, my);
					}
				}
			}
			else {
				for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
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
					for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
						TGUIWidget *w2 = stack[0]->widgets[i];
						if (w2->getParent() == NULL) {
							w2->mouseDownAll(leftOut, mx, my, event->mouse.button);
						}
					}
				}
				else {
					TGUIWidget *leftOut = w->chainMouseUp(rel_x, rel_y, mx, my, event->mouse.button);
					for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
						TGUIWidget *w2 = stack[0]->widgets[i];
						if (w2->getParent() == NULL) {
							w2->mouseUpAll(leftOut, mx, my, event->mouse.button);
						}
					}
				}
			}
			else {
				for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
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
			for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
				if (stack[0]->widgets[i]->getParent() == NULL) {
					stack[0]->widgets[i]->chainKeyDown(event->keyboard.keycode);
				}
			}
			break;
		}
		case ALLEGRO_EVENT_KEY_UP: {
			keyState[event->keyboard.keycode] = false;
			for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
				if (stack[0]->widgets[i]->getParent() == NULL) {
					stack[0]->widgets[i]->chainKeyUp(event->keyboard.keycode);
				}
			}
			break;
		}
		case ALLEGRO_EVENT_KEY_CHAR: {
			for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
				if (stack[0]->widgets[i]->getParent() == NULL) {
					stack[0]->widgets[i]->chainKeyChar(event->keyboard.keycode, event->keyboard.unichar);
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

void TGUIWidget::chainKeyChar(int keycode, int unichar)
{
	// handle it within outself
	keyChar(keycode, unichar);

	// pass it on to the child
	if (child) {
		child->chainKeyChar(
			keycode, unichar
		);
	}
}

void TGUIWidget::chainDraw(void)
{
	int abs_x, abs_y;
	determineAbsolutePosition(this, &abs_x, &abs_y);
	draw(abs_x, abs_y);

	if (child) {
		//determineAbsolutePosition(child, &abs_x, &abs_y);
		//child->draw(abs_x, abs_y);
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
		for (unsigned int i = 0; i < stack[0]->widgets.size(); i++) {
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
	al_set_clipping_rectangle(x, y, width, height);
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
	for (unsigned int i = 0; i < toRaise.size(); i++) {
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
	for (unsigned int i = 0; i < toLower.size(); i++) {
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
	
	if (child) {
		child->remove();
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

} // end namespace tgui

static void drawRect(tgui::TGUI *gui, int x1, int y1, int x2, int y2)
{
	for (unsigned int i = 0; i < gui->widgets.size(); i++) {
		tgui::TGUIWidget* widget = gui->widgets[i];
		if (widget->getParent() == NULL) {
			widget->chainDraw();
		}
	}
}

