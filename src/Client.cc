#include "Manager.h"
#include "Client.h"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#if I18N
#include <X11/Xmu/Atoms.h>
#endif

// needed this to be able to use CARD32
#include <X11/Xmd.h>

const char *const Client::m_defaultLabel = "incognito";

implementList(EdgeRectList, EdgeRect);

Client::Client(WindowManager *const wm, Window w, Boolean shaped) :
    m_window(w),
    m_transient(None),
    m_groupParent(None),
    m_border(0),
    m_shaped(shaped),
    m_revert(0),
    m_wroot(None),
    m_screen(0),
    m_doSomething(False),
    m_sticky(False),
    m_skipFocus(False),
    m_focusOnClick(False),
    m_layer(NORMAL_LAYER),
    m_type(NormalClient),
    m_levelRaised(False),
    m_speculating(False),
    m_fixedSize(False),
    m_movable(True),
    m_minWidth(0),
    m_minHeight(0),
    m_state(WithdrawnState),
    m_protocol(0),
    m_managed(False),
    m_reparenting(False),
    m_stubborn(False),
    m_lastPopTime(0L),
    m_isFullHeight(False),
    m_isFullWidth(False),
    m_name(NULL),
    m_iconName(NULL),
    m_label(NULL),
    m_colormap(None),
    m_colormapWinCount(0),
    m_colormapWindows(NULL),
    m_windowColormaps(NULL),
    m_windowManager(wm)
{
    XWindowAttributes attr;
    XGetWindowAttributes(display(), m_window, &attr);

    m_x = attr.x;
    m_y = attr.y;
    m_w = attr.width;
    m_h = attr.height;
    m_bw = attr.border_width;
    m_wroot = attr.root;
    m_name = m_iconName = 0;
    m_sizeHints.flags = 0L;

    wm->setScreenFromRoot(m_wroot);
    m_screen = wm->screen();

    m_label = NewString(m_defaultLabel);
    m_border = new Border(this, w);

    // fprintf(stderr, "new client at %d,%d %dx%d, window = %lx, name = \"%s\"\n", m_x, m_y, m_w, m_h, m_window, m_label);

    m_speculating = m_levelRaised = False;

    if (attr.map_state == IsViewable) {
        manage(True);
    } else {
        fprintf(stderr, "client with window %lx is not viewable\n", m_window);
    }
}

Client::~Client() {
    delete m_border;
}

Boolean Client::hasWindow(Window w) {
    return ((m_window == w) || m_border->hasWindow(w));
}

Window Client::root() {
    return m_wroot;
}

int Client::screen() {
    return m_screen;
}

void Client::release() {
    // assume wm called for this, and will remove me from its list itself

    if (m_window == None) {
        fprintf(stderr, "wmx: invalid parent in Client::release (released twice?)\n");
    }

    windowManager()->skipInRevert(this, m_revert);

    if (isHidden()) {
        unhide(False);
    }
    windowManager()->removeFromOrderedList(this);

    if (isActive()) {
        if (CONFIG_CLICK_TO_FOCUS || isFocusOnClick()) {
            if (m_revert) {
                windowManager()->setActiveClient(m_revert);
                m_revert->activate();
            } else {
                windowManager()->setActiveClient(0);
            }
        } else {
            windowManager()->setActiveClient(0);
        }
    }

    m_window = None;

    if (m_colormapWinCount > 0) {
        XFree((char*) m_colormapWindows);
        free((char*) m_windowColormaps); // not allocated through X
    }

    if (m_iconName) {
        XFree(m_iconName);
    }
    if (m_name) {
        XFree(m_name);
    }
    if (m_label) {
        free((void*) m_label);
    }
    delete this;
}

void Client::unreparent() {
    XWindowChanges wc;
    if (!isWithdrawn()) {
        gravitate(True);
        XReparentWindow(display(), m_window, root(), m_x, m_y);
    }
    wc.border_width = m_bw;
    XConfigureWindow(display(), m_window, CWBorderWidth, &wc);
    XSync(display(), True);
}

void Client::installColormap() {
    Client *cc = 0;
    int i, found;
    if (m_colormapWinCount != 0) {
        found = 0;
        for (i = m_colormapWinCount - 1; i >= 0; --i) {
            windowManager()->installColormap(m_windowColormaps[i]);
            if (m_colormapWindows[i] == m_window) {
                ++found;
            }
        }
        if (found == 0) {
            windowManager()->installColormap(m_colormap);
        }
    } else if (m_transient != None && (cc = windowManager()->windowToClient(m_transient))) {
        if (cc) {
            cc->installColormap();
        }
    } else {
        windowManager()->installColormap(m_colormap);
    }
}

void Client::manage(Boolean mapped) {
    static int lastX = 0, lastY = 0;
    Boolean shouldHide, reshape;
    XWMHints *hints;
    Display *d = display();
    long mSize;
    int state;

    //!!!
    XSelectInput(d, m_window, ColormapChangeMask | EnterWindowMask | PropertyChangeMask | FocusChangeMask | KeyPressMask | KeyReleaseMask);

    if (CONFIG_USE_KEYBOARD) {
        int i;
        int keycode;

        static KeySym keys[] = {
            CONFIG_CIRCULATE_KEY,
            CONFIG_HIDE_KEY, CONFIG_DESTROY_KEY, CONFIG_RAISE_KEY,
            CONFIG_LOWER_KEY, CONFIG_FULLHEIGHT_KEY, CONFIG_NORMALHEIGHT_KEY,
            CONFIG_FULLWIDTH_KEY, CONFIG_NORMALWIDTH_KEY,
            CONFIG_MAXIMISE_KEY, CONFIG_UNMAXIMISE_KEY,
            CONFIG_STICKY_KEY, CONFIG_DEBUG_KEY

#if CONFIG_WANT_KEYBOARD_MENU
            , CONFIG_CLIENT_MENU_KEY, CONFIG_COMMAND_MENU_KEY
#endif
        };

        XGrabKey(display(), XKeysymToKeycode(display(), CONFIG_ALT_KEY), 0, m_window, True, GrabModeAsync, GrabModeAsync);

        // for dragging windows from anywhere with Alt pressed
        XGrabButton(display(), Button1, m_windowManager->altModMask(), m_window, False, 0, GrabModeAsync, GrabModeSync, None, None);

        for (i = 0; i < (int) (sizeof(keys) / sizeof(keys[0])); ++i) {
            keycode = XKeysymToKeycode(display(), keys[i]);
            if (keycode) {
                XGrabKey(display(), keycode, m_windowManager->altModMask() | LockMask | Mod2Mask, m_window, True, GrabModeAsync, GrabModeAsync);
                XGrabKey(display(), keycode, m_windowManager->altModMask() | LockMask, m_window, True, GrabModeAsync, GrabModeAsync);
                XGrabKey(display(), keycode, m_windowManager->altModMask() | Mod2Mask, m_window, True, GrabModeAsync, GrabModeAsync);
                XGrabKey(display(), keycode, m_windowManager->altModMask(), m_window, True, GrabModeAsync, GrabModeAsync);
            }
        }
    }

    m_iconName = getProperty(XA_WM_ICON_NAME);
    m_name = getProperty(XA_WM_NAME);
    setLabel();

    getColormaps();
    getProtocols();
    getTransient();
    getClientType();

    // fprintf(stderr, "managing client, name = \"%s\"\n", m_name);

    hints = XGetWMHints(d, m_window);
    if (!getState(&state)) {
        state = hints ? hints->initial_state : NormalState;
    }
    shouldHide = (state == IconicState);
    if (hints) {
        XFree(hints);
    }
    if (XGetWMNormalHints(d, m_window, &m_sizeHints, &mSize) == 0 || m_sizeHints.flags == 0) {
        m_sizeHints.flags = PSize;
    }

    m_fixedSize = False;
    if ((m_sizeHints.flags & (PMinSize | PMaxSize)) == (PMinSize | PMaxSize) && (m_sizeHints.min_width == m_sizeHints.max_width && m_sizeHints.min_height == m_sizeHints.max_height)) {
        m_fixedSize = True;
    }
    reshape = !mapped;

    if (m_fixedSize) {
        if ((m_sizeHints.flags & USPosition)) {
            reshape = False;
        }
        if ((m_sizeHints.flags & PPosition) && shouldHide) {
            reshape = False;
        }
        if ((m_transient != None)) {
            reshape = False;
        }
    }

    if ((m_sizeHints.flags & PBaseSize)) {
        m_minWidth = m_sizeHints.base_width;
        m_minHeight = m_sizeHints.base_height;
    } else if ((m_sizeHints.flags & PMinSize)) {
        m_minWidth = m_sizeHints.min_width;
        m_minHeight = m_sizeHints.min_height;
    } else if (!isBorderless()) {
        m_minWidth = m_minHeight = 50;
    } else {
        m_minWidth = m_minHeight = 1;
    }

    // act

    if (!isBorderless()) {
        gravitate(False);

        // zeros are iffy, should be calling some Manager method
        int dw = DisplayWidth(display(), 0), dh = DisplayHeight(display(), 0);

        if (m_w < m_minWidth) {
            m_w = m_minWidth;
            m_fixedSize = False;
            reshape = True;
        }
        if (m_h < m_minHeight) {
            m_h = m_minHeight;
            m_fixedSize = False;
            reshape = True;
        }
        if (m_w > dw - 8) {
            m_w = dw - 8;
        }
        if (m_h > dh - 8) {
            m_h = dh - 8;
        }
        if (!mapped && m_transient == None && !(m_sizeHints.flags & (PPosition | USPosition))) {
            lastX += 60;
            lastY += 40;
            if (lastX + m_w + m_border->xIndent() > dw) {
                lastX = 0;
            }
            if (lastY + m_h + m_border->yIndent() > dh) {
                lastY = 0;
            }
            m_x = lastX;
            m_y = lastY;
        }
        if (m_x > dw - m_border->xIndent()) {
            m_x = dw - m_border->xIndent();
        }
        if (m_y > dh - m_border->yIndent()) {
            m_y = dh - m_border->yIndent();
        }
        if (m_x < m_border->xIndent()) {
            m_x = m_border->xIndent();
        }
        if (m_y < m_border->yIndent()) {
            m_y = m_border->yIndent();
        }
    } else {
        reshape = False;
    }

    m_border->configure(m_x, m_y, m_w, m_h, 0L, Above);

    if (mapped) {
        m_reparenting = True;
    }
    if (reshape && !m_fixedSize) {
        XResizeWindow(d, m_window, m_w, m_h);
    }
    XSetWindowBorderWidth(d, m_window, 0);

    m_border->reparent();

    // (support for shaped windows absent)

    XAddToSaveSet(d, m_window);
    m_managed = True;

    if (shouldHide) {
        hide();
    } else {
        XMapWindow(d, m_window);
        m_border->map();
        setState(NormalState);
        if ((CONFIG_CLICK_TO_FOCUS || isFocusOnClick() || (m_transient != None && activeClient() && activeClient()->m_window == m_transient))) {
            activate();
            mapRaised();
        } else {
            deactivate();
        }
    }
    if (activeClient() && !isActive()) {
        activeClient()->installColormap();
    }
    if (CONFIG_AUTO_RAISE) {
        m_windowManager->stopConsideringFocus();
        focusIfAppropriate(False);
    }

    char *property = 0;
    int length = 0;

    if ((property = getProperty(Atoms::netwm_winState, XA_CARDINAL, length))) {
        updateFromNetwmProperty(Atoms::netwm_winState, property[0]);
        XFree(property);
    }
    if ((property = getProperty(Atoms::netwm_winHints, XA_CARDINAL, length))) {
        updateFromNetwmProperty(Atoms::netwm_winHints, property[0]);
        XFree(property);
    }

    m_windowManager->hoistToTop(this);

    sendConfigureNotify(); // due to Martin Andrews
}

void Client::selectOnMotion(Window w, Boolean select) {
    if (!CONFIG_AUTO_RAISE) {
        return;
    }
    if (!w || w == root()) {
        return;
    }
    if (w == m_window || m_border->hasWindow(w)) {
        XSelectInput(display(), m_window, ColormapChangeMask | EnterWindowMask | PropertyChangeMask | FocusChangeMask | (select ? PointerMotionMask : 0L));
    } else {
        XSelectInput(display(), w, select ? PointerMotionMask : 0L);
    }
}

void Client::gotoClient() {
    if (isKilled()) {
        // fprintf(stderr, "Client[%p]::gotoClient: client is killed\n", this);
        return;
    }
    if (isHidden()) {
        // fprintf(stderr, "Client[%p]::gotoClient: unhiding\n", this);
        unhide(True);
    } else {
        // fprintf(stderr, "Client[%p]::gotoClient: bringing to front\n", this);
        if (CONFIG_CLICK_TO_FOCUS || isFocusOnClick()) {
            activate();
        } else {
            mapRaised();
        }
        ensureVisible();
    }
}

void Client::decorate(Boolean active) {
    m_border->decorate(active, m_w, m_h);
}

void Client::activate() {
    // fprintf(stderr, "Client::activate (this = %p, window = %p, parent = %p)\n", this, (void *)m_window, (void *)parent());

    if (isNonFocusable()) {
        return;
    }
    if (parent() == root()) {
        fprintf(stderr, "wmx: warning: bad parent in Client::activate\n");
        return;
    }

    if (!m_managed || isHidden() || isWithdrawn()) {
        return;
    }
    if (isActive()) {
        decorate(True);
        if (CONFIG_AUTO_RAISE || CONFIG_RAISE_ON_FOCUS) {
            mapRaised();
        }
        return;
    }

    Client *previouslyActive = windowManager()->activeClient();

    windowManager()->setActiveClient(this); // deactivates any other

    XUngrabButton(display(), AnyButton, AnyModifier, parent());

    XSetInputFocus(display(), m_window, RevertToPointerRoot, windowManager()->timestamp(False));

    if (m_protocol & PtakeFocus) {
        sendMessage(Atoms::wm_protocols, Atoms::wm_takeFocus);
    }

    // now set revert of window that reverts to this one so as to
    // revert to the window this one used to revert to (huh?)

    windowManager()->skipInRevert(this, m_revert);

    if (previouslyActive && previouslyActive != this) {
        m_revert = previouslyActive;
        while (m_revert && !m_revert->isNormal()) {
            m_revert = m_revert->revertTo();
        }
    }

    decorate(True);
    installColormap();
}

void Client::deactivate() { // called from wm?
    if (parent() == root()) {
        fprintf(stderr, "wmx: warning: bad parent in Client::deactivate\n");
        return;
    }
    XGrabButton(display(), AnyButton, AnyModifier, parent(), False,
    ButtonPressMask | ButtonReleaseMask,
    GrabModeSync, GrabModeSync, None, None);
    decorate(False);
}

void Client::setSticky(Boolean sticky) {
    m_sticky = sticky;
    setNetwmProperty(Atoms::netwm_winState, WIN_STATE_STICKY, sticky);
}

void Client::setMovable(Boolean movable) {
    setNetwmProperty(Atoms::netwm_winState, WIN_STATE_FIXED_POSITION, !movable);
    m_movable = movable;
}

void Client::setSkipFocus(Boolean skipFocus) {
    setNetwmProperty(Atoms::netwm_winHints, WIN_HINTS_SKIP_FOCUS, skipFocus);
    m_skipFocus = skipFocus;
    // fprintf(stderr, "Setting \"%s\" to %sskip focus\n", name(), skipFocus ? "" : "not ");
}

void Client::setFocusOnClick(Boolean focusOnClick) {
    setNetwmProperty(Atoms::netwm_winHints, WIN_HINTS_FOCUS_ON_CLICK, focusOnClick);
    m_focusOnClick = focusOnClick;
}

void Client::setNetwmProperty(Atom property, int state, Boolean to) {
    // fprintf(stderr, "wmx: Client::setNetwmProperty needs rewriting\n");
}

void Client::setLayer(int newLayer) {
    if (newLayer < 0) {
        newLayer = 0;
    } else if (newLayer > MAX_LAYER) {
        newLayer = MAX_LAYER;
    }
    if (newLayer == m_layer) {
        return;
    }
    windowManager()->removeFromOrderedList(this);
    m_layer = newLayer;
    windowManager()->hoistToTop(this);  // Puts this client at the top of the list for its layer.
    windowManager()->updateStackingOrder();
    // fprintf(stderr, "wmx: Moving client \"%s\" to layer %d\n", name(), m_layer);
}

void Client::sendMessage(Atom a, long l) {
    XEvent ev;
    int status;
    long mask;

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = m_window;
    ev.xclient.message_type = a;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = l;
    ev.xclient.data.l[1] = windowManager()->timestamp(False);
    mask = 0L;
    status = XSendEvent(display(), m_window, False, mask, &ev);

    if (status == 0) {
        fprintf(stderr, "wmx: warning: Client::sendMessage failed\n");
    }
}

static int getProperty_aux(Display *d, Window w, Atom a, Atom type, long len, unsigned char **p) {
    Atom realType;
    int format;
    unsigned long n, extra;
    int status;
    Boolean retried = False;

    tryagain:
       status = XGetWindowProperty(d, w, a, 0L, len, False, type, &realType, &format, &n, &extra, p);
       // fprintf(stderr, "XGetWindowProperty: len = %ld, return count = %lu, format = %d, pointer = %p\n", len, n, format, *p);

    if (status != Success || *p == 0) {
        return -1;
    }
    if (n == 0) {
        XFree((void*) *p);
        *p = 0;
        return n;
    }
    if (type == XA_STRING || type == AnyPropertyType || retried) {
        if (realType != XA_STRING || format != 8) {
            fprintf(stderr, "string property for window %lx is not an 8-bit string\n", w);
        }
        // XA_STRING is needed by a caller.
        // But XGetWindowProperty() returns other typed data.
        // So try to convert them.
        XTextProperty textprop;
        textprop.value = *p;
        textprop.encoding = realType;
        textprop.format = format;
        textprop.nitems = n;

        char **list;
        int num, cnt;
        cnt = XmbTextPropertyToTextList(d, &textprop, &list, &num);
        if (cnt > Success) {
            fprintf(stderr, "wmx: WARNING: Failed to convert text property using Xmb method, trying again using Xutf8\n");
            XFreeStringList(list);
            cnt = Xutf8TextPropertyToTextList(d, &textprop, &list, &num);
        }
        unsigned char *string;
        if (cnt == Success && num > 0 && *list) {
            string = (unsigned char*) NewString(*list);
            XFreeStringList(list);
            n = num;
        } else if (cnt > Success) {
            fprintf(stderr, "Something wrong, cannot convert text property\n"
                "  original type %ld, string %s\n"
                "  converted string %s\n"
                "  decide to use original value as STRING\n",
                textprop.encoding, textprop.value, *list);
            string = (unsigned char*) NewString(*(char** )p);
            XFreeStringList(list);
            // n = n;
        } else if (!retried) {
            retried = True;
            type = AnyPropertyType;
            goto tryagain;
        } else {
            string = NULL;
            n = 0;
        }
        if (*p) {
            XFree((void*) *p);
        }
        *p = string;
    }
    return n;
}

char* Client::getProperty(Atom a) {
    unsigned char *p;
    if (getProperty_aux(display(), m_window, a, AnyPropertyType, 100L, &p) <= 0) {
        return NULL;
    }
    return (char*) p;
}

char* Client::getProperty(Atom a, Atom type, int &length) {
    unsigned char *p;
    if ((length = getProperty_aux(display(), m_window, a, type, 100L, &p)) <= 0) {
        return NULL;
    }
    return (char*) p;
}

void Client::setState(int state) {
    m_state = state;
    CARD32 data[2];
    data[0] = (CARD32) state;
    data[1] = (CARD32) None;
    XChangeProperty(display(), m_window, Atoms::wm_state, Atoms::wm_state, 32, PropModeReplace, (unsigned char*) data, 2);
}

Boolean Client::getState(int *state) {
    CARD32 *p = 0;
    if (getProperty_aux(display(), m_window, Atoms::wm_state, Atoms::wm_state, 2L, (unsigned char**) &p) <= 0) {
        return False;
    }
    *state = (int) *p;
    XFree((char*) p);
    return True;
}

void Client::getProtocols() {
    long n;
    Atom *p;
    m_protocol = 0;
    if ((n = getProperty_aux(display(), m_window, Atoms::wm_protocols, XA_ATOM, 20L, (unsigned char**) &p)) <= 0) {
        return;
    }
    for (int i = 0; i < n; ++i) {
        if (p[i] == Atoms::wm_delete) {
            m_protocol |= Pdelete;
        } else if (p[i] == Atoms::wm_takeFocus) {
            m_protocol |= PtakeFocus;
        }
    }
    XFree((char*) p);
}

void Client::gravitate(Boolean invert) {
    int gravity;
    int w = 0, h = 0, xdelta, ydelta;

    // possibly shouldn't work if we haven't been managed yet?

    gravity = NorthWestGravity;
    if (m_sizeHints.flags & PWinGravity) {
        gravity = m_sizeHints.win_gravity;
    }
    xdelta = m_bw - m_border->xIndent();
    ydelta = m_bw - m_border->yIndent();

    // note that right and bottom borders have indents of 1

    switch (gravity) {

      case NorthWestGravity: {
        break;
      }
      case NorthGravity: {
        w = xdelta;
        break;
      }
      case NorthEastGravity: {
        w = xdelta + m_bw - 1;
        break;
      }
      case WestGravity: {
        h = ydelta;
        break;
      }
      case CenterGravity:
      case StaticGravity: {
        w = xdelta;
        h = ydelta;
        break;
      }
      case EastGravity: {
        w = xdelta + m_bw - 1;
        h = ydelta;
        break;
      }
      case SouthWestGravity: {
        h = ydelta + m_bw - 1;
        break;
      }
      case SouthGravity: {
        w = xdelta;
        h = ydelta + m_bw - 1;
        break;
      }
      case SouthEastGravity: {
        w = xdelta + m_bw - 1;
        h = ydelta + m_bw - 1;
        break;
      }
      default: {
        fprintf(stderr, "wmx: bad window gravity %d for window 0x%lx\n", gravity, m_window);
        return;
      }

    } // switch

    w += m_border->xIndent();
    h += m_border->yIndent();
    if (invert) {
        w = -w;
        h = -h;
    }
    m_x += w;
    m_y += h;
}

Boolean Client::setLabel(void) {
    const char *newLabel;

    if (m_name) {
        newLabel = m_name;
    } else if (m_iconName) {
        newLabel = m_iconName;
    } else {
        newLabel = m_defaultLabel;
    }
    if (!m_label) {
        m_label = NewString(newLabel);
        return True;
    } else if (strcmp(m_label, newLabel)) {
        free((void*) m_label);
        m_label = NewString(newLabel);
        return True;
    } else {
        return True; // False; // dammit!
    }
}

void Client::getColormaps(void) {
    int i, n;
    Window *cw;
    XWindowAttributes attr;

    if (!m_managed) {
        XGetWindowAttributes(display(), m_window, &attr);
        m_colormap = attr.colormap;
    }

    n = getProperty_aux(display(), m_window, Atoms::wm_colormaps, XA_WINDOW, 100L, (unsigned char**) &cw);

    if (m_colormapWinCount != 0) {
        XFree((char*) m_colormapWindows);
        free((char*) m_windowColormaps);
    }

    if (n <= 0) {
        m_colormapWinCount = 0;
        return;
    }

    m_colormapWinCount = n;
    m_colormapWindows = cw;

    m_windowColormaps = (Colormap*) malloc(n * sizeof(Colormap));

    for (i = 0; i < n; ++i) {
        if (cw[i] == m_window) {
            m_windowColormaps[i] = m_colormap;
        } else {
            XSelectInput(display(), cw[i], ColormapChangeMask);
            XGetWindowAttributes(display(), cw[i], &attr);
            m_windowColormaps[i] = attr.colormap;
        }
    }
}

void Client::getClientType() {
    m_type = NormalClient;

    if (m_transient != None) {
        m_type = DialogClient;
    }
    // fprintf(stderr, "trying to get client type...\n");

    int count = 0;
    char *property = getProperty(Atoms::netwm_winType, XA_ATOM, count);

    if (property) {
        // fprintf(stderr, "got property, count = %d\n", count);
        for (int i = 0; i < count; ++i) {
            Atom typeAtom = ((Atom*) property)[i];
            char *name = XGetAtomName(display(), typeAtom);
            // fprintf(stderr, "window type property item %d is \"%s\"\n", i, name);
            if (name) {
                XFree(name);
            }
            if (typeAtom == Atoms::netwm_winType_desktop) {
                m_type = DesktopClient;
                m_layer = DESKTOP_LAYER;
                setSticky(True);
                break;
            } else if (typeAtom == Atoms::netwm_winType_dock) {
                m_type = DockClient;
                m_layer = DOCK_LAYER;
                setSticky(True);
                break;
            } else if (typeAtom == Atoms::netwm_winType_toolbar) {
                m_type = ToolbarClient;
                m_layer = TOOLBAR_LAYER;
                break;
            } else if (typeAtom == Atoms::netwm_winType_menu) {
                m_type = MenuClient;
                m_layer = TOOLBAR_LAYER;
                break;
            } else if (typeAtom == Atoms::netwm_winType_utility) {
                m_type = UtilityClient;
                m_layer = UTILITY_LAYER;
                break;
            } else if (typeAtom == Atoms::netwm_winType_dialog) {
                m_type = DialogClient;
                m_layer = DIALOG_LAYER;
                break;
            } else if (typeAtom == Atoms::netwm_winType_notify) {
                m_type = NotifyClient;
                m_layer = DOCK_LAYER;
                break;
            } else if (typeAtom == Atoms::netwm_winType_normal) {
                m_type = NormalClient;
                break;
            }
        }
        XFree(property);
    }
    // fprintf(stderr, "client window type = %d\n", (int) m_type);
}

void Client::getTransient() {
    Window t = None;
    if (XGetTransientForHint(display(), m_window, &t) != 0) {
        if (windowManager()->windowToClient(t) == this) {
            fprintf(stderr, "wmx: warning: client \"%s\" thinks it's a transient for itself -- ignoring WM_TRANSIENT_FOR property...\n", m_label ? m_label : "(no name)");
            m_transient = None;
        } else {
            m_transient = t;
        }
    } else {
        m_transient = None;
    }
}

void Client::hide() {
    if (isHidden()) {
        fprintf(stderr, "wmx: Client already hidden in Client::hide\n");
        return;
    }
    m_border->unmap();
    XUnmapWindow(display(), m_window);
    if (isActive()) {
        windowManager()->clearFocus();
    }
    setState(IconicState);
    windowManager()->addToHiddenList(this);
}

void Client::unhide(Boolean map) {
    if (CONFIG_MAD_FEEDBACK) {
        m_speculating = False;
        if (!isHidden()) {
            return;
        }
    }
    if (!isHidden()) {
        fprintf(stderr, "wmx: Client not hidden in Client::unhide\n");
        return;
    }
    windowManager()->removeFromHiddenList(this);
    if (map) {
        setState(NormalState);
        XMapWindow(display(), m_window);
        mapRaised();
        if (CONFIG_AUTO_RAISE) {
            focusIfAppropriate(False);
        } else if (CONFIG_CLICK_TO_FOCUS || isFocusOnClick()) {
            activate();
        }
    }
}

void Client::sendConfigureNotify() {
    XConfigureEvent ce;
    ce.type = ConfigureNotify;
    ce.event = m_window;
    ce.window = m_window;
    ce.x = m_x;
    ce.y = m_y;
    ce.width = m_w;
    ce.height = m_h;
    ce.border_width = m_bw;
    ce.above = None;
    ce.override_redirect = 0;
    XSendEvent(display(), m_window, False, StructureNotifyMask, (XEvent*) &ce);
}

void Client::withdraw(Boolean changeState) {
    // fprintf(stderr, "withdraw: changeState = %d\n", (int) changeState);
    m_border->unmap();
    gravitate(True);
    XReparentWindow(display(), m_window, root(), m_x, m_y);
    gravitate(False);
    if (changeState) {
        XRemoveFromSaveSet(display(), m_window);
        setState(WithdrawnState);
    }
    ignoreBadWindowErrors = True;
    XSync(display(), False);
    ignoreBadWindowErrors = False;
}

void Client::unwithdraw() {
    if (!m_managed) {
        if (!m_reparenting) {
            manage(True);
        }
        return;
    }
    // fprintf(stderr, "unwithdraw: reparenting\n");
    m_reparenting = true;
    m_border->reparent();
    m_border->map();
    setState(NormalState);
}

void Client::rename() {
    m_border->configure(0, 0, m_w, m_h, CWWidth | CWHeight, Above);
    m_border->expose(0);
}

void Client::mapRaised() {
    m_border->map(); // or mapRaised() ?
    windowManager()->hoistToTop(this);
    windowManager()->raiseTransients(this);
    windowManager()->updateStackingOrder();
}

void Client::kill() {
    if (m_protocol & Pdelete) {
        sendMessage(Atoms::wm_protocols, Atoms::wm_delete);
    } else {
        XKillClient(display(), m_window);
    }
}

void Client::ensureVisible() {
    int mx = DisplayWidth(display(), 0) - 1; // hack
    int my = DisplayHeight(display(), 0) - 1;
    int px = m_x;
    int py = m_y;
    if (m_x + m_w > mx) {
        m_x = mx - m_w;
    }
    if (m_y + m_h > my) {
        m_y = my - m_h;
    }
    if (m_x < 0) {
        m_x = 0;
    }
    if (m_y < 0) {
        m_y = 0;
    }
    if (m_x != px || m_y != py) {
        m_border->moveTo(m_x, m_y);
        sendConfigureNotify();
    }
}

void Client::lower() {
    windowManager()->hoistToBottom(this);
    windowManager()->updateStackingOrder();
}

void Client::raiseOrLower() {
    if (windowManager()->isTop(this)) {
        lower();
    } else {
        mapRaised();
    }
}

void Client::maximise(int max) {
    enum {
        Vertical, Maximum, Horizontal
    };

    if (max != Vertical && max != Horizontal && max != Maximum) {
        return;
    }
    if (m_fixedSize || (m_transient != None)) {
        return;
    }
    if (CONFIG_SAME_KEY_MAX_UNMAX) {
        if (m_isFullHeight && m_isFullWidth) {
            unmaximise(max);
            return;
        } else if (m_isFullHeight && max == Vertical) {
            unmaximise(max);
            return;
        } else if (m_isFullWidth && max == Horizontal) {
            unmaximise(max);
            return;
        }
    } else if ((m_isFullHeight && max == Vertical) || (m_isFullHeight && m_isFullWidth && max == Maximum) || (m_isFullWidth && max == Horizontal)) {
        return;
    }

    int w = (max == Horizontal || (max == Maximum && !m_isFullWidth));
    int h = (max == Vertical || (max == Maximum && !m_isFullHeight));
    if (h) {
        m_normalH = m_h;
        m_normalY = m_y;
        m_h = DisplayHeight(display(), windowManager()->screen()) - m_border->yIndent() - 1;
    }
    if (w) {
        m_normalW = m_w;
        m_normalX = m_x;
        m_w = DisplayWidth(display(), windowManager()->screen()) - m_border->xIndent() - 1;
    }

    int dw, dh;
    fixResizeDimensions(m_w, m_h, dw, dh);

    if (h) {
        if (m_h > m_normalH) {
            m_y -= (m_h - m_normalH);
            if (m_y < m_border->yIndent()) {
                m_y = m_border->yIndent();
            }
        }
        m_isFullHeight = True;
    }
    if (w) {
        if (m_w > m_normalW) {
            m_x -= (m_w - m_normalW);
            if (m_x < m_border->xIndent()) {
                m_x = m_border->xIndent();
            }
        }
        m_isFullWidth = True;
    }

    unsigned long mask;

    if (h & w) {
        mask = CWY | CWX | CWHeight | CWWidth;
    } else if (h) {
        mask = CWY | CWHeight;
    } else {
        mask = CWX | CWWidth;
    }
    m_border->configure(m_x, m_y, m_w, m_h, mask, 0, True);

    XResizeWindow(display(), m_window, m_w, m_h);
    sendConfigureNotify();
    if (max == Vertical || max == Maximum) {
        setNetwmProperty(Atoms::netwm_winState, WIN_STATE_MAXIMIZED_VERT, m_isFullHeight);
    }
    if (max == Horizontal || max == Maximum) {
        setNetwmProperty(Atoms::netwm_winState, WIN_STATE_MAXIMIZED_HORIZ, m_isFullWidth);
    }
}

void Client::unmaximise(int max) {
    enum {
        Vertical, Maximum, Horizontal
    };

    if (max != Vertical && max != Horizontal && max != Maximum) {
        return;
    }
    if ((!m_isFullHeight && max == Vertical) || (!m_isFullWidth && max == Horizontal) || (!(m_isFullHeight && m_isFullWidth) && max == Maximum)) {
        return;
    }

    int w = (max == Horizontal || (max == Maximum && m_isFullWidth));
    int h = (max == Vertical || (max == Maximum && m_isFullHeight));
    if (h) {
        m_h = m_normalH;
        m_y = m_normalY;
        m_isFullHeight = False;
    }
    if (w) {
        m_w = m_normalW;
        m_x = m_normalX;
        m_isFullWidth = False;
    }

    unsigned long mask;

    if (h & w) {
        mask = CWY | CWX | CWHeight | CWWidth;
    } else if (h) {
        mask = CWY | CWHeight;
    } else {
        mask = CWX | CWWidth;
    }
    m_border->configure(m_x, m_y, m_w, m_h, mask, 0, True);

    XResizeWindow(display(), m_window, m_w, m_h);
    sendConfigureNotify();
    if (max == Vertical || max == Maximum) {
        setNetwmProperty(Atoms::netwm_winState, WIN_STATE_MAXIMIZED_VERT, m_isFullHeight);
    }
    if (max == Horizontal || max == Maximum) {
        setNetwmProperty(Atoms::netwm_winState, WIN_STATE_MAXIMIZED_HORIZ, m_isFullWidth);
    }
}

void Client::warpPointer() {
    XWarpPointer(display(), None, parent(), 0, 0, 0, 0, m_border->xIndent() / 2, m_border->xIndent() + 8);
}

void Client::showFeedback() {
    if (CONFIG_MAD_FEEDBACK) {
        if (m_speculating || m_levelRaised) {
            removeFeedback(False);
        }
        m_border->showFeedback(m_x, m_y, m_w, m_h);
    }
}

void Client::raiseFeedbackLevel() {
    if (CONFIG_MAD_FEEDBACK) {
        m_border->removeFeedback();
        m_levelRaised = True;
        if (isNormal()) {
            mapRaised();
        } else if (isHidden()) {
            unhide(True);
            m_speculating = True;
        }
    }
}

void Client::removeFeedback(Boolean mapped) {
    if (CONFIG_MAD_FEEDBACK) {
        m_border->removeFeedback();
        if (m_levelRaised) {
            if (m_speculating) {
                if (!mapped) {
                    hide();
                }
                XSync(display(), False);
            } else {
                // not much we can do
            }
        }
        m_speculating = m_levelRaised = False;
    }
}

void Client::updateFromNetwmProperty(Atom property, unsigned char state) {
    fprintf(stderr, "Client(\"%s\")::updateFromNetwmProperty(\"%s\", %d)\n", name(), XGetAtomName(display(), property), (int) state);
    return;
}

void Client::appendEdges(EdgeRectList &list) {
    if (!m_managed || !isNormal() || isTransient()) {
        return;
    }
    EdgeRect r;
    if (isBorderless()) {
        r.left = m_x - 1;
        r.top = m_y - 1;
    } else {
        r.left = m_x - CONFIG_FRAME_THICKNESS;
        r.top = m_y - CONFIG_FRAME_THICKNESS;
    }
    r.right = m_x + m_w;
    r.bottom = m_y + m_h;
    // fprintf(stderr, "edges: H %d - %d  V %d - %d  for \"%s\"\n", r.left, r.right, r.top, r.bottom, name());
    list.append(r);
}

void Client::printClientData() {
    printf("     * Window: %lx - Name: \"%s\"\n", window(), name() ? name() : "");
    printf("     * Managed: %s - Reparenting: %s - Type: ", m_managed ? "Y" : "N", m_reparenting ? "Y" : "N");
    switch (m_type) {

      case NormalClient: {
        printf("Normal ");
        break;
      }
      case DialogClient: {
        printf("Dialog ");
        break;
      }
      case DesktopClient: {
        printf("Desktop");
        break;
      }
      case DockClient: {
        printf("Dock   ");
        break;
      }
      case ToolbarClient: {
        printf("Toolbar");
        break;
      }
      case MenuClient: {
        printf("Menu   ");
        break;
      }
      case UtilityClient: {
        printf("Utility");
        break;
      }
      case SplashClient: {
        printf("Splash ");
        break;
      }
      default: {
        printf("(unknown)");
        break; // shouldn't happen
      }

    } // switch

    printf(" - State: ");

    switch (m_state) {

      case WithdrawnState: {
        printf("Withdrawn (unmapped)");
        break;
      }
      case NormalState: {
        printf("Normal (visible)");
        break;
      }
      case IconicState: {
        printf("Iconic (hidden)");
        break;
      }
      default: {
        printf("Unexpected state %d", m_state);
      }

    } // switch

    printf("\n");

    printf(
            "     * Geometry: %dx%d%s%d%s%d - Shaped: %s - Screen: %d - Layer: %d",
            m_w, m_h, (m_x >= 0 ? "+" : ""), m_x, (m_y >= 0 ? "+" : ""), m_y,
            m_shaped ? "Y" : "N", m_screen, m_layer);

    printf(
            "\n     * Transient for: %lx - Group parent: %lx - Revert to: %p (%lx)\n",
            m_transient, m_groupParent, m_revert,
            m_revert ? m_revert->window() : None);
}
