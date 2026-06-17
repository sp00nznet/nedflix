import { useSearchParams } from 'react-router-dom';
import { useQuery } from '@tanstack/react-query';
import { colors, font, gradFor, radius } from '../../theme';
import { PillTabs } from '../../components/PillTabs';
import { ProgressBar } from '../../components/ProgressBar';
import { SeekBar } from '../../components/SeekBar';
import { ArtPlaceholder } from '../../components/ArtPlaceholder';
import { listBooks, getBook, saveBookProgress } from '../../api/audiobooks';
import { useAudioPlayer, type QueueItem } from '../../state/audioPlayer';
import type { Audiobook, Chapter } from '../../api/types';

type Tab = 'library' | 'listening';
const pct = (s: string) => (parseInt(s, 10) || 0) / 100;

export function Audiobooks() {
  const [params, setParams] = useSearchParams();
  const tab = (params.get('tab') as Tab) || 'library';
  const bookId = params.get('book') || '';
  const open = (id: string) => setParams({ tab: 'listening', book: id });

  return (
    <div className="nf-rise" style={{ padding: '48px 40px' }}>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: 28 }}>
        <h1 style={{ margin: 0, font: `700 40px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>Audiobooks</h1>
        <PillTabs<Tab>
          tabs={[{ key: 'library', label: 'Library' }, { key: 'listening', label: 'Listening' }]}
          value={tab}
          onChange={(t) => setParams(t === 'listening' && bookId ? { tab: 'listening', book: bookId } : { tab: t })}
        />
      </div>
      {tab === 'library' ? <Library onOpen={open} /> : <Listening bookId={bookId} />}
    </div>
  );
}

function Library({ onOpen }: { onOpen: (id: string) => void }) {
  const q = useQuery({ queryKey: ['books'], queryFn: listBooks });
  const books = q.data ?? [];
  const inProgress = books.filter((b) => pct(b.progressPct) > 0);

  return (
    <>
      {!!inProgress.length && (
        <section style={{ marginBottom: 36 }}>
          <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, marginBottom: 14 }}>CONTINUE LISTENING</div>
          <div style={{ display: 'flex', gap: 18, overflowX: 'auto', paddingBottom: 6 }}>
            {inProgress.map((b) => (
              <div key={b.id} data-focusable tabIndex={0} onClick={() => onOpen(b.id)} style={{ flex: '0 0 330px', display: 'flex', gap: 14, padding: 12, borderRadius: 14, background: 'rgba(236,239,247,.03)', border: '1px solid rgba(236,239,247,.07)', cursor: 'pointer' }}>
                <div style={{ position: 'relative', width: 80, height: 120, borderRadius: 8, overflow: 'hidden', flex: '0 0 auto' }}><ArtPlaceholder label="Cover" grad={gradFor(b.title)} src={b.coverUrl} /></div>
                <div style={{ flex: 1, display: 'flex', flexDirection: 'column' }}>
                  <div style={{ font: `600 16px ${font.ui}`, color: colors.ink1 }}>{b.title}</div>
                  <div style={{ font: `400 12px ${font.ui}`, color: colors.ink3, marginBottom: 'auto' }}>{b.author}</div>
                  <ProgressBar value={pct(b.progressPct)} track="rgba(236,239,247,.12)" />
                  <div style={{ font: `500 11px ${font.mono}`, color: 'var(--ac)', marginTop: 8 }}>{b.progressPct} · RESUME</div>
                </div>
              </div>
            ))}
          </div>
        </section>
      )}

      <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, marginBottom: 14 }}>ALL AUDIOBOOKS</div>
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(150px, 1fr))', gap: 22 }}>
        {books.map((b) => (
          <div key={b.id} data-focusable tabIndex={0} onClick={() => onOpen(b.id)} style={{ cursor: 'pointer' }}>
            <div style={{ position: 'relative', aspectRatio: '2/3', borderRadius: 10, overflow: 'hidden' }}>
              <ArtPlaceholder label="Cover" grad={gradFor(b.title)} src={b.coverUrl} />
              {pct(b.progressPct) > 0 && <div style={{ position: 'absolute', left: 0, right: 0, bottom: 0 }}><ProgressBar value={pct(b.progressPct)} height={3} /></div>}
            </div>
            <div style={{ marginTop: 8, font: `600 14px ${font.ui}`, color: colors.ink1 }}>{b.title}</div>
            <div style={{ font: `400 12px ${font.ui}`, color: colors.ink3 }}>{b.author}</div>
          </div>
        ))}
      </div>
    </>
  );
}

function Listening({ bookId }: { bookId: string }) {
  const q = useQuery({ queryKey: ['book', bookId], queryFn: () => getBook(bookId), enabled: !!bookId });
  const player = useAudioPlayer();
  const book = q.data as Audiobook | undefined;

  if (!bookId || !book) return <div style={{ color: colors.ink3, font: `400 15px ${font.ui}` }}>Choose a book from your library.</div>;

  const playChapter = (ch: Chapter & { path?: string }, i: number) => {
    const items: QueueItem[] = book.chapters.map((c) => ({ id: `${book.id}-${c.number}`, path: (c as Chapter & { path?: string }).path || '', title: c.title, artist: book.author, album: book.title }));
    player.load(items, i, { onProgress: (posSec, durSec) => saveBookProgress(book.id, posSec, durSec) });
    void ch;
  };
  const frac = player.durationSec ? player.positionSec / player.durationSec : 0;

  return (
    <div style={{ display: 'grid', gridTemplateColumns: '380px 1fr', gap: 30 }}>
      <div style={{ background: colors.panel, borderRadius: radius.panel, padding: 22 }}>
        <div style={{ position: 'relative', aspectRatio: '2/3', borderRadius: 12, overflow: 'hidden', marginBottom: 18 }}><ArtPlaceholder label="Cover" grad={gradFor(book.title)} src={book.coverUrl} /></div>
        <div style={{ font: `700 22px ${font.ui}`, letterSpacing: '-.01em', color: colors.ink0 }}>{book.title}</div>
        <div style={{ font: `400 13px ${font.ui}`, color: colors.ink3, marginBottom: 18 }}>{book.author}{book.narrator ? ` · ${book.narrator}` : ''}</div>
        <SeekBar value={frac} onSeek={player.seek} />
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 20, marginTop: 18 }}>
          <button data-focusable tabIndex={0} onClick={player.prev} style={transp}>−15s</button>
          <button data-focusable tabIndex={0} onClick={player.toggle} style={{ width: 64, height: 64, borderRadius: '50%', border: 0, background: 'var(--ac)', color: colors.acInk, cursor: 'pointer' }}>
            {player.playing ? <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="5" width="4" height="14" rx="1" /><rect x="14" y="5" width="4" height="14" rx="1" /></svg> : <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor"><path d="M8 5 L19 12 L8 19 Z" /></svg>}
          </button>
          <button data-focusable tabIndex={0} onClick={player.next} style={transp}>+30s</button>
        </div>
      </div>

      <div>
        <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, marginBottom: 6 }}>CHAPTERS · {book.progressPct} COMPLETE</div>
        <ProgressBar value={pct(book.progressPct)} track="rgba(236,239,247,.1)" />
        <div style={{ marginTop: 16, display: 'flex', flexDirection: 'column', gap: 2 }}>
          {book.chapters.map((c, i) => (
            <button key={c.number} data-focusable tabIndex={0} onClick={() => playChapter(c, i)} style={{ display: 'flex', alignItems: 'center', gap: 14, padding: '12px 14px', borderRadius: 10, border: 0, cursor: 'pointer', textAlign: 'left', background: c.state === 'current' ? 'rgba(236,239,247,.05)' : 'transparent', color: c.state === 'current' ? colors.ink0 : c.state === 'done' ? colors.ink4 : colors.ink2 }}>
              <span style={{ font: `400 13px ${font.mono}`, color: c.state === 'current' ? 'var(--ac)' : 'inherit', width: 24 }}>{String(c.number).padStart(2, '0')}</span>
              <span style={{ flex: 1, font: `500 14px ${font.ui}` }}>{c.title}</span>
              {c.durationSec > 0 && <span style={{ font: `400 12px ${font.mono}`, color: colors.ink4 }}>{Math.round(c.durationSec / 60)}m</span>}
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}

const transp: React.CSSProperties = { border: '1px solid rgba(236,239,247,.2)', borderRadius: 10, padding: '10px 14px', background: 'rgba(236,239,247,.06)', color: '#eceef4', font: "600 13px 'Schibsted Grotesk'", cursor: 'pointer' };
