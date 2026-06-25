/**
 * @file native_data_binding.h
 * @brief Stage 1 MVP 原生数据绑定运行时
 */

#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/bridge/state_manager.h"
#include "core/dom/element.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/node.h"
#include "nlohmann/json.hpp"

namespace mblink {

using json = nlohmann::json;

enum class NativeBindingKind {
    Text,
    Attr,
    Visible,
    ModelValue,
    ModelChecked,
};

struct ScopeSlot {
    std::string graphPath;
    bool readOnly = false;
    bool declarative = false;
};

struct ScopeRef {
    uint64_t scopeId = 0;
    uintptr_t ownerElementId = 0;
    uint64_t parentScopeId = 0;
    std::unordered_map<std::string, ScopeSlot> aliases;
    bool readOnly = false;
};

struct ScopeChain {
    uint64_t scopeChainId = 0;
    std::vector<uint64_t> scopeIds;
    uintptr_t ownerElementId = 0;
};

struct BindingRecord {
    uint64_t bindingId = 0;
    NativeBindingKind kind = NativeBindingKind::Text;
    std::weak_ptr<Node> targetNode;
    std::weak_ptr<Element> ownerElement;
    uintptr_t targetNodeId = 0;
    uintptr_t ownerElementId = 0;
    uint64_t scopeChainId = 0;
    std::string sourcePath;
    std::string resolvedPath;
    std::string attrName;
    bool mounted = false;
    bool dirty = false;
    uint64_t orderIndex = 0;
    uint64_t lastAppliedRevision = 0;
    json lastRenderedValue = nullptr;
    std::vector<uint64_t> subscriptionIds;
    bool writeBackGuard = false;
    bool readOnlyResolved = false;
    uint64_t listenerId = 0;
    std::string declarativeKey;
};

struct DirtyBindingQueue {
    std::set<std::pair<uint64_t, uint64_t>> pendingBindingIds;
    bool scheduled = false;
    uint32_t batchDepth = 0;
    uint64_t pendingRevisionMax = 0;
};

struct FlushContext {
    bool isFlushing = false;
    uint64_t flushEpoch = 0;
    uint64_t currentBindingId = 0;
    std::set<std::pair<uint64_t, uint64_t>> deferredBindingIds;
    uint32_t reentrantMutationCount = 0;
};

class NativeDataBindingRuntime {
public:
    explicit NativeDataBindingRuntime(StateManager& stateManager);

    uint64_t createScope(const std::shared_ptr<Element>& ownerElement,
                         const std::unordered_map<std::string, ScopeSlot>& aliases,
                         bool readOnly = false);
    uint64_t createTextBinding(const std::shared_ptr<Element>& ownerElement,
                               const std::shared_ptr<Node>& targetNode,
                               const std::string& sourcePath);
    uint64_t createAttrBinding(const std::shared_ptr<Element>& ownerElement,
                               const std::shared_ptr<Element>& targetElement,
                               const std::string& attrName,
                               const std::string& sourcePath);
    uint64_t createVisibleBinding(const std::shared_ptr<Element>& ownerElement,
                                  const std::shared_ptr<Element>& targetElement,
                                  const std::string& sourcePath);
    uint64_t createModelValueBinding(const std::shared_ptr<Element>& ownerElement,
                                     const std::shared_ptr<HTMLInputElement>& input,
                                     const std::string& sourcePath);
    uint64_t createModelCheckedBinding(const std::shared_ptr<Element>& ownerElement,
                                       const std::shared_ptr<HTMLInputElement>& input,
                                       const std::string& sourcePath);

    bool mountBinding(uint64_t bindingId);
    bool unmountBinding(uint64_t bindingId);
    void clear();
    uint32_t mountDeclarative(const std::shared_ptr<Element>& rootElement);
    uint32_t refreshDeclarative(const std::shared_ptr<Element>& rootElement);
    uint32_t unmountDeclarative(const std::shared_ptr<Element>& rootElement);
    uint32_t flush();
    uint32_t beginBatch();
    uint32_t endBatch();
    const std::vector<std::string>& errors() const { return errors_; }

private:
    static constexpr uint32_t kMaxFlushRounds = 32;

    uint64_t createBinding(const std::shared_ptr<Element>& ownerElement,
                           const std::shared_ptr<Node>& targetNode,
                           NativeBindingKind kind,
                           const std::string& sourcePath,
                           const std::string& attrName = {},
                           const std::string& declarativeKey = {});
    uint32_t mountDeclarativeRecursive(const std::shared_ptr<Element>& element);
    uint64_t mountDeclarativeBinding(const std::shared_ptr<Element>& ownerElement,
                                     const std::shared_ptr<Node>& targetNode,
                                     NativeBindingKind kind,
                                     const std::string& sourcePath,
                                     const std::string& attrName,
                                     const std::string& declarativeKey);
    bool destroyBinding(uint64_t bindingId);
    uint32_t destroyDeclarativeSubtree(const std::shared_ptr<Element>& element);
    void enqueueBinding(uint64_t bindingId, uint64_t revision);
    ScopeChain buildScopeChain(const std::shared_ptr<Element>& ownerElement);
    std::pair<std::string, bool> resolvePath(const ScopeChain& chain, const std::string& sourcePath) const;
    bool applyBinding(BindingRecord& binding);
    bool handleModelEvent(BindingRecord& binding, const std::shared_ptr<HTMLInputElement>& input);
    void recordError(const std::string& message);
    static bool isScalarLike(const json& value);

    StateManager& stateManager_;
    uint64_t nextScopeId_ = 1;
    uint64_t nextScopeChainId_ = 1;
    uint64_t nextBindingId_ = 1;
    uint64_t nextOrderIndex_ = 1;
    std::unordered_map<uint64_t, ScopeRef> scopes_;
    std::unordered_map<uintptr_t, uint64_t> elementToScope_;
    std::unordered_map<uint64_t, BindingRecord> bindings_;
    std::unordered_map<uintptr_t, std::unordered_set<uint64_t>> declarativeBindingsByOwnerElement_;
    std::unordered_set<std::string> declarativeBindingKeys_;
    DirtyBindingQueue dirtyQueue_;
    FlushContext flushContext_;
    std::vector<std::string> errors_;
};

} // namespace mblink