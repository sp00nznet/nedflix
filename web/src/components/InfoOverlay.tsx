import { useEffect, useState } from 'react';
import { useQuery } from '@tanstack/react-query';
import { focusFirst } from '../focus/spatialNav';
import { colors, font, radius } from '../theme';
import { ArtPlaceholder } from './ArtPlaceholder';
import { getFileInfo, searchArtwork, refreshArtwork, setArtworkOverride, artworkHasTmdb, getDescription, type ArtCandidate } from '../api/info';
import { useMyList } from '../state/myList';
import type { InfoItem } from '../state/info';

const Row = ({ k, v }: { k: string; v?: string }) =>
  v ? (
    <div style={{ display: 'flex', justifyContent: 'space-between', gap: 16, padding: '7px 0', borderBottom: '1px solid rgba(236,239,247,.05)', font: `400 13px ${font.mono}` }}>
      <span style={{ color: colors.ink4 }}>{k}</span>
      <span style={{ color: colors.ink2, textAlign: 'right' }}>{v}</span>
    </div>
  ) : null;

const btn = (accent = false): React.CSSProperties => ({
  border: accent ? 0 : '1px solid rgba(236,239,247,.2)', borderRadius: 10, padding: '9px 16px', cursor: 'pointer',
  background: accent ? 'var(--ac)' : 'rgba(236,239,247,.06)', color: accent ? colors.acInk : colors.ink1, font: `600 13px ${font.ui}`,
});

export function InfoOverlay({ item, onClose }: { item: InfoItem; onClose: () => void }) {
  const [artVersion, setArtVersion] = useState(0);
  const [q, setQ] = useState(item.title);
  const [results, setResults] = useState<ArtCandidate[] | null>(null);
  const [searching, setSearching] = useState(false);
  const { has, toggle } = useMyList();

  const info = useQuery({ queryKey: ['fileinfo', item.path], queryFn: () => getFileInfo(item.path!), enabled: !!item.path });
  const tmdb = useQuery({ queryKey: ['art-tmdb'], queryFn: artworkHasTmdb });
  const year = (item.subtitle || '').match(/\b(?:19|20)\d{2}\b/)?.[0];
  const desc = useQuery({ queryKey: ['desc', item.id], queryFn: () => getDescription(item.kind, item.title, year), enabled: item.kind !== 'music' && item.kind !== 'channel' });

  // Esc / backdrop closes; flag so the global back-handler doesn't also navigate.
  useEffect(() => {
    (window as unknown as { __marqueeOverlayOpen?: boolean }).__marqueeOverlayOpen = true;
    const onKey = (e: KeyboardEvent) => { if (e.key === 'Escape' || e.key === 'Backspace') { e.preventDefault(); e.stopImmediatePropagation(); onClose(); } };
    window.addEventListener('keydown', onKey, true);
    const r = requestAnimationFrame(() => requestAnimationFrame(() => focusFirst())); // focus into the trap
    return () => {
      window.removeEventListener('keydown', onKey, true);
      cancelAnimationFrame(r);
      (window as unknown as { __marqueeOverlayOpen?: boolean }).__marqueeOverlayOpen = false;
    };
  }, [onClose]);

  const artSrc = item.artUrl ? `${item.artUrl}${item.artUrl.includes('?') ? '&' : '?'}v=${artVersion}` : undefined;

  const doRefresh = async () => { if (!item.path) return; await refreshArtwork(item.path); setArtVersion((v) => v + 1); };
  const runSearch = async (provider: 'itunes' | 'tmdb') => { setSearching(true); setResults(await searchArtwork(provider, item.kind, q)); setSearching(false); };
  const pick = async (c: ArtCandidate) => { if (!item.path) return; await setArtworkOverride(item.path, c.url); setArtVersion((v) => v + 1); setResults(null); };

  const t = info.data;

  return (
    <div onClick={onClose} style={{ position: 'fixed', inset: 0, zIndex: 2000, background: 'rgba(6,7,11,.72)', backdropFilter: 'blur(6px)', display: 'flex', alignItems: 'center', justifyContent: 'center', padding: 24 }}>
      <div data-focus-trap onClick={(e) => e.stopPropagation()} className="nf-rise" style={{ width: 720, maxWidth: '100%', maxHeight: '86vh', overflowY: 'auto', background: colors.bg1, border: '1px solid rgba(236,239,247,.1)', borderRadius: radius.panel, boxShadow: '0 30px 80px -20px rgba(0,0,0,.8)' }}>
        {/* header */}
        <div style={{ display: 'flex', gap: 18, padding: 22 }}>
          <div style={{ position: 'relative', width: 120, height: 180, borderRadius: 10, overflow: 'hidden', flex: '0 0 auto' }}>
            <ArtPlaceholder label="Artwork" src={artSrc} seed={item.id} />
          </div>
          <div style={{ flex: 1, minWidth: 0 }}>
            <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: 'var(--ac)', marginBottom: 8 }}>{item.kind.toUpperCase()} · INFO</div>
            <div style={{ font: `700 26px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>{item.title}</div>
            {item.subtitle && <div style={{ font: `400 14px ${font.ui}`, color: colors.ink3, marginTop: 4 }}>{item.subtitle}</div>}
            {desc.data && <p style={{ font: `400 14px/1.5 ${font.ui}`, color: colors.ink2, margin: '12px 0 0', maxHeight: 110, overflow: 'auto' }}>{desc.data}</p>}
            <div style={{ display: 'flex', gap: 10, marginTop: 16 }}>
              {item.path && (
                <button data-focusable tabIndex={0} onClick={() => toggle(item)} style={btn(!has(item.id) ? false : true)}>{has(item.id) ? '✓ In My List' : '+ My List'}</button>
              )}
              <button data-focusable tabIndex={0} onClick={onClose} style={btn()}>Close</button>
            </div>
          </div>
        </div>

        {/* technical */}
        <div style={{ padding: '0 22px 18px' }}>
          <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, marginBottom: 8 }}>FILE · TECHNICAL</div>
          {info.isLoading ? (
            <div style={{ font: `400 13px ${font.ui}`, color: colors.ink4, padding: '8px 0' }}>Reading…</div>
          ) : t ? (
            <div>
              <Row k="CONTAINER" v={t.container} />
              <Row k="VIDEO" v={t.video} />
              <Row k="RESOLUTION" v={t.resolution} />
              <Row k="FRAME RATE" v={t.fps} />
              <Row k="BITRATE" v={t.bitrate} />
              <Row k="DURATION" v={t.duration} />
              <Row k="SIZE" v={t.size} />
              <Row k="AUDIO" v={t.audio && t.audio.length ? t.audio.join('  ·  ') : undefined} />
              <Row k="SUBTITLES" v={t.subtitles && t.subtitles.length ? t.subtitles.join(', ') : undefined} />
              {t.note && <div style={{ font: `400 12px ${font.ui}`, color: colors.ink4, paddingTop: 8 }}>{t.note}</div>}
            </div>
          ) : (
            <div style={{ font: `400 13px ${font.ui}`, color: colors.ink4 }}>No technical details available.</div>
          )}
        </div>

        {/* artwork */}
        <div style={{ padding: '0 22px 22px', borderTop: '1px solid rgba(236,239,247,.06)' }}>
          <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, margin: '16px 0 10px' }}>ARTWORK</div>
          <div style={{ marginBottom: 12 }}>
            <button data-focusable tabIndex={0} onClick={doRefresh} style={btn()}>Refresh</button>
            <span style={{ font: `400 12px ${font.ui}`, color: colors.ink4, marginLeft: 10 }}>Re-fetches from your configured providers (local files, tags, TMDB, iTunes).</span>
          </div>
          <div style={{ display: 'flex', gap: 10, alignItems: 'center' }}>
            <input data-focusable value={q} onChange={(e) => setQ(e.target.value)} onKeyDown={(e) => e.key === 'Enter' && runSearch('itunes')} placeholder="Search artwork…" style={{ flex: 1, padding: '9px 12px', borderRadius: 10, border: '1px solid rgba(236,239,247,.14)', background: 'rgba(236,239,247,.04)', color: colors.ink0, font: `500 13px ${font.ui}`, outline: 'none' }} />
            <button data-focusable tabIndex={0} onClick={() => runSearch('itunes')} style={btn(true)}>{searching ? '…' : 'Search iTunes'}</button>
            {tmdb.data && <button data-focusable tabIndex={0} onClick={() => runSearch('tmdb')} style={btn()}>Search TMDB</button>}
          </div>
          {results && (
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(96px, 1fr))', gap: 10, marginTop: 14 }}>
              {results.length ? results.map((c, i) => (
                <button key={i} data-focusable tabIndex={0} onClick={() => pick(c)} title={`${c.title}${c.year ? ' (' + c.year + ')' : ''}`} style={{ border: 0, padding: 0, cursor: 'pointer', borderRadius: 8, overflow: 'hidden', background: 'none' }}>
                  <img src={c.thumb} alt={c.title} loading="lazy" style={{ width: '100%', aspectRatio: '2/3', objectFit: 'cover', display: 'block' }} />
                </button>
              )) : <div style={{ font: `400 13px ${font.ui}`, color: colors.ink4, gridColumn: '1/-1' }}>No results.</div>}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
