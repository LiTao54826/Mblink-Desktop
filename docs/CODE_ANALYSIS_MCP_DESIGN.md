# Code Analysis MCP Service 设计文档

## 概述

一个基于数据库的代码分析 MCP 服务，为 AI 编码助手提供精确的代码上下文信息。

## 架构设计

```
┌─────────────┐     MCP Protocol      ┌──────────────────┐
│    Kiro     │ ◄──────────────────► │  MCP Server      │
│  (AI IDE)   │                       │  (Python/Node)   │
└─────────────┘                       └────────┬─────────┘
                                               │
                                      ┌────────▼─────────┐
                                      │   SQLite DB      │
                                      │  (代码索引)       │
                                      └────────┬─────────┘
                                               │
                                      ┌────────▼─────────┐
                                      │   Indexer        │
                                      │  (clang/treesit) │
                                      └────────┬─────────┘
                                               │
                                      ┌────────▼─────────┐
                                      │   Source Files   │
                                      └──────────────────┘
```

## 数据库 Schema

### 核心表结构

```sql
-- 文件表
CREATE TABLE files (
    id INTEGER PRIMARY KEY,
    path TEXT UNIQUE NOT NULL,
    language TEXT,              -- cpp, h, js, py
    hash TEXT,                  -- 文件内容hash，用于增量更新
    last_indexed TIMESTAMP,
    line_count INTEGER
);

-- 符号表（函数、类、变量、宏等）
CREATE TABLE symbols (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    qualified_name TEXT,        -- 完全限定名 namespace::class::method
    kind TEXT NOT NULL,         -- function, class, method, variable, macro, enum, typedef
    file_id INTEGER REFERENCES files(id),
    line_start INTEGER,
    line_end INTEGER,
    column_start INTEGER,
    signature TEXT,             -- 函数签名或变量类型
    access TEXT,                -- public, protected, private
    is_virtual BOOLEAN,
    is_static BOOLEAN,
    is_const BOOLEAN,
    doc_comment TEXT,           -- 文档注释
    UNIQUE(qualified_name, file_id, line_start)
);

CREATE INDEX idx_symbols_name ON symbols(name);
CREATE INDEX idx_symbols_qualified ON symbols(qualified_name);
CREATE INDEX idx_symbols_file ON symbols(file_id);
CREATE INDEX idx_symbols_kind ON symbols(kind);

-- 类继承关系
CREATE TABLE inheritance (
    id INTEGER PRIMARY KEY,
    derived_id INTEGER REFERENCES symbols(id),   -- 派生类
    base_id INTEGER REFERENCES symbols(id),      -- 基类
    access TEXT,                                  -- public, protected, private
    is_virtual BOOLEAN
);

CREATE INDEX idx_inheritance_derived ON inheritance(derived_id);
CREATE INDEX idx_inheritance_base ON inheritance(base_id);

-- 类成员关系（成员变量、成员函数属于哪个类）
CREATE TABLE class_members (
    id INTEGER PRIMARY KEY,
    class_id INTEGER REFERENCES symbols(id),
    member_id INTEGER REFERENCES symbols(id),
    UNIQUE(class_id, member_id)
);

CREATE INDEX idx_class_members_class ON class_members(class_id);

-- 函数调用关系
CREATE TABLE calls (
    id INTEGER PRIMARY KEY,
    caller_id INTEGER REFERENCES symbols(id),  -- 调用者函数
    callee_id INTEGER REFERENCES symbols(id),  -- 被调用函数
    file_id INTEGER REFERENCES files(id),
    line INTEGER,
    column INTEGER
);

CREATE INDEX idx_calls_caller ON calls(caller_id);
CREATE INDEX idx_calls_callee ON calls(callee_id);

-- 符号引用（变量使用、类型引用等）
CREATE TABLE references (
    id INTEGER PRIMARY KEY,
    symbol_id INTEGER REFERENCES symbols(id),
    file_id INTEGER REFERENCES files(id),
    line INTEGER,
    column INTEGER,
    kind TEXT      -- read, write, type_use, instantiation
);

CREATE INDEX idx_references_symbol ON references(symbol_id);
CREATE INDEX idx_references_file ON references(file_id);

-- 文件依赖（include 关系）
CREATE TABLE file_dependencies (
    id INTEGER PRIMARY KEY,
    source_file_id INTEGER REFERENCES files(id),  -- 包含者
    target_file_id INTEGER REFERENCES files(id),  -- 被包含文件
    line INTEGER,
    is_system BOOLEAN     -- <> vs ""
);

CREATE INDEX idx_deps_source ON file_dependencies(source_file_id);
CREATE INDEX idx_deps_target ON file_dependencies(target_file_id);

-- 命名空间
CREATE TABLE namespaces (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    parent_id INTEGER REFERENCES namespaces(id),
    full_path TEXT UNIQUE    -- a::b::c
);

-- 符号所属命名空间
CREATE TABLE symbol_namespaces (
    symbol_id INTEGER REFERENCES symbols(id),
    namespace_id INTEGER REFERENCES namespaces(id),
    PRIMARY KEY(symbol_id, namespace_id)
);
```

## MCP 工具定义

### 1. 符号查询工具

#### `get_symbol`
查询符号定义信息。

```json
{
  "name": "get_symbol",
  "description": "获取符号（函数、类、变量等）的完整定义信息",
  "inputSchema": {
    "type": "object",
    "properties": {
      "name": {
        "type": "string",
        "description": "符号名称，支持模糊匹配"
      },
      "qualified_name": {
        "type": "string", 
        "description": "完全限定名，如 namespace::class::method"
      },
      "kind": {
        "type": "string",
        "enum": ["function", "class", "method", "variable", "macro", "enum", "typedef"],
        "description": "符号类型过滤"
      },
      "file": {
        "type": "string",
        "description": "限定在某个文件中搜索"
      }
    },
    "required": ["name"]
  }
}
```

**返回示例：**
```json
{
  "symbols": [{
    "name": "paint",
    "qualified_name": "RenderObject::paint",
    "kind": "method",
    "signature": "virtual void paint(PaintContext& ctx, const Rect& clip)",
    "location": {
      "file": "core/render/render_object.h",
      "line_start": 45,
      "line_end": 45
    },
    "access": "public",
    "is_virtual": true,
    "doc_comment": "/**\n * Paints this render object within the given clip rect.\n * @param ctx The paint context\n * @param clip The clipping rectangle\n */",
    "parent_class": "RenderObject"
  }]
}
```

#### `get_class_info`
获取类的完整信息，包括成员和继承关系。

```json
{
  "name": "get_class_info",
  "description": "获取类的完整信息：成员列表、继承关系、实现的接口",
  "inputSchema": {
    "type": "object",
    "properties": {
      "class_name": {
        "type": "string",
        "description": "类名"
      },
      "include_inherited": {
        "type": "boolean",
        "default": true,
        "description": "是否包含继承的成员"
      },
      "include_private": {
        "type": "boolean", 
        "default": false,
        "description": "是否包含私有成员"
      }
    },
    "required": ["class_name"]
  }
}
```

**返回示例：**
```json
{
  "class": {
    "name": "RenderBox",
    "qualified_name": "RenderBox",
    "location": {"file": "core/render/render_box.h", "line": 15},
    "doc_comment": "A render object with a rectangular bounding box.",
    "bases": [
      {"name": "RenderObject", "access": "public", "is_virtual": false}
    ],
    "derived_classes": ["RenderBlock", "RenderInline", "RenderImage"],
    "members": {
      "public": [
        {"name": "width", "kind": "method", "signature": "float width() const"},
        {"name": "height", "kind": "method", "signature": "float height() const"},
        {"name": "setSize", "kind": "method", "signature": "void setSize(float w, float h)"}
      ],
      "protected": [
        {"name": "layout", "kind": "method", "signature": "void layout() override", "is_virtual": true}
      ],
      "inherited": [
        {"name": "paint", "kind": "method", "signature": "virtual void paint(...)", "from": "RenderObject"}
      ]
    }
  }
}
```

### 2. 调用关系工具

#### `get_callers`
查找调用指定函数的所有位置。

```json
{
  "name": "get_callers",
  "description": "查找所有调用指定函数/方法的位置",
  "inputSchema": {
    "type": "object",
    "properties": {
      "function_name": {
        "type": "string",
        "description": "函数名或完全限定名"
      },
      "limit": {
        "type": "integer",
        "default": 20,
        "description": "返回结果数量限制"
      }
    },
    "required": ["function_name"]
  }
}
```

**返回示例：**
```json
{
  "function": "RenderObject::paint",
  "caller_count": 5,
  "callers": [
    {
      "caller": "RenderLayer::paintLayer",
      "location": {"file": "core/render/render_layer.cpp", "line": 120},
      "context": "    child->paint(ctx, childClip);"
    },
    {
      "caller": "RenderView::paint", 
      "location": {"file": "core/render/render_view.cpp", "line": 85},
      "context": "    rootObject_->paint(context, viewport_);"
    }
  ]
}
```

#### `get_callees`
查找指定函数调用的所有函数。

```json
{
  "name": "get_callees",
  "description": "查找指定函数内部调用的所有函数",
  "inputSchema": {
    "type": "object",
    "properties": {
      "function_name": {
        "type": "string",
        "description": "函数名或完全限定名"
      }
    },
    "required": ["function_name"]
  }
}
```

#### `get_call_graph`
获取调用图（支持多层深度）。

```json
{
  "name": "get_call_graph",
  "description": "获取以指定函数为中心的调用图",
  "inputSchema": {
    "type": "object",
    "properties": {
      "function_name": {"type": "string"},
      "direction": {
        "type": "string",
        "enum": ["callers", "callees", "both"],
        "default": "both"
      },
      "depth": {
        "type": "integer",
        "default": 2,
        "maximum": 5,
        "description": "展开深度"
      }
    },
    "required": ["function_name"]
  }
}
```

### 3. 依赖分析工具

#### `get_file_dependencies`
获取文件的 include 依赖。

```json
{
  "name": "get_file_dependencies",
  "description": "获取文件的 include 依赖关系",
  "inputSchema": {
    "type": "object",
    "properties": {
      "file": {
        "type": "string",
        "description": "文件路径"
      },
      "direction": {
        "type": "string",
        "enum": ["includes", "included_by", "both"],
        "default": "both",
        "description": "includes=该文件包含哪些, included_by=被哪些文件包含"
      },
      "recursive": {
        "type": "boolean",
        "default": false,
        "description": "是否递归展开"
      },
      "depth": {
        "type": "integer",
        "default": 1,
        "description": "递归深度"
      }
    },
    "required": ["file"]
  }
}
```

**返回示例：**
```json
{
  "file": "core/render/render_object.h",
  "includes": [
    {"file": "core/layout/layout_object.h", "line": 5, "is_system": false},
    {"file": "core/utils/rect.h", "line": 6, "is_system": false},
    {"file": "memory", "line": 8, "is_system": true}
  ],
  "included_by": [
    {"file": "core/render/render_box.h"},
    {"file": "core/render/render_text.h"},
    {"file": "core/render/render_layer.cpp"}
  ]
}
```

#### `get_module_dependencies`
获取模块（目录）级别的依赖关系。

```json
{
  "name": "get_module_dependencies",
  "description": "获取模块/目录级别的依赖关系图",
  "inputSchema": {
    "type": "object",
    "properties": {
      "module": {
        "type": "string",
        "description": "模块目录路径，如 core/render"
      },
      "detect_cycles": {
        "type": "boolean",
        "default": true,
        "description": "是否检测循环依赖"
      }
    },
    "required": ["module"]
  }
}
```

**返回示例：**
```json
{
  "module": "core/render",
  "depends_on": ["core/layout", "core/dom", "core/utils"],
  "depended_by": ["core/compositor", "core/window"],
  "internal_files": 12,
  "cycles_detected": [],
  "dependency_graph": {
    "core/render -> core/layout": ["render_object.h", "render_box.cpp"],
    "core/render -> core/dom": ["render_tree_builder.cpp"]
  }
}
```

### 4. 引用查找工具

#### `find_references`
查找符号的所有引用位置。

```json
{
  "name": "find_references",
  "description": "查找符号在项目中的所有引用位置",
  "inputSchema": {
    "type": "object",
    "properties": {
      "symbol": {
        "type": "string",
        "description": "符号名称"
      },
      "kind_filter": {
        "type": "array",
        "items": {"type": "string", "enum": ["read", "write", "type_use", "call"]},
        "description": "引用类型过滤"
      },
      "file_pattern": {
        "type": "string",
        "description": "文件路径模式过滤，如 core/render/*"
      },
      "limit": {
        "type": "integer",
        "default": 50
      }
    },
    "required": ["symbol"]
  }
}
```

**返回示例：**
```json
{
  "symbol": "RenderObject",
  "total_references": 156,
  "references": [
    {"file": "core/render/render_box.h", "line": 12, "kind": "type_use", "context": "class RenderBox : public RenderObject {"},
    {"file": "core/render/render_layer.cpp", "line": 45, "kind": "type_use", "context": "    RenderObject* child = ..."},
    {"file": "core/dom/element.cpp", "line": 89, "kind": "call", "context": "    renderObject_->setNeedsLayout();"}
  ]
}
```

### 5. 代码示例工具

#### `get_usage_examples`
获取符号在项目中的使用示例。

```json
{
  "name": "get_usage_examples",
  "description": "获取符号在项目中的典型使用示例",
  "inputSchema": {
    "type": "object",
    "properties": {
      "symbol": {
        "type": "string",
        "description": "符号名称"
      },
      "limit": {
        "type": "integer",
        "default": 5,
        "description": "返回示例数量"
      },
      "context_lines": {
        "type": "integer",
        "default": 3,
        "description": "上下文行数"
      }
    },
    "required": ["symbol"]
  }
}
```

**返回示例：**
```json
{
  "symbol": "PaintContext",
  "examples": [
    {
      "file": "core/render/render_box.cpp",
      "line": 78,
      "code": "void RenderBox::paint(PaintContext& ctx, const Rect& clip) {\n    ctx.save();\n    ctx.clipRect(clip);\n    paintBackground(ctx);\n    paintBorder(ctx);\n    ctx.restore();\n}"
    },
    {
      "file": "core/render/render_text.cpp", 
      "line": 45,
      "code": "void RenderText::paint(PaintContext& ctx, const Rect& clip) {\n    ctx.setFont(style_.font());\n    ctx.setColor(style_.color());\n    ctx.drawText(text_, bounds_);\n}"
    }
  ]
}
```

### 6. 索引管理工具

#### `index_status`
查看索引状态。

```json
{
  "name": "index_status",
  "description": "查看代码索引的状态和统计信息",
  "inputSchema": {
    "type": "object",
    "properties": {}
  }
}
```

**返回示例：**
```json
{
  "status": "ready",
  "last_full_index": "2026-01-05T10:30:00Z",
  "last_incremental": "2026-01-05T14:22:15Z",
  "statistics": {
    "files": 245,
    "symbols": 3420,
    "functions": 1856,
    "classes": 312,
    "calls": 8934,
    "references": 24567
  },
  "pending_updates": 3,
  "index_size_mb": 12.5
}
```

#### `reindex`
触发重新索引。

```json
{
  "name": "reindex",
  "description": "触发代码重新索引",
  "inputSchema": {
    "type": "object",
    "properties": {
      "mode": {
        "type": "string",
        "enum": ["full", "incremental", "file"],
        "default": "incremental"
      },
      "file": {
        "type": "string",
        "description": "mode=file 时指定要重新索引的文件"
      }
    }
  }
}
```

## 服务实现架构

### 技术选型

```
┌─────────────────────────────────────────────────────────┐
│                    MCP Server (Python)                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │
│  │ MCP Handler │  │ Query Engine│  │ Index Manager   │  │
│  │ (FastMCP)   │  │             │  │                 │  │
│  └──────┬──────┘  └──────┬──────┘  └────────┬────────┘  │
│         │                │                   │           │
│         └────────────────┼───────────────────┘           │
│                          │                               │
│                  ┌───────▼───────┐                       │
│                  │   SQLite DB   │                       │
│                  │  (索引存储)    │                       │
│                  └───────────────┘                       │
└─────────────────────────────────────────────────────────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
      ┌───────▼──────┐ ┌───▼───┐ ┌──────▼──────┐
      │ clang-based  │ │ File  │ │ Incremental │
      │   Indexer    │ │Watcher│ │   Updater   │
      └──────────────┘ └───────┘ └─────────────┘
```

### 核心组件

#### 1. Indexer（索引器）

使用 libclang 或 clangd 解析 C++ 代码：

```python
# indexer.py
import clang.cindex
from pathlib import Path
import sqlite3

class CodeIndexer:
    def __init__(self, db_path: str, compile_commands_path: str):
        self.db = sqlite3.connect(db_path)
        self.index = clang.cindex.Index.create()
        self.compile_commands = clang.cindex.CompilationDatabase.fromDirectory(
            compile_commands_path
        )
    
    def index_file(self, file_path: str):
        """索引单个文件"""
        commands = self.compile_commands.getCompileCommands(file_path)
        args = list(commands[0].arguments)[1:-1] if commands else []
        
        tu = self.index.parse(file_path, args=args)
        
        # 提取符号
        self._extract_symbols(tu.cursor, file_path)
        # 提取调用关系
        self._extract_calls(tu.cursor, file_path)
        # 提取 include 依赖
        self._extract_includes(tu, file_path)
        
        self.db.commit()
    
    def _extract_symbols(self, cursor, file_path):
        """递归提取所有符号定义"""
        for child in cursor.get_children():
            if child.location.file and child.location.file.name == file_path:
                if child.kind in SYMBOL_KINDS:
                    self._insert_symbol(child)
            self._extract_symbols(child, file_path)
    
    def _extract_calls(self, cursor, file_path):
        """提取函数调用关系"""
        for child in cursor.get_children():
            if child.kind == clang.cindex.CursorKind.CALL_EXPR:
                self._insert_call(child)
            self._extract_calls(child, file_path)
```

#### 2. MCP Server（服务端）

```python
# server.py
from mcp.server.fastmcp import FastMCP
import sqlite3
from typing import Optional

mcp = FastMCP("code-analyzer")

class QueryEngine:
    def __init__(self, db_path: str):
        self.db = sqlite3.connect(db_path, check_same_thread=False)
        self.db.row_factory = sqlite3.Row
    
    def get_symbol(self, name: str, kind: Optional[str] = None, 
                   file: Optional[str] = None) -> list:
        query = """
            SELECT s.*, f.path as file_path
            FROM symbols s
            JOIN files f ON s.file_id = f.id
            WHERE s.name LIKE ?
        """
        params = [f"%{name}%"]
        
        if kind:
            query += " AND s.kind = ?"
            params.append(kind)
        if file:
            query += " AND f.path LIKE ?"
            params.append(f"%{file}%")
        
        return [dict(row) for row in self.db.execute(query, params)]
    
    def get_callers(self, function_name: str, limit: int = 20) -> list:
        query = """
            SELECT 
                caller.qualified_name as caller_name,
                f.path as file_path,
                c.line,
                c.column
            FROM calls c
            JOIN symbols callee ON c.callee_id = callee.id
            JOIN symbols caller ON c.caller_id = caller.id
            JOIN files f ON c.file_id = f.id
            WHERE callee.name = ? OR callee.qualified_name = ?
            LIMIT ?
        """
        return [dict(row) for row in self.db.execute(
            query, [function_name, function_name, limit]
        )]
    
    def get_class_hierarchy(self, class_name: str) -> dict:
        # 获取基类
        bases_query = """
            SELECT base.name, base.qualified_name, i.access, i.is_virtual
            FROM inheritance i
            JOIN symbols derived ON i.derived_id = derived.id
            JOIN symbols base ON i.base_id = base.id
            WHERE derived.name = ?
        """
        bases = [dict(row) for row in self.db.execute(bases_query, [class_name])]
        
        # 获取派生类
        derived_query = """
            SELECT derived.name, derived.qualified_name
            FROM inheritance i
            JOIN symbols derived ON i.derived_id = derived.id
            JOIN symbols base ON i.base_id = base.id
            WHERE base.name = ?
        """
        derived = [dict(row) for row in self.db.execute(derived_query, [class_name])]
        
        return {"bases": bases, "derived_classes": derived}

engine = QueryEngine("code_index.db")

@mcp.tool()
def get_symbol(name: str, kind: str = None, file: str = None) -> dict:
    """获取符号定义信息"""
    symbols = engine.get_symbol(name, kind, file)
    return {"symbols": symbols, "count": len(symbols)}

@mcp.tool()
def get_callers(function_name: str, limit: int = 20) -> dict:
    """查找调用指定函数的所有位置"""
    callers = engine.get_callers(function_name, limit)
    return {"function": function_name, "callers": callers, "count": len(callers)}

@mcp.tool()
def get_class_info(class_name: str, include_inherited: bool = True) -> dict:
    """获取类的完整信息"""
    # 基本信息
    symbol = engine.get_symbol(class_name, kind="class")
    if not symbol:
        return {"error": f"Class {class_name} not found"}
    
    # 继承关系
    hierarchy = engine.get_class_hierarchy(class_name)
    
    # 成员
    members = engine.get_class_members(class_name, include_inherited)
    
    return {
        "class": symbol[0],
        "bases": hierarchy["bases"],
        "derived_classes": hierarchy["derived_classes"],
        "members": members
    }

@mcp.tool()
def get_file_dependencies(file: str, direction: str = "both", 
                          recursive: bool = False) -> dict:
    """获取文件依赖关系"""
    result = {"file": file}
    
    if direction in ["includes", "both"]:
        result["includes"] = engine.get_file_includes(file, recursive)
    
    if direction in ["included_by", "both"]:
        result["included_by"] = engine.get_file_included_by(file)
    
    return result

@mcp.tool()
def find_references(symbol: str, kind_filter: list = None, 
                    limit: int = 50) -> dict:
    """查找符号的所有引用"""
    refs = engine.find_references(symbol, kind_filter, limit)
    return {"symbol": symbol, "references": refs, "total": len(refs)}

@mcp.tool()
def get_usage_examples(symbol: str, limit: int = 5, 
                       context_lines: int = 3) -> dict:
    """获取符号使用示例"""
    examples = engine.get_usage_examples(symbol, limit, context_lines)
    return {"symbol": symbol, "examples": examples}

@mcp.tool()
def index_status() -> dict:
    """查看索引状态"""
    return engine.get_index_status()

@mcp.tool()
def reindex(mode: str = "incremental", file: str = None) -> dict:
    """触发重新索引"""
    from indexer import CodeIndexer
    indexer = CodeIndexer("code_index.db", "build")
    
    if mode == "file" and file:
        indexer.index_file(file)
        return {"status": "completed", "files_indexed": 1}
    elif mode == "incremental":
        count = indexer.index_changed_files()
        return {"status": "completed", "files_indexed": count}
    else:
        count = indexer.full_reindex()
        return {"status": "completed", "files_indexed": count}
```

#### 3. 增量更新机制

```python
# watcher.py
import hashlib
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

class IndexUpdateHandler(FileSystemEventHandler):
    def __init__(self, indexer, db):
        self.indexer = indexer
        self.db = db
        self.pending_updates = set()
    
    def on_modified(self, event):
        if event.src_path.endswith(('.cpp', '.h', '.hpp', '.c')):
            self._queue_update(event.src_path)
    
    def on_created(self, event):
        if event.src_path.endswith(('.cpp', '.h', '.hpp', '.c')):
            self._queue_update(event.src_path)
    
    def _queue_update(self, file_path):
        # 检查文件是否真的变化了（通过 hash）
        new_hash = self._compute_hash(file_path)
        old_hash = self._get_stored_hash(file_path)
        
        if new_hash != old_hash:
            self.pending_updates.add(file_path)
    
    def process_pending(self):
        """处理待更新的文件（可以定时调用或手动触发）"""
        for file_path in self.pending_updates:
            self.indexer.index_file(file_path)
        
        count = len(self.pending_updates)
        self.pending_updates.clear()
        return count
    
    def _compute_hash(self, file_path):
        with open(file_path, 'rb') as f:
            return hashlib.md5(f.read()).hexdigest()
    
    def _get_stored_hash(self, file_path):
        row = self.db.execute(
            "SELECT hash FROM files WHERE path = ?", [file_path]
        ).fetchone()
        return row['hash'] if row else None
```

## MCP 配置文件

### mcp.json 配置示例

```json
{
  "mcpServers": {
    "code-analyzer": {
      "command": "python",
      "args": ["-m", "code_analyzer.server"],
      "env": {
        "CODE_INDEX_DB": "${workspaceFolder}/.code_index/index.db",
        "COMPILE_COMMANDS": "${workspaceFolder}/build/compile_commands.json",
        "PROJECT_ROOT": "${workspaceFolder}",
        "LOG_LEVEL": "INFO"
      },
      "disabled": false,
      "autoApprove": [
        "get_symbol",
        "get_class_info", 
        "get_callers",
        "get_callees",
        "get_file_dependencies",
        "find_references",
        "get_usage_examples",
        "index_status"
      ]
    }
  }
}
```

## 项目结构

```
code-analyzer-mcp/
├── pyproject.toml
├── README.md
├── src/
│   └── code_analyzer/
│       ├── __init__.py
│       ├── server.py          # MCP 服务入口
│       ├── query_engine.py    # 查询引擎
│       ├── indexer.py         # 代码索引器
│       ├── watcher.py         # 文件监控
│       ├── schema.sql         # 数据库 schema
│       └── models.py          # 数据模型
├── tests/
│   ├── test_indexer.py
│   ├── test_queries.py
│   └── fixtures/
│       └── sample_project/
└── scripts/
    ├── init_db.py             # 初始化数据库
    └── full_reindex.py        # 完整重建索引
```

## pyproject.toml

```toml
[project]
name = "code-analyzer-mcp"
version = "0.1.0"
description = "MCP server for C++ code analysis"
requires-python = ">=3.10"
dependencies = [
    "mcp>=1.0.0",
    "libclang>=16.0.0",
    "watchdog>=3.0.0",
]

[project.scripts]
code-analyzer = "code_analyzer.server:main"

[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"
```

## 使用流程

### 1. 初始化索引

```bash
# 首次使用，生成 compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 初始化数据库并完整索引
python -m code_analyzer.scripts.init_db
python -m code_analyzer.scripts.full_reindex
```

### 2. AI 使用示例

当 AI 需要修改 `RenderBox::layout` 函数时：

```
AI 内部流程:

1. 调用 get_symbol("layout", kind="method", file="render_box")
   → 获取函数签名、参数类型、返回值

2. 调用 get_class_info("RenderBox", include_inherited=true)
   → 了解 RenderBox 的所有成员和继承的方法

3. 调用 get_callers("RenderBox::layout")
   → 知道谁调用了这个函数，修改会影响哪里

4. 调用 get_callees("RenderBox::layout")
   → 知道这个函数内部调用了什么

5. 调用 get_usage_examples("LayoutContext")
   → 参考项目中 LayoutContext 的使用方式

→ 现在 AI 有足够信息精确编写代码，而不是猜测
```

### 3. 典型查询场景

| 场景 | 使用的工具 |
|------|-----------|
| 实现一个新的派生类 | `get_class_info` 获取基类所有虚函数 |
| 修改函数签名 | `get_callers` 找到所有调用点 |
| 理解模块边界 | `get_module_dependencies` 查看依赖图 |
| 添加新功能 | `get_usage_examples` 参考现有实现 |
| 重构代码 | `find_references` 找到所有使用位置 |
| 检查循环依赖 | `get_module_dependencies` + detect_cycles |

## 性能优化

### 索引策略

1. **增量索引**：只更新变化的文件
2. **延迟索引**：文件修改后延迟 2 秒再索引（避免频繁写入）
3. **后台索引**：不阻塞查询操作

### 查询优化

1. **索引覆盖**：常用查询字段都有索引
2. **结果缓存**：热点查询结果缓存 5 分钟
3. **分页返回**：大结果集分页，避免内存爆炸

### 数据库优化

```sql
-- 定期执行 VACUUM 和 ANALYZE
VACUUM;
ANALYZE;

-- 使用 WAL 模式提升并发性能
PRAGMA journal_mode=WAL;
PRAGMA synchronous=NORMAL;
```

## 扩展方向

### 未来可添加的工具

1. **`get_type_info`** - 获取类型定义（typedef、using、enum）
2. **`get_macro_expansion`** - 宏展开结果
3. **`get_template_instantiations`** - 模板实例化信息
4. **`suggest_includes`** - 根据使用的符号建议需要的 include
5. **`detect_code_smells`** - 检测代码异味（过长函数、过多参数等）
6. **`get_similar_code`** - 查找相似代码片段（用于参考或去重）

### 多语言支持

架构设计支持扩展到其他语言：
- JavaScript/TypeScript: 使用 tree-sitter 或 TypeScript compiler API
- Python: 使用 ast 模块或 jedi
- Rust: 使用 rust-analyzer

只需实现对应语言的 Indexer，数据库 schema 和查询引擎可复用。
