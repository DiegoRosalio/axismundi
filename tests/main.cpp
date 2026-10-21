// The test runner: every test function in tests/*.cpp, run in order, then the totals.
// A test is a function that calls CHECK(...) many times; one failing CHECK doesn't stop the others.
#include "TestSupport.hpp"

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const char* expr, const char* file, int line) {
    g_checks++;
    if (!ok) {
        g_failures++;
        std::printf("  FAIL %s:%d: %s\n", file, line, expr);
    }
}

// Declared here, defined in the topic files (test_linear.cpp, test_expr.cpp, ...)
void testVector();
void testMatrix3D();
void testMatrix4D();
void testLU();
void testQR();
void testSVD();
void testEigen();
void testComplex();
void testExpression();
void testPlayback();
void testBlendFunctions();
void testSceneTime();
void testReference();
void testContour();
void testTextLayout();
void testFieldVariable();
void testQuaternions();
void testIntegrals();
void testCurveGeometry();
void testOrbitCamera();
void testGridMath();
void testTaylor();
void testDerivatives();
void testFourier();
void testAllObjectTypes();
void testVectorField();
void testLabelLayout();
void testLatexExport();
void testPdfCompile();
void testUsability();
void testTemplates();
void testAnalysis();
void testExplain();
void testQuaternion();
void testBreakdown();
void testBreakdownPlayer();
void testStepTopics();
void testExprEngine();
void testScene();

int main() {
    struct Test { const char* name; std::function<void()> fn; };
    const Test tests[] = {
        {"Vector3D", testVector},
        {"Matrix3D", testMatrix3D},
        {"Matrix4D", testMatrix4D},
        {"LU", testLU},
        {"QR", testQR},
        {"SVD", testSVD},
        {"Eigen", testEigen},
        {"Complex", testComplex},
        {"Expression", testExpression},
        {"Playback", testPlayback},
        {"Blend functions", testBlendFunctions},
        {"Scene time", testSceneTime},
        {"Reference", testReference},
        {"Contour", testContour},
        {"Text layout", testTextLayout},
        {"Field variable z", testFieldVariable},
        {"Quaternions", testQuaternions},
        {"Integrals", testIntegrals},
        {"Curve geometry", testCurveGeometry},
        {"Orbit camera", testOrbitCamera},
        {"Grid math", testGridMath},
        {"Taylor, Newton", testTaylor},
        {"Derivatives", testDerivatives},
        {"Fourier", testFourier},
        {"All object types", testAllObjectTypes},
        {"Vector field", testVectorField},
        {"Label layout", testLabelLayout},
        {"LaTeX export", testLatexExport},
        {"PDF compile", testPdfCompile},
        {"Usability", testUsability},
        {"Add-menu templates", testTemplates},
        {"Analysis panel content", testAnalysis},
        {"Explain", testExplain},
        {"Quaternion", testQuaternion},
        {"Breakdown", testBreakdown},
        {"Breakdown player", testBreakdownPlayer},
        {"Steps topics", testStepTopics},
        {"ExprEngine", testExprEngine},
        {"Scene", testScene},
    };

    for (const auto& t : tests) {
        int before = g_failures;
        t.fn();
        std::printf("%-12s %s\n", t.name, g_failures == before ? "ok" : "FAILED");
    }
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
