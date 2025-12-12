// Test advanced module resolution features

// 1. Extension omission test
import { greet, libName } from './libs/lib';  // Should find lib.js

// 2. package.json exports test (manual path since we can't use node_modules)
import { identify } from './test_packages/my-package';  // Should use exports "."

console.log('Advanced module resolution test:');
console.log('  1. Extension omission:', libName, '-', greet('World'));
console.log('  2. Exports field:', identify());

globalThis.advancedResults = {
    libName: libName,
    greeting: greet('Test'),
    packageIdentity: identify()
};
