/**
 * @file test_quickjs_runtime.cpp
 * @brief QuickJS Runtime Tests
 */

#include "core/quickjs/quickjs_runtime.h"
#include <iostream>
#include <cassert>

using namespace lightui;

void test_basic_eval() {
    std::cout << "Test: Basic Eval..." << std::endl;
    QuickJSRuntime runtime;
    
    // Test simple expression
    auto result = runtime.Eval("1 + 1");
    assert(result.is_number());
    assert(result.get<int>() == 2);
    
    // Test string
    result = runtime.Eval("'Hello, World!'");
    assert(result.is_string());
    assert(result.get<std::string>() == "Hello, World!");
    
    std::cout << "  ✓ Basic eval passed" << std::endl;
}

void test_variables() {
    std::cout << "Test: Variables..." << std::endl;
    QuickJSRuntime runtime;
    
    runtime.Eval("var x = 42;");
    auto result = runtime.Eval("x");
    assert(result.is_number());
    assert(result.get<int>() == 42);
    
    std::cout << "  ✓ Variables passed" << std::endl;
}

void test_functions() {
    std::cout << "Test: Functions..." << std::endl;
    QuickJSRuntime runtime;
    
    runtime.Eval("function add(a, b) { return a + b; }");
    
    json args = json::array({5, 3});
    auto result = runtime.CallFunction("add", args);
    assert(result.is_number());
    assert(result.get<int>() == 8);
    
    std::cout << "  ✓ Functions passed" << std::endl;
}

void test_arrays() {
    std::cout << "Test: Arrays..." << std::endl;
    QuickJSRuntime runtime;
    
    auto result = runtime.Eval("[1, 2, 3, 4, 5]");
    assert(result.is_array());
    assert(result.size() == 5);
    assert(result[0].get<int>() == 1);
    assert(result[4].get<int>() == 5);
    
    std::cout << "  ✓ Arrays passed" << std::endl;
}

void test_objects() {
    std::cout << "Test: Objects..." << std::endl;
    QuickJSRuntime runtime;
    
    auto result = runtime.Eval("({name: 'John', age: 30})");
    assert(result.is_object());
    assert(result["name"].get<std::string>() == "John");
    assert(result["age"].get<int>() == 30);
    
    std::cout << "  ✓ Objects passed" << std::endl;
}

void test_global_properties() {
    std::cout << "Test: Global Properties..." << std::endl;
    QuickJSRuntime runtime;
    
    runtime.SetGlobalProperty("myValue", 123);
    auto result = runtime.GetGlobalProperty("myValue");
    assert(result.is_number());
    assert(result.get<int>() == 123);
    
    runtime.SetGlobalProperty("myString", "test");
    result = runtime.GetGlobalProperty("myString");
    assert(result.is_string());
    assert(result.get<std::string>() == "test");
    
    std::cout << "  ✓ Global properties passed" << std::endl;
}

void test_native_functions() {
    std::cout << "Test: Native Functions..." << std::endl;
    QuickJSRuntime runtime;
    
    // Register a native function
    runtime.RegisterFunction("multiply", [](const json& args) -> json {
        if (args.is_array() && args.size() >= 2) {
            return args[0].get<int>() * args[1].get<int>();
        }
        return 0;
    });
    
    auto result = runtime.Eval("multiply(6, 7)");
    assert(result.is_number());
    assert(result.get<int>() == 42);
    
    std::cout << "  ✓ Native functions passed" << std::endl;
}

void test_type_conversion() {
    std::cout << "Test: Type Conversion..." << std::endl;
    QuickJSRuntime runtime;
    
    // Test null
    auto result = runtime.Eval("null");
    assert(result.is_null());
    
    // Test boolean
    result = runtime.Eval("true");
    assert(result.is_boolean());
    assert(result.get<bool>() == true);
    
    result = runtime.Eval("false");
    assert(result.is_boolean());
    assert(result.get<bool>() == false);
    
    // Test numbers
    result = runtime.Eval("42");
    assert(result.is_number());
    
    result = runtime.Eval("3.14");
    assert(result.is_number());
    
    std::cout << "  ✓ Type conversion passed" << std::endl;
}

void test_error_handling() {
    std::cout << "Test: Error Handling..." << std::endl;
    QuickJSRuntime runtime;
    
    try {
        runtime.Eval("throw new Error('Test error');");
        assert(false && "Should have thrown an exception");
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());
        assert(error_msg.find("Test error") != std::string::npos);
    }
    
    std::cout << "  ✓ Error handling passed" << std::endl;
}

void test_complex_operations() {
    std::cout << "Test: Complex Operations..." << std::endl;
    QuickJSRuntime runtime;

    // Test array operations
    runtime.Eval("var arr = [1, 2, 3, 4, 5];");
    auto result = runtime.Eval("arr.map(x => x * 2)");
    assert(result.is_array());
    assert(result[0].get<int>() == 2);
    assert(result[4].get<int>() == 10);

    // Test object operations
    runtime.Eval("var obj = {a: 1, b: 2};");
    result = runtime.Eval("Object.keys(obj)");
    assert(result.is_array());
    assert(result.size() == 2);

    std::cout << "  ✓ Complex operations passed" << std::endl;
}

void test_console_api() {
    std::cout << "Test: Console API..." << std::endl;
    QuickJSRuntime runtime;

    // Test console.log
    std::cout << "  Testing console.log:" << std::endl;
    runtime.Eval("console.log('Hello from JavaScript!')");
    runtime.Eval("console.log('Multiple', 'arguments', 123, true)");
    runtime.Eval("console.log({name: 'test', value: 42})");

    // Test console.error
    std::cout << "  Testing console.error:" << std::endl;
    runtime.Eval("console.error('This is an error message')");

    // Test console.warn
    std::cout << "  Testing console.warn:" << std::endl;
    runtime.Eval("console.warn('This is a warning')");

    // Test console.info
    std::cout << "  Testing console.info:" << std::endl;
    runtime.Eval("console.info('This is info')");

    // Test console in function
    runtime.Eval("function testFunc() { console.log('Inside function'); }");
    runtime.Eval("testFunc()");

    // Test console with expressions
    runtime.Eval("console.log('2 + 2 =', 2 + 2)");
    runtime.Eval("console.log('Array:', [1, 2, 3])");

    std::cout << "  ✓ Console API passed" << std::endl;
}

void test_module_loading() {
    std::cout << "Test: Module Loading..." << std::endl;
    QuickJSRuntime runtime;

    // Test 1: Register and load a simple module
    std::cout << "  Testing simple module registration:" << std::endl;
    runtime.RegisterModule("math",
        "export const PI = 3.14159;\n"
        "export function add(a, b) { return a + b; }\n"
    );

    // Test 2: Load module using import in module context
    std::cout << "  Testing module import:" << std::endl;
    runtime.RegisterModule("test_import",
        "import { PI, add } from 'math';\n"
        "globalThis.testPI = PI;\n"
        "globalThis.testAdd = add;\n"
        "export const result = add(5, 3);\n"
    );

    runtime.LoadModule("test_import");

    auto pi_value = runtime.GetGlobalProperty("testPI");
    assert(pi_value.is_number());
    assert(pi_value.get<double>() > 3.14 && pi_value.get<double>() < 3.15);

    // Test module function
    json args = json::array({5, 3});
    auto result = runtime.CallFunction("testAdd", args);
    assert(result.is_number());
    assert(result.get<int>() == 8);

    // Test 3: Module with default export
    std::cout << "  Testing default export:" << std::endl;
    runtime.RegisterModule("utils",
        "export default function greet(name) { return 'Hello, ' + name; }\n"
    );

    runtime.RegisterModule("test_default",
        "import greet from 'utils';\n"
        "globalThis.greet = greet;\n"
    );

    runtime.LoadModule("test_default");
    json greet_args = json::array({"World"});
    result = runtime.CallFunction("greet", greet_args);
    assert(result.is_string());
    assert(result.get<std::string>() == "Hello, World");

    // Test 4: Module with multiple exports
    std::cout << "  Testing multiple exports:" << std::endl;
    runtime.RegisterModule("data",
        "export const name = 'LightUI';\n"
        "export const version = '0.1.0';\n"
        "export const features = ['QuickJS', 'SDL3', 'Skia'];\n"
    );

    runtime.RegisterModule("test_data",
        "import { name, version, features } from 'data';\n"
        "globalThis.projectName = name;\n"
        "globalThis.projectVersion = version;\n"
        "globalThis.projectFeatures = features;\n"
    );

    runtime.LoadModule("test_data");

    auto name = runtime.GetGlobalProperty("projectName");
    assert(name.is_string());
    assert(name.get<std::string>() == "LightUI");

    auto features = runtime.GetGlobalProperty("projectFeatures");
    assert(features.is_array());
    assert(features.size() == 3);

    // Test 5: Module importing another module
    std::cout << "  Testing module dependencies:" << std::endl;
    runtime.RegisterModule("base",
        "export const BASE_URL = 'https://example.com';\n"
    );

    runtime.RegisterModule("api",
        "import { BASE_URL } from 'base';\n"
        "export function getApiUrl(path) { return BASE_URL + path; }\n"
    );

    runtime.RegisterModule("test_api",
        "import { getApiUrl } from 'api';\n"
        "globalThis.getApiUrl = getApiUrl;\n"
    );

    runtime.LoadModule("test_api");
    json api_args = json::array({"/users"});
    result = runtime.CallFunction("getApiUrl", api_args);
    assert(result.is_string());
    assert(result.get<std::string>() == "https://example.com/users");

    std::cout << "  ✓ Module loading passed" << std::endl;
}

void test_async_timers() {
    std::cout << "Test: Async Timers..." << std::endl;
    QuickJSRuntime runtime;

    // Test 1: setTimeout basic functionality
    std::cout << "  Testing setTimeout:" << std::endl;
    runtime.Eval("globalThis.timeoutExecuted = false;");
    runtime.Eval("setTimeout(() => { globalThis.timeoutExecuted = true; }, 50);");

    // Run event loop
    runtime.RunEventLoop(100);

    auto result = runtime.GetGlobalProperty("timeoutExecuted");
    assert(result.is_boolean());
    assert(result.get<bool>() == true);

    // Test 2: setTimeout with arguments
    std::cout << "  Testing setTimeout with arguments:" << std::endl;
    runtime.Eval("globalThis.timeoutResult = 0;");
    runtime.Eval("setTimeout((a, b) => { globalThis.timeoutResult = a + b; }, 50, 10, 20);");

    runtime.RunEventLoop(100);

    result = runtime.GetGlobalProperty("timeoutResult");
    assert(result.is_number());
    assert(result.get<int>() == 30);

    // Test 3: Multiple timeouts
    std::cout << "  Testing multiple timeouts:" << std::endl;
    runtime.Eval("globalThis.order = [];");
    runtime.Eval("setTimeout(() => { globalThis.order.push(1); }, 100);");
    runtime.Eval("setTimeout(() => { globalThis.order.push(2); }, 50);");
    runtime.Eval("setTimeout(() => { globalThis.order.push(3); }, 150);");

    runtime.RunEventLoop(200);

    result = runtime.GetGlobalProperty("order");
    assert(result.is_array());
    assert(result.size() == 3);
    assert(result[0].get<int>() == 2);  // 50ms
    assert(result[1].get<int>() == 1);  // 100ms
    assert(result[2].get<int>() == 3);  // 150ms

    // Test 4: setInterval
    std::cout << "  Testing setInterval:" << std::endl;
    runtime.Eval("globalThis.intervalCount = 0;");
    runtime.Eval("globalThis.intervalId = setInterval(() => { globalThis.intervalCount++; }, 30);");

    runtime.RunEventLoop(100);

    result = runtime.GetGlobalProperty("intervalCount");
    assert(result.is_number());
    assert(result.get<int>() >= 2);  // Should execute at least 2-3 times

    // Clear the interval
    runtime.Eval("clearInterval(globalThis.intervalId);");

    // Test 5: clearTimeout
    std::cout << "  Testing clearTimeout:" << std::endl;
    runtime.Eval("globalThis.shouldNotExecute = false;");
    runtime.Eval("const tid = setTimeout(() => { globalThis.shouldNotExecute = true; }, 50);");
    runtime.Eval("clearTimeout(tid);");

    runtime.RunEventLoop(100);

    result = runtime.GetGlobalProperty("shouldNotExecute");
    assert(result.is_boolean());
    assert(result.get<bool>() == false);

    std::cout << "  ✓ Async timers passed" << std::endl;
}

void test_async_promises() {
    std::cout << "Test: Async Promises..." << std::endl;
    QuickJSRuntime runtime;

    // Test 1: Basic Promise
    std::cout << "  Testing basic Promise:" << std::endl;
    runtime.Eval("globalThis.promiseResult = null;");
    runtime.Eval(R"(
        const p = new Promise((resolve) => {
            setTimeout(() => resolve(42), 50);
        });
        p.then(value => { globalThis.promiseResult = value; });
    )");

    runtime.RunEventLoop(100);

    auto result = runtime.GetGlobalProperty("promiseResult");
    assert(result.is_number());
    assert(result.get<int>() == 42);

    // Test 2: Promise chain
    std::cout << "  Testing Promise chain:" << std::endl;
    runtime.Eval("globalThis.chainResult = null;");
    runtime.Eval(R"(
        Promise.resolve(10)
            .then(x => x * 2)
            .then(x => x + 5)
            .then(x => { globalThis.chainResult = x; });
    )");

    runtime.RunEventLoop(10);

    result = runtime.GetGlobalProperty("chainResult");
    assert(result.is_number());
    assert(result.get<int>() == 25);  // (10 * 2) + 5

    // Test 3: Promise.all
    std::cout << "  Testing Promise.all:" << std::endl;
    runtime.Eval("globalThis.allResult = null;");
    runtime.Eval(R"(
        Promise.all([
            Promise.resolve(1),
            Promise.resolve(2),
            Promise.resolve(3)
        ]).then(values => { globalThis.allResult = values; });
    )");

    runtime.RunEventLoop(10);

    result = runtime.GetGlobalProperty("allResult");
    assert(result.is_array());
    assert(result.size() == 3);
    assert(result[0].get<int>() == 1);
    assert(result[1].get<int>() == 2);
    assert(result[2].get<int>() == 3);

    // Test 4: Promise with setTimeout
    std::cout << "  Testing Promise with setTimeout:" << std::endl;
    runtime.Eval("globalThis.mixedResult = [];");
    runtime.Eval(R"(
        setTimeout(() => { globalThis.mixedResult.push('timeout'); }, 50);
        Promise.resolve().then(() => { globalThis.mixedResult.push('promise'); });
    )");

    runtime.RunEventLoop(100);

    result = runtime.GetGlobalProperty("mixedResult");
    assert(result.is_array());
    assert(result.size() == 2);
    // Promise (microtask) should execute before setTimeout (macrotask)
    assert(result[0].get<std::string>() == "promise");
    assert(result[1].get<std::string>() == "timeout");

    std::cout << "  ✓ Async promises passed" << std::endl;
}

void test_filesystem_module_loading() {
    std::cout << "Test: Filesystem Module Loading..." << std::endl;
    QuickJSRuntime runtime;

    // Test loading module from filesystem with relative imports
    std::cout << "  Testing module file loading with relative imports:" << std::endl;
    
    try {
        // Load main.js which imports from ./math.js
        // Use absolute path for reliability
        runtime.LoadModuleFile("c:/Users/Administrator/Desktop/code/MBink/examples/module_test/main.js");

        // Verify the imported values
        auto results = runtime.GetGlobalProperty("testResults");
        assert(results.is_object());
        
        // Check imported function results
        assert(results["add"].get<int>() == 8);      // 5 + 3
        assert(results["square"].get<int>() == 25);  // 5 * 5 (uses nested multiply)
        assert(results["half"].get<int>() == 5);     // 10 / 2 (uses nested divide)
        
        // Check imported constant
        auto pi = results["pi"].get<double>();
        assert(pi > 3.14 && pi < 3.15);

        std::cout << "  ✓ Filesystem module loading passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Filesystem module loading failed: " << e.what() << std::endl;
        throw;
    }
}

void test_folder_imports() {
    std::cout << "Test: Folder Imports..." << std::endl;
    QuickJSRuntime runtime;

    // Test folder imports (index.js and package.json)
    std::cout << "  Testing folder imports (index.js and package.json):" << std::endl;
    
    try {
        // Load folder-test.js which imports from folders
        runtime.LoadModuleFile("c:/Users/Administrator/Desktop/code/MBink/examples/module_test/folder-test.js");

        // Verify the imported values
        auto results = runtime.GetGlobalProperty("folderImportResults");
        assert(results.is_object());
        
        // Check values from ./utils (index.js)
        assert(results["utilName"].get<std::string>() == "Utils Package");
        assert(results["message"].get<std::string>() == "[UTILS] Test");
        
        // Check values from ./config (package.json -> config-main.js)
        assert(results["appName"].get<std::string>() == "My Application");
        assert(results["configEnv"].get<std::string>() == "production");

        std::cout << "  ✓ Folder imports passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Folder imports failed: " << e.what() << std::endl;
        throw;
    }
}


int main() {
    std::cout << "=== QuickJS Runtime Tests ===" << std::endl << std::endl;

    try {
        test_basic_eval();
        test_variables();
        test_functions();
        test_arrays();
        test_objects();
        test_global_properties();
        test_native_functions();
        test_type_conversion();
        test_error_handling();
        test_complex_operations();
        test_console_api();
        test_module_loading();
        test_filesystem_module_loading();
        test_folder_imports();
        test_async_timers();
        test_async_promises();

        std::cout << std::endl << "=== All Tests Passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

