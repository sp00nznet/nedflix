import { useState } from 'react';
import { useSearchParams } from 'react-router-dom';
import { useQuery, useQueryClient } from '@tanstack/react-query';
import { colors, font, gradFor } from '../../theme';
import { ArtPlaceholder } from '../../components/ArtPlaceholder';
import { ProgressBar } from '../../components/ProgressBar';
import {
  searchPodcasts, listPodcasts, subscribePodcast, unsubscribePodcast, podcastEpisodes,
  type Podcast, type PodcastEpisode,
} from '../../api/podcasts';
import { useAudioPlayer } from '../../state/audioPlayer';
import { saveProgress } from '../../api/playback';

const btn = (accent = false): React.CSSProperties => ({
  border: accent ? 0 : '1px solid rgba(236,239,247,.2)', borderRadius: 10, padding: '8px 14px', cursor: 'pointer',
  background: accent ? 'var(--ac)' : 'rgba(236,239,247,.06)', color: accent ? colors.acInk : colors.ink1, font: `600 12px ${font.ui}`,
});

export function Podcasts() {
  const [params, setParams] = useSearchParams();
  const show = params.get('show'); // feedUrl of opened podcast
  return (
    <div className="nf-rise" style={{ padding: '48px 40px' }}>
      <h1 style={{ margin: '0 0 24px', font: `700 40px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>Podcasts</h1>
      {show ? <EpisodeList feedUrl={show} onBack={() => setParams({})} /> : <Subscriptions onOpen={(f) => setParams({ show: f })} />}
    </div>
  );
}

function Subscriptions({ onOpen }: { onOpen: (feedUrl: string) => void }) {
  const qc = useQueryClient();
  const subs = useQuery({ queryKey: ['podcasts'], queryFn: listPodcasts });
  const [q, setQ] = useState('');
  const [results, setResults] = useState<Podcast[] | null>(null);
  const [busy, setBusy] = useState(false);

  const runSearch = async () => { if (q.trim().length < 2) return; setBusy(true); setResults(await searchPodcasts(q)); setBusy(false); };
  const subscribed = (feedUrl: string) => (subs.data ?? []).some((s) => s.feedUrl === feedUrl);
  const sub = async (p: Podcast) => { await subscribePodcast(p); qc.invalidateQueries({ queryKey: ['podcasts'] }); };
  const unsub = async (feedUrl: string) => { await unsubscribePodcast(feedUrl); qc.invalidateQueries({ queryKey: ['podcasts'] }); };

  return (
    <>
      <div style={{ display: 'flex', gap: 10, maxWidth: 620, marginBottom: 28 }}>
        <input data-focusable value={q} onChange={(e) => setQ(e.target.value)} onKeyDown={(e) => e.key === 'Enter' && runSearch()} placeholder="Find a podcast…" style={{ flex: 1, padding: '12px 16px', borderRadius: 12, border: '1px solid rgba(236,239,247,.14)', background: 'rgba(236,239,247,.04)', color: colors.ink0, font: `500 15px ${font.ui}`, outline: 'none' }} />
        <button data-focusable tabIndex={0} onClick={runSearch} style={btn(true)}>{busy ? '…' : 'Search'}</button>
      </div>

      {results && (
        <section style={{ marginBottom: 34 }}>
          <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, marginBottom: 14 }}>RESULTS</div>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(150px, 1fr))', gap: 22 }}>
            {results.map((p) => (
              <div key={p.id} data-focusable tabIndex={0} onClick={() => onOpen(p.feedUrl)} style={{ cursor: 'pointer' }}>
                <div style={{ position: 'relative', aspectRatio: '1', borderRadius: 12, overflow: 'hidden' }}><ArtPlaceholder label="" grad={gradFor(p.title)} src={p.artUrl} /></div>
                <div style={{ marginTop: 8, font: `600 14px ${font.ui}`, color: colors.ink1, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{p.title}</div>
                <div style={{ font: `400 12px ${font.ui}`, color: colors.ink3, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{p.author}</div>
                <button data-focusable tabIndex={0} onClick={(e) => { e.stopPropagation(); subscribed(p.feedUrl) ? unsub(p.feedUrl) : sub(p); }} style={{ ...btn(!subscribed(p.feedUrl)), marginTop: 8, width: '100%' }}>{subscribed(p.feedUrl) ? '✓ Subscribed' : '+ Subscribe'}</button>
              </div>
            ))}
          </div>
        </section>
      )}

      <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, marginBottom: 14 }}>SUBSCRIPTIONS</div>
      {subs.data?.length ? (
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(150px, 1fr))', gap: 22 }}>
          {subs.data.map((p) => (
            <div key={p.id} data-focusable tabIndex={0} onClick={() => onOpen(p.feedUrl)} style={{ cursor: 'pointer' }}>
              <div style={{ position: 'relative', aspectRatio: '1', borderRadius: 12, overflow: 'hidden' }}><ArtPlaceholder label="" grad={gradFor(p.title)} src={p.artUrl} /></div>
              <div style={{ marginTop: 8, font: `600 14px ${font.ui}`, color: colors.ink1, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{p.title}</div>
              <div style={{ font: `400 12px ${font.ui}`, color: colors.ink3 }}>{p.author}</div>
            </div>
          ))}
        </div>
      ) : (
        <div style={{ font: `400 15px ${font.ui}`, color: colors.ink4 }}>No subscriptions yet — search above to add one.</div>
      )}
    </>
  );
}

function EpisodeList({ feedUrl, onBack }: { feedUrl: string; onBack: () => void }) {
  const q = useQuery({ queryKey: ['pod-episodes', feedUrl], queryFn: () => podcastEpisodes(feedUrl) });
  const player = useAudioPlayer();
  const show = q.data;

  const play = (e: PodcastEpisode) => {
    player.load([{ id: e.guid || e.audioUrl, path: e.audioUrl, title: e.title, artist: show?.title, album: show?.title, artUrl: e.artUrl }], 0, {
      onProgress: (pos, dur) => saveProgress({ filePath: e.audioUrl, kind: 'audiobook', titleId: e.guid, positionSec: pos, durationSec: dur }),
    });
  };

  return (
    <>
      <button data-focusable tabIndex={0} onClick={onBack} style={{ ...btn(), marginBottom: 18 }}>← Back to podcasts</button>
      <div style={{ display: 'flex', gap: 18, alignItems: 'center', marginBottom: 22 }}>
        <div style={{ position: 'relative', width: 90, height: 90, borderRadius: 12, overflow: 'hidden', flex: '0 0 auto' }}><ArtPlaceholder label="" grad={gradFor(show?.title || feedUrl)} src={show?.artUrl} /></div>
        <div style={{ font: `700 24px ${font.ui}`, color: colors.ink0 }}>{show?.title || 'Episodes'}</div>
      </div>
      {q.isLoading ? (
        <div style={{ color: colors.ink4, font: `400 14px ${font.ui}` }}>Loading episodes…</div>
      ) : (
        <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
          {(show?.episodes ?? []).map((e, i) => {
            const pct = e.resume && e.resume.durationSec ? e.resume.positionSec / e.resume.durationSec : 0;
            return (
              <button key={e.guid || i} data-focusable tabIndex={0} onClick={() => play(e)} style={{ textAlign: 'left', border: 0, borderBottom: '1px solid rgba(236,239,247,.05)', background: 'none', cursor: 'pointer', padding: '14px 6px', display: 'flex', gap: 14, alignItems: 'flex-start' }}>
                <svg width="18" height="18" viewBox="0 0 24 24" fill="var(--ac)" style={{ marginTop: 3, flex: '0 0 auto' }}><path d="M8 5 L19 12 L8 19 Z" /></svg>
                <div style={{ minWidth: 0, flex: 1 }}>
                  <div style={{ font: `600 15px ${font.ui}`, color: colors.ink1 }}>{e.title}</div>
                  <div style={{ font: `400 12px ${font.mono}`, color: colors.ink4, margin: '3px 0' }}>{[e.date && new Date(e.date).toLocaleDateString(), e.durationSec ? `${Math.round(e.durationSec / 60)} min` : ''].filter(Boolean).join(' · ')}</div>
                  {e.description && <div style={{ font: `400 13px/1.45 ${font.ui}`, color: colors.ink3, maxHeight: 40, overflow: 'hidden' }}>{e.description}</div>}
                  {pct > 0 && <div style={{ marginTop: 8, maxWidth: 320 }}><ProgressBar value={pct} height={3} /></div>}
                </div>
              </button>
            );
          })}
        </div>
      )}
    </>
  );
}
