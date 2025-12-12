// Test folder imports
import { formatMessage, utilName } from './utils';  // Should load ./utils/index.js
import { getConfig, appName } from './config';      // Should load ./config/config-main.js (from package.json)

console.log('Folder import test:');
console.log('  Utils:', utilName);
console.log('  Message:', formatMessage('Hello'));
console.log('  App Name:', appName);
console.log('  Config:', JSON.stringify(getConfig()));

// Export for testing
globalThis.folderImportResults = {
    utilName: utilName,
    message: formatMessage('Test'),
    appName: appName,
    configEnv: getConfig().env
};
