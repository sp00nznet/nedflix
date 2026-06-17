import { colors, font } from '../theme';

export interface Tab<T extends string> {
  key: T;
  label: string;
}

/** Sub-nav pill tabs (Music: Now/Albums/Artists · Audiobooks: Library/Listening). */
export function PillTabs<T extends string>({
  tabs,
  value,
  onChange,
}: {
  tabs: Tab<T>[];
  value: T;
  onChange: (k: T) => void;
}) {
  return (
    <div
      style={{
        display: 'inline-flex',
        gap: 4,
        padding: 4,
        borderRadius: 999,
        background: 'rgba(236,239,247,.04)',
        border: '1px solid rgba(236,239,247,.08)',
      }}
    >
      {tabs.map((t) => {
        const on = t.key === value;
        return (
          <button
            key={t.key}
            data-focusable
            tabIndex={0}
            onClick={() => onChange(t.key)}
            style={{
              border: 0,
              cursor: 'pointer',
              padding: '8px 16px',
              borderRadius: 999,
              background: on ? 'var(--ac)' : 'transparent',
              color: on ? colors.acInk : colors.ink2,
              font: `600 13px ${font.ui}`,
              transition: 'background .15s,color .15s',
            }}
          >
            {t.label}
          </button>
        );
      })}
    </div>
  );
}
