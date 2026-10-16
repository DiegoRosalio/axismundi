#pragma once

#include "rendering/ArgandRenderer.hpp"
#include "ui/SceneIO.hpp"

namespace linalg {

// Side bar content for the complex numbers mode: inputs for z1 and z2,
// rectangular/polar results for their product, and Euler's formula controls.
class ComplexPanel {
public:
    ComplexPanel();

    void update(float deltaTime);  // Tab navigation, θ animation
    float draw(float x, float y, float width);

    const ComplexScene& scene() const { return m_scene; }
    bool isEditing() const { return m_activeField >= 0; }
    void finishEditing();

    void saveState(SceneData& data) const;
    void loadState(const SceneData& data);

private:
    static constexpr int kTextSize = 32;
    static constexpr int kFieldCount = 4;  // z1.re, z1.im, z2.re, z2.im

    float& fieldValue(int id);
    void finishField(int id);
    void syncText();

    ComplexScene m_scene;
    char m_text[kFieldCount][kTextSize];
    int m_activeField;
    bool m_animateTheta;
};

} // namespace linalg
