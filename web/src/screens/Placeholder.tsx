import { useFocusable } from '../focus/useFocusable';
import { colors, font } from '../theme';

// Temporary surface for routes not yet built (Phases 2–5). Carries one focusable so the
// spatial-focus engine has a target on every screen.
export function Placeholder({ title, note }: { title: string; note?: string }) {
  const focus = useFocusable();
  return (
    <div className="nf-rise" style={{ padding: '64px 40px' }}>
      <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.22em', color: 'var(--ac)', marginBottom: 14 }}>MARQUEE</div>
      <h1 style={{ margin: '0 0 12px', font: `700 40px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>{title}</h1>
      <p style={{ font: `400 15px ${font.ui}`, color: colors.ink3, maxWidth: 520 }}>{note ?? 'This surface is part of the Marquee build and will be wired up in an upcoming phase.'}</p>
      <button {...focus} style={{ marginTop: 22, padding: '12px 22px', borderRadius: 12, border: '1px solid rgba(236,239,247,.2)', background: 'rgba(236,239,247,.06)', color: colors.ink1, font: `600 14px ${font.ui}`, cursor: 'pointer' }}>
        Coming soon
      </button>
    </div>
  );
}
