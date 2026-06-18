import { colors, font, shadow } from '../theme';
import { ArtPlaceholder } from './ArtPlaceholder';
import { ProgressBar } from './ProgressBar';
import { infoProps, useInfo, type InfoItem } from '../state/info';

export interface WideCardProps {
  title: string;
  context?: string; // "S2 E11 · 18 min left"
  backdropUrl?: string;
  grad?: string;
  seed?: string;
  width?: number;
  progress?: number; // 0..1
  onClick?: () => void;
  info?: InfoItem;
}

/** 16:9 continue-watching card: progress bar + resume play overlay. */
export function WideCard({ title, context, backdropUrl, grad, seed, width = 300, progress = 0, onClick, info }: WideCardProps) {
  const { open } = useInfo();
  return (
    <div
      data-focusable
      tabIndex={0}
      onClick={onClick}
      {...(info ? infoProps(info, open) : {})}
      style={{ position: 'relative', flex: `0 0 ${width}px`, width, cursor: 'pointer', borderRadius: 12 }}
      className="wide-card"
    >
      <div style={{ position: 'relative', aspectRatio: '16 / 9', borderRadius: 12, overflow: 'hidden', boxShadow: shadow.card }}>
        <ArtPlaceholder label="Backdrop" src={backdropUrl} grad={grad} seed={seed ?? title} />
        <div style={{ position: 'absolute', inset: 0, background: 'linear-gradient(180deg,transparent 45%,rgba(8,9,13,.82) 100%)' }} />
        <div
          style={{
            position: 'absolute',
            top: '50%',
            left: '50%',
            transform: 'translate(-50%,-50%)',
            width: 44,
            height: 44,
            borderRadius: '50%',
            background: 'rgba(11,12,17,.55)',
            border: '1px solid rgba(236,239,247,.4)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            backdropFilter: 'blur(4px)',
          }}
        >
          <svg width="16" height="16" viewBox="0 0 24 24" fill="#fff">
            <path d="M7 5 L19 12 L7 19 Z" />
          </svg>
        </div>
        <div style={{ position: 'absolute', left: 14, right: 14, bottom: 12 }}>
          <div style={{ font: `600 16px ${font.ui}`, color: colors.ink0 }}>{title}</div>
        </div>
        {progress > 0 && (
          <div style={{ position: 'absolute', left: 0, right: 0, bottom: 0 }}>
            <ProgressBar value={progress} height={4} />
          </div>
        )}
      </div>
      {context && <div style={{ marginTop: 8, font: `400 12px ${font.ui}`, color: colors.ink3 }}>{context}</div>}
    </div>
  );
}
