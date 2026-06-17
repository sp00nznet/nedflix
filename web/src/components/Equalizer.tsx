import { useMemo } from 'react';

export type VizMode = 'bars' | 'mirror' | 'pulse';

/**
 * Decorative animated equalizer (nf-eq). Per-bar randomized duration/delay, three
 * modes. Optionally drive from a Web Audio AnalyserNode later (see DATA_SPEC §3).
 */
export function Equalizer({ mode = 'bars', bars = 52 }: { mode?: VizMode; bars?: number }) {
  // Memoized per (mode,bars) so the random pattern is stable across renders.
  const specs = useMemo(
    () =>
      Array.from({ length: bars }, () => ({
        h: 30 + Math.round(Math.random() * 55),
        dur: (0.6 + Math.random() * 0.85).toFixed(2),
        del: (Math.random() * 0.9).toFixed(2),
      })),
    [mode, bars],
  );

  return (
    <div
      style={{
        display: 'flex',
        alignItems: mode === 'mirror' ? 'center' : 'flex-end',
        gap: 3,
        height: '100%',
        width: '100%',
      }}
    >
      {specs.map((s, i) => (
        <div
          key={i}
          style={{
            flex: 1,
            height: mode === 'pulse' ? '70%' : `${s.h}%`,
            borderRadius: mode === 'pulse' ? 999 : 3,
            background: 'var(--ac)',
            transformOrigin: mode === 'mirror' ? 'center' : 'bottom',
            animation: `nf-eq ${s.dur}s ease-in-out ${s.del}s infinite`,
          }}
        />
      ))}
    </div>
  );
}

/** Small live 3-bar equalizer (now-playing row indicator). */
export function MiniEq() {
  return (
    <div style={{ display: 'flex', alignItems: 'flex-end', gap: 2, height: 14, width: 14 }}>
      {[0, 0.2, 0.4].map((d, i) => (
        <div
          key={i}
          style={{
            flex: 1,
            height: '100%',
            background: 'var(--ac)',
            borderRadius: 2,
            transformOrigin: 'bottom',
            animation: `nf-eq ${0.7 + i * 0.15}s ease-in-out ${d}s infinite`,
          }}
        />
      ))}
    </div>
  );
}
