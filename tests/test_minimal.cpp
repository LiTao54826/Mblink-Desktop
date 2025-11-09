#include <cstdio>
#include "json.hpp"

int main() {
    printf("Test 1\n");
    nlohmann::json j;
    j["test"] = 123;
    printf("Test 2: %s\n", j.dump().c_str());
    return 0;
}

