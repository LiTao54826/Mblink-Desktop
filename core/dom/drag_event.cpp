/**
 * @file drag_event.cpp
 * @brief HTML5 拖拽事件类实现
 */

#include "drag_event.h"
#include "core/event/types/data_transfer.h"

namespace mbink {

// 事件类型常量定义
const std::string DragEvent::DRAG_START = "dragstart";
const std::string DragEvent::DRAG = "drag";
const std::string DragEvent::DRAG_END = "dragend";
const std::string DragEvent::DRAG_ENTER = "dragenter";
const std::string DragEvent::DRAG_LEAVE = "dragleave";
const std::string DragEvent::DRAG_OVER = "dragover";
const std::string DragEvent::DROP = "drop";

DragEvent::DragEvent(const std::string& type,
                     int client_x,
                     int client_y,
                     int button,
                     std::shared_ptr<DataTransfer> data_transfer,
                     bool ctrl_key,
                     bool shift_key,
                     bool alt_key,
                     bool meta_key)
    : MouseEvent(type, client_x, client_y, button)
    , data_transfer_(data_transfer)
    , ctrl_key_(ctrl_key)
    , shift_key_(shift_key)
    , alt_key_(alt_key)
    , meta_key_(meta_key)
    , screen_x_(client_x)  // 默认屏幕坐标等于客户端坐标
    , screen_y_(client_y) {
}

void DragEvent::SetScreenPosition(int x, int y) {
    screen_x_ = x;
    screen_y_ = y;
}

} // namespace mbink
