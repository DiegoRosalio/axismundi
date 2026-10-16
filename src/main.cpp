#include "app/App.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace linalg;

namespace {

void printUsage(const char* program) {
    std::printf("usage: %s [--size WxH] [--tour OUTPUT_DIR]\n", program);
    std::printf("  --size WxH     initial window size (default 1600x900)\n");
    std::printf("  --tour DIR     save screenshots of a scripted UI tour to DIR, then exit\n");
}

} // namespace

int main(int argc, char** argv) {
    AppOptions options;

    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            int w = 0, h = 0;
            if (std::sscanf(argv[++i], "%dx%d", &w, &h) == 2 && w >= 320 && h >= 240) {
                options.width = w;
                options.height = h;
            } else {
                std::fprintf(stderr, "invalid --size, expected WxH\n");
                return 1;
            }
        } else if (std::strcmp(argv[i], "--tour") == 0 && i + 1 < argc) {
            options.tourDir = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            printUsage(argv[0]);
            return 1;
        }
    }

    App app(options);
    app.run();
    return 0;
}
