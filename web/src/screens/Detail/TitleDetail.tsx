import { useNavigate, useParams } from 'react-router-dom';
import { useQuery } from '@tanstack/react-query';
import { colors, font, radius } from '../../theme';
import { ArtPlaceholder } from '../../components/ArtPlaceholder';
import { getTitle } from '../../api/library';
import { decodePathId } from '../../api/ids';
import type { Title } from '../../api/types';

const Back = () => (
  <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M15 5 L8 12 L15 19" /></svg>
);

function Chip({ children }: { children: React.ReactNode }) {
  return <span style={{ font: `500 11px ${font.mono}`, letterSpacing: '.08em', color: colors.ink2, padding: '4px 8px', borderRadius: 6, background: 'rgba(236,239,247,.08)' }}>{children}</span>;
}

function InfoCard({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <div style={{ border: '1px solid rgba(236,239,247,.08)', background: 'rgba(236,239,247,.02)', borderRadius: radius.panel, padding: 18 }}>
      <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.ink4, marginBottom: 14 }}>{title}</div>
      {children}
    </div>
  );
}

export function TitleDetail() {
  const { id = '' } = useParams();
  const navigate = useNavigate();
  const q = useQuery({ queryKey: ['title', id], queryFn: () => getTitle(id) });
  const t = q.data as Title | undefined;
  if (!t) return <div style={{ padding: 40, color: colors.ink3 }} className="nf-rise">Loading…</div>;

  const ext = (t.path ? t.path.split('.').pop() : '')?.toUpperCase() || '—';

  return (
    <div className="nf-rise">
      <div style={{ position: 'relative', height: 470 }}>
        <ArtPlaceholder label="Backdrop" src={t.backdropUrl} seed={t.id} style={{ position: 'absolute' }} />
        <div style={{ position: 'absolute', inset: 0, background: 'linear-gradient(0deg,rgba(11,12,17,.98) 4%,transparent 55%)' }} />
        <button data-focusable tabIndex={0} onClick={() => navigate(-1)} style={{ position: 'absolute', top: 24, left: 40, display: 'flex', alignItems: 'center', gap: 8, padding: '10px 16px', border: '1px solid rgba(236,239,247,.16)', borderRadius: 10, background: 'rgba(11,12,17,.45)', backdropFilter: 'blur(8px)', color: colors.ink1, font: `600 13px ${font.ui}`, cursor: 'pointer' }}><Back /> Back</button>
        <div style={{ position: 'absolute', left: 40, bottom: 30, maxWidth: 720 }}>
          <div style={{ font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: 'var(--ac)', marginBottom: 12 }}>{t.type === 'Series' ? 'SERIES' : 'FILM'} · IN YOUR LIBRARY</div>
          <h1 style={{ margin: '0 0 12px', font: `700 58px/1 ${font.ui}`, letterSpacing: '-.03em', color: colors.ink0 }}>{t.title}</h1>
          <div style={{ display: 'flex', gap: 10, alignItems: 'center', flexWrap: 'wrap' }}>
            <span style={{ font: `400 14px ${font.ui}`, color: colors.ink2 }}>{[t.year, t.genre, t.runtime].filter(Boolean).join(' · ')}</span>
            {t.rating && <Chip>{t.rating}</Chip>}
            {t.tags?.map((g) => <Chip key={g}>{g}</Chip>)}
          </div>
        </div>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 300px', gap: 34, padding: '28px 40px 60px' }}>
        <div>
          <div style={{ display: 'flex', gap: 12, marginBottom: 22 }}>
            <button data-focusable tabIndex={0} onClick={() => navigate(`/watch/${t.id}`)} style={{ display: 'flex', alignItems: 'center', gap: 10, padding: '15px 30px', border: 0, borderRadius: 12, background: 'var(--ac)', color: colors.acInk, font: `700 15px ${font.ui}`, cursor: 'pointer' }}>
              <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M7 5 L19 12 L7 19 Z" /></svg> Play
            </button>
            <button data-focusable tabIndex={0} style={{ display: 'flex', alignItems: 'center', gap: 8, padding: '15px 22px', border: '1px solid rgba(236,239,247,.2)', borderRadius: 12, background: 'rgba(236,239,247,.06)', color: colors.ink1, font: `600 14px ${font.ui}`, cursor: 'pointer' }}>My List</button>
          </div>
          <p style={{ font: `400 17px/1.6 ${font.ui}`, color: colors.ink2, maxWidth: 680 }}>{t.synopsis || 'No synopsis available.'}</p>
        </div>

        <div style={{ display: 'flex', flexDirection: 'column', gap: 16, position: 'sticky', top: 24, alignSelf: 'start' }}>
          {!!t.cast?.length && (
            <InfoCard title="CAST">
              {t.cast.slice(0, 6).map((c) => (
                <div key={c.name} style={{ display: 'flex', alignItems: 'center', gap: 10, marginBottom: 10 }}>
                  <div style={{ width: 32, height: 32, borderRadius: '50%', background: 'rgba(236,239,247,.1)' }} />
                  <span style={{ font: `500 13px ${font.ui}`, color: colors.ink1 }}>{c.name}</span>
                </div>
              ))}
            </InfoCard>
          )}
          <InfoCard title="FILE · TECHNICAL">
            {[['CONTAINER', ext], ['PATH', t.path ? decodePathId(t.id).split(/[\\/]/).pop() : '—']].map(([k, v]) => (
              <div key={k} style={{ display: 'flex', justifyContent: 'space-between', gap: 12, marginBottom: 8, font: `400 12px ${font.mono}` }}>
                <span style={{ color: colors.ink4 }}>{k}</span>
                <span style={{ color: colors.ink2, textAlign: 'right', overflow: 'hidden', textOverflow: 'ellipsis', maxWidth: 180, whiteSpace: 'nowrap' }}>{v}</span>
              </div>
            ))}
          </InfoCard>
        </div>
      </div>
    </div>
  );
}
