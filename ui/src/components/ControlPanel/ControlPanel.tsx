import { useState } from 'react';
import { sendSetParam } from '../../lib/engineSocket';
import './controlpanel.css';

interface SliderControlProps {
  label: string;
  min: number;
  max: number;
  step: number;
  defaultValue: number;
  node: string;
  param: string;
  formatValue?: (v: number) => string;
}

function SliderControl({ label, min, max, step, defaultValue, node, param, formatValue }: SliderControlProps) {
  const [value, setValue] = useState(defaultValue);

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const v = parseFloat(e.target.value);
    setValue(v);
    sendSetParam(node, param, v);
  };

  return (
    <div className="control-row">
      <label className="control-label">{label}</label>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={handleChange}
        className="control-slider"
      />
      <span className="control-value">{formatValue ? formatValue(value) : value.toFixed(2)}</span>
    </div>
  );
}

export default function ControlPanel() {
  return (
    <div className="control-panel">
      <div className="control-section">
        <h3 className="control-section-title">Gain</h3>
        <SliderControl
          label="Level"
          min={0}
          max={2}
          step={0.01}
          defaultValue={1.0}
          node="gain-1"
          param="gain"
        />
      </div>

      <div className="control-section">
        <h3 className="control-section-title">Panner</h3>
        <SliderControl
          label="Pan"
          min={-1}
          max={1}
          step={0.01}
          defaultValue={0.0}
          node="panner-1"
          param="pan"
          formatValue={(v) => (v < 0 ? `L${Math.abs(v).toFixed(2)}` : v > 0 ? `R${v.toFixed(2)}` : 'C')}
        />
      </div>

      <div className="control-section">
        <h3 className="control-section-title">Gate</h3>
        <SliderControl
          label="Threshold"
          min={-60}
          max={0}
          step={1}
          defaultValue={-40}
          node="gate-1"
          param="threshold"
          formatValue={(v) => `${v.toFixed(0)} dB`}
        />
      </div>
    </div>
  );
}