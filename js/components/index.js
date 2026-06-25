/**
 * MBlink 组件库入口
 */

// 主题
export { default as theme } from './theme.js';
export * from './theme.js';

// 工具
export * from './utils.js';

// 基础组件
export { Button } from './button.js';
export { Input } from './input.js';
export { Textarea } from './textarea.js';
export { Select } from './select.js';
export { Checkbox } from './checkbox.js';
export { Radio } from './radio.js';
export { Switch } from './switch.js';

// 布局组件
export { Row, Column, Grid, Stack, Spacer } from './layout.js';

// 数据展示
export { Text } from './text.js';
export { Card } from './card.js';

// 反馈组件
export { Modal } from './modal.js';
export { Toast } from './toast.js';

// 默认导出所有组件
import { Button } from './button.js';
import { Input } from './input.js';
import { Textarea } from './textarea.js';
import { Select } from './select.js';
import { Checkbox } from './checkbox.js';
import { Radio } from './radio.js';
import { Switch } from './switch.js';

export default {
  Button,
  Input,
  Textarea,
  Select,
  Checkbox,
  Radio,
  Switch,
};
