const { spawn } = require('child_process');
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
    socket.once('error', reject);
  });
}

async function main() {
  const exe = path.resolve('build', 'bin', 'Release', 'esm_loader.exe');
  const entry = path.resolve('tmp', 'mbink_idle_cpu_probe', 'open_idempotent_app', '.dist', 'App.js');
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

  socket.destroy();
  console.log('[PASS] Devtools HTTP stop guard');
}

main().catch(err => {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});
