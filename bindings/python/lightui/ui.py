"""
LightUI 声明式 Python UI 组件库

用法:
    from lightui.ui import Column, Row, Text, Button, Input

    app.ui(
        Column(
            Text("Hello World", font_size=32),
            Button("Click me", on_click="my_func"),
            align="center", padding=32
        )
    )
"""

import html as _html

# ========== 响应式JS脚本 ==========
# 自动注入到 HTML 中，实现 state → DOM 的响应式绑定

_REACTIVE_JS = r"""
(function() {
  // 文本绑定: data-bind="state_name"
  document.querySelectorAll('[data-bind]').forEach(function(el) {
    var name = el.getAttribute('data-bind');
    host.state.watch(name, function(v) { el.textContent = v; });
  });

  // 输入双向绑定: data-bind-input="state_name"
  document.querySelectorAll('[data-bind-input]').forEach(function(el) {
    var name = el.getAttribute('data-bind-input');
    host.state.watch(name, function(v) {
      if (document.activeElement !== el) el.value = v;
    });
    el.addEventListener('input', function(e) {
      host.state.set(name, e.target.value);
    });
  });

  // Checkbox 绑定: data-bind-checked="state_name"
  document.querySelectorAll('[data-bind-checked]').forEach(function(el) {
    var name = el.getAttribute('data-bind-checked');
    host.state.watch(name, function(v) { el.checked = !!v; });
    el.addEventListener('change', function(e) {
      host.state.set(name, e.target.checked);
    });
  });

  // 可见性绑定: data-bind-visible="state_name"
  document.querySelectorAll('[data-bind-visible]').forEach(function(el) {
    var name = el.getAttribute('data-bind-visible');
    var orig = el.style.display || '';
    host.state.watch(name, function(v) {
      el.style.display = v ? orig : 'none';
    });
  });
})();
"""

# ========== 样式映射 ==========

_CSS_MAP = {
    "padding": "padding", "margin": "margin",
    "width": "width", "height": "height", "min_width": "min-width",
    "min_height": "min-height", "max_width": "max-width", "max_height": "max-height",
    "font_size": "font-size", "font_weight": "font-weight", "font_family": "font-family",
    "color": "color", "bg": "background", "background": "background",
    "border": "border", "border_radius": "border-radius",
    "gap": "gap", "align": "align-items", "justify": "justify-content",
    "flex": "flex", "opacity": "opacity", "cursor": "cursor",
    "overflow": "overflow", "text_align": "text-align",
    "line_height": "line-height", "letter_spacing": "letter-spacing",
    "box_shadow": "box-shadow", "transition": "transition",
    "white_space": "white-space", "word_break": "word-break",
}

# 这些属性的数值会自动加 px
_PX_PROPS = {
    "padding", "margin", "width", "height", "min_width", "min_height",
    "max_width", "max_height", "font_size", "gap", "border_radius",
    "line_height", "letter_spacing",
}


# ========== 基类 ==========

class Component:
    """所有 UI 组件的基类"""
    _id_counter = 0

    def __init__(self, *children, **kwargs):
        Component._id_counter += 1
        self._id = f"_lui_{Component._id_counter}"
        self._children = list(children)
        self._class = kwargs.pop("class_", "")
        self._extra_attrs = {}
        self._style_kwargs = {}
        # 分离样式 kwargs 和其他参数
        for k, v in kwargs.items():
            if k in _CSS_MAP:
                self._style_kwargs[k] = v
            # 子类会在 __init__ 中处理其他 kwargs

    def render(self):
        """渲染为 HTML 字符串"""
        raise NotImplementedError

    def _style_str(self):
        parts = []
        for k, v in self._style_kwargs.items():
            css_name = _CSS_MAP.get(k)
            if css_name:
                if isinstance(v, (int, float)) and k in _PX_PROPS:
                    v = f"{v}px"
                parts.append(f"{css_name}: {v}")
        return "; ".join(parts)

    def _render_children(self):
        parts = []
        for child in self._children:
            if isinstance(child, Component):
                parts.append(child.render())
            elif isinstance(child, str):
                parts.append(_html.escape(child))
        return "\n".join(parts)

    def _tag(self, tag, content="", extra_style="", extra_attrs="", self_closing=False):
        style = self._style_str()
        if extra_style:
            style = f"{extra_style}; {style}" if style else extra_style
        attrs = f' id="{self._id}"'
        if self._class:
            attrs += f' class="{_html.escape(self._class)}"'
        if style:
            attrs += f' style="{style}"'
        if extra_attrs:
            attrs += f" {extra_attrs}"
        if self_closing:
            return f"<{tag}{attrs} />"
        return f"<{tag}{attrs}>{content}</{tag}>"


# ========== 布局组件 ==========

class Column(Component):
    """垂直弹性布局容器"""
    def render(self):
        return self._tag("div", self._render_children(),
                         extra_style="display: flex; flex-direction: column")


class Row(Component):
    """水平弹性布局容器"""
    def render(self):
        return self._tag("div", self._render_children(),
                         extra_style="display: flex; flex-direction: row; align-items: center")


class Box(Component):
    """通用 div 容器"""
    def render(self):
        return self._tag("div", self._render_children())


class Spacer(Component):
    """弹性间距"""
    def __init__(self, flex=1, **kwargs):
        super().__init__(**kwargs)
        if "flex" not in self._style_kwargs:
            self._style_kwargs["flex"] = flex

    def render(self):
        return self._tag("div", "")


# ========== 内容组件 ==========

class Text(Component):
    """文本显示组件，支持 bind 到 State 实现响应式更新"""
    def __init__(self, content="", bind=None, tag="span", **kwargs):
        super().__init__(**kwargs)
        self._content = content
        self._bind = bind
        self._tag_name = tag

    def render(self):
        extra_attrs = ""
        if self._bind is not None:
            state_name = self._bind._name if hasattr(self._bind, '_name') else str(self._bind)
            extra_attrs = f'data-bind="{_html.escape(state_name)}"'
            text = str(self._bind.value) if hasattr(self._bind, 'value') else str(self._bind)
        else:
            text = _html.escape(self._content)
        return self._tag(self._tag_name, text, extra_attrs=extra_attrs)


class Button(Component):
    """按钮组件"""
    def __init__(self, label="", on_click=None, **kwargs):
        super().__init__(**kwargs)
        self._label = label
        self._on_click = on_click  # 字符串(函数名) 或 callable

    def render(self):
        extra_attrs = ""
        if isinstance(self._on_click, str):
            extra_attrs = f"onclick=\"host.call('{_html.escape(self._on_click)}')\""
        extra_style = "cursor: pointer"
        return self._tag("button", _html.escape(self._label),
                         extra_style=extra_style, extra_attrs=extra_attrs)


class Input(Component):
    """输入框组件，支持双向绑定"""
    def __init__(self, placeholder="", bind=None, input_type="text", **kwargs):
        super().__init__(**kwargs)
        self._placeholder = placeholder
        self._bind = bind
        self._input_type = input_type

    def render(self):
        extra_attrs = f'type="{self._input_type}" placeholder="{_html.escape(self._placeholder)}"'
        if self._bind is not None:
            state_name = self._bind._name if hasattr(self._bind, '_name') else str(self._bind)
            val = str(self._bind.value) if hasattr(self._bind, 'value') else ""
            extra_attrs += f' data-bind-input="{_html.escape(state_name)}"'
            extra_attrs += f' value="{_html.escape(val)}"'
        return self._tag("input", extra_attrs=extra_attrs, self_closing=True)


class Checkbox(Component):
    """复选框组件，支持双向绑定"""
    def __init__(self, label="", bind=None, **kwargs):
        super().__init__(**kwargs)
        self._label = label
        self._bind = bind

    def render(self):
        extra_attrs = 'type="checkbox"'
        if self._bind is not None:
            state_name = self._bind._name if hasattr(self._bind, '_name') else str(self._bind)
            checked = self._bind.value if hasattr(self._bind, 'value') else False
            extra_attrs += f' data-bind-checked="{_html.escape(state_name)}"'
            if checked:
                extra_attrs += ' checked'
        inp = self._tag("input", extra_attrs=extra_attrs, self_closing=True)
        if self._label:
            return f'<label>{inp} {_html.escape(self._label)}</label>'
        return inp


class Image(Component):
    """图片组件"""
    def __init__(self, src="", alt="", **kwargs):
        super().__init__(**kwargs)
        self._src = src
        self._alt = alt

    def render(self):
        extra_attrs = f'src="{_html.escape(self._src)}" alt="{_html.escape(self._alt)}"'
        return self._tag("img", extra_attrs=extra_attrs, self_closing=True)


class Link(Component):
    """链接组件"""
    def __init__(self, text="", href="#", **kwargs):
        super().__init__(**kwargs)
        self._text = text
        self._href = href

    def render(self):
        extra_attrs = f'href="{_html.escape(self._href)}"'
        return self._tag("a", _html.escape(self._text), extra_attrs=extra_attrs)


class Divider(Component):
    """分割线"""
    def render(self):
        return self._tag("hr", self_closing=True,
                         extra_style="border: none; border-top: 1px solid #e0e0e0; width: 100%")


# ========== HTML 包装 ==========

_DEFAULT_CSS = """
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif; }
button { cursor: pointer; padding: 6px 16px; border: 1px solid #ccc; border-radius: 4px; background: #fff; }
button:hover { background: #f5f5f5; }
button:active { background: #e0e0e0; }
input { padding: 6px 10px; border: 1px solid #ccc; border-radius: 4px; outline: none; }
input:focus { border-color: #2196f3; box-shadow: 0 0 0 2px rgba(33,150,243,0.2); }
hr { margin: 8px 0; }
"""


def build_html(root_component):
    """将组件树渲染为完整的 HTML 文档"""
    body_html = root_component.render()
    return f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
{_DEFAULT_CSS}
</style>
</head>
<body>
{body_html}
<script>
{_REACTIVE_JS}
</script>
</body>
</html>"""

