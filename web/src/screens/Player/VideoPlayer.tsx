import { useEffect, useRef, useState } from 'react';
import { useNavigate, useParams } from 'react-router-dom';
import { useQuery } from '@tanstack/react-query';
import { colors, font } from '../../theme';
import { SeekBar } from '../../components/SeekBar';
import { getTitle } from '../../api/library';
import { videoUrl, saveProgress, getProgress } from '../../api/playback';
import { getPrefs } from '../../api/settings';
import { matchesLang } from '../../api/languages';

// Apply default audio/subtitle language prefs to a <video> (best-effort — depends on the
// browser exposing embedded textTracks/audioTracks for the stream).
function applyTrackPrefs(v: HTMLVideoElement, audioLanguage?: string, subtitleLanguage?: string) {
  const tts = v.textTracks;
  if (tts && subtitleLanguage) {
    for (let i = 0; i < tts.length; i++) {
      const tk = tts[i];
      tk.mode = subtitleLanguage !== 'off' && matchesLang(tk.language, subtitleLanguage) ? 'showing' : 'disabled';
    }
  }
  const ats = (v as unknown as { audioTracks?: { length: number; [i: number]: { language: string; enabled: boolean } } }).audioTracks;
  if (ats && audioLanguage && ats.length) {
    let any = false;
    for (let i = 0; i < ats.length; i++) { ats[i].enabled = matchesLang(ats[i].language, audioLanguage); if (ats[i].enabled) any = true; }
    if (!any) ats[0].enabled = true;
  }
}

const fmt = (s: number) => {
  s = Math.max(0, Math.round(s));
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const ss = s % 60;
  const mm = h ? String(m).padStart(2, '0') : String(m);
  return (h ? h + ':' : '') + mm + ':' + String(ss).padStart(2, '0');
};

// Full-viewport player. Real <video> against /api/video-transcode; resume restore + save;
// Media Session; Space (via SpatialFocusProvider) toggles play/pause.
export function VideoPlayer() {
  const { id = '' } = useParams();
  const navigate = useNavigate();
  const vidRef = useRef<HTMLVideoElement>(null);
  const t = useQuery({ queryKey: ['title', id], queryFn: () => getTitle(id) }).data;

  const [playing, setPlaying] = useState(true);
  const [cur, setCur] = useState(0);
  const [dur, setDur] = useState(0);
  const [buffered, setBuffered] = useState(0);
  const resumed = useRef(false);
  const prefs = useRef<{ audioLanguage?: string; subtitleLanguage?: string }>({});

  const path = t?.path;

  // Load default audio/subtitle language prefs once.
  useEffect(() => {
    getPrefs().then((p) => { prefs.current = { audioLanguage: p.audioLanguage, subtitleLanguage: p.subtitleLanguage }; });
  }, []);

  // Restore resume position once metadata + path are known.
  useEffect(() => {
    if (!path) return;
    getProgress(path).then((p) => {
      if (p.positionSec && vidRef.current) vidRef.current.currentTime = p.positionSec;
      resumed.current = true;
    });
  }, [path]);

  // Periodic + unmount progress save.
  useEffect(() => {
    if (!path) return;
    const iv = setInterval(() => {
      const v = vidRef.current;
      if (v && v.currentTime > 0) saveProgress({ filePath: path, titleId: t?.id, kind: 'video', positionSec: v.currentTime, durationSec: v.duration || 0 });
    }, 10_000);
    return () => {
      clearInterval(iv);
      const v = vidRef.current;
      if (v && v.currentTime > 0) saveProgress({ filePath: path, titleId: t?.id, kind: 'video', positionSec: v.currentTime, durationSec: v.duration || 0 });
    };
  }, [path, t?.id]);

  // Space → toggle (dispatched by SpatialFocusProvider).
  useEffect(() => {
    const toggle = () => {
      const v = vidRef.current;
      if (!v) return;
      if (v.paused) v.play();
      else v.pause();
    };
    window.addEventListener('marquee:toggleplay', toggle);
    return () => window.removeEventListener('marquee:toggleplay', toggle);
  }, []);

  // Media Session (lock-screen / headphone controls).
  useEffect(() => {
    if (!('mediaSession' in navigator) || !t) return;
    navigator.mediaSession.metadata = new MediaMetadata({ title: t.title, artist: t.genre, album: 'Marquee' });
    navigator.mediaSession.setActionHandler('play', () => vidRef.current?.play());
    navigator.mediaSession.setActionHandler('pause', () => vidRef.current?.pause());
    navigator.mediaSession.setActionHandler('seekto', (e) => { if (vidRef.current && e.seekTime != null) vidRef.current.currentTime = e.seekTime; });
  }, [t]);

  const togglePlay = () => {
    const v = vidRef.current;
    if (!v) return;
    if (v.paused) v.play();
    else v.pause();
  };
  const seekTo = (frac: number) => {
    const v = vidRef.current;
    if (v && dur) v.currentTime = frac * dur;
  };
  const nudge = (delta: number) => {
    const v = vidRef.current;
    if (v) v.currentTime = Math.max(0, Math.min(dur, v.currentTime + delta));
  };

  return (
    <div style={{ position: 'fixed', inset: 0, background: '#000' }}>
      {path ? (
        <video
          ref={vidRef}
          src={videoUrl(path)}
          autoPlay
          style={{ width: '100%', height: '100%', objectFit: 'contain', background: '#000' }}
          onPlay={() => setPlaying(true)}
          onPause={() => setPlaying(false)}
          onTimeUpdate={(e) => {
            const v = e.currentTarget;
            setCur(v.currentTime);
            if (v.buffered.length) setBuffered(v.buffered.end(v.buffered.length - 1) / (v.duration || 1));
          }}
          onLoadedMetadata={(e) => {
            const v = e.currentTarget;
            setDur(v.duration || 0);
            applyTrackPrefs(v, prefs.current.audioLanguage, prefs.current.subtitleLanguage);
            // tracks can arrive slightly after metadata — apply again shortly.
            setTimeout(() => applyTrackPrefs(v, prefs.current.audioLanguage, prefs.current.subtitleLanguage), 500);
          }}
        />
      ) : (
        <div style={{ position: 'absolute', inset: 0, display: 'flex', alignItems: 'center', justifyContent: 'center', color: colors.ink4, font: `500 12px ${font.mono}`, letterSpacing: '.2em' }}>LOADING…</div>
      )}

      {/* Top bar */}
      <div style={{ position: 'absolute', top: 0, left: 0, right: 0, padding: '20px 28px', display: 'flex', alignItems: 'center', justifyContent: 'space-between', background: 'linear-gradient(180deg,rgba(0,0,0,.6),transparent)' }}>
        <button data-focusable tabIndex={0} onClick={() => navigate(-1)} style={{ display: 'flex', alignItems: 'center', gap: 8, padding: '9px 14px', border: '1px solid rgba(236,239,247,.2)', borderRadius: 10, background: 'rgba(0,0,0,.4)', color: colors.ink1, font: `600 13px ${font.ui}`, cursor: 'pointer' }}>
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M15 5 L8 12 L15 19" /></svg>
          <span style={{ font: `500 11px ${font.mono}`, letterSpacing: '.16em', color: colors.ink3 }}>NOW PLAYING</span> {t?.title}
        </button>
        <div style={{ display: 'flex', gap: 8 }}>
          {t?.tags?.slice(0, 2).map((q) => (
            <span key={q} style={{ font: `500 10px ${font.mono}`, letterSpacing: '.12em', color: colors.ink2, padding: '5px 9px', borderRadius: 6, background: 'rgba(236,239,247,.1)' }}>{q}</span>
          ))}
        </div>
      </div>

      {/* Center play/pause */}
      <button data-focusable tabIndex={0} onClick={togglePlay} aria-label={playing ? 'Pause' : 'Play'} style={{ position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%,-50%)', width: 88, height: 88, borderRadius: '50%', border: '1px solid rgba(236,239,247,.3)', background: 'rgba(11,12,17,.4)', color: '#fff', cursor: 'pointer', backdropFilter: 'blur(4px)' }}>
        {playing ? (
          <svg width="30" height="30" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="5" width="4" height="14" rx="1" /><rect x="14" y="5" width="4" height="14" rx="1" /></svg>
        ) : (
          <svg width="30" height="30" viewBox="0 0 24 24" fill="currentColor"><path d="M8 5 L19 12 L8 19 Z" /></svg>
        )}
      </button>

      {/* Bottom controls */}
      <div style={{ position: 'absolute', left: 0, right: 0, bottom: 0, padding: '40px 28px 22px', background: 'linear-gradient(0deg,rgba(0,0,0,.75),transparent)' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 14 }}>
          <span style={{ font: `500 12px ${font.mono}`, color: colors.ink2, width: 56 }}>{fmt(cur)}</span>
          <div style={{ flex: 1 }}>
            <SeekBar value={dur ? cur / dur : 0} buffered={buffered} onSeek={seekTo} />
          </div>
          <span style={{ font: `500 12px ${font.mono}`, color: colors.ink2, width: 56, textAlign: 'right' }}>-{fmt(Math.max(0, dur - cur))}</span>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: 14 }}>
          <button data-focusable tabIndex={0} onClick={() => nudge(-10)} style={ctrlBtn}>−10s</button>
          <button data-focusable tabIndex={0} onClick={togglePlay} style={{ ...ctrlBtn, width: 52, height: 52, borderRadius: '50%', background: 'var(--ac)', color: colors.acInk, border: 0 }}>
            {playing ? <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="5" width="4" height="14" rx="1" /><rect x="14" y="5" width="4" height="14" rx="1" /></svg> : <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor"><path d="M8 5 L19 12 L8 19 Z" /></svg>}
          </button>
          <button data-focusable tabIndex={0} onClick={() => nudge(30)} style={ctrlBtn}>+30s</button>
          <div style={{ flex: 1 }} />
          <button data-focusable tabIndex={0} onClick={() => vidRef.current?.requestFullscreen?.()} style={ctrlBtn}>⛶</button>
        </div>
      </div>
    </div>
  );
}

const ctrlBtn: React.CSSProperties = {
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  padding: '10px 14px',
  border: '1px solid rgba(236,239,247,.2)',
  borderRadius: 10,
  background: 'rgba(236,239,247,.06)',
  color: '#eceef4',
  font: "600 13px 'Schibsted Grotesk'",
  cursor: 'pointer',
};
