# Inserts a temporary lesson walk into src/app/Tour.cpp (restore the file afterwards)
import sys
p = sys.argv[1]
s = open(p).read()
walk = r'''    auto solve = [](App& app, size_t L, size_t K) { return solveTutorialStep(app, static_cast<int>(L), static_cast<int>(K)); };
    for (size_t L = 0; L < lessons().size(); L++) {
        for (size_t K = 0; K < lessons()[L].steps.size(); K++) {
            char nm[64];
            snprintf(nm, sizeof(nm), "L%02zu_%02zu", L, K);
            step(nm, [L, K](App& app) {
                if (auto* p = app.panel("tutorial")) p->setVisible(true);
                app.tutorialPanel().startLesson(static_cast<int>(L));
                app.tutorialPanel().goToStep(static_cast<int>(K));
            }, 8);
            step((std::string(nm) + "_sol").c_str(), [L, K, solve](App& app) {
                const auto& st = lessons()[L].steps[K];
                bool early = st.check && st.check(app, TutorialContext{});
                bool scripted = solve(app, L, K);
                fprintf(stderr, "WALK %zu.%zu task=%d early=%d scripted=%d %s\n", L, K, st.check ? 1 : 0, early ? 1 : 0, scripted ? 1 : 0, st.title);
            }, (std::string(lessons()[L].steps[K].title) == "Watch the steps") ? 500 : 6);
            step((std::string(nm) + "_chk").c_str(), [L, K](App& app) {
                const auto& st = lessons()[L].steps[K];
                fprintf(stderr, "SOLVED %zu.%zu %d\n", L, K, st.check && st.check(app, TutorialContext{}) ? 1 : 0);
            }, 1);
        }
    }
'''
anchor = '    step("00_'
i = s.find(anchor)
if i < 0:
    anchor = '    step("01_space_default"'
    i = s.find(anchor)
s = s[:i] + walk + s[i:]
if '#include "edu/Tutorial.hpp"' not in s:
    s = s.replace('#include "edu/Examples.hpp"', '#include "edu/Examples.hpp"\n#include "edu/Tutorial.hpp"', 1)
open(p, "w").write(s)
