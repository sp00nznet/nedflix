// The coral "bulb-dot" Marquee mark: monogram `m` in a dark rounded square with a
// coral dot top-right (rail/app icon), plus the `marquee` wordmark + glowing dot (topbar).
import { colors, font } from '../theme';

export function BrandGlyph({ size = 30 }: { size?: number }) {
  const r = Math.round(size * 0.3);
  const dot = Math.max(3, Math.round(size * 0.13));
  return (
    <div
      style={{
        position: 'relative',
        width: size,
        height: size,
        borderRadius: r,
        background: colors.panel,
        border: '1px solid rgba(251,113,89,.5)',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        color: 'var(--ac)',
        font: `700 ${Math.round(size * 0.53)}px/1 ${font.ui}`,
        boxShadow: '0 6px 18px -8px rgba(251,113,89,.6)',
      }}
    >
      <span>m</span>
      <span
        style={{
          position: 'absolute',
          top: Math.round(size * 0.17),
          right: Math.round(size * 0.17),
          width: dot,
          height: dot,
          borderRadius: '50%',
          background: 'var(--ac)',
        }}
      />
    </div>
  );
}

export function Wordmark({ size = 22 }: { size?: number }) {
  return (
    <div
      style={{
        display: 'flex',
        alignItems: 'flex-end',
        gap: 4,
        font: `700 ${size}px/1 ${font.ui}`,
        letterSpacing: '-.02em',
        color: colors.ink1,
      }}
    >
      <span>marquee</span>
      <span
        style={{
          width: 6,
          height: 6,
          borderRadius: '50%',
          background: 'var(--ac)',
          marginBottom: 4,
          boxShadow: '0 0 10px 1px rgba(251,113,89,.55)',
        }}
      />
    </div>
  );
}
