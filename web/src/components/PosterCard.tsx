import { colors, font, shadow } from '../theme';
import { ArtPlaceholder } from './ArtPlaceholder';
import { ProgressBar } from './ProgressBar';

export interface PosterCardProps {
  title: string;
  meta?: string;
  posterUrl?: string;
  grad?: string;
  seed?: string;
  width?: number;
  progress?: number; // 0..1, shows resume bar when > 0
  onClick?: () => void;
}

/** 2:3 poster card (Home rails, Browse, Search) with focus lift/scale + accent ring. */
export function PosterCard({ title, meta, posterUrl, grad, seed, width = 186, progress = 0, onClick }: PosterCardProps) {
  return (
    <div
      data-focusable
      tabIndex={0}
      onClick={onClick}
      style={{ position: 'relative', flex: `0 0 ${width}px`, width, cursor: 'pointer', borderRadius: 12 }}
      className="poster-card"
    >
      <div
        style={{
          position: 'relative',
          aspectRatio: '2 / 3',
          borderRadius: 12,
          overflow: 'hidden',
          background: grad,
          boxShadow: shadow.card,
        }}
      >
        <ArtPlaceholder label="Artwork" src={posterUrl} grad={grad} seed={seed ?? title} />
        <div
          style={{
            position: 'absolute',
            inset: 0,
            background: 'linear-gradient(180deg,transparent 55%,rgba(8,9,13,.86) 100%)',
          }}
        />
        <div style={{ position: 'absolute', left: 12, right: 12, bottom: progress > 0 ? 12 : 10 }}>
          <div style={{ font: `600 15px ${font.ui}`, color: colors.ink0, letterSpacing: '-.01em', lineHeight: 1.15 }}>{title}</div>
        </div>
        {progress > 0 && (
          <div style={{ position: 'absolute', left: 0, right: 0, bottom: 0 }}>
            <ProgressBar value={progress} height={3} />
          </div>
        )}
      </div>
      {meta && <div style={{ marginTop: 8, font: `400 12px ${font.ui}`, color: colors.ink3 }}>{meta}</div>}
    </div>
  );
}
