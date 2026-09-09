// mock-engine.js
//
// Fakes EngineWebSocketServer's wire protocol so you can test patchbay.tsx
// without building/running the real JUCE engine. Speaks the exact same
// message shapes as the real server (see EngineWebSocketServer.h/.cpp).
//
// Usage:
//   node mock-engine\mock-engine.js
//
// Then point the UI at ws://localhost:9001 as usual — it can't tell the
// difference between this and the real engine.

import { WebSocketServer } from 'ws';

const wss = new WebSocketServer({ port: 9001 });
console.log('Mock engine listening on ws://localhost:9001');

// Mirrors what registerEndpoint() + EndpointKind::Internal filtering produce.
const endpoints = [
  { id: 'mic-1', label: 'Mic', kind: 'source' },
  { id: 'mixminus-1', label: 'Mix-Minus', kind: 'source' },
  { id: 'speaker-out', label: 'Speakers', kind: 'destination' },
  { id: 'zoomSend-1', label: 'Zoom Send', kind: 'destination' },
];

// Mutated as fake "connect" messages come in, same as connectionNames on the server.
let connections = [{ from: 'mic-1', to: 'speaker-out' }];

function graphMessage() {
  return JSON.stringify({ type: 'endpoints', endpoints, connections });
}

function deviceListMessage() {
  return JSON.stringify({
    type: 'deviceList',
    devices: ['CABLE Input (VB-Audio Virtual Cable)', 'Built-in Microphone', 'Built-in Output'],
  });
}

wss.on('connection', (ws) => {
  console.log('client connected');

  // Matches setOpenHandler: deviceList, then the graph, sent unprompted.
  ws.send(deviceListMessage());
  ws.send(graphMessage());

  // Fake periodic level meters — sine-wave-ish so the bars visibly move.
  const start = Date.now();
  const levelsInterval = setInterval(() => {
    const t = (Date.now() - start) / 1000;
    for (const ep of endpoints) {
      const rms = (Math.sin(t * 2 + ep.id.length) + 1) / 2; // 0..1
      ws.send(JSON.stringify({ type: 'levels', node: ep.id, peak: Math.min(1, rms + 0.15), rms }));
    }
  }, 100);

  ws.on('message', (raw) => {
    let msg;
    try {
      msg = JSON.parse(raw.toString());
    } catch {
      console.warn('bad JSON from client:', raw.toString());
      return;
    }

    console.log('received:', msg);

    if (msg.type === 'getEndpoints') {
      ws.send(graphMessage());
    } else if (msg.type === 'connect') {
      const knownIds = new Set(endpoints.map((e) => e.id));
      const ok = knownIds.has(msg.from) && knownIds.has(msg.to);

      if (ok) connections.push({ from: msg.from, to: msg.to });

      // Broadcast to every connected client, same as the real server.
      const ack = JSON.stringify({ type: 'connect', ok, from: msg.from, to: msg.to });
      wss.clients.forEach((client) => client.readyState === 1 && client.send(ack));
    } else {
      console.log('unhandled type:', msg.type);
    }
  });

  ws.on('close', () => {
    clearInterval(levelsInterval);
    console.log('client disconnected');
  });
});