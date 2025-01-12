#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

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
    instance->filePath = realloc(instance->filePath, sizeof(char) * size + 1);
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
    char *path = malloc(sizeof(char) * size);
    memset(path, '\0', size);
    FILE *fp;
    fp = popen(command, "r");
    if (fp == NULL)
    {
        return path;
    }
    if (fgets(path, size - 1, fp) == NULL)
    {
        pclose(fp);
        return path;
    }
    pclose(fp);
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

    XStoreName(xw.dpy, xw.win, "File Watcher");
    XMapWindow(xw.dpy, xw.win);
    xw.wm_delete_window = XInternAtom(xw.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(xw.dpy, xw.win, &xw.wm_delete_window, 1);
    XGetWindowAttributes(xw.dpy, xw.win, &xw.attr);
    xw.width = (unsigned int)xw.attr.width;
    xw.height = (unsigned int)xw.attr.height;

    /* GUI */
    setlocale(LC_ALL, "");
    xw.font = nk_xfont_create(xw.dpy, "fixed");
    ctx = nk_xlib_init(xw.font, xw.dpy, xw.screen, xw.win, xw.width, xw.height);
    enum nk_collapse_states optionsState = NK_MAXIMIZED;
    const float optionsHeightClosed = 25;
    const float optionsHeightOpen = 115;
    float optionsHeight = optionsHeightOpen;
    nk_bool autoScroll = false;
    nk_bool lineNumbers = true;
    nk_bool wrap = false;
    // struct nk_font_atlas *atlas;
    // struct nk_font_config config = nk_font_config(14);

    // config.oversample_h = 1;
    // config.oversample_v = 1;
    // config.range = nk_font_cyrillic_glyph_ranges();

    // nk_sdl_font_stash_begin(&atlas);
    //  struct nk_font *ubuntu = nk_font_atlas_add_from_file(atlas, "path_to_your_font", 14, &config)
    //  nk_sdl_font_stash_end();
    //  nk_style_set_font(ctx, &ubuntu->handle);
    //  struct nk_font_atlas *atlas;
    //  struct nk_font_config cfg = nk_font_config(0);
    //  struct nk_font *font;
    //  cfg.range = nk_font_cyrillic_glyph_ranges();
    //  /* assign Glyph ranges, disable oversampling, enable pixel snapping */
    //  cfg.oversample_h = cfg.oversample_v = 1;
    //  cfg.pixel_snap = true;
    //  font = nk_font_atlas_add_from_file(NULL, "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 10.0f, &cfg);
    //  nk_style_set_font(ctx, &font->handle);

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
        if (nk_begin(ctx, "Main", nk_rect(0, 0, xw.width, xw.height), 0))
        {
            nk_layout_row_dynamic(ctx, optionsHeight, 1);
            if (nk_group_begin(ctx, "optionsgroup", NK_WINDOW_NO_SCROLLBAR))
            {
                if (nk_tree_state_push(ctx, NK_TREE_TAB, "Options", &optionsState))
                {
                    nk_layout_row_static(ctx, 30, 80, 1);
                    if (nk_button_label(ctx, "Select File"))
                    {
                        FreeLines(&(instance.lines), &(instance.fileText));
                        free(instance.filePath);
                        instance.filePath = NULL;
                        static const unsigned SIZE = 1000;
                        char *path = get_command_out("zenity --file-selection 2>> /dev/null", SIZE);
                        instance.filePath = path;
                        if (instance.filePath != NULL)
                        {
                            CleanFilePath(&instance);
                            if (DoesFileExist(instance.filePath))
                            {
                                printf("%s\n", instance.filePath);
                                instance.splitCount = GetLinesFromFile(instance.filePath, &(instance.lines), &(instance.fileText));
                                printf("%d\n", instance.splitCount);
                                optionsState = NK_MINIMIZED;
                            }
                        }
                    }
                    nk_layout_row_static(ctx, 15, 80, 1);
                    nk_checkbox_label(ctx, "Auto-Scroll", &autoScroll);
                    nk_checkbox_label(ctx, "Wrap", &wrap);
                    nk_checkbox_label(ctx, "Line Numbers", &lineNumbers);
                    optionsHeight = optionsHeightOpen;
                    nk_tree_pop(ctx);
                }
                else
                {
                    optionsHeight = optionsHeightClosed;
                }
                nk_group_end(ctx);
            }
            nk_layout_row_dynamic(ctx, xw.height - optionsHeight - 22.0f, 1);
            if (nk_group_begin(ctx, "textgroup", 0))
            {
                if (instance.filePath != NULL)
                {
                    FreeLines(&(instance.lines), &(instance.fileText));
                    instance.splitCount = GetLinesFromFile(instance.filePath, &(instance.lines), &(instance.fileText));
                }
                char lineNum[sizeof(long) + 3];
                if (!wrap)
                {
                    int width = 0;
                    for (linesIndex = 0; linesIndex < instance.splitCount; linesIndex++)
                    {
                        int len;
                        if (lineNumbers)
                        {
                            nk_itoa(lineNum, (long)linesIndex);
                            strcat(lineNum, ": ");
                            len = (strlen(lineNum) + strlen(instance.lines[linesIndex])) * 7;
                        }
                        else
                        {
                            len = strlen(instance.lines[linesIndex]) * 7;
                        }
                        if (len > width)
                        {
                            width = len;
                        }
                    }
                    nk_layout_row_static(ctx, 10.0, width * .9, 1);
                    for (linesIndex = 0; linesIndex < instance.splitCount; linesIndex++)
                    {
                        if (lineNumbers)
                        {
                            nk_itoa(lineNum, (long)linesIndex);
                            strcat(lineNum, ": ");
                            char line[strlen(lineNum) + strlen(instance.lines[linesIndex]) + 1];
                            strcpy(line, lineNum);
                            strcat(line, instance.lines[linesIndex]);
                            nk_text(ctx, line, strlen(line), NK_TEXT_ALIGN_LEFT);
                        }
                        else
                        {
                            nk_text(ctx, instance.lines[linesIndex], strlen(instance.lines[linesIndex]), NK_TEXT_ALIGN_LEFT);
                        }
                    }
                }
                else
                {
                    nk_layout_row_dynamic(ctx, 10.0, 1);
                    for (linesIndex = 0; linesIndex < instance.splitCount; linesIndex++)
                    {
                        int charSize;
                        if (lineNumbers)
                        {
                            // Size of long and 3 bytes for :, space, and null term
                            charSize = (sizeof(long) + 3) + strlen(instance.lines[linesIndex]);
                        }
                        else
                        {
                            charSize = strlen(instance.lines[linesIndex]) + 1;
                        }
                        char line[charSize];
                        if (lineNumbers)
                        {
                            nk_itoa(lineNum, (long)linesIndex);
                            strcat(lineNum, ": ");
                            // char line[(strlen(lineNum) + strlen(instance.lines[linesIndex] + 1))];
                            strcpy(line, lineNum);
                            strcat(line, instance.lines[linesIndex]);
                        }
                        else
                        {
                            strcpy(line, instance.lines[linesIndex]);
                        }

                        int len = strlen(line);
                        int size = (xw.width - 30) * .166;
                        if (len > size)
                        {
                            // printf("%s\n", instance.lines[linesIndex]);
                            int loops = len / size;
                            int rem = len % size;
                            char *ptr = line;
                            for (int i = 1; i <= loops; i++)
                            {
                                nk_text(ctx, ptr, size, NK_TEXT_ALIGN_LEFT);
                                ptr = line + (i * size);
                            }
                            if (rem > 0)
                            {
                                nk_text(ctx, ptr, rem, NK_TEXT_ALIGN_LEFT);
                            }
                        }
                        else
                        {
                            nk_text(ctx, line, strlen(line), NK_TEXT_ALIGN_LEFT);
                        }
                    }
                }

                if (autoScroll)
                {
                    nk_uint x;
                    nk_group_get_scroll(ctx, "textgroup", &x, NULL);
                    nk_group_set_scroll(ctx, "textgroup", x, 99999999);
                }
                nk_group_end(ctx);
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
