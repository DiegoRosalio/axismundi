#pragma once

#include "math/Matrix4D.hpp"
#include "rendering/PlotRenderer.hpp"
#include "scene/Scene.hpp"
#include "ui/ViewPanel.hpp"
#include <map>
#include <vector>

namespace linalg::view {

struct DrawOptions {
    const ViewSettings* view = nullptr;
    const scene::Selection* selection = nullptr;
    // Show this matrix object with a different value (animation, breakdown)
    scene::ObjectId overrideMatrix = scene::kNoObject;
    Matrix4D overrideValue = Matrix4D::identity();
    // Faded bases for completed breakdown steps
    std::vector<Matrix4D> ghosts;
};

// Draws the objects of a space. 3D space: math coordinates with z up (see
// WorldMapping.hpp). Complex plane: real part along world X, imaginary along Y.
class SceneView {
public:
    // Call inside BeginMode3D
    void draw3D(scene::Space& space, const DrawOptions& options);
    // Call after EndMode3D
    void drawLabels(scene::Space& space, const DrawOptions& options, const Camera3D& camera);

    // Theme palette color for an object (or its custom color)
    static Color colorOf(const scene::SceneObject& object);
    // Value a matrix object is drawn with (taking the override into account)
    static Matrix4D displayedMatrix(const scene::SceneObject& object, const DrawOptions& options);

    // World position of an object's handle (arrow tip, point), for labels and picking
    static bool anchorPoint(const scene::SceneObject& object, scene::SpaceKind kind, const DrawOptions& options,
                            Vector3& out);

    // Plot status (error text and value range) for the inspector
    const PlotRenderer* plot(scene::ObjectId id) const;

private:
    void drawEuclidean(scene::Space& space, const DrawOptions& options);
    void drawComplex(scene::Space& space, const DrawOptions& options);
    void updatePlots(scene::Space& space);

    struct PlotCache {
        PlotRenderer renderer;
        std::uint64_t builtAt = 0;
        bool dark = true;
        unsigned color = 0;
    };
    std::map<scene::ObjectId, PlotCache> m_plots;
};

} // namespace linalg::view
