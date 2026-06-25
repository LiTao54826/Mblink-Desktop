const mockState = {
  count: 0
};

function backendFunction(name) {
  const backend = globalThis.backend;
  if (!backend || typeof backend[name] !== 'function') return null;
  return backend[name].bind(backend);
}

async function callBackend(name, payload, fallback) {
  const fn = backendFunction(name);
  if (fn) return (await fn(payload ?? {})) ?? {};
  return fallback(payload ?? {});
}

function cleanText(value) {
  return String(value ?? '').trim();
}

export const hostApi = {
  getTemplateInfo(defaults = {}) {
    return callBackend('getTemplateInfo', defaults, async () => ({
      purpose: defaults.purpose ?? 'minimal',
      runtime: 'tool',
      host: 'mblink-ui-dev mock backend',
      mode: 'dev-mock',
      capabilities: {
        backend: false,
        borderless: defaults.purpose === 'desktop-app',
        tray: defaults.purpose === 'desktop-app',
        reload: true,
        snapshot: true
      }
    }));
  },

  incrementCounter(payload = {}) {
    return callBackend('incrementCounter', payload, async (args) => {
      mockState.count += Number(args.delta ?? 1);
      return {
        count: mockState.count,
        source: 'dev-mock'
      };
    });
  },

  submitValidation(payload = {}) {
    return callBackend('submitValidation', payload, async (args) => {
      const text = cleanText(args.text);
      return {
        ok: true,
        value: text,
        message: text ? `dev mock accepted: ${text}` : 'dev mock accepted an empty value'
      };
    });
  },

  trayAction(payload = {}) {
    return callBackend('trayAction', payload, async (args) => ({
      ok: true,
      action: args.action ?? 'status',
      message: `dev mock tray action: ${args.action ?? 'status'}`
    }));
  }
};
