/**
 * @file native_data_binding.cpp
 * @brief Stage 1 MVP 原生数据绑定运行时
 */

#include "native_data_binding.h"

#include "core/dom/event.h"
#include "core/dom/text.h"

#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace mblink {

namespace {

constexpr const char* kMbTextAttr = "mb-text";
constexpr const char* kMbVisibleAttr = "mb-visible";
constexpr const char* kMbModelAttr = "mb-model";
constexpr const char* kMbAttrPrefix = "mb-attr:";
constexpr const char* kMbScopePrefix = "mb-scope:";
constexpr const char* kMbScopeReadOnlyPrefix = "mb-scope-ro:";

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> segments;
    size_t start = 0;
    while (start < path.size()) {
        const size_t pos = path.find('.', start);
        const std::string segment = path.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
        if (segment.empty()) return {};
        segments.push_back(segment);
        if (pos == std::string::npos) break;
        start = pos + 1;
    }
    return segments;
}

std::string stringifyScalar(const json& value) {
    if (value.is_string()) return value.get<std::string>();
    if (value.is_boolean()) return value.get<bool>() ? "true" : "false";
    if (value.is_number_integer()) return std::to_string(value.get<int64_t>());
    if (value.is_number_unsigned()) return std::to_string(value.get<uint64_t>());
    if (value.is_number_float()) {
        std::ostringstream oss;
        oss << value.get<double>();
        return oss.str();
    }
    return {};
}

bool hasPrefix(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

std::string makeDeclarativeBindingKey(uintptr_t targetNodeId, NativeBindingKind kind, const std::string& attrName) {
    return std::to_string(targetNodeId) + "|" + std::to_string(static_cast<int>(kind)) + "|" + attrName;
}

} // namespace

NativeDataBindingRuntime::NativeDataBindingRuntime(StateManager& stateManager)
    : stateManager_(stateManager) {
}

uint64_t NativeDataBindingRuntime::createScope(const std::shared_ptr<Element>& ownerElement,
                                               const std::unordered_map<std::string, ScopeSlot>& aliases,
                                               bool readOnly) {
    if (!ownerElement) return 0;

    const uintptr_t ownerElementId = reinterpret_cast<uintptr_t>(ownerElement.get());
    auto existingIt = elementToScope_.find(ownerElementId);
    if (existingIt != elementToScope_.end()) {
        auto scopeIt = scopes_.find(existingIt->second);
        if (scopeIt != scopes_.end()) {
            scopeIt->second.readOnly = scopeIt->second.readOnly || readOnly;
            for (const auto& [alias, slot] : aliases) {
                scopeIt->second.aliases[alias] = slot;
            }
            return scopeIt->second.scopeId;
        }
    }

    const uint64_t scopeId = nextScopeId_++;
    uint64_t parentScopeId = 0;
    auto current = ownerElement->GetParentNode();
    while (current) {
        auto parentElement = std::dynamic_pointer_cast<Element>(current);
        if (parentElement) {
            auto it = elementToScope_.find(reinterpret_cast<uintptr_t>(parentElement.get()));
            if (it != elementToScope_.end()) {
                parentScopeId = it->second;
                break;
            }
        }
        current = current->GetParentNode();
    }
    scopes_[scopeId] = ScopeRef{scopeId, ownerElementId, parentScopeId, aliases, readOnly};
    elementToScope_[ownerElementId] = scopeId;
    return scopeId;
}

uint64_t NativeDataBindingRuntime::createTextBinding(const std::shared_ptr<Element>& ownerElement,
                                                     const std::shared_ptr<Node>& targetNode,
                                                     const std::string& sourcePath) {
    return createBinding(ownerElement, targetNode, NativeBindingKind::Text, sourcePath);
}

uint64_t NativeDataBindingRuntime::createAttrBinding(const std::shared_ptr<Element>& ownerElement,
                                                     const std::shared_ptr<Element>& targetElement,
                                                     const std::string& attrName,
                                                     const std::string& sourcePath) {
    return createBinding(ownerElement, targetElement, NativeBindingKind::Attr, sourcePath, attrName);
}

uint64_t NativeDataBindingRuntime::createVisibleBinding(const std::shared_ptr<Element>& ownerElement,
                                                        const std::shared_ptr<Element>& targetElement,
                                                        const std::string& sourcePath) {
    return createBinding(ownerElement, targetElement, NativeBindingKind::Visible, sourcePath);
}

uint64_t NativeDataBindingRuntime::createModelValueBinding(const std::shared_ptr<Element>& ownerElement,
                                                           const std::shared_ptr<HTMLInputElement>& input,
                                                           const std::string& sourcePath) {
    return createBinding(ownerElement, input, NativeBindingKind::ModelValue, sourcePath);
}

uint64_t NativeDataBindingRuntime::createModelCheckedBinding(const std::shared_ptr<Element>& ownerElement,
                                                             const std::shared_ptr<HTMLInputElement>& input,
                                                             const std::string& sourcePath) {
    return createBinding(ownerElement, input, NativeBindingKind::ModelChecked, sourcePath);
}

uint64_t NativeDataBindingRuntime::createBinding(const std::shared_ptr<Element>& ownerElement,
                                                 const std::shared_ptr<Node>& targetNode,
                                                 NativeBindingKind kind,
                                                 const std::string& sourcePath,
                                                 const std::string& attrName,
                                                 const std::string& declarativeKey) {
    if (!ownerElement || !targetNode) return 0;
    const uint64_t bindingId = nextBindingId_++;
    bindings_[bindingId] = BindingRecord{
        bindingId, kind, targetNode, ownerElement,
        reinterpret_cast<uintptr_t>(targetNode.get()), reinterpret_cast<uintptr_t>(ownerElement.get()),
        0, sourcePath, {}, attrName, false, false, nextOrderIndex_++, 0, nullptr, {}, false, false, 0, declarativeKey,
    };
    return bindingId;
}

uint64_t NativeDataBindingRuntime::mountDeclarativeBinding(const std::shared_ptr<Element>& ownerElement,
                                                           const std::shared_ptr<Node>& targetNode,
                                                           NativeBindingKind kind,
                                                           const std::string& sourcePath,
                                                           const std::string& attrName,
                                                           const std::string& declarativeKey) {
    if (sourcePath.empty()) {
        recordError("declarative binding path is empty: " + declarativeKey);
        return 0;
    }
    if (!declarativeKey.empty() && declarativeBindingKeys_.count(declarativeKey) != 0) return 0;

    const uint64_t bindingId = createBinding(ownerElement, targetNode, kind, sourcePath, attrName, declarativeKey);
    if (bindingId == 0) return 0;
    if (!mountBinding(bindingId)) {
        bindings_.erase(bindingId);
        return 0;
    }
    if (!declarativeKey.empty()) {
        declarativeBindingKeys_.insert(declarativeKey);
        declarativeBindingsByOwnerElement_[reinterpret_cast<uintptr_t>(ownerElement.get())].insert(bindingId);
    }
    return bindingId;
}

bool NativeDataBindingRuntime::destroyBinding(uint64_t bindingId) {
    auto it = bindings_.find(bindingId);
    if (it == bindings_.end()) return false;
    const uintptr_t ownerElementId = it->second.ownerElementId;
    const std::string declarativeKey = it->second.declarativeKey;
    unmountBinding(bindingId);
    if (!declarativeKey.empty()) {
        auto ownerIt = declarativeBindingsByOwnerElement_.find(ownerElementId);
        if (ownerIt != declarativeBindingsByOwnerElement_.end()) {
            ownerIt->second.erase(bindingId);
            if (ownerIt->second.empty()) declarativeBindingsByOwnerElement_.erase(ownerIt);
        }
    }
    bindings_.erase(bindingId);
    return true;
}

uint32_t NativeDataBindingRuntime::destroyDeclarativeSubtree(const std::shared_ptr<Element>& element) {
    if (!element) return 0;

    uint32_t destroyed = 0;
    for (const auto& child : element->GetChildNodes()) {
        if (auto childElement = std::dynamic_pointer_cast<Element>(child)) {
            destroyed += destroyDeclarativeSubtree(childElement);
        }
    }

    const uintptr_t elementId = reinterpret_cast<uintptr_t>(element.get());
    if (auto scopeIdIt = elementToScope_.find(elementId); scopeIdIt != elementToScope_.end()) {
        auto scopeIt = scopes_.find(scopeIdIt->second);
        if (scopeIt != scopes_.end()) {
            for (auto aliasIt = scopeIt->second.aliases.begin(); aliasIt != scopeIt->second.aliases.end();) {
                if (aliasIt->second.declarative) aliasIt = scopeIt->second.aliases.erase(aliasIt);
                else ++aliasIt;
            }
            if (scopeIt->second.aliases.empty()) {
                scopes_.erase(scopeIt);
                elementToScope_.erase(scopeIdIt);
            }
        }
    }

    if (auto bindingsIt = declarativeBindingsByOwnerElement_.find(elementId); bindingsIt != declarativeBindingsByOwnerElement_.end()) {
        std::vector<uint64_t> bindingIds(bindingsIt->second.begin(), bindingsIt->second.end());
        for (uint64_t bindingId : bindingIds) {
            destroyed += destroyBinding(bindingId) ? 1u : 0u;
        }
    }
    return destroyed;
}

uint32_t NativeDataBindingRuntime::mountDeclarative(const std::shared_ptr<Element>& rootElement) {
    if (!rootElement) return 0;
    return mountDeclarativeRecursive(rootElement);
}

uint32_t NativeDataBindingRuntime::refreshDeclarative(const std::shared_ptr<Element>& rootElement) {
    if (!rootElement) return 0;
    destroyDeclarativeSubtree(rootElement);
    return mountDeclarative(rootElement);
}

uint32_t NativeDataBindingRuntime::unmountDeclarative(const std::shared_ptr<Element>& rootElement) {
    if (!rootElement) return 0;
    return destroyDeclarativeSubtree(rootElement);
}

uint32_t NativeDataBindingRuntime::mountDeclarativeRecursive(const std::shared_ptr<Element>& element) {
    if (!element) return 0;

    uint32_t mounted = 0;
    std::unordered_map<std::string, ScopeSlot> scopeAliases;
    for (const auto& [name, value] : element->GetAllAttributes()) {
        if (hasPrefix(name, kMbScopePrefix)) {
            const std::string alias = name.substr(std::char_traits<char>::length(kMbScopePrefix));
            if (alias.empty() || value.empty()) {
                recordError("invalid declarative scope attribute: " + name);
                continue;
            }
            scopeAliases[alias] = ScopeSlot{value, false, true};
        } else if (hasPrefix(name, kMbScopeReadOnlyPrefix)) {
            const std::string alias = name.substr(std::char_traits<char>::length(kMbScopeReadOnlyPrefix));
            if (alias.empty() || value.empty()) {
                recordError("invalid declarative readonly scope attribute: " + name);
                continue;
            }
            scopeAliases[alias] = ScopeSlot{value, true, true};
        }
    }
    if (!scopeAliases.empty()) createScope(element, scopeAliases, false);

    const uintptr_t elementId = reinterpret_cast<uintptr_t>(element.get());
    if (element->HasAttribute(kMbTextAttr)) {
        mounted += mountDeclarativeBinding(element, element, NativeBindingKind::Text, element->GetAttribute(kMbTextAttr), {},
                                           makeDeclarativeBindingKey(elementId, NativeBindingKind::Text, kMbTextAttr)) != 0;
    }
    if (element->HasAttribute(kMbVisibleAttr)) {
        mounted += mountDeclarativeBinding(element, element, NativeBindingKind::Visible, element->GetAttribute(kMbVisibleAttr), {},
                                           makeDeclarativeBindingKey(elementId, NativeBindingKind::Visible, kMbVisibleAttr)) != 0;
    }
    if (element->HasAttribute(kMbModelAttr)) {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
        if (!input) recordError("mb-model target must be input element");
        else mounted += mountDeclarativeBinding(element, input, NativeBindingKind::ModelValue, element->GetAttribute(kMbModelAttr), {},
                                                makeDeclarativeBindingKey(elementId, NativeBindingKind::ModelValue, kMbModelAttr)) != 0;
    }
    for (const auto& [name, value] : element->GetAllAttributes()) {
        if (hasPrefix(name, kMbAttrPrefix)) {
            const std::string attrName = name.substr(std::char_traits<char>::length(kMbAttrPrefix));
            if (attrName.empty()) {
                recordError("invalid declarative attr binding: " + name);
                continue;
            }
            mounted += mountDeclarativeBinding(element, element, NativeBindingKind::Attr, value, attrName,
                                               makeDeclarativeBindingKey(elementId, NativeBindingKind::Attr, attrName)) != 0;
        } else if (hasPrefix(name, std::string(kMbModelAttr) + ":")) {
            const std::string modelKind = name.substr(std::char_traits<char>::length(kMbModelAttr) + 1);
            auto input = std::dynamic_pointer_cast<HTMLInputElement>(element);
            if (!input) recordError("mb-model target must be input element");
            else if (modelKind == "checked") {
                mounted += mountDeclarativeBinding(element, input, NativeBindingKind::ModelChecked, value, {},
                                                   makeDeclarativeBindingKey(elementId, NativeBindingKind::ModelChecked, modelKind)) != 0;
            } else {
                recordError("unsupported mb-model kind: " + modelKind);
            }
        }
    }

    for (const auto& child : element->GetChildNodes()) {
        if (auto childElement = std::dynamic_pointer_cast<Element>(child)) mounted += mountDeclarativeRecursive(childElement);
    }
    return mounted;
}

ScopeChain NativeDataBindingRuntime::buildScopeChain(const std::shared_ptr<Element>& ownerElement) {
    ScopeChain chain{nextScopeChainId_++, {}, reinterpret_cast<uintptr_t>(ownerElement.get())};
    auto current = ownerElement;
    while (current) {
        auto it = elementToScope_.find(reinterpret_cast<uintptr_t>(current.get()));
        if (it != elementToScope_.end()) chain.scopeIds.push_back(it->second);
        current = std::dynamic_pointer_cast<Element>(current->GetParentNode());
    }
    return chain;
}

std::pair<std::string, bool> NativeDataBindingRuntime::resolvePath(const ScopeChain& chain, const std::string& sourcePath) const {
    auto segments = splitPath(sourcePath);
    if (segments.empty()) return {sourcePath, false};
    for (uint64_t scopeId : chain.scopeIds) {
        auto scopeIt = scopes_.find(scopeId);
        if (scopeIt == scopes_.end()) continue;
        auto aliasIt = scopeIt->second.aliases.find(segments.front());
        if (aliasIt == scopeIt->second.aliases.end()) continue;
        std::string resolved = aliasIt->second.graphPath;
        for (size_t i = 1; i < segments.size(); ++i) resolved += "." + segments[i];
        return {resolved, scopeIt->second.readOnly || aliasIt->second.readOnly};
    }
    return {sourcePath, false};
}

bool NativeDataBindingRuntime::mountBinding(uint64_t bindingId) {
    auto it = bindings_.find(bindingId);
    if (it == bindings_.end()) return false;
    if (it->second.mounted) return true;
    auto owner = it->second.ownerElement.lock();
    if (!owner || it->second.sourcePath.empty()) return false;
    auto chain = buildScopeChain(owner);
    auto [resolvedPath, readOnly] = resolvePath(chain, it->second.sourcePath);
    it->second.scopeChainId = chain.scopeChainId;
    it->second.resolvedPath = resolvedPath;
    it->second.readOnlyResolved = readOnly;
    it->second.mounted = true;
    auto subId = stateManager_.subscribe(resolvedPath, SubscriptionMode::Exact, bindingId,
        [this, bindingId](const std::string&, uint64_t revision) { enqueueBinding(bindingId, revision); });
    it->second.subscriptionIds.push_back(subId);

    if (it->second.kind == NativeBindingKind::ModelValue || it->second.kind == NativeBindingKind::ModelChecked) {
        auto input = std::dynamic_pointer_cast<HTMLInputElement>(it->second.targetNode.lock());
        if (input) {
            const std::string eventName = it->second.kind == NativeBindingKind::ModelValue ? "input" : "change";
            it->second.listenerId = input->AddEventListener(eventName, [this, bindingId](std::shared_ptr<Event>) {
                auto bit = bindings_.find(bindingId);
                if (bit == bindings_.end()) return;
                auto inputNode = std::dynamic_pointer_cast<HTMLInputElement>(bit->second.targetNode.lock());
                if (!inputNode) return;
                handleModelEvent(bit->second, inputNode);
            });
        }
    }

    enqueueBinding(bindingId, stateManager_.revisionOf(resolvedPath));
    return true;
}

bool NativeDataBindingRuntime::unmountBinding(uint64_t bindingId) {
    auto it = bindings_.find(bindingId);
    if (it == bindings_.end()) return false;
    if (auto input = std::dynamic_pointer_cast<HTMLInputElement>(it->second.targetNode.lock())) {
        if (it->second.listenerId != 0) {
            const std::string eventName = it->second.kind == NativeBindingKind::ModelValue ? "input" : "change";
            input->RemoveEventListener(eventName, it->second.listenerId);
        }
    }
    for (uint64_t subId : it->second.subscriptionIds) stateManager_.unsubscribe(subId);
    it->second.subscriptionIds.clear();
    it->second.listenerId = 0;
    it->second.mounted = false;
    it->second.dirty = false;
    const auto entry = std::make_pair(it->second.orderIndex, bindingId);
    dirtyQueue_.pendingBindingIds.erase(entry);
    flushContext_.deferredBindingIds.erase(entry);
    if (!it->second.declarativeKey.empty()) declarativeBindingKeys_.erase(it->second.declarativeKey);
    return true;
}

void NativeDataBindingRuntime::enqueueBinding(uint64_t bindingId, uint64_t revision) {
    auto it = bindings_.find(bindingId);
    if (it == bindings_.end() || !it->second.mounted) return;
    if (it->second.dirty) return;
    it->second.dirty = true;
    auto entry = std::make_pair(it->second.orderIndex, bindingId);
    if (flushContext_.isFlushing) flushContext_.deferredBindingIds.insert(entry);
    else dirtyQueue_.pendingBindingIds.insert(entry);
    dirtyQueue_.pendingRevisionMax = std::max(dirtyQueue_.pendingRevisionMax, revision);
}

bool NativeDataBindingRuntime::isScalarLike(const json& value) {
    return value.is_null() || value.is_boolean() || value.is_number() || value.is_string();
}

void NativeDataBindingRuntime::recordError(const std::string& message) {
    errors_.push_back(message);
}

bool NativeDataBindingRuntime::handleModelEvent(BindingRecord& binding, const std::shared_ptr<HTMLInputElement>& input) {
    if (binding.writeBackGuard) return false;
    if (binding.readOnlyResolved) {
        recordError("attempted write to readonly scope: " + binding.resolvedPath);
        return false;
    }
    const json value = binding.kind == NativeBindingKind::ModelChecked ? json(input->GetChecked()) : json(input->GetValue());
    return stateManager_.set(binding.resolvedPath, value);
}

bool NativeDataBindingRuntime::applyBinding(BindingRecord& binding) {
    auto target = binding.targetNode.lock();
    if (!target || !target->IsConnected()) {
        unmountBinding(binding.bindingId);
        return false;
    }

    const json value = stateManager_.get(binding.resolvedPath);
    binding.lastAppliedRevision = stateManager_.revisionOf(binding.resolvedPath);
    binding.lastRenderedValue = value;

    switch (binding.kind) {
        case NativeBindingKind::Text: {
            std::string rendered;
            if (!value.is_null()) {
                if (!isScalarLike(value)) recordError("TextBinding type mismatch: " + binding.resolvedPath);
                else rendered = stringifyScalar(value);
            }
            if (auto text = std::dynamic_pointer_cast<Text>(target)) text->SetData(rendered);
            else target->SetTextContent(rendered);
            return true;
        }
        case NativeBindingKind::Attr: {
            auto element = std::dynamic_pointer_cast<Element>(target);
            if (!element) return false;
            if (value.is_null()) element->RemoveAttribute(binding.attrName);
            else if (!isScalarLike(value)) {
                recordError("AttrBinding type mismatch: " + binding.resolvedPath);
                element->RemoveAttribute(binding.attrName);
            } else {
                element->SetAttribute(binding.attrName, stringifyScalar(value));
            }
            return true;
        }
        case NativeBindingKind::Visible: {
            auto element = std::dynamic_pointer_cast<Element>(target);
            if (!element) return false;
            const bool visible = !(value.is_null() || (value.is_boolean() && !value.get<bool>()));
            if (visible) element->RemoveAttribute("hidden");
            else element->SetAttribute("hidden", "");
            return true;
        }
        case NativeBindingKind::ModelValue:
        case NativeBindingKind::ModelChecked: {
            auto input = std::dynamic_pointer_cast<HTMLInputElement>(target);
            if (!input) return false;
            binding.writeBackGuard = true;
            if (binding.kind == NativeBindingKind::ModelChecked) {
                input->SetChecked(!value.is_null() && (!value.is_boolean() || value.get<bool>()), false);
            } else if (!value.is_null()) {
                if (!isScalarLike(value)) {
                    recordError("ModelBinding.value type mismatch: " + binding.resolvedPath);
                    input->SetValue("", false);
                } else {
                    input->SetValue(stringifyScalar(value), false);
                }
            } else {
                input->SetValue("", false);
            }
            binding.writeBackGuard = false;
            return true;
        }
    }
    return false;
}

uint32_t NativeDataBindingRuntime::flush() {
    uint32_t applied = 0;
    uint32_t rounds = 0;
    while (true) {
        stateManager_.flush();
        if (dirtyQueue_.pendingBindingIds.empty()) return applied;

        flushContext_.isFlushing = true;
        ++flushContext_.flushEpoch;
        auto current = dirtyQueue_.pendingBindingIds;
        dirtyQueue_.pendingBindingIds.clear();

        for (const auto& [_, bindingId] : current) {
            auto it = bindings_.find(bindingId);
            if (it == bindings_.end()) continue;
            flushContext_.currentBindingId = bindingId;
            it->second.dirty = false;
            if (applyBinding(it->second)) ++applied;
        }

        flushContext_.currentBindingId = 0;
        flushContext_.isFlushing = false;
        if (!flushContext_.deferredBindingIds.empty()) {
            dirtyQueue_.pendingBindingIds.insert(flushContext_.deferredBindingIds.begin(), flushContext_.deferredBindingIds.end());
            flushContext_.deferredBindingIds.clear();
        } else if (stateManager_.queueSize() == 0) {
            return applied;
        }

        if (++rounds > kMaxFlushRounds) {
            recordError("binding flush exceeded re-entrant limit");
            return applied;
        }
    }
}

uint32_t NativeDataBindingRuntime::beginBatch() {
    ++dirtyQueue_.batchDepth;
    return dirtyQueue_.batchDepth;
}

uint32_t NativeDataBindingRuntime::endBatch() {
    if (dirtyQueue_.batchDepth == 0) return 0;
    --dirtyQueue_.batchDepth;
    if (dirtyQueue_.batchDepth == 0) flush();
    return dirtyQueue_.batchDepth;
}

void NativeDataBindingRuntime::clear() {
    std::vector<uint64_t> bindingIds;
    bindingIds.reserve(bindings_.size());
    for (const auto& [bindingId, _] : bindings_) bindingIds.push_back(bindingId);
    for (uint64_t bindingId : bindingIds) unmountBinding(bindingId);

    bindings_.clear();
    scopes_.clear();
    elementToScope_.clear();
    declarativeBindingsByOwnerElement_.clear();
    declarativeBindingKeys_.clear();
    dirtyQueue_.pendingBindingIds.clear();
    dirtyQueue_.scheduled = false;
    dirtyQueue_.batchDepth = 0;
    dirtyQueue_.pendingRevisionMax = 0;
    flushContext_.currentBindingId = 0;
    flushContext_.deferredBindingIds.clear();
    flushContext_.isFlushing = false;
    flushContext_.reentrantMutationCount = 0;
}

} // namespace mblink
