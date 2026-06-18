import { useNavigate } from 'react-router-dom';
import { useQuery } from '@tanstack/react-query';
import { colors, font } from '../../theme';
import { PosterCard } from '../../components/PosterCard';
import { listTitles } from '../../api/library';
import { titleToInfo } from '../../state/info';

// Full library grid for one media type (Films / Series).
export function BrowseGrid({ type }: { type: 'film' | 'series' }) {
  const navigate = useNavigate();
  const q = useQuery({ queryKey: ['titles', type], queryFn: () => listTitles(type) });
  const items = q.data ?? [];

  return (
    <div className="nf-rise" style={{ padding: '48px 40px' }}>
      <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.22em', color: 'var(--ac)', marginBottom: 12 }}>LIBRARY</div>
      <div style={{ display: 'flex', alignItems: 'baseline', justifyContent: 'space-between', marginBottom: 24 }}>
        <h1 style={{ margin: 0, font: `700 40px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>{type === 'film' ? 'Films' : 'Series'}</h1>
        <span style={{ font: `400 13px ${font.ui}`, color: colors.ink3 }}>{items.length} titles</span>
      </div>
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(176px, 1fr))', gap: 20 }}>
        {items.map((t) => (
          <PosterCard key={t.id} title={t.title} meta={[t.year, t.genre].filter(Boolean).join(' · ')} seed={t.id} posterUrl={t.posterUrl} width={176} info={titleToInfo(t)} onClick={() => navigate(`/title/${t.id}`)} />
        ))}
      </div>
    </div>
  );
}
