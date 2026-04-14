# SharedObject 颗粒度更新设计预案

## 1. 背景
当前 SharedObject 已支持宿主 ↔ JS 双向同步，但同步粒度仍是 **顶层 key**。

例如：
- `data.a.b = 2` 会同步整个 `a`
- 宿主侧 `set_json("a", {...})` 也属于顶层整块替换

这能解决正确性问题，但在以下场景成本偏高：
- 大对象/大数组的深层高频修改
- 希望只同步叶子路径，不想整块 JSON 回传
- 需要更精确的依赖刷新粒度

## 2. 目标
后续若需要升级，目标是把 SharedObject 从“顶层 key 同步”演进为“路径 patch 同步”。

预期能力：
1. JS 修改 `a.b.c` 时，只同步该路径对应 patch
2. 宿主与 JS 使用统一 patch 模型
3. 保留现有 `data_` 线程安全镜像架构
4. 减少大对象深层修改时的 JSON 整块转换成本
5. 为后续更细粒度依赖追踪预留基础

## 3. 非目标
本预案默认不追求以下能力：
- CRDT / 多主自动合并
- 任意复杂 JS 对象语义（class instance、DOM 节点、函数）
- 直接替代现有 Preact 响应式系统

## 4. 推荐架构
### 4.1 单一权威状态
- `data_`：C++ 权威状态（authoritative state）
- `js_obj`：JS 侧镜像对象

所有写入都转换为 patch，再统一落到 `data_`，之后再把变更投影到 `js_obj`。

### 4.2 Patch 数据模型
建议最小 patch 结构：

```json
{
  "op": "set",
  "path": ["a", "b", "c"],
  "value": 2,
  "source": "js",
  "baseRevision": 12
}
```

建议支持：
- `set(path, value)`
- `delete(path)`
- 可选：`splice(path, start, deleteCount, items)`

### 4.3 Path 表达
建议统一使用数组路径：
- 对象字段：字符串
- 数组下标：整数

示例：
- `a.b.c` → `["a", "b", "c"]`
- `list[3].name` → `["list", 3, "name"]`

## 5. 核心改造点
### 5.1 C++ 层
新增能力：
- `applySetPath(path, value)`
- `applyDeletePath(path)`
- `readPath(path)`
- `applyPatch(patch)`

要求：
- 自动创建中间对象/数组节点
- 明确数组下标越界策略
- 明确删除不存在路径时的返回值

### 5.2 JS Runtime 层
deep proxy 不再只记 `rootKey`，而是记录完整路径。

例如：
- 访问 `data.a.b` 时，代理上下文持有路径 `["a", "b"]`
- `set` 时发送 `set(["a", "b", "c"], value)`

### 5.3 宿主 API 层
保留当前顶层 API 兼容：
- `set_json("a", {...})`
- `set_int("c", 2)`

后续可选新增 path API：
- `set_path_json("a.b", {...})`
- `set_path_int("a.b.c", 2)`
- `delete_path("a.b")`

建议先实现内部 patch 能力，再决定是否暴露公开 API。

## 6. 依赖追踪演进建议
可分两阶段：

### 阶段 A：同步细粒度，通知仍按顶层 key
- JS 改 `a.b.c`
- C++ 只更新叶子路径
- 仍然 schedule `data:a`

优点：
- 改动中等
- 已能明显降低整块同步成本
- 不影响现有依赖模型

### 阶段 B：依赖也升级到 path/subtree 粒度
新增两类依赖：
- 精确依赖：`data:a.b.c`
- 子树依赖：`data:a/*`

这样才能做到更精准的组件刷新。

## 7. 一致性与版本控制
建议每个 SharedObject 增加 revision：
- 每次 patch 成功应用后 `revision++`
- patch 带 `baseRevision`

用途：
- 识别旧 patch
- 避免回声（echo）
- 为后续冲突处理预留空间

冲突策略建议初期采用：
- 默认 last-write-wins
- debug 模式可记录冲突日志

## 8. 风险点
1. 数组 patch 语义复杂
2. path 自动建树规则需定义清楚
3. path 级 delete/replace 与 JS 行为要一致
4. path 依赖实现不当会增加调度复杂度
5. 宿主公开 API 若过早暴露，后续兼容成本会上升

## 9. 分阶段实施建议
### 方案 1（推荐首选）
先做：
- C++ path patch apply
- JS deep proxy 发送完整 path
- 但通知仍按顶层 key

价值：
- 成本适中
- 收益明显
- 对现有代码侵入可控

### 方案 2（需求明确后再做）
再做：
- path/subtree 依赖
- revision 冲突检查
- 宿主 path API

## 10. 结论
当 SharedObject 面临“大对象 + 高频深层修改”时，本预案值得启动。

如果当前主要需求仍是“正确同步 + 立即刷新”，则现有顶层 key 同步方案已足够，颗粒度升级可留待后续性能需求明确时再实施。
