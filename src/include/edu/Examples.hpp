#pragma once

#include <functional>
#include <string>
#include <vector>

namespace linalg {

class App;

enum class ExampleCategory { Matrices, Complex, Plots };

struct Example {
    const char* title;
    ExampleCategory category;
    const char* description;  // what to look at, in a sentence or two
    std::function<void(App&)> apply;
};

// The curated example list shown by the gallery
const std::vector<Example>& examples();
const char* categoryName(ExampleCategory category);

} // namespace linalg
