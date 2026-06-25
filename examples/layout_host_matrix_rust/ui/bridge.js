const fallbackState = {
  sequence: 0,
  rows: 8
};

function backendFunction(name) {
  const backend = globalThis.backend;
  if (!backend || typeof backend[name] !== 'function') return null;
  return backend[name].bind(backend);
}

async function callHost(name, payload, fallback) {
  const fn = backendFunction(name);
  if (fn) return (await fn(payload ?? {})) ?? {};
  return fallback(payload ?? {});
}

function cleanText(value) {
  return String(value ?? '').trim();
}

export const hostApi = {
  getMatrixInfo(payload = {}) {
    return callHost('getMatrixInfo', payload, async () => ({
      host: 'mblink-ui-dev mock backend',
      runtime: 'tool',
      mode: 'dev-mock',
      receivedPurpose: payload.purpose ?? 'layout-matrix',
      capabilities: {
        backend: false,
        asyncRoundTrip: true,
        layoutMutation: true,
        errorPath: true
      }
    }));
  },

  mutateMatrix(payload = {}) {
    return callHost('mutateMatrix', payload, async (args) => {
      fallbackState.sequence += 1;
      const delta = Number(args.delta ?? 0);
      fallbackState.rows = Math.max(3, Math.min(24, fallbackState.rows + delta));
      return {
        ok: true,
        source: 'dev-mock',
        sequence: fallbackState.sequence,
        rows: fallbackState.rows,
        label: `mock mutation ${fallbackState.sequence}`
      };
    });
  },

  validatePayload(payload = {}) {
    return callHost('validatePayload', payload, async (args) => {
      const text = cleanText(args.text);
      return {
        ok: text.length > 0,
        source: 'dev-mock',
        normalized: text.toUpperCase(),
        length: text.length,
        message: text ? `mock accepted ${text.length} chars` : 'mock rejected empty text'
      };
    });
  },

  simulateFailure(payload = {}) {
    return callHost('simulateFailure', payload, async (args) => ({
      ok: false,
      source: 'dev-mock',
      code: 'MOCK_FAILURE',
      message: `mock failure for ${args.caseName ?? 'unknown'}`
    }));
  }
};
