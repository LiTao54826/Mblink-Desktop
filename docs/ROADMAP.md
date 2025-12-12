# MBink 开发路线图

> **更新时间**: 2025-12-12  
> **战略定位**: 轻量高效 + 可选企业级扩展（混合策略）  
> **布局引擎**: NativeLayoutEngine (Block + IFC + Flexbox + Grid)

---

## 🎯 项目定位

### 核心理念
```
"轻量、高效、可控的企业级桌面应用框架"
```

### 差异化定位
| 维度 | MBink | Electron | Tauri |
|------|-------|----------|-------|
| **体积** | ~50MB | ~150MB | ~10MB |
| **启动** | ~100ms | ~1s | ~100ms |
| **内存** | ~50MB | ~300MB | ~50MB |
| **定位** | 轻量企业级 | 全功能 | 极致轻量 |

### 混合策略
```
Layer 1: 核心层（轻量，必备）
  → 20个基础组件 + 路由 + 状态管理
  → 体积 50-100KB
  
Layer 2: 扩展层（可选，按需）
  → 高级组件 + 第三方集成
  → 按需加载
  
Layer 3: 企业层（商业，增值）
  → 完整组件库 + 可视化工具 + 企业支持
```

---

## 📅 开发阶段

### Phase 1: 核心企业级能力（4个月）

**目标**: 完成核心组件和开发工具，达到企业可用状态

#### 1.1 UI 组件库（6周）

| 组件 | 优先级 | 状态 | 说明 |
|------|--------|------|------|
| Button | P0 | ✅ 完成 | 3种样式变体 |
| Input | P0 | ✅ 完成 | 带标签和验证 |
| Card | P0 | ✅ 完成 | 阴影层级 |
| Select | P0 | ⏳ 待开发 | 下拉选择器 |
| Checkbox | P0 | ⏳ 待开发 | 复选框 |
| Radio | P0 | ⏳ 待开发 | 单选框 |
| Table | P0 | ⏳ 待开发 | 数据表格 |
| Dialog | P0 | ⏳ 待开发 | 对话框 |
| Tabs | P1 | ⏳ 待开发 | 选项卡 |
| Menu | P1 | ⏳ 待开发 | 菜单 |
| Toast | P1 | ⏳ 待开发 | 提示消息 |
| Progress | P1 | ⏳ 待开发 | 进度条 |
| Switch | P1 | ⏳ 待开发 | 开关 |
| Slider | P2 | ⏳ 待开发 | 滑块 |
| DatePicker | P2 | ⏳ 待开发 | 日期选择 |
| Upload | P2 | ⏳ 待开发 | 文件上传 |
| Tree | P2 | ⏳ 待开发 | 树形控件 |
| Pagination | P2 | ⏳ 待开发 | 分页 |
| Drawer | P2 | ⏳ 待开发 | 抽屉 |
| Tooltip | P2 | ⏳ 待开发 | 提示 |

#### 1.2 状态管理（2周）

```javascript
// MBink Store - 简洁的状态管理
const store = MBink.createStore({
    count: 0,
    todos: []
});

// 组件中使用
function Counter() {
    const [state, dispatch] = MBink.useStore(store);
    return preact.h('button', {
        onclick: () => dispatch({ count: state.count + 1 })
    }, state.count);
}
```

#### 1.3 路由系统（2周）

```javascript
// MBink Router
const router = MBink.createRouter({
    '/': HomePage,
    '/about': AboutPage,
    '/users/:id': UserPage
});

function App() {
    return preact.h(MBink.RouterView, { router });
}
```

#### 1.4 主题系统（1周）

```javascript
const theme = MBink.createTheme({
    colors: {
        primary: '#1976d2',
        secondary: '#dc004e',
        success: '#4caf50',
        warning: '#ff9800',
        error: '#f44336'
    },
    spacing: (n) => n * 8 + 'px',
    borderRadius: '4px'
});
```

---

### Phase 2: 扩展能力（2个月）

**目标**: 提供第三方库集成机制，支持复杂应用场景

#### 2.1 外部库集成（2周）

```javascript
// 集成 Chart.js
MBink.loadExternal('chart.js', {
    type: 'umd',
    url: 'path/to/chart.umd.js'
}).then(() => {
    // 使用 Charts
});
```

支持的集成：
- ✅ Chart.js - 图表
- ✅ Quill.js - 富文本编辑器
- ✅ Flatpickr - 日期选择器
- ⏳ Monaco Editor - 代码编辑器

#### 2.2 插件系统（2周）

```javascript
// 创建插件
const myPlugin = MBink.createPlugin({
    name: 'my-plugin',
    install(app) {
        app.component('MyComponent', MyComponent);
        app.directive('my-directive', myDirective);
    }
});

// 使用插件
MBink.use(myPlugin);
```

#### 2.3 高级组件（4周）

| 组件 | 类型 | 说明 |
|------|------|------|
| DataGrid | 扩展 | 高级表格 |
| FormBuilder | 扩展 | 表单生成器 |
| RichEditor | 集成 | 富文本（Quill） |
| Charts | 集成 | 图表（Chart.js） |
| Calendar | 集成 | 日历（Flatpickr） |

---

### Phase 3: 企业级特性（持续）

**目标**: 提供企业级增值功能

#### 3.1 可视化设计器
- 拖拽式界面设计
- 组件属性编辑
- 代码导出

#### 3.2 完整组件库（100+）
- 企业级 UI 组件
- 行业解决方案组件
- 定制主题

#### 3.3 企业支持
- 技术支持
- 定制开发
- 培训服务

---

## 📊 时间线

```
2025年1月 ─────────────────────────────────────────────────────────────
   ├── Week 1-2: Select, Checkbox, Radio 组件
   ├── Week 3-4: Table, Dialog 组件
   ├── Week 5-6: Tabs, Menu, Toast 组件
   └── Week 7-8: 状态管理 + 路由

2025年2月 ─────────────────────────────────────────────────────────────
   ├── Week 1-2: 主题系统 + 剩余基础组件
   ├── Week 3-4: 外部库集成机制
   └── 🎯 核心版发布 v1.0

2025年3月 ─────────────────────────────────────────────────────────────
   ├── Week 1-2: 插件系统
   ├── Week 3-4: 高级组件
   └── 🎯 扩展版发布 v1.1

2025年4月+ ────────────────────────────────────────────────────────────
   └── 企业级特性（持续迭代）
```

---

## 🎯 里程碑

### v1.0 - 核心版（2025年2月）
- ✅ 20个基础组件
- ✅ 路由 + 状态管理
- ✅ 主题系统
- ✅ 完整文档

### v1.1 - 扩展版（2025年3月）
- ✅ 外部库集成
- ✅ 插件系统
- ✅ 高级组件

### v2.0 - 企业版（2025年Q3）
- ✅ 可视化设计器
- ✅ 完整组件库
- ✅ 企业支持

---

## 📋 当前优先级

### 立即开始（本周）
1. **Select 组件** - 下拉选择器
2. **Checkbox 组件** - 复选框
3. **Radio 组件** - 单选框

### 下一步（2周内）
4. **Table 组件** - 数据表格
5. **Dialog 组件** - 对话框

### 后续（1个月内）
6. **状态管理** - MBink.createStore
7. **路由系统** - MBink.createRouter

---

## 📚 相关文档

- [项目状态](PROJECT_STATUS.md) - 当前进度
- [架构设计](ARCHITECTURE.md) - 技术架构
- [UI 组件指南](UI_COMPONENT_GUIDE.md) - 组件开发
- [API 设计](API_DESIGN.md) - API 规范

---

**维护者**: MBink Team  
**最后更新**: 2025-12-12
