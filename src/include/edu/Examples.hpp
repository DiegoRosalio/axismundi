#pragma once

// The example gallery as data: each Example has a title, a category, a one-line description and a
// function that builds its scene through the App API. GalleryPanel shows them; the tour runs them.

#include <functional>
#include <string>
#include <vector>

namespace linalg {

class AppApi;

enum class ExampleCategory { Matrices, Complex, Plots };

struct Example {
    const char* title;
    ExampleCategory category;
    const char* description;  // what to look at, in a sentence or two
    std::function<void(AppApi&)> apply;
};

// The curated example list shown by the gallery
const std::vector<Example>& examples();
const char* categoryName(ExampleCategory category);

} // namespace linalg
