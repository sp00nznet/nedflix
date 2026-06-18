import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useQuery, useQueryClient } from '@tanstack/react-query';
import { colors, font, gradFor } from '../../theme';
import { ProgressBar } from '../../components/ProgressBar';
import { getGuide, getErsatzGuide, getLocalGuide, toggleFavorite } from '../../api/livetv';
import { getErsatzSettings } from '../../api/settings';

const pct = (s: string) => (parseInt(s, 10) || 0) / 100;

type View = 'iptv' | 'ersatz';

// Live TV / EPG guide: favorites row + per-channel program grid. With "your channels"
// enabled (built-in local or external server), a second view renders the same guide.
export function LiveTV() {
  const qc = useQueryClient();
  const navigate = useNavigate();
  const ersatz = useQuery({ queryKey: ['ersatz-settings'], queryFn: getErsatzSettings });
  const mode = ersatz.data?.mode ?? 'off';
  const showErsatz = mode === 'local' || (mode === 'server' && !!ersatz.data?.url);
  const [view, setView] = useState<View>('iptv');
  const activeView: View = showErsatz ? view : 'iptv';

  const q = useQuery({
    queryKey: ['guide', activeView, mode],
    queryFn: () => (activeView === 'ersatz' ? (mode === 'local' ? getLocalGuide() : getErsatzGuide()) : getGuide()),
  });
  const channels = q.data ?? [];
  const favs = channels.filter((c) => c.favorite);

  const star = async (id: string, fav: boolean) => {
    await toggleFavorite(id, fav);
    qc.invalidateQueries({ queryKey: ['guide'] });
  };

  return (
    <div className="nf-rise" style={{ padding: '48px 40px' }}>
      <div style={{ display: 'flex', alignItems: 'baseline', gap: 14, marginBottom: 8 }}>
        <span style={{ display: 'inline-flex', alignItems: 'center', gap: 8, font: `500 11px ${font.mono}`, letterSpacing: '.2em', color: colors.liveRed }}>
          <span style={{ width: 8, height: 8, borderRadius: '50%', background: colors.liveRed, animation: 'nf-pulse 1.4s ease-in-out infinite' }} /> ON AIR NOW
        </span>
      </div>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: 24 }}>
        <h1 style={{ margin: 0, font: `700 40px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>Live TV</h1>
        {showErsatz && (
          <div style={{ display: 'inline-flex', gap: 4, padding: 4, borderRadius: 999, background: 'rgba(236,239,247,.04)', border: '1px solid rgba(236,239,247,.08)' }}>
            {([['iptv', 'IPTV'], ['ersatz', 'Your Channels']] as [View, string][]).map(([k, label]) => (
              <button key={k} data-focusable tabIndex={0} onClick={() => setView(k)} style={{ border: 0, cursor: 'pointer', padding: '8px 16px', borderRadius: 999, font: `600 13px ${font.ui}`, background: activeView === k ? 'var(--ac)' : 'transparent', color: activeView === k ? colors.acInk : colors.ink2 }}>
                {label}
              </button>
            ))}
          </div>
        )}
      </div>

      {!!favs.length && (
        <div style={{ display: 'flex', gap: 14, overflowX: 'auto', paddingBottom: 6, marginBottom: 26 }}>
          {favs.map((c) => (
            <div key={c.id} data-focusable tabIndex={0} style={{ flex: '0 0 220px', padding: 14, borderRadius: 12, background: 'rgba(236,239,247,.03)', border: '1px solid rgba(236,239,247,.08)', cursor: 'pointer' }}>
              <div style={{ display: 'flex', alignItems: 'center', gap: 10, marginBottom: 10 }}>
                <span style={{ font: `600 11px ${font.mono}`, color: colors.ink4, padding: '3px 7px', borderRadius: 6, background: 'rgba(236,239,247,.08)' }}>{c.num}</span>
                <span style={{ font: `600 14px ${font.ui}`, color: colors.ink1 }}>{c.name}</span>
              </div>
              <div style={{ font: `400 12px ${font.ui}`, color: colors.ink3, marginBottom: 8 }}><span style={{ color: colors.liveRed }}>● </span>{c.nowPlaying.title}</div>
              <ProgressBar value={pct(c.nowPlaying.progressPct)} height={3} />
            </div>
          ))}
        </div>
      )}

      {/* Guide grid */}
      <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
        {channels.map((c) => (
          <div key={c.id} style={{ display: 'flex', gap: 12, alignItems: 'stretch' }}>
            <div style={{ flex: '0 0 190px', display: 'flex', alignItems: 'center', gap: 10, padding: 12, borderRadius: 10, background: 'rgba(236,239,247,.03)' }}>
              <div style={{ width: 38, height: 38, borderRadius: 8, background: gradFor(c.name), flex: '0 0 auto' }} />
              <div style={{ minWidth: 0 }}>
                <div style={{ font: `600 13px ${font.ui}`, color: colors.ink1, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{c.name}</div>
                <button data-focusable tabIndex={0} onClick={() => star(c.id, !c.favorite)} style={{ border: 0, background: 'none', cursor: 'pointer', font: `400 11px ${font.mono}`, color: c.favorite ? 'var(--ac)' : colors.ink4, padding: 0 }}>{c.favorite ? '★ pinned' : '☆ pin'}</button>
              </div>
            </div>
            <div style={{ flex: 1, display: 'flex', gap: 6, minWidth: 0 }}>
              {c.programs.length ? c.programs.map((p, i) => (
                <div key={i} data-focusable tabIndex={0} onClick={() => p.titleId && navigate(`/watch/${p.titleId}`)} style={{ flex: p.widthWeight, minWidth: 90, padding: '10px 12px', borderRadius: 10, cursor: p.titleId ? 'pointer' : 'default', background: p.isLive ? 'rgba(251,113,89,.12)' : 'rgba(236,239,247,.045)', border: `1px solid ${p.isLive ? 'var(--ac)' : 'rgba(236,239,247,.08)'}`, overflow: 'hidden' }}>
                  <div style={{ font: `400 10px ${font.mono}`, color: p.isLive ? 'var(--ac)' : colors.ink4 }}>{p.startTime}{p.isLive ? ' · LIVE' : ''}</div>
                  <div style={{ font: `500 13px ${font.ui}`, color: colors.ink1, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{p.title}</div>
                </div>
              )) : <div style={{ flex: 1, padding: '10px 12px', borderRadius: 10, background: 'rgba(236,239,247,.03)', font: `400 12px ${font.ui}`, color: colors.ink4 }}>No guide data</div>}
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
