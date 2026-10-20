#include "ui/AppIcon.hpp"
#include "generated/icon.inc"

namespace linalg::ui {

void setWindowIcons() {
    struct Png {
        const unsigned char* data;
        int size;
    };
    const Png pngs[] = {{kIconPng16, kIconPng16Size}, {kIconPng32, kIconPng32Size}, {kIconPng48, kIconPng48Size},
                        {kIconPng64, kIconPng64Size}, {kIconPng128, kIconPng128Size}};
    Image images[5];
    int count = 0;
    for (const Png& p : pngs) {
        Image img = LoadImageFromMemory(".png", p.data, p.size);
        if (img.data) images[count++] = img;
    }
    if (count > 0) SetWindowIcons(images, count);  // the window manager picks the size it needs
    for (int i = 0; i < count; i++) UnloadImage(images[i]);
}

void drawAppIcon(Rectangle r) {
    // Made the first time it's drawn (needs the window); freed with the window when the app closes
    static Texture2D texture{};
    static bool tried = false;
    if (!tried) {
        tried = true;
        Image img = LoadImageFromMemory(".png", kIconPng128, kIconPng128Size);
        if (img.data) {
            texture = LoadTextureFromImage(img);
            UnloadImage(img);
            GenTextureMipmaps(&texture);  // smooth when shrunk to ~20 px
            SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        }
    }
    if (texture.id == 0) return;
    Rectangle src = {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)};
    DrawTexturePro(texture, src, r, {0, 0}, 0.0f, WHITE);
}

} // namespace linalg::ui
