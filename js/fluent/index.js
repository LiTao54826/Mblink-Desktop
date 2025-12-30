/**
 * MBink Fluent Design 组件库
 * 
 * 基于 Microsoft Fluent UI 设计规范实现的轻量级组件库
 * 专为 MBink 桌面应用框架优化
 * 
 * @version 1.0.0
 * @see https://fluent2.microsoft.design/
 */

// ============================================
// 主题系统
// ============================================
export { default as theme } from './theme.js';
export {
  colorPalette,
  colors,
  borderRadius,
  shadow,
  spacing,
  fontFamily,
  fontSize,
  fontWeight,
  lineHeight,
  componentSizes,
  duration,
  easing,
  transition,
} from './theme.js';

// ============================================
// 工具函数
// ============================================
export { mergeStyles, classNames, useId, debounce, throttle } from './utils.js';

// ============================================
// 按钮组件
// ============================================
export { Button, CompoundButton, ToggleButton } from './Button.js';

// ============================================
// 输入组件
// ============================================
export { Input, SearchBox } from './Input.js';
export { Textarea } from './Textarea.js';
export { Select } from './Select.js';
export { Checkbox } from './Checkbox.js';
export { Switch } from './Switch.js';
export { Radio, RadioGroup } from './Radio.js';

// ============================================
// 数据展示组件
// ============================================
export { Text, Title, Subtitle, Body, Caption, LargeTitle, Display } from './Text.js';
export { Badge, CounterBadge, PresenceBadge } from './Badge.js';
export { Avatar, AvatarGroup } from './Avatar.js';
export { Card, CardHeader, CardPreview, CardFooter } from './Card.js';
export { Divider } from './Divider.js';

// ============================================
// 反馈组件
// ============================================
export { Spinner, LoadingDots } from './Spinner.js';

// ============================================
// 导航组件
// ============================================
export { Link } from './Link.js';

// ============================================
// 默认导出
// ============================================
import { Button, CompoundButton, ToggleButton } from './Button.js';
import { Input, SearchBox } from './Input.js';
import { Textarea } from './Textarea.js';
import { Select } from './Select.js';
import { Checkbox } from './Checkbox.js';
import { Switch } from './Switch.js';
import { Radio, RadioGroup } from './Radio.js';
import { Text, Title, Subtitle, Body, Caption, LargeTitle, Display } from './Text.js';
import { Badge, CounterBadge, PresenceBadge } from './Badge.js';
import { Avatar, AvatarGroup } from './Avatar.js';
import { Card, CardHeader, CardPreview, CardFooter } from './Card.js';
import { Divider } from './Divider.js';
import { Spinner, LoadingDots } from './Spinner.js';
import { Link } from './Link.js';

export default {
  // 按钮
  Button,
  CompoundButton,
  ToggleButton,
  
  // 输入
  Input,
  SearchBox,
  Textarea,
  Select,
  Checkbox,
  Switch,
  Radio,
  RadioGroup,
  
  // 数据展示
  Text,
  Title,
  Subtitle,
  Body,
  Caption,
  LargeTitle,
  Display,
  Badge,
  CounterBadge,
  PresenceBadge,
  Avatar,
  AvatarGroup,
  Card,
  CardHeader,
  CardPreview,
  CardFooter,
  Divider,
  
  // 反馈
  Spinner,
  LoadingDots,
  
  // 导航
  Link,
};
