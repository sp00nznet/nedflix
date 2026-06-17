/** Thin progress fill (accent over a faint track). `value` is 0..1. */
export function ProgressBar({
  value,
  height = 4,
  track = 'rgba(255,255,255,.16)',
}: {
  value: number;
  height?: number;
  track?: string;
}) {
  const pct = Math.max(0, Math.min(1, value)) * 100;
  return (
    <div style={{ height, background: track, borderRadius: 999, overflow: 'hidden' }}>
      <div style={{ height: '100%', width: `${pct}%`, background: 'var(--ac)' }} />
    </div>
  );
}
