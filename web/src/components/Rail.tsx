import type { ReactNode } from 'react';
import { colors, font } from '../theme';

/** Horizontal scroller with a heading + optional "view all". 40px gutters, 18px gap. */
export function Rail({
  title,
  onViewAll,
  children,
}: {
  title: string;
  onViewAll?: () => void;
  children: ReactNode;
}) {
  return (
    <section style={{ marginBottom: 34 }}>
      <div
        style={{
          display: 'flex',
          alignItems: 'baseline',
          justifyContent: 'space-between',
          padding: '0 40px',
          marginBottom: 16,
        }}
      >
        <h2 style={{ margin: 0, font: `600 21px ${font.ui}`, letterSpacing: '-.01em', color: colors.ink1 }}>{title}</h2>
        {onViewAll && (
          <button
            data-focusable
            tabIndex={0}
            onClick={onViewAll}
            style={{ border: 0, background: 'none', cursor: 'pointer', color: colors.ink3, font: `600 12px ${font.mono}`, letterSpacing: '.1em' }}
          >
            VIEW ALL
          </button>
        )}
      </div>
      <div
        style={{
          display: 'flex',
          gap: 18,
          overflowX: 'auto',
          padding: '6px 40px 10px',
          scrollbarWidth: 'none',
        }}
      >
        {children}
      </div>
    </section>
  );
}
