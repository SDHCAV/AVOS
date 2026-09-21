import { useState, useEffect } from 'react';

interface MeterBarProps {
  peak: number; // raw linear value from engine, 0.0-1.0
}

// Convert linear amplitude to dBFS. 0 dBFS = full scale (1.0), -60 dBFS = near-silence.
function linearToDb(linear: number): number {
  if (linear <= 0) return -60;
  const db = 20 * Math.log10(linear);
  return Math.max(-60, db); // clamp floor at -60dB per the guide's spec
}

// Map dBFS to a 0-100% fill, per your build guide's Step 4.2 spec:
// -60dB -> 0%, 0dB -> 100%
function dbToPercent(db: number): number {
  const percent = ((db + 60) / 60) * 100;
  return Math.min(100, Math.max(0, percent));
}

export function MeterBar({ peak }: MeterBarProps) {
  const [isClipping, setIsClipping] = useState(false);

  const db = linearToDb(peak);
  const fillPercent = dbToPercent(db);
  const clipping = db >= 0; // crossed 0 dBFS

  // Hold the clip indicator lit briefly after a clip, so fast transients
  // are actually visible rather than flashing for a single video frame.
  useEffect(() => {
    if (clipping) {
      setIsClipping(true);
      const timeout = setTimeout(() => setIsClipping(false), 1500);
      return () => clearTimeout(timeout);
    }
  }, [clipping]);

  return (
    <div style={{
      position: 'relative',
      width: '100%',
      height: '8px',
      backgroundColor: '#1a1a1a',
      borderRadius: '4px',
      overflow: 'hidden',
    }}>
      {/* Normal fill */}
      <div style={{
        position: 'absolute',
        left: 0,
        top: 0,
        bottom: 0,
        width: `${fillPercent}%`,
        backgroundColor: fillPercent > 85 ? '#e0c34a' : '#4ade80', // amber as it approaches clip
        transition: 'width 50ms linear',
      }} />

      {/* Clip indicator - top segment, lights up red on crossing 0dBFS */}
      <div style={{
        position: 'absolute',
        right: 0,
        top: 0,
        bottom: 0,
        width: '6px',
        backgroundColor: isClipping ? '#ef4444' : '#3a1a1a',
        transition: 'background-color 100ms',
      }} />
    </div>
  );
}