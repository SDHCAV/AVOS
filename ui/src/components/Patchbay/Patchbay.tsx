import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  ReactFlow,
  Background,
  Controls,
  Handle,
  Position,
  addEdge,
  useNodesState,
  useEdgesState,
  type Node,
  type Edge,
  type Connection,
  type NodeProps,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';
import {
  onEndpoints,
  onConnectAck,
  onDisconnectAck,
  onLevels,
  sendConnect,
  sendDisconnect,
  type EngineEndpoint,
} from '../../lib/engineSocket';
import ControlPanel from '../ControlPanel/ControlPanel';
import './patchbay.css';

interface PortNodeData {
  label: string;
  level: number; // 0..1, drives the meter bar fill
  clipping: boolean;
}

// Linear amplitude -> dBFS. 0 dBFS = 1.0
function linearToDb(linear: number): number {
  if (linear <= 0) return -Infinity;
  return 20 * Math.log10(linear);
}

// --- Meter Bar Component ---
function MeterBar({ level, clipping }: { level: number; clipping: boolean }) {
  const clamped = Math.min(1, Math.max(0, level));
  return (
    <div className="patchbay-meter">
      <div className="patchbay-meter-fill" style={{ width: `${clamped * 100}%` }} />
      <div className={`patchbay-meter-clip ${clipping ? 'patchbay-meter-clip--active' : ''}`} />
    </div>
  );
}

// --- Custom React Flow Nodes ---
function SourceNode({ data }: NodeProps<Node<PortNodeData>>) {
  return (
    <div className="patchbay-node patchbay-node--source">
      <div className="patchbay-node-label">{data.label}</div>
      <MeterBar level={data.level} clipping={data.clipping} />
      <Handle type="source" position={Position.Right} className="patchbay-handle" />
    </div>
  );
}

function DestinationNode({ data }: NodeProps<Node<PortNodeData>>) {
  return (
    <div className="patchbay-node patchbay-node--destination">
      <Handle type="target" position={Position.Left} className="patchbay-handle" />
      <div className="patchbay-node-label">{data.label}</div>
      <MeterBar level={data.level} clipping={data.clipping} />
    </div>
  );
}

const nodeTypes = {
  source: SourceNode,
  destination: DestinationNode,
};

// --- Graph Layout ---
const COLUMN_X = { source: 60, destination: 460 };
const ROW_HEIGHT = 90;
const ROW_START_Y = 40;

function layoutEndpoints(endpoints: EngineEndpoint[]): Node<PortNodeData>[] {
  const counters = { source: 0, destination: 0 };
  return endpoints.map((ep) => {
    const row = counters[ep.kind]++;
    return {
      id: ep.id,
      type: ep.kind,
      position: { x: COLUMN_X[ep.kind], y: ROW_START_Y + row * ROW_HEIGHT },
      data: { label: ep.label, level: 0, clipping: false },
    };
  });
}

export default function Patchbay() {
  const [nodes, setNodes, onNodesChange] = useNodesState<Node<PortNodeData>>([]);
  const [edges, setEdges, onEdgesChange] = useEdgesState<Edge>([]);
  const clipTimeouts = useRef<Map<string, ReturnType<typeof setTimeout>>>(new Map());

  // Subscribe to endpoint configuration updates from engine
  useEffect(() => {
    const unsubscribe = onEndpoints((msg) => {
      setNodes(layoutEndpoints(msg.endpoints));
      setEdges(
        msg.connections.map((c) => ({
          id: `${c.from}->${c.to}`,
          source: c.from,
          target: c.to,
        })),
      );
    });
    return unsubscribe;
  }, [setNodes, setEdges]);

  // Handle connection acknowledgments & rollbacks
  useEffect(() => {
  const unsubscribe = onDisconnectAck((msg) => {
    if (!msg.ok) {
      // If server disconnect failed, restore edge in UI
      setEdges((eds) =>
        addEdge(
          { id: `${msg.from}->${msg.to}`, source: msg.from, target: msg.to },
          eds,
        ),
      );
    }
  });
  return unsubscribe;
}, [setEdges]);

  // Meter level updates & clip indicator hold timing
  useEffect(() => {
    const unsubscribe = onLevels((msg) => {
      const isClippingNow = linearToDb(msg.peak) >= 0;

      if (isClippingNow) {
        const existing = clipTimeouts.current.get(msg.node);
        if (existing) clearTimeout(existing);

        const timeout = setTimeout(() => {
          setNodes((nds) =>
            nds.map((n) => (n.id === msg.node ? { ...n, data: { ...n.data, clipping: false } } : n)),
          );
          clipTimeouts.current.delete(msg.node);
        }, 1500);

        clipTimeouts.current.set(msg.node, timeout);
      }

      setNodes((nds) =>
        nds.map((n) =>
          n.id === msg.node
            ? { ...n, data: { ...n.data, level: msg.rms, clipping: isClippingNow || n.data.clipping } }
            : n,
        ),
      );
    });
    return unsubscribe;
  }, [setNodes]);

  const onConnect = useCallback(
    (connection: Connection) => {
      if (!connection.source || !connection.target) return;
      sendConnect(connection.source, connection.target);
      setEdges((eds) => addEdge(connection, eds));
    },
    [setEdges],
  );

  const proOptions = useMemo(() => ({ hideAttribution: true }), []);

  const onEdgesDelete = useCallback((deletedEdges: Edge[]) => {
    deletedEdges.forEach((edge) => {
      // Send disconnect message over WebSocket
      sendDisconnect(edge.source, edge.target);
    });
  }, []);

  return (
    <div className="patchbay" style={{ display: 'flex' }}>
      <div style={{ flex: 1 }}>
        <ReactFlow
          nodes={nodes}
          edges={edges}
          nodeTypes={nodeTypes}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
          onEdgesDelete={onEdgesDelete}
          onConnect={onConnect}
          proOptions={proOptions}
          fitView
        >
          <Background gap={24} size={1} />
          <Controls showInteractive={false} />
        </ReactFlow>
      </div>
      <ControlPanel />
    </div>
  );
}