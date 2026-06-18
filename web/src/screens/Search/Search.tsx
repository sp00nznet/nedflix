import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useQuery } from '@tanstack/react-query';
import { colors, font } from '../../theme';
import { PosterCard } from '../../components/PosterCard';
import { search } from '../../api/search';
import { decodePathId } from '../../api/ids';
import type { InfoItem } from '../../state/info';

const SUGGESTIONS = ['Films', 'Series', '4K', 'Drama', 'Sci-Fi', 'Documentary'];

export function Search() {
  const navigate = useNavigate();
  const [q, setQ] = useState('');
  const results = useQuery({ queryKey: ['search', q], queryFn: () => search(q), enabled: q.trim().length >= 2 });
  const hits = results.data ?? [];

  return (
    <div className="nf-rise" style={{ padding: '48px 40px' }}>
      <div style={{ position: 'relative', maxWidth: 720, marginBottom: 20 }}>
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke={colors.ink4} strokeWidth="1.8" style={{ position: 'absolute', left: 18, top: 18 }}><circle cx="11" cy="11" r="7" /><path d="M21 21 L16 16" /></svg>
        <input
          data-focusable
          autoFocus
          value={q}
          onChange={(e) => setQ(e.target.value)}
          placeholder="Search your library…"
          style={{ width: '100%', padding: '16px 18px 16px 48px', borderRadius: 14, border: '1px solid rgba(236,239,247,.14)', background: 'rgba(236,239,247,.04)', color: colors.ink0, font: `500 17px ${font.ui}`, outline: 'none' }}
        />
      </div>

      {q.trim().length < 2 && (
        <div style={{ display: 'flex', gap: 10, flexWrap: 'wrap', marginBottom: 28 }}>
          {SUGGESTIONS.map((s) => (
            <button key={s} data-focusable tabIndex={0} onClick={() => setQ(s)} style={{ border: '1px solid rgba(236,239,247,.14)', background: 'rgba(236,239,247,.04)', color: colors.ink2, borderRadius: 999, padding: '8px 16px', font: `500 13px ${font.ui}`, cursor: 'pointer' }}>{s}</button>
          ))}
        </div>
      )}

      {q.trim().length >= 2 && (
        <>
          <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, marginBottom: 18 }}>{results.isLoading ? 'SEARCHING…' : `${hits.length} RESULTS`}</div>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(176px, 1fr))', gap: 20 }}>
            {hits.map((h) => {
              const path = (() => { try { return decodePathId(h.id); } catch { return undefined; } })();
              const info: InfoItem = { id: h.id, path, kind: 'movie', title: h.title, subtitle: h.meta, artUrl: `/api/artwork?kind=movie&path=${encodeURIComponent(path ?? '')}&title=${encodeURIComponent(h.title)}` };
              return <PosterCard key={h.id} title={h.title} meta={h.meta} seed={h.id} posterUrl={h.posterUrl} width={176} info={info} onClick={() => navigate(`/title/${h.id}`)} />;
            })}
          </div>
        </>
      )}
    </div>
  );
}
