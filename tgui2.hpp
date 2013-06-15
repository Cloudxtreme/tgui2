#ifndef TGUI_H
#define TGUI_H

#include <vector>
#include <algorithm>

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#ifndef ALLEGRO_WINDOWS
#include <sys/time.h>
#endif

namespace tgui {

enum TGUIEventType {
	TGUI_EVENT_OBJECT = 0
};

// forward declarations
void getScreenSize(int *w, int *h);

class TGUIWidget {
public:
	friend void drawRect(int x1, int y1, int x2, int y2);
	friend void handleEvent_pretransformed(void *allegro_event);
	friend void handleEvent(void *allegro_event);

	float getX() { return x; }
	float getY() { return y; }
	virtual void setX(float newX) { x = newX; }
	virtual void setY(float newY) { y = newY; }

	int getWidth() { return width; }
	int getHeight() { return height; }
	virtual void setWidth(int w) { width = w; }
	virtual void setHeight(int h) { height = h; }

	TGUIWidget *getParent() { return parent; }
	void setParent(TGUIWidget *p) { parent = p; }

	TGUIWidget *getChild() { return child; }
	void setChild(TGUIWidget *c) { child = c; }

	virtual void draw(int abs_x, int abs_y) {}
	// -- only called if registered
	virtual void preDraw(int abs_x, int abs_y) {}
	virtual void postDraw(int abs_x, int abs_y) {}
	// --

	virtual TGUIWidget *update(void) {
		TGUIWidget *w;
		if (child) {
			w = child->update();
			if (w) return w;
		}
		return NULL;
	}
	virtual void resize(void) {
		resize_self();
		resize_child();
	}
	virtual void translate(int xx, int yy) {
		if (child) {
			child->translate(xx, yy);
		}
	}

	virtual void raise(void);
	virtual void lower(void);

	// give relative and absolute coordinates. rel_x/y can be -1 if not
	// over widget
	virtual void mouseMove(int rel_x, int rel_y, int abs_x, int abs_y) {}
	virtual void mouseScroll(int z, int w) {}
	virtual void mouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb) {}
	virtual void mouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int b) {}
	virtual void keyDown(int keycode) {}
	virtual void keyUp(int keycode) {}
	virtual void keyChar(int keycode, int unichar) {}

	virtual void mouseMoveAll(TGUIWidget *leftOut, int abs_x, int abs_y)
	{
		if (this != leftOut) {
			mouseMove(-1, -1, abs_x, abs_y);
		}
		if (child) {
			child->mouseMoveAll(leftOut, abs_x, abs_y);
		}
	}
	virtual void mouseDownAll(TGUIWidget *leftOut, int abs_x, int abs_y, int mb)
	{
		if (this != leftOut) {
			mouseDown(-1, -1, abs_x, abs_y, mb);
		}
		if (child) {
			child->mouseDownAll(leftOut, abs_x, abs_y, mb);
		}
	}
	virtual void mouseUpAll(TGUIWidget *leftOut, int abs_x, int abs_y, int mb)
	{
		if (this != leftOut) {
			mouseUp(-1, -1, abs_x, abs_y, mb);
		}
		if (child) {
			child->mouseUpAll(leftOut, abs_x, abs_y, mb);
		}
	}

	virtual void remove(void);
	
	virtual bool acceptsFocus() { return false; }


	virtual TGUIWidget *chainMouseMove(int rel_x, int rel_y, int abs_x, int abs_y, int z, int w);
	virtual TGUIWidget *chainMouseDown(int rel_x, int rel_y, int abs_x, int abs_y, int mb);
	virtual TGUIWidget *chainMouseUp(int rel_x, int rel_y, int abs_x, int abs_y, int b);
	virtual void chainKeyDown(int keycode);
	virtual void chainKeyUp(int keycode);
	virtual void chainKeyChar(int keycode, int unichar);
	virtual void chainDraw(void);

	/* losing/gaining: return true to lose/gain, false to retain focus */
	virtual bool losingFocus(void) { return true; }
	virtual bool gainingFocus(void) { return true; }
	virtual void lostFocus(void) {}
	virtual void gainedFocus(void) {}

	TGUIWidget() {
		parent = child = NULL;
	}

	virtual ~TGUIWidget() {}

protected:

	void resize_self(void) {
		if (parent) {
			width = parent->getWidth();
			height = parent->getHeight();
		}
		else {
			int w, h;
			tgui::getScreenSize(&w, &h);
			width = w;
			height = h;
		}
	}

	void resize_child(void) {
		if (child)
			child->resize();
	}

	float x;
	float y;
	float width;
	float height;
	TGUIWidget *parent;
	TGUIWidget *child;
};

long currentTimeMillis();
void init(ALLEGRO_DISPLAY *display);
void shutdown();
void setFocus(TGUIWidget *widget);
TGUIWidget *getFocussedWidget();
void focusPrevious();
void focusNext();
void translateAll(int x, int y);
void addWidget(TGUIWidget *widget);
TGUIWidget *update();
std::vector<TGUIWidget *> updateAll();
void draw();
void drawRect(int x1, int y1, int x2, int y2);
void push();
bool pop();
void setNewWidgetParent(TGUIWidget *parent);
TGUIWidget *getNewWidgetParent(void);
void centerWidget(TGUIWidget *widget, int x, int y);
bool widgetIsChildOf(TGUIWidget *widget, TGUIWidget *parent);
void setScale(float x_scale, float y_scale);
void setOffset(float x_offset, float y_offset);
void ignore(int type);
void convertMousePosition(int *x, int *y);
void bufferToScreenPos(int *x, int *y, int bw, int bh);
void handleEvent_pretransformed(void *allegro_event);
void handleEvent(void *allegro_event);
TGUIWidget *getTopLevelParent(TGUIWidget *widget);
ALLEGRO_FONT *getFont(void);
void setFont(ALLEGRO_FONT *font);
void determineAbsolutePosition(TGUIWidget *widget, int *x, int *y);
TGUIWidget *determineTopLevelOwner(int x, int y);
bool pointOnWidget(TGUIWidget *widget, int x, int y);
void resize(TGUIWidget *parent);
void clearClip(void);
void setClippedClip(int x, int y, int width, int height);
void setClip(int x, int y, int width, int height);
bool isClipSet(void);
void getClip(int *x, int *y, int *w, int *h);
void raiseWidget(TGUIWidget *widget);
void lowerWidget(TGUIWidget *widget);
bool isDeepChild(TGUIWidget *parent, TGUIWidget *widget);
std::vector<TGUIWidget *> removeChildren(TGUIWidget *widget);
void addPreDrawWidget(TGUIWidget *widget);
void addPostDrawWidget(TGUIWidget *widget);
void pushEvent(TGUIEventType type, void *data);
ALLEGRO_EVENT_SOURCE *getEventSource(void);
bool isKeyDown(int keycode);
void setScreenSize(int w, int h);
ALLEGRO_DISPLAY *getDisplay(void);
// Callback should return true to cancel dialog, it will get each widget
// returned from tgui::update
void doModal(
	ALLEGRO_EVENT_QUEUE *queue, 
	ALLEGRO_BITMAP *background,
	bool (*callback)(TGUIWidget *widget),
	void (*before_flip_callback)(),
	void (*resize_callback)()
);

} // End namespace tgui

#endif
