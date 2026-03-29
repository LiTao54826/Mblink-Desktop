/**
 * MBink 布局组件
 */

import { h } from 'preact';
import { mergeStyles } from './utils.js';

/**
 * Row - 水平布局
 *
 * Props:
 * - gap: number | string
 * - align: 'start' | 'center' | 'end' | 'stretch' | 'baseline'
 * - justify: 'start' | 'center' | 'end' | 'between' | 'around' | 'evenly'
 * - wrap: boolean
 */
export function Row(props) {
  const {
    gap = 0,
    align = 'stretch',
    justify = 'start',
    wrap = false,
    children,
    style,
    ...rest
  } = props;

  const alignMap = {
    start: 'flex-start',
    center: 'center',
    end: 'flex-end',
    stretch: 'stretch',
    baseline: 'baseline',
  };

  const justifyMap = {
    start: 'flex-start',
    center: 'center',
    end: 'flex-end',
    between: 'space-between',
    around: 'space-around',
    evenly: 'space-evenly',
  };

  const rowStyle = {
    display: 'flex',
    flexDirection: 'row',
    alignItems: alignMap[align] || align,
    justifyContent: justifyMap[justify] || justify,
    gap: typeof gap === 'number' ? `${gap}px` : gap,
    flexWrap: wrap ? 'wrap' : 'nowrap',
  };

  return h('div', { style: mergeStyles(rowStyle, style), ...rest }, children);
}

/**
 * Column - 垂直布局
 *
 * Props:
 * - gap: number | string
 * - align: 'start' | 'center' | 'end' | 'stretch'
 * - justify: 'start' | 'center' | 'end' | 'between' | 'around'
 */
export function Column(props) {
  const {
    gap = 0,
    align = 'stretch',
    justify = 'start',
    children,
    style,
    ...rest
  } = props;

  const alignMap = {
    start: 'flex-start',
    center: 'center',
    end: 'flex-end',
    stretch: 'stretch',
  };

  const justifyMap = {
    start: 'flex-start',
    center: 'center',
    end: 'flex-end',
    between: 'space-between',
    around: 'space-around',
  };

  const columnStyle = {
    display: 'flex',
    flexDirection: 'column',
    alignItems: alignMap[align] || align,
    justifyContent: justifyMap[justify] || justify,
    gap: typeof gap === 'number' ? `${gap}px` : gap,
  };

  return h('div', { style: mergeStyles(columnStyle, style), ...rest }, children);
}

/**
 * Grid - 网格布局
 *
 * Props:
 * - columns: number | string
 * - rows: number | string
 * - gap: number | string
 * - rowGap: number | string
 * - columnGap: number | string
 */
export function Grid(props) {
  const {
    columns = 1,
    rows,
    gap = 0,
    rowGap,
    columnGap,
    children,
    style,
    ...rest
  } = props;

  const gridStyle = {
    display: 'grid',
    gridTemplateColumns:
      typeof columns === 'number' ? `repeat(${columns}, 1fr)` : columns,
    gridTemplateRows: rows
      ? typeof rows === 'number'
        ? `repeat(${rows}, 1fr)`
        : rows
      : undefined,
    gap: typeof gap === 'number' ? `${gap}px` : gap,
    rowGap: rowGap ? (typeof rowGap === 'number' ? `${rowGap}px` : rowGap) : undefined,
    columnGap: columnGap
      ? typeof columnGap === 'number'
        ? `${columnGap}px`
        : columnGap
      : undefined,
  };

  return h('div', { style: mergeStyles(gridStyle, style), ...rest }, children);
}

/**
 * Stack - 层叠布局（类似 Row/Column 的简化版）
 *
 * Props:
 * - direction: 'horizontal' | 'vertical'
 * - gap: number | string
 * - align: 'start' | 'center' | 'end' | 'stretch'
 */
export function Stack(props) {
  const {
    direction = 'vertical',
    gap = 0,
    align = 'stretch',
    children,
    style,
    ...rest
  } = props;

  const alignMap = {
    start: 'flex-start',
    center: 'center',
    end: 'flex-end',
    stretch: 'stretch',
  };

  const stackStyle = {
    display: 'flex',
    flexDirection: direction === 'horizontal' ? 'row' : 'column',
    alignItems: alignMap[align] || align,
    gap: typeof gap === 'number' ? `${gap}px` : gap,
  };

  return h('div', { style: mergeStyles(stackStyle, style), ...rest }, children);
}

/**
 * Spacer - 弹性空白
 *
 * Props:
 * - size: number | string (固定大小，不设置则自动填充)
 */
export function Spacer(props) {
  const { size, style, ...rest } = props;

  const spacerStyle = size
    ? {
        flexShrink: 0,
        width: typeof size === 'number' ? `${size}px` : size,
        height: typeof size === 'number' ? `${size}px` : size,
      }
    : {
        flex: 1,
      };

  return h('div', { style: mergeStyles(spacerStyle, style), ...rest });
}

export default { Row, Column, Grid, Stack, Spacer };
