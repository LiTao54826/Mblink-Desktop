#include "core/quickjs/quickjs_runtime.h"
#include <cstdio>

using namespace lightui;

int main() {
    try {
        printf("Creating QuickJS runtime...\n");
        QuickJSRuntime runtime;

        printf("Evaluating: 1 + 1\n");
        auto result = runtime.Eval("1 + 1");
        printf("Result: %s\n", result.dump().c_str());

        printf("Test passed!\n");
        return 0;
    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
}

