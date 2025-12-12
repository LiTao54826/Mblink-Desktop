// Math utilities - imports from nested helper
import { multiply, divide } from './helpers/calc.js';

export const PI = 3.14159;

export function add(a, b) {
    return a + b;
}

// Using imported multiply function
export function square(x) {
    return multiply(x, x);
}

// Using imported divide function
export function half(x) {
    return divide(x, 2);
}

export default square;
