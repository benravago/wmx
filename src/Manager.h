#ifndef _MANAGER_H_
#define _MANAGER_H_

#include "General.h"
#include "listmacro.h"

class Client;
declarePList(ClientList, Client);

class WindowManager {

public:
    WindowManager(int argc, char **argv);
    ~WindowManager();

    void fatal(const char*);

    // for call from Client and within:

    Client* windowToClient(Window, Boolean create = False);

    Client* activeClient() {
        return m_activeClient;
    }

    Boolean raiseTransients(Client*); // true if raised any
    Time timestamp(Boolean reset);
    void clearFocus();

    void setActiveClient(Client *const c);

    void addToHiddenList(Client*);
    void removeFromHiddenList(Client*);
    void skipInRevert(Client*, Client*);

    Display* display() {
        return m_display;
    }
    Window root() {
        return m_root[screen()];
    }
    Window mroot(int i) {
        return m_root[i];
    }
    int screen() {
        return m_screenNumber;
    }
    int screensTotal() {
        return m_screensTotal;
    }
    void setScreen(int i) {
        m_screenNumber = i;
    }
    void setScreenFromRoot(Window);
    void setScreenFromPointer();

    int altModMask() {
        return m_altModMask;
    }

    enum RootCursor {
        NormalCursor, DeleteCursor, DownCursor, RightCursor, DownrightCursor
    };

    void installCursor(RootCursor);
    void installCursorOnWindow(RootCursor, Window);
    void installColormap(Colormap);
    unsigned long allocateColour(int, const char*, const char*);

    void considerFocusChange(Client*, Window, Time timestamp);
    void stopConsideringFocus();

    // shouldn't really be public
    int attemptGrab(Window, Window, int, int);
    int attemptGrabKey(Window, int);
    void releaseGrab(XButtonEvent*);
    void releaseGrabKeyMode(XButtonEvent*);
    void releaseGrabKeyMode(XKeyEvent*);
    void spawn(char*, char*);

    void setSignalled() {
        m_looping = False;
    } // ...

    ClientList& clients() {
        return m_clients;
    }
    ClientList& hiddenClients() {
        return m_hiddenClients;
    }

    void hoistToTop(Client*);
    void hoistToBottom(Client*);
    void removeFromOrderedList(Client*);
    Boolean isTop(Client*);

    // Instruct the X-Server to reorder the windows to match our ordering.
    // Takes account of layering.
    void updateStackingOrder();

    // for exposures during client grab, and window map/unmap/destroy during menu display:
    void dispatchEvent(XEvent*);

    // debug output:
    void printClientList();

    void netwmUpdateWindowList();
    void netwmUpdateStackingOrder();
    void netwmUpdateActiveClient();

    // Stupid little helper function
    static int numdigits(int);

private:
    int loop();
    void release();

    Display *m_display;
    int m_screenNumber;
    int m_screensTotal;

    Window *m_root;

    Colormap *m_defaultColormap;

    Cursor m_cursor;
    Cursor m_xCursor;
    Cursor m_vCursor;
    Cursor m_hCursor;
    Cursor m_vhCursor;

    char *m_terminal;
    char *m_shell;

    ClientList m_clients;
    ClientList m_hiddenClients;

    ClientList m_orderedClients[MAX_LAYER + 1];
    // One list for each netwm/MWM layer
    // Layer 4 (NORMAL_LAYER) is the default.
    Client *m_activeClient;

    int m_shapeEvent;
    int m_currentTime;

    Boolean m_looping;
    int m_returnCode;

    static Boolean m_initialising;
    static int errorHandler(Display*, XErrorEvent*);
    static void sigHandler(int);
    static int m_signalled;
    static int m_restart;

    void initialiseScreen();
    void scanInitialWindows();

    void circulate(Boolean activeFirst);

    Boolean m_focusChanging; // checking times for focus change
    Client *m_focusCandidate;
    Window m_focusCandidateWindow;
    Time m_focusTimestamp; // time of last crossing event
    Boolean m_focusPointerMoved;
    Boolean m_focusPointerNowStill;
    void checkDelaysForFocus();

    void nextEvent(XEvent*); // return

    void eventButton(XButtonEvent*, XEvent*);
    void eventKeyRelease(XKeyEvent*);
    void eventMapRequest(XMapRequestEvent*);
    void eventMap(XMapEvent*);
    void eventConfigureRequest(XConfigureRequestEvent*);
    void eventUnmap(XUnmapEvent*);
    void eventCreate(XCreateWindowEvent*);
    void eventDestroy(XDestroyWindowEvent*);
    void eventClient(XClientMessageEvent*);
    void eventColormap(XColormapEvent*);
    void eventProperty(XPropertyEvent*);
    void eventEnter(XCrossingEvent*);
    void eventReparent(XReparentEvent*);
    void eventFocusIn(XFocusInEvent*);
    void eventExposure(XExposeEvent*);

    Boolean m_altPressed;
    Boolean m_altStateRetained;
    void eventKeyPress(XKeyEvent*);

    void netwmInitialiseCompliance();
    Window m_netwmCheckWin;

    int m_altModMask;
};

#endif
