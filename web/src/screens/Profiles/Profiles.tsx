import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useQuery, useQueryClient } from '@tanstack/react-query';
import { colors, font, ACCENTS } from '../../theme';
import { listProfiles, createProfile } from '../../api/settings';
import { isDesktop, quitApp } from '../../api/desktop';
import { useActiveProfile } from '../../state/activeProfile';
import type { Profile } from '../../api/types';

// "Who's watching?" gate. Selecting sets the active accent and returns home; the dashed
// tile creates a new profile (POST /api/profiles).
export function Profiles() {
  const navigate = useNavigate();
  const qc = useQueryClient();
  const { setProfile } = useActiveProfile();
  const q = useQuery({ queryKey: ['profiles'], queryFn: listProfiles });
  const profiles = q.data ?? [];

  const [adding, setAdding] = useState(false);
  const [name, setName] = useState('');
  const [busy, setBusy] = useState(false);

  const choose = (p: Profile) => {
    setProfile({ id: p.id, name: p.name, initial: p.initial, color: p.color, accent: p.accent });
    navigate('/');
  };

  const create = async () => {
    const n = name.trim();
    if (!n) return;
    setBusy(true);
    try {
      const p = await createProfile(n, 'coral');
      await qc.invalidateQueries({ queryKey: ['profiles'] });
      setAdding(false);
      setName('');
      choose(p);
    } catch {
      alert('Could not create profile');
    } finally {
      setBusy(false);
    }
  };

  return (
    <div style={{ height: '100dvh', display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 40, background: colors.bg0 }} className="nf-rise">
      <h1 style={{ margin: 0, font: `700 44px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>Who's watching?</h1>
      <div style={{ display: 'flex', gap: 28, flexWrap: 'wrap', justifyContent: 'center', maxWidth: 760 }}>
        {profiles.map((p) => (
          <button key={p.id} data-focusable tabIndex={0} onClick={() => choose(p)} style={{ border: 0, background: 'none', cursor: 'pointer', display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 14 }}>
            <div style={{ width: 124, height: 124, borderRadius: 22, background: p.color === 'var(--ac)' ? ACCENTS[p.accent] : p.color, display: 'flex', alignItems: 'center', justifyContent: 'center', font: `700 46px ${font.ui}`, color: colors.bg0 }}>{p.initial}</div>
            <span style={{ font: `500 16px ${font.ui}`, color: colors.ink2 }}>{p.name}</span>
          </button>
        ))}

        {adding ? (
          <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 14 }}>
            <div style={{ width: 124, height: 124, borderRadius: 22, border: '2px solid var(--ac)', display: 'flex', alignItems: 'center', justifyContent: 'center', font: `700 46px ${font.ui}`, color: 'var(--ac)' }}>{(name.trim()[0] || '?').toUpperCase()}</div>
            <input data-focusable autoFocus value={name} onChange={(e) => setName(e.target.value)} onKeyDown={(e) => e.key === 'Enter' && create()} placeholder="Name" style={{ width: 140, textAlign: 'center', padding: '8px 10px', borderRadius: 10, border: '1px solid rgba(236,239,247,.2)', background: 'rgba(236,239,247,.05)', color: colors.ink0, font: `500 15px ${font.ui}`, outline: 'none' }} />
            <button data-focusable tabIndex={0} disabled={busy} onClick={create} style={{ border: 0, borderRadius: 10, padding: '8px 18px', background: 'var(--ac)', color: colors.acInk, font: `600 13px ${font.ui}`, cursor: 'pointer' }}>{busy ? 'Creating…' : 'Create'}</button>
          </div>
        ) : (
          <button data-focusable tabIndex={0} onClick={() => setAdding(true)} style={{ border: 0, background: 'none', cursor: 'pointer', display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 14 }}>
            <div style={{ width: 124, height: 124, borderRadius: 22, border: '2px dashed rgba(236,239,247,.2)', display: 'flex', alignItems: 'center', justifyContent: 'center', color: colors.ink4 }}>
              <svg width="34" height="34" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round"><path d="M12 5 V19 M5 12 H19" /></svg>
            </div>
            <span style={{ font: `500 16px ${font.ui}`, color: colors.ink4 }}>Add profile</span>
          </button>
        )}
      </div>

      {isDesktop() && (
        <button
          data-focusable
          tabIndex={0}
          onClick={() => quitApp()}
          style={{ marginTop: 8, display: 'flex', alignItems: 'center', gap: 9, padding: '12px 22px', border: '1px solid rgba(236,239,247,.16)', borderRadius: 12, background: 'rgba(236,239,247,.04)', color: colors.ink3, font: `600 14px ${font.ui}`, cursor: 'pointer' }}
        >
          <svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round"><path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4 M16 17l5-5-5-5 M21 12H9" /></svg>
          Exit Marquee
        </button>
      )}
    </div>
  );
}
