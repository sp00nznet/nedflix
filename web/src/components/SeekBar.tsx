import { useRef, type PointerEvent } from 'react';

/**
 * Clickable/draggable seek track with a lighter "buffered" fill behind the accent
 * progress fill and a draggable knob. `value`/`buffered` are 0..1. onSeek(fraction).
 */
export function SeekBar({
  value,
  buffered = 0,
  onSeek,
  height = 6,
}: {
  value: number;
  buffered?: number;
  onSeek: (fraction: number) => void;
  height?: number;
}) {
  const ref = useRef<HTMLDivElement>(null);
  const dragging = useRef(false);

  const frac = (clientX: number) => {
    const el = ref.current;
    if (!el) return 0;
    const r = el.getBoundingClientRect();
    return Math.max(0, Math.min(1, (clientX - r.left) / r.width));
  };

  const down = (e: PointerEvent) => {
    dragging.current = true;
    (e.currentTarget as HTMLElement).setPointerCapture(e.pointerId);
    onSeek(frac(e.clientX));
  };
  const move = (e: PointerEvent) => {
    if (dragging.current) onSeek(frac(e.clientX));
  };
  const up = () => {
    dragging.current = false;
  };

  const v = Math.max(0, Math.min(1, value)) * 100;
  const b = Math.max(0, Math.min(1, buffered)) * 100;

  return (
    <div
      ref={ref}
      data-focusable
      tabIndex={0}
      role="slider"
      aria-valuenow={Math.round(v)}
      onPointerDown={down}
      onPointerMove={move}
      onPointerUp={up}
      onKeyDown={(e) => {
        if (e.key === 'ArrowLeft') onSeek(Math.max(0, value - 0.02));
        else if (e.key === 'ArrowRight') onSeek(Math.min(1, value + 0.02));
      }}
      style={{
        position: 'relative',
        height,
        borderRadius: 999,
        background: 'rgba(236,239,247,.16)',
        cursor: 'pointer',
        touchAction: 'none',
      }}
    >
      <div style={{ position: 'absolute', inset: 0, width: `${b}%`, background: 'rgba(236,239,247,.3)', borderRadius: 999 }} />
      <div style={{ position: 'absolute', inset: 0, width: `${v}%`, background: 'var(--ac)', borderRadius: 999 }} />
      <div
        style={{
          position: 'absolute',
          top: '50%',
          left: `${v}%`,
          width: height + 8,
          height: height + 8,
          borderRadius: '50%',
          background: '#fff',
          transform: 'translate(-50%,-50%)',
          boxShadow: '0 2px 6px rgba(0,0,0,.5)',
        }}
      />
    </div>
  );
}
