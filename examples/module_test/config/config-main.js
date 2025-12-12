export const appName = 'My Application';
export const version = '2.0.0';

export function getConfig() {
    return {
        name: appName,
        version: version,
        env: 'production'
    };
}
