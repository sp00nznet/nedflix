import { useNavigate } from 'react-router-dom';
import { useQuery } from '@tanstack/react-query';
import { Hero } from '../../components/Hero';
import { Rail } from '../../components/Rail';
import { PosterCard } from '../../components/PosterCard';
import { WideCard } from '../../components/WideCard';
import { listTitles, continueWatching } from '../../api/library';
import { useActiveProfile } from '../../state/activeProfile';

const PlayIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M7 5 L19 12 L7 19 Z" /></svg>
);
const PlusIcon = () => (
  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round"><path d="M12 5 V19 M5 12 H19" /></svg>
);

// Home — curated landing (hero + horizontal rails), wired to /api/library/titles +
// /api/profiles/:pid/continue (dev falls back to prototype fixtures).
export function Home() {
  const navigate = useNavigate();
  const { profile } = useActiveProfile();
  const films = useQuery({ queryKey: ['titles', 'film'], queryFn: () => listTitles('film') });
  const series = useQuery({ queryKey: ['titles', 'series'], queryFn: () => listTitles('series') });
  const cont = useQuery({ queryKey: ['continue', profile.id], queryFn: () => continueWatching(profile.id) });

  const feat = films.data?.[0];

  return (
    <div className="nf-rise">
      {feat && (
        <Hero
          eyebrow="FEATURED FILM · IN YOUR LIBRARY"
          title={feat.title}
          meta={[feat.year, feat.genre, feat.runtime].filter(Boolean).join(' · ')}
          tags={feat.tags}
          synopsis={feat.synopsis}
          seed={feat.id}
          backdropUrl={feat.backdropUrl}
          actions={[
            { label: 'Play', variant: 'accent', icon: <PlayIcon />, onClick: () => navigate(`/watch/${feat.id}`) },
            { label: 'More info', variant: 'glass', onClick: () => navigate(`/title/${feat.id}`) },
            { label: 'Add to My List', variant: 'icon', icon: <PlusIcon /> },
          ]}
        />
      )}

      <div style={{ marginTop: 30 }}>
        {!!cont.data?.length && (
          <Rail title="Continue watching">
            {cont.data.map((c) => (
              <WideCard key={c.id} title={c.title} context={c.context} progress={c.progress ?? 0} seed={c.id} backdropUrl={c.backdropUrl} onClick={() => navigate(`/watch/${c.id}`)} />
            ))}
          </Rail>
        )}

        <Rail title="Films" onViewAll={() => navigate('/films')}>
          {(films.data ?? []).map((t) => (
            <PosterCard key={t.id} title={t.title} meta={[t.year, t.genre].filter(Boolean).join(' · ')} seed={t.id} posterUrl={t.posterUrl} onClick={() => navigate(`/title/${t.id}`)} />
          ))}
        </Rail>

        <Rail title="Series" onViewAll={() => navigate('/series')}>
          {(series.data ?? []).map((t) => (
            <PosterCard key={t.id} title={t.title} meta={[t.year, t.genre].filter(Boolean).join(' · ')} seed={t.id} posterUrl={t.posterUrl} onClick={() => navigate(`/title/${t.id}`)} />
          ))}
        </Rail>
      </div>
    </div>
  );
}
