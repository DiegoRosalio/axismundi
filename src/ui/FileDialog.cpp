#include "ui/FileDialog.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace linalg {

namespace {
constexpr int kNameId = 1;

std::string withExtension(std::string name, const char* ext) {
    if (fs::path(name).extension().empty()) name += ext;
    return name;
}
} // namespace

void FileDialog::open(Mode mode, const std::string& folder, const std::string& name,
                      const std::vector<std::string>& recent, Kind kind) {
    m_open = true;
    m_justOpened = true;
    m_mode = mode;
    m_kind = kind;
    m_recent = recent;
    m_message.clear();
    m_confirmReplace = false;
    m_activeId = mode == Mode::Save ? kNameId : -1;
    std::snprintf(m_name, sizeof(m_name), "%s", name.c_str());
    // The suggested name starts selected, so typing replaces it
    m_nameState = ui::EditorState{};
    m_nameState.id = kNameId;
    m_nameState.anchor = 0;
    m_nameState.cursor = std::strlen(m_name);
    std::error_code ec;
    std::string start = folder;
    if (start.empty() || !fs::is_directory(start, ec)) start = fs::current_path(ec).string();
    setFolder(start);
}

void FileDialog::setFolder(const std::string& folder) {
    std::error_code ec;
    fs::path p = fs::absolute(folder, ec).lexically_normal();
    m_folder = p.string();
    if (m_folder.size() > 1 && (m_folder.back() == '/' || m_folder.back() == '\\')) m_folder.pop_back();
    m_dirs.clear();
    m_files.clear();
    m_scroll = 0.0f;
    m_confirmReplace = false;
    for (fs::directory_iterator it(p, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end;
         it.increment(ec)) {
        std::string name = it->path().filename().string();
        if (name.empty() || name[0] == '.') continue;  // hidden files and folders
        std::error_code e2;
        if (it->is_directory(e2)) m_dirs.push_back(name);
        else if (it->path().extension() == extension()) m_files.push_back(name);
    }
    if (ec) {
        m_message = "Cannot read this folder: " + ec.message();
        m_messageIsError = true;
    }
    std::sort(m_dirs.begin(), m_dirs.end());
    std::sort(m_files.begin(), m_files.end());
}

bool FileDialog::confirm() {
    std::string name = m_name;
    while (!name.empty() && name.back() == ' ') name.pop_back();
    if (name.empty()) {
        m_message = "Type a file name or pick a file.";
        m_messageIsError = true;
        return false;
    }
    fs::path p(name);
    if (p.is_relative()) p = fs::path(m_folder) / p;
    std::error_code ec;
    if (fs::is_directory(p, ec)) {  // a typed folder: go there
        setFolder(p.string());
        m_name[0] = '\0';
        return false;
    }
    if (m_mode == Mode::Save) {
        p = withExtension(p.string(), extension());
        if (!fs::is_directory(p.parent_path(), ec)) {
            m_message = "The folder " + p.parent_path().string() + " does not exist.";
            m_messageIsError = true;
            return false;
        }
        if (fs::exists(p, ec) && !m_confirmReplace) {
            m_confirmReplace = true;
            m_message = p.filename().string() + " already exists. Click Replace to overwrite it.";
            m_messageIsError = true;
            return false;
        }
    } else if (!fs::exists(p, ec)) {
        m_message = p.filename().string() + " does not exist in this folder.";
        m_messageIsError = true;
        return false;
    }
    m_result = p.lexically_normal().string();
    m_open = false;
    m_activeId = -1;
    return true;
}

bool FileDialog::draw() {
    if (!m_open) return false;
    const ui::Theme& t = ui::theme();
    bool done = false;

    // Dim everything behind: the dialog is modal
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, t.dark ? 0.45f : 0.25f));
    float w = std::min(ui::px(580.0f), GetScreenWidth() - ui::px(32.0f));
    float h = std::min(ui::px(520.0f), GetScreenHeight() - ui::px(32.0f));
    Rectangle box = {(GetScreenWidth() - w) / 2, (GetScreenHeight() - h) / 2, w, h};
    ui::card(box, true);
    ui::Column col(box.x + ui::px(18), box.y + ui::px(16), box.width - ui::px(36), 8.0f);

    std::string title = std::string(m_mode == Mode::Open ? "Open " : "Save ") + what();
    ui::textIn(col.row(26), title.c_str(), ui::FontRole::Title, t.text);

    // Folder and Up
    Rectangle folderRow = col.row(30.0f);
    Rectangle up = ui::takeLeft(folderRow, 30.0f, 8.0f);
    std::error_code ec;
    fs::path parent = fs::path(m_folder).parent_path();
    bool canGoUp = !parent.empty() && parent.string() != m_folder;
    if (ui::iconButton(up, ui::Icon::ChevronLeft, false, canGoUp) && canGoUp) setFolder(parent.string());
    ui::tooltip(up, "Parent folder", "Up");
    std::string shown = m_folder;
    while (shown.size() > 4 && ui::textWidth(shown.c_str(), ui::FontRole::Mono) > folderRow.width)
        shown = "…" + shown.substr(std::min<size_t>(shown.size(), 4));
    ui::textIn(folderRow, shown.c_str(), ui::FontRole::Mono, t.textMuted);
    ui::tooltip(folderRow, m_folder.c_str(), "Folder");

    // Folders and scene files, scrollable
    float recentH = m_recent.empty() ? 0.0f : ui::px(22.0f) + ui::px(26.0f) * std::min<size_t>(m_recent.size(), 4);
    float listH = (box.y + box.height) - col.y() - ui::px(16) - ui::px(34 + 8 + 30 + 8 + 22 + 8) - recentH;
    listH = std::max(listH, ui::px(80.0f));
    Rectangle list = {col.x(), col.y(), col.width(), listH};
    ui::fillRounded(list, ui::px(ui::kRadius), t.panelAlt);
    ui::strokeRounded(list, ui::px(ui::kRadius), ui::pxi(1.0f), t.border);
    float rowH = ui::px(28.0f);
    size_t count = m_dirs.size() + m_files.size();
    float contentH = rowH * static_cast<float>(count) + ui::px(8);
    if (CheckCollisionPointRec(GetMousePosition(), list)) m_scroll -= GetMouseWheelMove() * rowH * 2;
    m_scroll = std::clamp(m_scroll, 0.0f, std::max(0.0f, contentH - list.height));
    BeginScissorMode(static_cast<int>(list.x), static_cast<int>(list.y), static_cast<int>(list.width),
                     static_cast<int>(list.height));
    ui::setClip(list);
    double now = GetTime();
    for (size_t i = 0; i < count; i++) {
        bool isDir = i < m_dirs.size();
        const std::string& name = isDir ? m_dirs[i] : m_files[i - m_dirs.size()];
        Rectangle r = {list.x + ui::px(4), list.y + ui::px(4) + rowH * i - m_scroll, list.width - ui::px(8), rowH};
        if (r.y + r.height < list.y || r.y > list.y + list.height) continue;
        bool selected = !isDir && name == m_name;
        bool hover = ui::hovered(r);
        if (selected || hover) ui::fillRounded(r, ui::px(5.0f), selected ? t.accentSoft : t.hover);
        Rectangle icon = {r.x + ui::px(6), r.y + ui::px(6), ui::px(16), ui::px(16)};
        ui::drawIcon(isDir ? ui::Icon::Load : ui::Icon::Save, icon, isDir ? t.accent : t.textMuted);
        ui::textIn({r.x + ui::px(30), r.y, r.width - ui::px(36), r.height},
                   (isDir ? name + "/" : name).c_str(), isDir ? ui::FontRole::BodyStrong : ui::FontRole::Body, t.text);
        if (ui::clicked(r)) {
            if (isDir) {
                setFolder((fs::path(m_folder) / name).string());
                break;
            }
            bool doubleClick = (m_lastClicked == name && now - m_lastClickTime < 0.4);
            std::snprintf(m_name, sizeof(m_name), "%s", name.c_str());
            m_confirmReplace = false;
            m_message.clear();
            m_lastClicked = name;
            m_lastClickTime = now;
            if (doubleClick) done = confirm();
        }
    }
    ui::clearClip();
    EndScissorMode();
    if (count == 0)
        ui::textIn(list, (std::string("No folders or ") + what() + " files (" + extension() + ") here").c_str(),
                   ui::FontRole::Small, t.textFaint, ui::Align::Center);
    col.setY(list.y + list.height + ui::px(8));

    // Recent files
    if (!m_recent.empty()) {
        ui::textIn(col.row(18), ui::upper("Recent").c_str(), ui::FontRole::Caption, t.textMuted);
        for (size_t i = 0; i < m_recent.size() && i < 4; i++) {
            Rectangle r = col.row(22.0f);
            bool hover = ui::hovered(r);
            if (hover) ui::fillRounded(r, ui::px(5.0f), t.hover);
            std::string label = m_recent[i];
            while (label.size() > 4 && ui::textWidth(label.c_str(), ui::FontRole::Small) > r.width - ui::px(12))
                label = "…" + label.substr(4);
            ui::textIn({r.x + ui::px(6), r.y, r.width - ui::px(12), r.height}, label.c_str(), ui::FontRole::Small,
                       t.text);
            if (ui::clicked(r)) {
                fs::path p(m_recent[i]);
                setFolder(p.parent_path().string());
                std::snprintf(m_name, sizeof(m_name), "%s", p.filename().string().c_str());
                if (m_mode == Mode::Open) done = confirm();
            }
        }
        col.space(4.0f);
    }

    // File name
    Rectangle nameRow = col.row(34.0f);
    Rectangle nameLabel = ui::takeLeft(nameRow, 76.0f, 8.0f);
    ui::textIn(nameLabel, "File name", ui::FontRole::Body, t.textMuted);
    std::string nameText = m_name;
    ui::EditorEvent ev = ui::textEditor(nameRow, kNameId, nameText, m_activeId == kNameId,
                                        m_messageIsError && !m_message.empty(), m_nameState);
    std::snprintf(m_name, sizeof(m_name), "%s", nameText.c_str());
    if (ev == ui::EditorEvent::Activated) m_activeId = kNameId;
    if (ev == ui::EditorEvent::Commit) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) done = confirm();
        else m_activeId = -1;  // a click elsewhere in the dialog
    }
    std::string nameTip = m_mode == Mode::Save
                              ? std::string("A name in this folder (") + extension() + " is added), or a full path."
                              : std::string("A file in this folder, or a full path.");
    ui::tooltip(nameRow, nameTip.c_str(), "File name");

    // Message and buttons
    Rectangle bottom = col.row(32.0f);
    Rectangle ok = ui::takeRight(bottom, 110.0f, 0.0f);
    Rectangle cancel = ui::takeRight(bottom, 96.0f, 8.0f);
    if (!m_message.empty())
        ui::textIn(bottom, ui::ellipsize(m_message.c_str(), ui::FontRole::Small, bottom.width).c_str(),
                   ui::FontRole::Small, m_messageIsError ? t.danger : t.textMuted);
    if (ui::button(cancel, "Cancel")) close();
    const char* okLabel = m_mode == Mode::Open ? "Open" : (m_confirmReplace ? "Replace" : "Save");
    if (ui::button(ok, okLabel, m_confirmReplace ? ui::ButtonKind::Danger : ui::ButtonKind::Primary)) done = confirm();

    // A click outside the dialog cancels it (not on the frame that opened it)
    if (!m_justOpened && ui::mousePressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), box))
        close();
    m_justOpened = false;
    return done;
}

} // namespace linalg
