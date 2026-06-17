import { useSearchParams } from 'react-router-dom';
import { useQuery } from '@tanstack/react-query';
import { useState } from 'react';
import { colors, font, gradFor, radius } from '../../theme';
import { PillTabs } from '../../components/PillTabs';
import { SeekBar } from '../../components/SeekBar';
import { Equalizer, MiniEq, type VizMode } from '../../components/Equalizer';
import { ArtPlaceholder } from '../../components/ArtPlaceholder';
import { listAlbums, listArtists, albumTracks } from '../../api/music';
import { useAudioPlayer, type QueueItem } from '../../state/audioPlayer';
import type { Album } from '../../api/types';

type Tab = 'now' | 'albums' | 'artists';

export function Music() {
  const [params, setParams] = useSearchParams();
  const tab = (params.get('tab') as Tab) || 'now';
  const setTab = (t: Tab) => setParams(t === 'now' ? {} : { tab: t });

  return (
    <div className="nf-rise" style={{ padding: '48px 40px' }}>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: 28 }}>
        <h1 style={{ margin: 0, font: `700 40px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>Music</h1>
        <PillTabs<Tab>
          tabs={[{ key: 'now', label: 'Now Playing' }, { key: 'albums', label: 'Albums' }, { key: 'artists', label: 'Artists' }]}
          value={tab}
          onChange={setTab}
        />
      </div>
      {tab === 'now' && <NowPlaying />}
      {tab === 'albums' && <AlbumGrid onPlay={() => setTab('now')} />}
      {tab === 'artists' && <ArtistGrid />}
    </div>
  );
}

function NowPlaying() {
  const p = useAudioPlayer();
  const [viz, setViz] = useState<VizMode>('bars');
  const cur = p.current;

  if (!cur) {
    return <div style={{ color: colors.ink3, font: `400 15px ${font.ui}` }}>Pick an album to start playing.</div>;
  }
  const frac = p.durationSec ? p.positionSec / p.durationSec : 0;

  return (
    <div style={{ display: 'grid', gridTemplateColumns: '1.25fr 1fr', gap: 28 }}>
      <div style={{ background: colors.panel, borderRadius: radius.panel, padding: 22 }}>
        <div style={{ position: 'relative', aspectRatio: '16/9', borderRadius: 14, overflow: 'hidden', marginBottom: 18 }}>
          <ArtPlaceholder label="Album art" grad={gradFor(cur.album || cur.title)} src={cur.artUrl} />
          <div style={{ position: 'absolute', left: 0, right: 0, bottom: 0, height: 64, padding: '0 16px 14px' }}>
            <Equalizer mode={viz} />
          </div>
        </div>
        <div style={{ display: 'flex', gap: 6, marginBottom: 16 }}>
          {(['bars', 'mirror', 'pulse'] as VizMode[]).map((m) => (
            <button key={m} data-focusable tabIndex={0} onClick={() => setViz(m)} style={{ border: 0, cursor: 'pointer', padding: '5px 10px', borderRadius: 6, font: `500 10px ${font.mono}`, letterSpacing: '.1em', background: viz === m ? 'var(--ac)' : 'rgba(236,239,247,.06)', color: viz === m ? colors.acInk : colors.ink3 }}>{m.toUpperCase()}</button>
          ))}
        </div>
        <div style={{ font: `700 30px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>{cur.title}</div>
        <div style={{ font: `400 14px ${font.ui}`, color: colors.ink3, marginBottom: 18 }}>{[cur.artist, cur.album].filter(Boolean).join(' · ')}</div>
        <SeekBar value={frac} onSeek={p.seek} />
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 20, marginTop: 20 }}>
          <TransportBtn label="prev" onClick={p.prev}><path d="M6 5 v14 M20 5 L9 12 L20 19 Z" /></TransportBtn>
          <button data-focusable tabIndex={0} onClick={p.toggle} style={{ width: 62, height: 62, borderRadius: '50%', border: 0, background: 'var(--ac)', color: colors.acInk, cursor: 'pointer' }}>
            {p.playing ? <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="5" width="4" height="14" rx="1" /><rect x="14" y="5" width="4" height="14" rx="1" /></svg> : <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor"><path d="M8 5 L19 12 L8 19 Z" /></svg>}
          </button>
          <TransportBtn label="next" onClick={p.next}><path d="M18 5 v14 M4 5 L15 12 L4 19 Z" /></TransportBtn>
        </div>
      </div>

      <div>
        <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, marginBottom: 14 }}>UP NEXT</div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
          {p.queue.map((q, i) => (
            <div key={q.id} data-focusable tabIndex={0} style={{ display: 'flex', alignItems: 'center', gap: 12, padding: '10px 12px', borderRadius: 10, cursor: 'pointer', background: i === p.index ? 'rgba(236,239,247,.05)' : 'transparent' }}>
              <div style={{ width: 16, display: 'flex', justifyContent: 'center', color: i === p.index ? 'var(--ac)' : colors.ink4 }}>
                {i === p.index && p.playing ? <MiniEq /> : <span style={{ font: `400 13px ${font.mono}` }}>♪</span>}
              </div>
              <div style={{ font: `500 14px ${font.ui}`, color: i === p.index ? colors.ink0 : colors.ink2 }}>{q.title}</div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

function TransportBtn({ children, onClick, label }: { children: React.ReactNode; onClick: () => void; label: string }) {
  return (
    <button data-focusable tabIndex={0} onClick={onClick} aria-label={label} style={{ border: 0, background: 'none', cursor: 'pointer', color: colors.ink2 }}>
      <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinejoin="round">{children}</svg>
    </button>
  );
}

function AlbumGrid({ onPlay }: { onPlay: () => void }) {
  const q = useQuery({ queryKey: ['albums'], queryFn: listAlbums });
  const player = useAudioPlayer();

  const openAlbum = async (a: Album) => {
    const tracks = await albumTracks(a.id);
    const items: QueueItem[] = tracks.map((t) => ({ id: t.id, path: t.path ?? '', title: t.title, artist: t.artist, album: t.album }));
    if (items.length) player.load(items, 0);
    onPlay();
  };

  return (
    <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(180px, 1fr))', gap: 22 }}>
      {(q.data ?? []).map((a) => (
        <div key={a.id} data-focusable tabIndex={0} onClick={() => openAlbum(a)} style={{ cursor: 'pointer' }}>
          <div style={{ position: 'relative', aspectRatio: '1', borderRadius: 12, overflow: 'hidden' }}>
            <ArtPlaceholder label="Album art" grad={gradFor(a.title)} src={a.artUrl} />
          </div>
          <div style={{ marginTop: 10, font: `600 15px ${font.ui}`, color: colors.ink1 }}>{a.title}</div>
          <div style={{ font: `400 12px ${font.ui}`, color: colors.ink3 }}>{a.artist} · {a.trackCount} tracks</div>
        </div>
      ))}
    </div>
  );
}

function ArtistGrid() {
  const q = useQuery({ queryKey: ['artists'], queryFn: listArtists });
  return (
    <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(150px, 1fr))', gap: 22 }}>
      {(q.data ?? []).map((a) => (
        <div key={a.id} data-focusable tabIndex={0} style={{ cursor: 'pointer', textAlign: 'center' }}>
          <div style={{ position: 'relative', width: '100%', aspectRatio: '1', borderRadius: '50%', overflow: 'hidden', margin: '0 auto' }}>
            <ArtPlaceholder label="" grad={gradFor(a.name)} src={a.imageUrl} />
          </div>
          <div style={{ marginTop: 10, font: `600 15px ${font.ui}`, color: colors.ink1 }}>{a.name}</div>
          <div style={{ font: `400 12px ${font.ui}`, color: colors.ink3 }}>{a.albumCount} albums</div>
        </div>
      ))}
    </div>
  );
}
