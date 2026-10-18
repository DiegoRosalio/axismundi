// The table of per-type view handlers (see ObjectViews.hpp); the entries live in view/objects/*.cpp
#include "view/ObjectViews.hpp"

namespace linalg::view {

const ObjectView* objectView(const std::string& typeId) {
    // Built the first time it is needed; `static` keeps it for the rest of the program
    static const ObjectViewTable table = [] {
        ObjectViewTable t;
        registerBasicViews(t);
        registerComplexViews(t);
        registerQuaternionViews(t);
        registerCalculusViews(t);
        registerFourierViews(t);
        return t;
    }();
    auto it = table.find(typeId);
    return it == table.end() ? nullptr : &it->second;
}

} // namespace linalg::view
