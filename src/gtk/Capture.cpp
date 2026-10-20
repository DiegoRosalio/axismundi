#include "gtk/Capture.hpp"
#include <gtk/gtk.h>

namespace linalg::gtk {

bool saveWidgetPng(GtkWidget* widget, const std::string& path) {
    int w = gtk_widget_get_width(widget), h = gtk_widget_get_height(widget);
    GtkNative* native = gtk_widget_get_native(widget);
    if (w <= 0 || h <= 0 || !native) return false;
    // Draw the widget into a render node, then let the window's renderer turn it into a texture
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

} // namespace linalg::gtk
