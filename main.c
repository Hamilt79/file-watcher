#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_XLIB_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_xlib.h"
#include "file_utils.h"

#define DTIME 20
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct XWindow
{
    Display *dpy;
    Window root;
    Visual *vis;
    Colormap cmap;
    XWindowAttributes attr;
    XSetWindowAttributes swa;
    Window win;
    int screen;
    XFont *font;
    unsigned int width;
    unsigned int height;
    Atom wm_delete_window;
} XWindow;

typedef struct Instance
{
    char *filePath;
    char *fileText;
    char **lines;
    unsigned splitCount;
} Instance;

static Instance NewInstance()
{
    Instance instance;
    instance.fileText = NULL;
    instance.filePath = NULL;
    instance.lines = NULL;
    instance.splitCount = 0;
    return instance;
}

static void CleanFilePath(Instance *instance)
{
    unsigned long size = strlen(instance->filePath);
    instance->filePath = realloc(instance->filePath, sizeof(char) * size);
    if (size > 0ul)
    {
        instance->filePath[size - 1] = '\0';
    }
}

static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

static long timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
        return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec / 1000);
}

static void sleep_for(long t)
{
    struct timespec req;
    const time_t sec = (int)(t / 1000);
    const long ms = t - (sec * 1000);
    req.tv_sec = sec;
    req.tv_nsec = ms * 1000000L;
    while (-1 == nanosleep(&req, &req))
        ;
}

static char *get_command_out(const char *command, const unsigned size)
{
    char *path = (char *)malloc(sizeof(char) * size);
    FILE *fp;
    unsigned i;
    char pathTemp[size];

    for (i = 0; i < size; i++)
    {
        path[i] = '\0';
    }
    fp = popen(command, "r");
    if (fp == NULL)
    {
        return path;
    }
    if (fgets(pathTemp, size, fp) == NULL)
    {
        pclose(fp);
        return path;
    }
    pclose(fp);
    strcpy(path, pathTemp);
    return path;
}

int main(void)
{
    long dt;
    long started;
    int running = 1;
    XWindow xw;
    struct nk_context *ctx;
    XEvent evt;
    Instance instance = NewInstance();
    unsigned long linesIndex = 0;

    /* X11 */
    memset(&xw, 0, sizeof xw);
    xw.dpy = XOpenDisplay(NULL);
    if (!xw.dpy)
        die("Could not open a display; perhaps $DISPLAY is not set?");
    xw.root = DefaultRootWindow(xw.dpy);
    xw.screen = XDefaultScreen(xw.dpy);
    xw.vis = XDefaultVisual(xw.dpy, xw.screen);
    xw.cmap = XCreateColormap(xw.dpy, xw.root, xw.vis, AllocNone);

    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask | ButtonMotionMask |
        Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask |
        PointerMotionMask | KeymapStateMask;
    xw.win = XCreateWindow(xw.dpy, xw.root, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
                           XDefaultDepth(xw.dpy, xw.screen), InputOutput,
                           xw.vis, CWEventMask | CWColormap, &xw.swa);

    XStoreName(xw.dpy, xw.win, "X11");
    XMapWindow(xw.dpy, xw.win);
    xw.wm_delete_window = XInternAtom(xw.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(xw.dpy, xw.win, &xw.wm_delete_window, 1);
    XGetWindowAttributes(xw.dpy, xw.win, &xw.attr);
    xw.width = (unsigned int)xw.attr.width;
    xw.height = (unsigned int)xw.attr.height;

    /* GUI */
    xw.font = nk_xfont_create(xw.dpy, "fixed");
    ctx = nk_xlib_init(xw.font, xw.dpy, xw.screen, xw.win, xw.width, xw.height);

    while (running)
    {
        XGetWindowAttributes(xw.dpy, xw.win, &xw.attr);
        xw.width = (unsigned int)xw.attr.width;
        xw.height = (unsigned int)xw.attr.height;
        started = timestamp();
        nk_input_begin(ctx);
        while (XPending(xw.dpy))
        {
            XNextEvent(xw.dpy, &evt);
            if (evt.type == ClientMessage)
                goto cleanup;
            if (XFilterEvent(&evt, xw.win))
                continue;
            nk_xlib_handle_event(xw.dpy, xw.screen, xw.win, &evt);
        }

        nk_input_end(ctx);

        /* GUI */
        /*
           if (nk_begin(ctx, "Demo", nk_rect(50, 50, 200, 200),
           NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
           NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
           {
           */
        free(NULL);
        if (nk_begin(ctx, "Main", nk_rect(0, 0, xw.width, xw.height), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
        {
            if (instance.filePath == NULL)
            {
                nk_layout_row_static(ctx, 30, 80, 1);
                if (nk_button_label(ctx, "Select File"))
                {
                    static const unsigned SIZE = 1000;
                    char *path = get_command_out("zenity --file-selection 2>> /dev/null", SIZE);
                    instance.filePath = path;
                    CleanFilePath(&instance);
                    printf("%s\n", instance.filePath);
                    instance.splitCount = GetLinesFromFile(instance.filePath, &(instance.lines), &(instance.fileText));
                    printf("%d\n", instance.splitCount);
                }
            }
            else
            {
                FreeLines(&(instance.lines), &(instance.fileText));
                instance.splitCount = GetLinesFromFile(instance.filePath, &(instance.lines), &(instance.fileText));
            }
            for (linesIndex = 0; linesIndex < instance.splitCount; linesIndex++)
            {
                nk_layout_row_dynamic(ctx, 10.0, 1);
                nk_text(ctx, instance.lines[linesIndex], strlen(instance.lines[linesIndex]), NK_TEXT_ALIGN_LEFT);
            }
        }
        nk_end(ctx);
        if (nk_window_is_hidden(ctx, "Main"))
            break;

        /* Draw */
        XClearWindow(xw.dpy, xw.win);
        nk_xlib_render(xw.win, nk_rgb(30, 30, 30));
        XFlush(xw.dpy);

        /* Timing */
        dt = timestamp() - started;
        if (dt < DTIME)
            sleep_for(DTIME - dt);
    }

cleanup:
    free(instance.filePath);
    FreeLines(&(instance.lines), &(instance.fileText));
    nk_xfont_del(xw.dpy, xw.font);
    nk_xlib_shutdown();
    XUnmapWindow(xw.dpy, xw.win);
    XFreeColormap(xw.dpy, xw.cmap);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}
