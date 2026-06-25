#pragma once

#include "core/dom/element.h"
#include "core/window/repaint_reason.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace mblink {

class NativeTextRepaintCoalescer {
public:
    static NativeTextRepaintCoalescer& Instance() {
        static NativeTextRepaintCoalescer instance;
        return instance;
    }

    void Request(const std::weak_ptr<Node>& weak_node,
                 RepaintReason reason,
                 int64_t min_interval_ms) {
        auto node = weak_node.lock();
        if (!node) {
            return;
        }

        const int64_t now = NowMs();
        Prune(now);

        auto it = Find(node.get());
        if (it == entries_.end()) {
            entries_.push_back({weak_node, 0, 0, reason});
            it = entries_.end() - 1;
        }

        const int64_t interval = std::max<int64_t>(1, min_interval_ms);
        if (it->last_repaint_ms == 0 || now - it->last_repaint_ms >= interval) {
            RequestNow(node, reason, now, *it);
            return;
        }

        it->pending = true;
        it->reason = reason;
        it->due_ms = it->last_repaint_ms + interval;
    }

    void FlushDue() {
        const int64_t now = NowMs();
        for (auto it = entries_.begin(); it != entries_.end();) {
            auto node = it->node.lock();
            if (!node) {
                it = entries_.erase(it);
                continue;
            }

            if (it->pending && it->due_ms <= now) {
                RequestNow(node, it->reason, now, *it);
            }

            if (!it->pending && now - it->last_repaint_ms > kIdleEntryRetentionMs) {
                it = entries_.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool HasPending() const {
        return std::any_of(entries_.begin(), entries_.end(), [](const Entry& entry) {
            return entry.pending && !entry.node.expired();
        });
    }

    int64_t MillisecondsUntilNextFlush() const {
        int64_t next_due = -1;
        const int64_t now = NowMs();
        for (const auto& entry : entries_) {
            if (!entry.pending || entry.node.expired()) {
                continue;
            }
            if (next_due < 0 || entry.due_ms < next_due) {
                next_due = entry.due_ms;
            }
        }

        if (next_due < 0) {
            return -1;
        }
        return std::max<int64_t>(0, next_due - now);
    }

private:
    struct Entry {
        std::weak_ptr<Node> node;
        int64_t last_repaint_ms = 0;
        int64_t due_ms = 0;
        RepaintReason reason = RepaintReason::Unknown;
        bool pending = false;
    };

    static constexpr int64_t kIdleEntryRetentionMs = 5000;

    static int64_t NowMs() {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch())
            .count();
    }

    std::vector<Entry>::iterator Find(Node* node) {
        return std::find_if(entries_.begin(), entries_.end(), [node](const Entry& entry) {
            auto locked = entry.node.lock();
            return locked && locked.get() == node;
        });
    }

    void RequestNow(const std::shared_ptr<Node>& node,
                    RepaintReason reason,
                    int64_t now,
                    Entry& entry) {
        auto element = std::dynamic_pointer_cast<Element>(node);
        if (!element) {
            entry.pending = false;
            return;
        }

        element->RequestRepaint(reason);
        entry.last_repaint_ms = now;
        entry.due_ms = 0;
        entry.pending = false;
        entry.reason = reason;
    }

    void Prune(int64_t now) {
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(), [now](const Entry& entry) {
                if (entry.node.expired()) {
                    return true;
                }
                return !entry.pending && entry.last_repaint_ms > 0 &&
                       now - entry.last_repaint_ms > kIdleEntryRetentionMs;
            }),
            entries_.end());
    }

    std::vector<Entry> entries_;
};

}  // namespace mblink
