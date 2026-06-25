const { spawn } = require('child_process');
const fs = require('fs');
const net = require('net');
const path = require('path');

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function waitForExit(child, timeoutMs) {
  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      reject(new Error(`process did not exit within ${timeoutMs}ms`));
    }, timeoutMs);
    child.once('exit', (code, signal) => {
      clearTimeout(timer);
      resolve({ code, signal });
    });
    child.once('error', err => {
      clearTimeout(timer);
      reject(err);
    });
  });
}

function waitForDevtoolsEvent(child) {
  return new Promise((resolve, reject) => {
    let buffer = '';
    const handleChunk = chunk => {
      buffer += chunk.toString('utf8');
      let newline;
      while ((newline = buffer.indexOf('\n')) >= 0) {
        const line = buffer.slice(0, newline).trim();
        buffer = buffer.slice(newline + 1);
        if (!line) continue;
        try {
          const parsed = JSON.parse(line);
          if (parsed && parsed.event === 'devtools_http_mcp') {
            resolve(parsed);
            return;
          }
        } catch (_) {
          // Ignore non-JSON chatter.
        }
      }
    };

    const fail = err => reject(err);
    child.stdout.on('data', handleChunk);
    child.stderr.on('data', handleChunk);
    child.once('exit', (code, signal) => {
      reject(new Error(`process exited before devtools event (code=${code}, signal=${signal})`));
    });
    child.once('error', fail);
  });
}

function requestWithBadOrigin(port, token) {
  return new Promise((resolve, reject) => {
    let data = '';
    const body = JSON.stringify({ jsonrpc: '2.0', id: 1, method: 'tools/list' });
    const socket = net.createConnection({ host: '127.0.0.1', port });
    socket.once('connect', () => {
      socket.write(
        `POST /mcp HTTP/1.1\r\n` +
        `Host: 127.0.0.1\r\n` +
        `Origin: http://127.0.0.1.evil.example\r\n` +
        `Authorization: Bearer ${token}\r\n` +
        `Content-Type: application/json\r\n` +
        `Content-Length: ${Buffer.byteLength(body)}\r\n` +
        `\r\n` +
        body
      );
    });
    socket.on('data', chunk => {
      data += chunk.toString('utf8');
    });
    socket.once('end', () => resolve(data));
    socket.once('close', () => resolve(data));
    socket.once('error', err => {
      if (err && err.code === 'ECONNRESET' && data) {
        resolve(data);
        return;
      }
      reject(err);
    });
  });
}

function requestRawJson(port, token, body) {
  return new Promise((resolve, reject) => {
    let data = '';
    const socket = net.createConnection({ host: '127.0.0.1', port });
    socket.once('connect', () => {
      socket.write(
        `POST /mcp HTTP/1.1\r\n` +
        `Host: 127.0.0.1\r\n` +
        `Origin: http://127.0.0.1:${port}\r\n` +
        `Authorization: Bearer ${token}\r\n` +
        `Content-Type: application/json\r\n` +
        `Content-Length: ${Buffer.byteLength(body)}\r\n` +
        `\r\n` +
        body
      );
    });
    socket.on('data', chunk => {
      data += chunk.toString('utf8');
    });
    socket.once('end', () => resolve(data));
    socket.once('close', () => resolve(data));
    socket.once('error', reject);
  });
}

function holdValidSnapshotRequest(port, token) {
  return new Promise((resolve, reject) => {
    const body = JSON.stringify({
      jsonrpc: '2.0',
      id: 2,
      method: 'tools/call',
      params: { name: 'snapshot_ui', arguments: { max_nodes: 100, max_depth: 8 } },
    });
    const socket = net.createConnection({ host: '127.0.0.1', port });
    socket.setTimeout(8000, () => {
      socket.destroy(new Error('valid request socket timed out'));
    });
    socket.once('connect', () => {
      socket.write(
        `POST /mcp HTTP/1.1\r\n` +
        `Host: 127.0.0.1\r\n` +
        `Origin: http://127.0.0.1:${port}\r\n` +
        `Authorization: Bearer ${token}\r\n` +
        `Content-Type: application/json\r\n` +
        `Content-Length: ${Buffer.byteLength(body)}\r\n` +
        `\r\n` +
        body
      );
      resolve(socket);
    });
    socket.once('error', reject);
  });
}

async function main() {
  const devtoolsSrc = fs.readFileSync(path.resolve('core', 'devtools', 'devtools_entry.cpp'), 'utf8');
  assert(devtoolsSrc.includes('BCryptGenRandom') && !devtoolsSrc.includes('mt19937'),
    'devtools HTTP auth token must use the OS RNG, not a non-cryptographic PRNG');
  const bridgeSrc = fs.readFileSync(path.resolve('core', 'devtools', 'devtools_bridge.h'), 'utf8');
  assert(bridgeSrc.includes('set_main_thread_sync_cancelled'),
    'devtools HTTP stop must be able to cancel in-flight main-thread marshaling');

  const exe = path.resolve('build', 'bin', 'Release', 'esm_loader.exe');
  const entry = path.resolve('tmp', 'mblink_idle_cpu_probe', 'open_idempotent_app', '.dist', 'App.js');
  const child = spawn(exe, [entry, '--devtools-http-mcp', '--quit', '3'], {
    cwd: process.cwd(),
    windowsHide: true,
  });

  const devtools = await waitForDevtoolsEvent(child);
  assert(devtools && devtools.port > 0, 'missing devtools_http_mcp port');
  assert(typeof devtools.auth_token === 'string' && devtools.auth_token.length > 0,
    'missing devtools auth token');

  const badOriginResponse = await requestWithBadOrigin(devtools.port, devtools.auth_token);
  assert(badOriginResponse.startsWith('HTTP/1.1 403'),
    `expected 403 for deceptive loopback origin, got: ${badOriginResponse.split('\r\n')[0]}`);

  const malformedJsonRpcResponse = await requestRawJson(devtools.port, devtools.auth_token, '"not-object"');
  assert(malformedJsonRpcResponse.startsWith('HTTP/1.1 200'),
    `expected malformed JSON-RPC to be handled as JSON-RPC error, got: ${malformedJsonRpcResponse.split('\r\n')[0]}`);
  assert(malformedJsonRpcResponse.includes('"code":-32600') || malformedJsonRpcResponse.includes('"code": -32600'),
    `expected JSON-RPC invalid request error, got: ${malformedJsonRpcResponse}`);

  const batchResponse = await requestRawJson(
    devtools.port,
    devtools.auth_token,
    JSON.stringify([
      { jsonrpc: '2.0', id: 31, method: 'tools/list' },
      { jsonrpc: '2.0', id: 32, method: 'tools/call', params: [] },
    ])
  );
  assert(batchResponse.startsWith('HTTP/1.1 200'),
    `expected batch JSON-RPC to be handled, got: ${batchResponse.split('\r\n')[0]}`);
  assert(batchResponse.includes('"id":31') || batchResponse.includes('"id": 31'),
    `expected first batch response, got: ${batchResponse}`);
  assert(batchResponse.includes('"id":32') || batchResponse.includes('"id": 32'),
    `expected second batch response, got: ${batchResponse}`);
  assert(batchResponse.includes('"code":-32602') || batchResponse.includes('"code": -32602'),
    `expected invalid params error in batch, got: ${batchResponse}`);

  const validSocket = await holdValidSnapshotRequest(devtools.port, devtools.auth_token);

  const socket = net.createConnection({ host: '127.0.0.1', port: devtools.port });
  await new Promise((resolve, reject) => {
    socket.once('connect', resolve);
    socket.once('error', reject);
  });

  socket.write(
    `POST /mcp HTTP/1.1\r\n` +
    `Host: 127.0.0.1\r\n` +
    `Authorization: Bearer ${devtools.auth_token}\r\n` +
    `Content-Type: application/json\r\n` +
    `Content-Length: 1000000\r\n` +
    `\r\n` +
    `{"jsonrpc":"2.0","id":1,"method":"tools/list"`
  );

  const exit = await waitForExit(child, 10000);
  assert(exit.code === 0, `expected clean exit, got code=${exit.code} signal=${exit.signal}`);

  validSocket.destroy();
  socket.destroy();
  console.log('[PASS] Devtools HTTP stop guard');
}

main().catch(err => {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});
