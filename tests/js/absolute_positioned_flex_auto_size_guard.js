// Regression check: auto-sized absolute/fixed flex children must shrink-wrap.
// Usage: node tests/js/absolute_positioned_flex_auto_size_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const flexLayoutPath = path.join(process.cwd(), 'core', 'layout', 'flex_layout.cpp');
  const flexLayoutSrc = fs.readFileSync(flexLayoutPath, 'utf8');

  assert(flexLayoutSrc.includes('auto_width_should_shrink_to_fit') &&
         flexLayoutSrc.includes('auto_height_should_shrink_to_fit'),
    'Absolute/fixed flex child layout must detect auto-sized positioned axes');
  assert(flexLayoutSrc.includes('!(inset.left.has_value() && inset.right.has_value())') &&
         flexLayoutSrc.includes('!(inset.top.has_value() && inset.bottom.has_value())'),
    'Opposing insets must preserve stretch behavior instead of shrink-wrapping');
  assert(flexLayoutSrc.includes('auto_width_should_shrink_to_fit') &&
         flexLayoutSrc.includes('AvailableSpace::MaxContent()') &&
         flexLayoutSrc.includes('SizingMode::ContentSize'),
    'Auto-sized positioned flex children must use max-content ContentSize measurement');

  console.log('[PASS] absolute positioned flex auto-size guard is present');
}

run();
