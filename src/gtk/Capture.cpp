#include "gtk/Capture.hpp"
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif
#include <vector>

namespace linalg::gtk {

namespace {

// Through GTK: the widget drawn into a render node, turned into a texture by the window's renderer. Popovers
// and other separate surfaces are not part of it.
bool saveThroughGtk(GtkWidget* widget, const std::string& path) {
    int w = gtk_widget_get_width(widget), h = gtk_widget_get_height(widget);
    GtkNative* native = gtk_widget_get_native(widget);
    if (w <= 0 || h <= 0 || !native) return false;
    GdkPaintable* paintable = gtk_widget_paintable_new(widget);
    GtkSnapshot* snapshot = gtk_snapshot_new();
    gdk_paintable_snapshot(paintable, GDK_SNAPSHOT(snapshot), w, h);
    GskRenderNode* node = gtk_snapshot_free_to_node(snapshot);
    bool ok = false;
    if (node) {
        graphene_rect_t area = GRAPHENE_RECT_INIT(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
        GdkTexture* texture = gsk_renderer_render_texture(gtk_native_get_renderer(native), node, &area);
        if (texture) {
            ok = gdk_texture_save_to_png(texture, path.c_str());
            g_object_unref(texture);
        }
        gsk_render_node_unref(node);
    }
    g_object_unref(paintable);
    return ok;
}

#ifdef GDK_WINDOWING_X11
// On X11: the screen pixels where the window is, so popovers, menus and toasts over it are included. GTK 4.18
// deprecated its X11 calls (X11 is on its way out); this is only for screenshots under Xvfb, so the warning is off here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
bool saveFromX11Screen(GtkWidget* window, const std::string& path) {
    GdkDisplay* display = gtk_widget_get_display(window);
    GtkNative* native = gtk_widget_get_native(window);
    if (!GDK_IS_X11_DISPLAY(display) || !native) return false;
    Display* x = gdk_x11_display_get_xdisplay(display);
    Window xid = gdk_x11_surface_get_xid(gtk_native_get_surface(native));
    XWindowAttributes attr;
    if (!XGetWindowAttributes(x, xid, &attr) || attr.width <= 0 || attr.height <= 0) return false;
    Window root = DefaultRootWindow(x), child = 0;
    int rx = 0, ry = 0;
    XTranslateCoordinates(x, xid, root, 0, 0, &rx, &ry, &child);
    XSync(x, False);
    XImage* image = XGetImage(x, root, rx, ry, static_cast<unsigned>(attr.width), static_cast<unsigned>(attr.height),
                              AllPlanes, ZPixmap);
    if (!image) return false;
    std::vector<unsigned char> rgb(static_cast<size_t>(attr.width) * attr.height * 3);
    for (int py = 0; py < attr.height; py++)
        for (int px = 0; px < attr.width; px++) {
            unsigned long p = XGetPixel(image, px, py);
            unsigned char* out = &rgb[(static_cast<size_t>(py) * attr.width + px) * 3];
            out[0] = static_cast<unsigned char>((p & image->red_mask) >> 16);
            out[1] = static_cast<unsigned char>((p & image->green_mask) >> 8);
            out[2] = static_cast<unsigned char>(p & image->blue_mask);
        }
    XDestroyImage(image);
    GBytes* bytes = g_bytes_new(rgb.data(), rgb.size());
    GdkTexture* texture = gdk_memory_texture_new(attr.width, attr.height, GDK_MEMORY_R8G8B8, bytes,
                                                 static_cast<gsize>(attr.width) * 3);
    bool ok = gdk_texture_save_to_png(texture, path.c_str());
    g_object_unref(texture);
    g_bytes_unref(bytes);
    return ok;
}
#pragma GCC diagnostic pop
#endif

} // namespace

bool saveWidgetPng(GtkWidget* widget, const std::string& path) {
#ifdef GDK_WINDOWING_X11
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    if (GTK_IS_WINDOW(widget) && saveFromX11Screen(widget, path)) return true;
#pragma GCC diagnostic pop
#endif
    return saveThroughGtk(widget, path);
}

} // namespace linalg::gtk
