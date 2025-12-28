# HTML Elements

HTML 元素实现目录。

## 文件说明

此目录包含所有 HTML 元素的实现：

### 表单元素
- `html_input_element.h/cpp` - 输入框元素
- `html_textarea_element.h/cpp` - 多行文本框元素
- `html_button_element.h/cpp` - 按钮元素
- `html_select_element.h/cpp` - 下拉选择框元素
- `html_option_element.h/cpp` - 选项元素
- `html_form_element.h/cpp` - 表单元素
- `html_form_controls.h/cpp` - 表单控件基类
- `html_label_element.h/cpp` - 标签元素

### 文本元素
- `html_heading_element.h/cpp` - 标题元素 (h1-h6)
- `html_paragraph_element.h/cpp` - 段落元素
- `html_span_element.h/cpp` - 行内元素

### 容器元素
- `html_div_element.h/cpp` - 块级容器元素

### 列表元素
- `html_ulist_element.h/cpp` - 无序列表元素
- `html_olist_element.h/cpp` - 有序列表元素
- `html_li_element.h/cpp` - 列表项元素

### 表格元素
- `html_table_element.h/cpp` - 表格元素

### 媒体元素
- `html_image_element.h/cpp` - 图片元素
- `html_canvas_element.h/cpp` - 画布元素

### 链接和脚本
- `html_anchor_element.h/cpp` - 锚点/链接元素
- `html_link_element.h/cpp` - 外部资源链接元素
- `html_script_element.h/cpp` - 脚本元素
- `html_style_element.h/cpp` - 样式元素

### 文档结构
- `html_head_element.h/cpp` - 头部元素

### SVG
- `svg_element.h/cpp` - SVG 元素

## 依赖关系

所有 HTML 元素都继承自 `Element` 类（定义在 `core/dom/element.h`）。

## 参考

- [MDN HTML Elements](https://developer.mozilla.org/en-US/docs/Web/HTML/Element)
- [WHATWG HTML Standard](https://html.spec.whatwg.org/)
