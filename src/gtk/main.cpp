// axismundi, GTK4 front end (work in progress, docs/gtk-port.md): options, the application, the main window
#include "app/Version.hpp"
#include "gtk/Capture.hpp"
#include "gtk/MainWindow.hpp"
#include "gtk/Tour.hpp"
#include <adwaita.h>
#include <glib-unix.h>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

using namespace linalg;

namespace {

struct Options {
    int width = 1280, height = 800;
    std::string theme;  // "", "dark", "light"
    std::string tour;   // screenshot directory
    std::string file;   // a scene to open
};

void usage(const char* program) {
    std::printf("usage: %s [--size WxH] [--theme dark|light] [--tour OUTPUT_DIR] [--version] [SCENE.txt]\n", program);
}

// SIGUSR1 saves signal_NNN.png in the working folder (the input tests use it, as with the raylib app)
gboolean onShotSignal(gpointer data) {
    static int count = 0;
    char name[32];
    std::snprintf(name, sizeof(name), "signal_%03d.png", count++);
    gtk::saveWidgetPng(static_cast<gtk::MainWindow*>(data)->widget(), name);
    return G_SOURCE_CONTINUE;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            std::sscanf(argv[++i], "%dx%d", &options.width, &options.height);
        } else if (std::strcmp(argv[i], "--theme") == 0 && i + 1 < argc) {
            options.theme = argv[++i];
        } else if (std::strcmp(argv[i], "--tour") == 0 && i + 1 < argc) {
            options.tour = argv[++i];
        } else if (std::strcmp(argv[i], "--version") == 0) {
            std::printf("axismundi %s\n", kVersion);
            return 0;
        } else if (argv[i][0] != '-' && options.file.empty()) {
            options.file = argv[i];
        } else {
            usage(argv[0]);
            return std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0 ? 0 : 1;
        }
    }

    // A tour runs next to other instances; normally a second launch raises the open window (GNOME style)
    auto flags = options.tour.empty() ? Gio::Application::Flags::DEFAULT_FLAGS : Gio::Application::Flags::NON_UNIQUE;
    auto app = Gtk::Application::create("com.bisonanon.axismundi", flags);
    std::unique_ptr<gtk::MainWindow> window;
    std::unique_ptr<gtk::Tour> tour;

    app->signal_startup().connect([] { adw_init(); });
    app->signal_activate().connect([&] {
        if (window) {
            window->present();
            return;
        }
        window = std::make_unique<gtk::MainWindow>(app, options.width, options.height);
        if (options.tour.empty()) window->loadSettings();  // the tour keeps the defaults
        if (!options.theme.empty()) window->setTheme(options.theme);
        gtk_window_set_default_icon_name("com.bisonanon.axismundi");
        window->present();
#ifdef SIGUSR1
        g_unix_signal_add(SIGUSR1, onShotSignal, window.get());
#endif
        // Start empty, as the raylib app does; the tour keeps the demo scene so its screenshots repeat
        if (options.tour.empty()) {
            window->session().scene().clearAll();
            window->session().scene().clearHistory();
            window->sceneChanged();
        }
        if (!options.file.empty()) window->openFile(options.file);
        if (!options.tour.empty()) {
            tour = std::make_unique<gtk::Tour>(*window, app, options.tour);
            tour->start();
        }
    });
    // Our options are parsed above; GApplication only sees the program name
    char* appArgv[] = {argv[0], nullptr};
    return app->run(1, appArgv);
}
