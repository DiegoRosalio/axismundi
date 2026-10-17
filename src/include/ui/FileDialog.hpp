#pragma once

#include <raylib.h>
#include <string>
#include <vector>

namespace linalg {

// Modal "Open" / "Save" dialog: browse folders, pick or type a file name,
// recent files. Scenes are .txt (key = value); LaTeX figures are .tex.
class FileDialog {
public:
    enum class Mode { Open, Save };
    enum class Kind { Scene, Latex };

    void open(Mode mode, const std::string& folder, const std::string& name, const std::vector<std::string>& recent,
              Kind kind = Kind::Scene);
    Kind kind() const { return m_kind; }
    void close() { m_open = false; m_activeId = -1; }
    bool isOpen() const { return m_open; }
    bool isEditing() const { return m_activeId >= 0; }
    void cancelEditing() { m_activeId = -1; }
    Mode mode() const { return m_mode; }

    // Draws the dialog; returns true when the user confirmed (then path() is the file)
    bool draw();
    const std::string& path() const { return m_result; }
    const std::string& folder() const { return m_folder; }

private:
    void setFolder(const std::string& folder);
    bool confirm();  // validate the name; returns true when done

    bool m_open = false;
    bool m_justOpened = false;
    Mode m_mode = Mode::Open;
    Kind m_kind = Kind::Scene;
    const char* extension() const { return m_kind == Kind::Latex ? ".tex" : ".txt"; }
    const char* what() const { return m_kind == Kind::Latex ? "LaTeX figure" : "scene"; }
    std::string m_folder;
    char m_name[256] = "";
    std::vector<std::string> m_dirs, m_files, m_recent;
    std::string m_result;
    std::string m_message;
    bool m_messageIsError = false;
    bool m_confirmReplace = false;
    int m_activeId = -1;
    float m_scroll = 0.0f;
    double m_lastClickTime = 0.0;
    std::string m_lastClicked;
};

} // namespace linalg
