import { colors, font, shadow } from '../theme';
import { ArtPlaceholder } from './ArtPlaceholder';

export interface HeroAction {
  label: string;
  icon?: React.ReactNode;
  variant: 'accent' | 'glass' | 'icon';
  onClick?: () => void;
}

/** Full-bleed featured hero: backdrop + stacked scrims + title block + actions. */
export function Hero({
  eyebrow,
  title,
  meta,
  synopsis,
  tags = [],
  backdropUrl,
  grad,
  seed,
  actions,
}: {
  eyebrow: string;
  title: string;
  meta?: string;
  synopsis?: string;
  tags?: string[];
  backdropUrl?: string;
  grad?: string;
  seed?: string;
  actions: HeroAction[];
}) {
  return (
    <section style={{ position: 'relative', height: 580, width: '100%', overflow: 'hidden' }}>
      <ArtPlaceholder label="Backdrop" src={backdropUrl} grad={grad} seed={seed ?? title} style={{ position: 'absolute' }} />
      {/* three stacked scrims */}
      <div style={{ position: 'absolute', inset: 0, background: 'linear-gradient(90deg,rgba(11,12,17,.92) 0%,rgba(11,12,17,.4) 45%,transparent 70%)' }} />
      <div style={{ position: 'absolute', inset: 0, background: 'linear-gradient(0deg,rgba(11,12,17,.95) 2%,transparent 40%)' }} />
      <div style={{ position: 'absolute', inset: 0, background: 'radial-gradient(120% 80% at 80% 20%,rgba(251,113,89,.10),transparent 60%)' }} />

      <div style={{ position: 'absolute', left: 40, bottom: 48, maxWidth: 620 }}>
        <div style={{ font: `500 11px/1 ${font.mono}`, letterSpacing: '.22em', color: 'var(--ac)', marginBottom: 16 }}>{eyebrow}</div>
        <h1 style={{ margin: '0 0 14px', font: `700 72px/0.98 ${font.ui}`, letterSpacing: '-.03em', color: colors.ink0 }}>{title}</h1>
        {meta && (
          <div style={{ display: 'flex', alignItems: 'center', gap: 12, flexWrap: 'wrap', marginBottom: 14 }}>
            <span style={{ font: `400 14px ${font.ui}`, color: colors.ink2 }}>{meta}</span>
            {tags.map((t) => (
              <span key={t} style={{ font: `500 11px ${font.mono}`, letterSpacing: '.08em', color: colors.ink2, padding: '4px 8px', borderRadius: 6, background: 'rgba(236,239,247,.08)' }}>
                {t}
              </span>
            ))}
          </div>
        )}
        {synopsis && <p style={{ margin: '0 0 22px', font: `400 16px/1.5 ${font.ui}`, color: colors.ink2 }}>{synopsis}</p>}
        <div style={{ display: 'flex', gap: 12 }}>
          {actions.map((a, i) =>
            a.variant === 'accent' ? (
              <button key={i} data-focusable tabIndex={0} onClick={a.onClick} style={{ display: 'flex', alignItems: 'center', gap: 10, padding: '15px 30px', border: 0, borderRadius: 12, background: 'var(--ac)', color: colors.acInk, font: `700 15px ${font.ui}`, cursor: 'pointer', boxShadow: shadow.accentBtn }}>
                {a.icon}
                {a.label}
              </button>
            ) : a.variant === 'glass' ? (
              <button key={i} data-focusable tabIndex={0} onClick={a.onClick} style={{ display: 'flex', alignItems: 'center', gap: 9, padding: '15px 26px', border: '1px solid rgba(236,239,247,.2)', borderRadius: 12, background: 'rgba(236,239,247,.06)', color: colors.ink1, font: `600 15px ${font.ui}`, cursor: 'pointer', backdropFilter: 'blur(6px)' }}>
                {a.icon}
                {a.label}
              </button>
            ) : (
              <button key={i} data-focusable tabIndex={0} onClick={a.onClick} title={a.label} style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', width: 52, height: 52, border: '1px solid rgba(236,239,247,.2)', borderRadius: 12, background: 'rgba(236,239,247,.06)', color: colors.ink1, cursor: 'pointer' }}>
                {a.icon}
              </button>
            ),
          )}
        </div>
      </div>
    </section>
  );
}
