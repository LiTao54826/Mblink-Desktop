/**
 * @file test_use_effect.cpp
 * @brief useEffect Hook测试
 */

#include <gtest/gtest.h>
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/component_manager.h"
#include "core/quickjs/hooks_bindings.h"
#include <fstream>
#include <thread>
#include <chrono>

using namespace lightui;

class UseEffectTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime = std::make_shared<QuickJSRuntime>();
        
        // Load hooks.js
        std::ifstream hooks_file("../js/preact/hooks.js");
        if (hooks_file.is_open()) {
            std::stringstream buffer;
            buffer << hooks_file.rdbuf();
            runtime->Eval(buffer.str(), "hooks.js");
        }
    }
    
    void TearDown() override {
        runtime.reset();
    }
    
    std::shared_ptr<QuickJSRuntime> runtime;
};

TEST_F(UseEffectTest, BasicEffectExecution) {
    // Create a component
    auto* comp = ComponentManager::CreateComponent();
    ComponentManager::SetCurrentComponent(comp->GetId());
    
    // Test useEffect execution
    std::string code = R"(
        let effectRan = false;
        PreactHooks.useEffect(() => {
            effectRan = true;
        }, []);
    )";
    
    runtime->Eval(code, "test");
    
    // Process event loop to run effects
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runtime->ProcessTasks();
    
    auto result = runtime->Eval("effectRan;", "test");
    EXPECT_EQ(result, true);
    
    ComponentManager::ClearCurrentComponent();
}

TEST_F(UseEffectTest, EffectWithDependencies) {
    // Create a component
    auto* comp = ComponentManager::CreateComponent();
    ComponentManager::SetCurrentComponent(comp->GetId());
    
    // Initialize with dependencies
    std::string code = R"(
        let count = 0;
        let dep = 1;
        
        PreactHooks.useEffect(() => {
            count++;
        }, [dep]);
    )";
    
    runtime->Eval(code, "test");
    
    // Process event loop
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runtime->ProcessTasks();
    
    auto result = runtime->Eval("count;", "test");
    EXPECT_EQ(result, 1);
    
    ComponentManager::ClearCurrentComponent();
}

TEST_F(UseEffectTest, EffectCleanup) {
    // Create a component
    auto* comp = ComponentManager::CreateComponent();
    ComponentManager::SetCurrentComponent(comp->GetId());
    
    // Test cleanup function
    std::string code = R"(
        let cleanupRan = false;
        
        PreactHooks.useEffect(() => {
            return () => {
                cleanupRan = true;
            };
        }, []);
    )";
    
    runtime->Eval(code, "test");
    
    // Process event loop to run effect
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runtime->ProcessTasks();
    
    // Cleanup effects
    comp->CleanupEffects();
    
    auto result = runtime->Eval("cleanupRan;", "test");
    EXPECT_EQ(result, true);
    
    ComponentManager::ClearCurrentComponent();
}

TEST_F(UseEffectTest, MultipleEffects) {
    // Create a component
    auto* comp = ComponentManager::CreateComponent();
    ComponentManager::SetCurrentComponent(comp->GetId());
    
    // Multiple effects
    std::string code = R"(
        let effect1Ran = false;
        let effect2Ran = false;
        let effect3Ran = false;
        
        PreactHooks.useEffect(() => {
            effect1Ran = true;
        }, []);
        
        PreactHooks.useEffect(() => {
            effect2Ran = true;
        }, []);
        
        PreactHooks.useEffect(() => {
            effect3Ran = true;
        }, []);
    )";
    
    runtime->Eval(code, "test");
    
    // Process event loop
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runtime->ProcessTasks();
    
    auto result1 = runtime->Eval("effect1Ran;", "test");
    auto result2 = runtime->Eval("effect2Ran;", "test");
    auto result3 = runtime->Eval("effect3Ran;", "test");
    
    EXPECT_EQ(result1, true);
    EXPECT_EQ(result2, true);
    EXPECT_EQ(result3, true);
    
    ComponentManager::ClearCurrentComponent();
}

TEST_F(UseEffectTest, EffectWithChangingDependencies) {
    // Create a component
    auto* comp = ComponentManager::CreateComponent();
    ComponentManager::SetCurrentComponent(comp->GetId());
    
    // Effect with changing dependencies
    std::string code = R"(
        let runCount = 0;
        let dep = 1;
        
        PreactHooks.useEffect(() => {
            runCount++;
        }, [dep]);
    )";
    
    runtime->Eval(code, "test");
    
    // Process event loop
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runtime->ProcessTasks();
    
    auto result1 = runtime->Eval("runCount;", "test");
    EXPECT_EQ(result1, 1);
    
    // Reset hook index and change dependency
    comp->ResetHookIndex();
    runtime->Eval("dep = 2;", "test");
    runtime->Eval(R"(
        PreactHooks.useEffect(() => {
            runCount++;
        }, [dep]);
    )", "test");
    
    // Process event loop again
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runtime->ProcessTasks();
    
    auto result2 = runtime->Eval("runCount;", "test");
    EXPECT_EQ(result2, 2);
    
    ComponentManager::ClearCurrentComponent();
}

TEST_F(UseEffectTest, EffectRunsOnlyOnce) {
    // Create a component
    auto* comp = ComponentManager::CreateComponent();
    ComponentManager::SetCurrentComponent(comp->GetId());
    
    // Effect with empty deps array should run only once
    std::string code = R"(
        let runCount = 0;
        
        PreactHooks.useEffect(() => {
            runCount++;
        }, []);
    )";
    
    runtime->Eval(code, "test");
    
    // Process event loop
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runtime->ProcessTasks();
    
    auto result1 = runtime->Eval("runCount;", "test");
    EXPECT_EQ(result1, 1);
    
    // Reset hook index and run again (simulating re-render)
    comp->ResetHookIndex();
    runtime->Eval(R"(
        PreactHooks.useEffect(() => {
            runCount++;
        }, []);
    )", "test");
    
    // Process event loop again
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runtime->ProcessTasks();
    
    // Should still be 1 because deps didn't change
    auto result2 = runtime->Eval("runCount;", "test");
    EXPECT_EQ(result2, 1);
    
    ComponentManager::ClearCurrentComponent();
}

