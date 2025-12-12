// Main module that imports from relative path
import { add, square, half, PI } from './math.js';

// Export results for C++ to test
export const result1 = add(5, 3);        // 8
export const result2 = square(5);        // 25 (使用嵌套导入的 multiply)
export const result3 = half(10);         // 5 (使用嵌套导入的 divide)
export const piValue = PI;

// Set global for testing
globalThis.testResults = {
    add: result1,
    square: result2,
    half: result3,
    pi: piValue
};

console.log('Nested module import test:');
console.log('  add(5, 3) =', result1);
console.log('  square(5) =', result2);
console.log('  half(10) =', result3);
