import { colors, font } from '../theme';

/** Fixed bottom-right key legend for the 10-foot UI. Hidden on the player route. */
export function DpadHint() {
  return (
    <div
      style={{
        position: 'fixed',
        right: 18,
        bottom: 16,
        padding: '8px 14px',
        borderRadius: 999,
        background: 'rgba(14,15,21,.82)',
        border: '1px solid rgba(236,239,247,.1)',
        backdropFilter: 'blur(8px)',
        color: colors.ink4,
        font: `500 10px/1 ${font.mono}`,
        letterSpacing: '.14em',
        pointerEvents: 'none',
        zIndex: 30,
      }}
    >
      ◄ ▲ ▼ ► NAVIGATE · ↵ SELECT · esc BACK
    </div>
  );
}
