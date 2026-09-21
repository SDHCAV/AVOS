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

interface DisconnectAckMessage {
  type: 'disconnect';
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

type EngineMessage = 
  | EndpointsMessage 
  | ConnectAckMessage 
  | DisconnectAckMessage
  | LevelsMessage 
  | DeviceListMessage;

type Listener<T> = (payload: T) => void;

const endpointsListeners = new Set<Listener<EndpointsMessage>>();
const connectAckListeners = new Set<Listener<ConnectAckMessage>>();
const disconnectAckListeners = new Set<Listener<DisconnectAckMessage>>();
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
      case 'disconnect':
        disconnectAckListeners.forEach((fn) => fn(msg));
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

export function onDisconnectAck(fn: Listener<DisconnectAckMessage>) {
  disconnectAckListeners.add(fn);
  return () => disconnectAckListeners.delete(fn);
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
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({ type: 'disconnect', from, to }));
  }
}

export function sendSetParam(node: string, param: string, value: number) {
  socket?.send(JSON.stringify({ type: 'setParam', node, param, value }));
}

