import { useLocation, useNavigate } from 'react-router-dom';
import { colors, font, gradFor } from '../theme';
import { ArtPlaceholder } from './ArtPlaceholder';
import { SeekBar } from './SeekBar';
import { useAudioPlayer } from '../state/audioPlayer';
import { useIsMobile } from '../shell/useIsMobile';

// Persistent now-playing bar for audio (music / audiobooks / podcasts). Hidden on the
// video player and when nothing is loaded. Tapping the track opens the full Now Playing.
export function MiniPlayer() {
  const p = useAudioPlayer();
  const isMobile = useIsMobile();
  const { pathname } = useLocation();
  const navigate = useNavigate();

  if (!p.current || pathname.startsWith('/watch')) return null;
  const cur = p.current;
  const frac = p.durationSec ? p.positionSec / p.durationSec : 0;

  return (
    <div
      style={{
        position: 'fixed',
        left: isMobile ? 0 : 98,
        right: 0,
        bottom: isMobile ? 'calc(58px + env(safe-area-inset-bottom))' : 0,
        height: 64,
        zIndex: 60,
        display: 'flex',
        alignItems: 'center',
        gap: 14,
        padding: '0 16px',
        background: 'rgba(16,18,26,.96)',
        borderTop: '1px solid rgba(236,239,247,.1)',
        backdropFilter: 'blur(14px)',
      }}
    >
      {/* progress line across the very top of the bar */}
      <div style={{ position: 'absolute', top: 0, left: 0, right: 0 }}>
        <SeekBar value={frac} onSeek={p.seek} height={3} />
      </div>

      <button
        data-focusable
        tabIndex={0}
        onClick={() => navigate('/music')}
        style={{ display: 'flex', alignItems: 'center', gap: 12, border: 0, background: 'none', cursor: 'pointer', minWidth: 0, flex: 1, padding: 0, textAlign: 'left' }}
      >
        <div style={{ position: 'relative', width: 44, height: 44, borderRadius: 8, overflow: 'hidden', flex: '0 0 auto' }}>
          <ArtPlaceholder label="" grad={gradFor(cur.album || cur.title)} src={cur.artUrl} />
        </div>
        <div style={{ minWidth: 0 }}>
          <div style={{ font: `600 14px ${font.ui}`, color: colors.ink0, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{cur.title}</div>
          {cur.artist && <div style={{ font: `400 12px ${font.ui}`, color: colors.ink3, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{cur.artist}</div>}
        </div>
      </button>

      <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
        {!isMobile && (
          <button data-focusable tabIndex={0} onClick={p.prev} aria-label="Previous" style={ctrl}>
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinejoin="round"><path d="M6 5 v14 M20 5 L9 12 L20 19 Z" /></svg>
          </button>
        )}
        <button data-focusable tabIndex={0} onClick={p.toggle} aria-label={p.playing ? 'Pause' : 'Play'} style={{ ...ctrl, width: 40, height: 40, borderRadius: '50%', background: 'var(--ac)', color: colors.acInk }}>
          {p.playing ? <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="5" width="4" height="14" rx="1" /><rect x="14" y="5" width="4" height="14" rx="1" /></svg> : <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M8 5 L19 12 L8 19 Z" /></svg>}
        </button>
        <button data-focusable tabIndex={0} onClick={p.next} aria-label="Next" style={ctrl}>
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinejoin="round"><path d="M18 5 v14 M4 5 L15 12 L4 19 Z" /></svg>
        </button>
      </div>
    </div>
  );
}

const ctrl: React.CSSProperties = {
  display: 'flex', alignItems: 'center', justifyContent: 'center', width: 36, height: 36,
  border: 0, background: 'none', cursor: 'pointer', color: '#eceef4', borderRadius: 8,
};
