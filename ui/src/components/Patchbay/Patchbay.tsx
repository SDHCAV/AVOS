import { useCallback, useEffect, useMemo, useState } from 'react';
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
  onLevels,
  sendConnect,
  type EngineEndpoint,
} from '../../lib/engineSocket'; // components/patchbay/ -> lib/
import ControlPanel from '../ControlPanel/ControlPanel';
import './patchbay.css';

interface PortNodeData {
  label: string;
  level: number; // 0..1, drives the meter bar fill
}

// --- Meter bar -------------------------------------------------------
// Small horizontal LED-style bar. Full stereo metering with peak-hold
// lands in Month 4 — this is a single averaged level for now.

function MeterBar({ level }: { level: number }) {
  const clamped = Math.min(1, Math.max(0, level));
  return (
    <div className="patchbay-meter">
      <div className="patchbay-meter-fill" style={{ width: `${clamped * 100}%` }} />
    </div>
  );
}

// --- Custom nodes ------------------------------------------------------

function SourceNode({ data }: NodeProps<Node<PortNodeData>>) {
  return (
    <div className="patchbay-node patchbay-node--source">
      <div className="patchbay-node-label">{data.label}</div>
      <MeterBar level={data.level} />
      <Handle type="source" position={Position.Right} className="patchbay-handle" />
    </div>
  );
}

function DestinationNode({ data }: NodeProps<Node<PortNodeData>>) {
  return (
    <div className="patchbay-node patchbay-node--destination">
      <Handle type="target" position={Position.Left} className="patchbay-handle" />
      <div className="patchbay-node-label">{data.label}</div>
      <MeterBar level={data.level} />
    </div>
  );
}

const nodeTypes = {
  source: SourceNode,
  destination: DestinationNode,
};

// --- Layout --------------------------------------------------------
// Sources in a left column, destinations in a right column, stacked
// top to bottom in the order the engine reports them.

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
      data: { label: ep.label, level: 0 },
    };
  });
}

export default function Patchbay() {
  const [nodes, setNodes, onNodesChange] = useNodesState<Node<PortNodeData>>([]);
  const [edges, setEdges, onEdgesChange] = useEdgesState<Edge>([]);

  // Populate the graph once the engine reports its endpoints + existing connections.
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

  // If the engine rejects a connect request, drop the optimistically-added edge.
  useEffect(() => {
    const unsubscribe = onConnectAck((msg) => {
      if (!msg.ok) {
        setEdges((eds) => eds.filter((e) => !(e.source === msg.from && e.target === msg.to)));
      }
    });
    return unsubscribe;
  }, [setEdges]);

  // Live meter updates — patch the matching node's data without touching
  // anything else (React Flow only re-renders the node whose data changed).
  useEffect(() => {
    const unsubscribe = onLevels((msg) => {
      // rms for the meter fill — steadier than peak, which is fine for a
      // glance-level bar; swap to msg.peak if you want it twitchier.
      setNodes((nds) =>
        nds.map((n) => (n.id === msg.node ? { ...n, data: { ...n.data, level: msg.rms } } : n)),
      );
    });
    return unsubscribe;
  }, [setNodes]);

  const onConnect = useCallback(
    (connection: Connection) => {
      if (!connection.source || !connection.target) return;
      sendConnect(connection.source, connection.target);
      // Optimistic UI update; onConnectAck rolls this back on rejection.
      setEdges((eds) => addEdge(connection, eds));
    },
    [setEdges],
  );

  const proOptions = useMemo(() => ({ hideAttribution: true }), []);

  return (
    <div className="patchbay" style={{ display: 'flex' }}>
      <div style={{ flex: 1 }}>
        <ReactFlow
          nodes={nodes}
          edges={edges}
          nodeTypes={nodeTypes}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
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