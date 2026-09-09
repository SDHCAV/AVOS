// engineSocket.ts
//
// Thin wrapper around the WebSocket connection to the AVOS audio engine.
// Matches EngineWebSocketServer.h / .cpp as of the "endpoints"/"connect ack"
// patch. Everything the engine sends is JSON with a `type` field:
//
//   -> (client sends on open — server also sends this unprompted on connect)
//   { type: "getEndpoints" }
//
//   <- (server responds with the current graph — internal nodes like
//      gain/panner are filtered out server-side via EndpointKind::Internal)
//   {
//     type: "endpoints",
//     endpoints: [
//       { id: "mic-1", label: "Mic", kind: "source" },
//       { id: "mixminus-1", label: "Mix-Minus", kind: "source" },
//       { id: "speaker-out", label: "Speakers", kind: "destination" },
//       { id: "zoomSend-1", label: "Zoom Send", kind: "destination" }
//     ],
//     connections: [
//       { from: "mic-1", to: "speaker-out" }
//     ]
//   }
//
//   <- (server confirms/rejects a connect request, broadcast to all clients)
//   { type: "connect", ok: true, from: "mixminus-1", to: "zoomSend-1" }
//
//   <- (periodic level meter update — matches wsServer.broadcastLevels(node, peak, rms))
//   { type: "levels", node: "mic-1", peak: 0.51, rms: 0.34 }
//
//   <- (hardware audio device names, sent once on open — separate from the
//      routing graph above; not consumed by Patchbay, but here for a future
//      device-picker component)
//   { type: "deviceList", devices: ["CABLE Input (VB-Audio Virtual Cable)", ...] }
//
// There's no "disconnect" handling on the server yet — sendDisconnect below
// sends the message but the engine currently just logs "Unknown message type".

export type EndpointKind = 'source' | 'destination';

export interface EngineEndpoint {
  id: string;
  label: string;
  kind: EndpointKind;
}

export interface EngineConnection {
  from: string;
  to: string;
}

interface EndpointsMessage {
  type: 'endpoints';
  endpoints: EngineEndpoint[];
  connections: EngineConnection[];
}

interface ConnectAckMessage {
  type: 'connect';
  ok: boolean;
  from: string;
  to: string;
}

interface LevelsMessage {
  type: 'levels';
  node: string;
  peak: number;
  rms: number;
}

interface DeviceListMessage {
  type: 'deviceList';
  devices: string[];
}

type EngineMessage = EndpointsMessage | ConnectAckMessage | LevelsMessage | DeviceListMessage;

type Listener<T> = (payload: T) => void;

const endpointsListeners = new Set<Listener<EndpointsMessage>>();
const connectAckListeners = new Set<Listener<ConnectAckMessage>>();
const levelsListeners = new Set<Listener<LevelsMessage>>();
const deviceListListeners = new Set<Listener<DeviceListMessage>>();

let socket: WebSocket | null = null;

function connect(url = 'ws://localhost:9001') {
  socket = new WebSocket(url);

  socket.onopen = () => {
    // Ask the engine for the current endpoint list + graph as soon as we're connected.
    socket?.send(JSON.stringify({ type: 'getEndpoints' }));
  };

  socket.onmessage = (event: MessageEvent) => {
    let msg: EngineMessage;
    try {
      msg = JSON.parse(event.data);
    } catch (err) {
      console.error('engineSocket: failed to parse message', event.data, err);
      return;
    }

    switch (msg.type) {
      case 'endpoints':
        endpointsListeners.forEach((fn) => fn(msg));
        break;
      case 'connect':
        connectAckListeners.forEach((fn) => fn(msg));
        break;
      case 'levels':
        levelsListeners.forEach((fn) => fn(msg));
        break;
      case 'deviceList':
        deviceListListeners.forEach((fn) => fn(msg));
        break;
      default:
        console.warn('engineSocket: unhandled message type', msg);
    }
  };

  socket.onclose = () => {
    // Basic reconnect — the engine may not be up yet, or the process restarted.
    setTimeout(() => connect(url), 1000);
  };

  socket.onerror = (err) => {
    console.error('engineSocket: socket error', err);
  };
}

connect();

export function onEndpoints(fn: Listener<EndpointsMessage>) {
  endpointsListeners.add(fn);
  return () => endpointsListeners.delete(fn);
}

export function onConnectAck(fn: Listener<ConnectAckMessage>) {
  connectAckListeners.add(fn);
  return () => connectAckListeners.delete(fn);
}

export function onLevels(fn: Listener<LevelsMessage>) {
  levelsListeners.add(fn);
  return () => levelsListeners.delete(fn);
}

export function onDeviceList(fn: Listener<DeviceListMessage>) {
  deviceListListeners.add(fn);
  return () => deviceListListeners.delete(fn);
}

export function sendConnect(from: string, to: string) {
  socket?.send(JSON.stringify({ type: 'connect', from, to }));
}

export function sendDisconnect(from: string, to: string) {
  socket?.send(JSON.stringify({ type: 'disconnect', from, to }));
}