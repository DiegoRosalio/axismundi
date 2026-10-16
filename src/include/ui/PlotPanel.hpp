#pragma once

#include "rendering/PlotRenderer.hpp"
#include "ui/SceneIO.hpp"
#include <vector>

namespace linalg {

// Side bar content for the function plotting mode.
class PlotPanel {
public:
    PlotPanel();

    void update();  // Tab navigation
    // `error` and the value range come from the renderer, shown under the inputs
    float draw(float x, float y, float width, const PlotRenderer& renderer);

    PlotSpec spec() const;
    PlotType type() const { return static_cast<PlotType>(m_type); }

    bool changed() const { return m_changed; }
    void clearChanged() { m_changed = false; }

    bool isEditing() const { return m_activeField >= 0; }
    void finishEditing();

    void saveState(SceneData& data) const;
    void loadState(const SceneData& data);

private:
    static constexpr int kExprSize = 128;
    static constexpr int kNumSize = 32;
    // Field ids: 0-2 expressions, 10-15 range bounds
    static constexpr int kRangeBase = 10;
    enum Range { XMin, XMax, YMin, YMax, TMin, TMax, RangeCount };

    char* exprText(int slot);
    std::vector<int> visibleFields() const;
    void finishField(int id);
    void loadExample();
    void syncRangeText();

    float drawRange(float x, float y, float inner, const char* name, Range lo, Range hi);

    int m_type;
    // Expressions per plot type (kept separately so switching types keeps them)
    char m_function2D[kExprSize];
    char m_param[3][kExprSize];
    char m_surface[kExprSize];
    char m_implicit[kExprSize];

    float m_range[RangeCount];
    char m_rangeText[RangeCount][kNumSize];
    float m_resolution;
    bool m_heatmap;

    int m_exampleIndex[kPlotTypeCount];
    int m_activeField;
    bool m_changed;
};

} // namespace linalg
