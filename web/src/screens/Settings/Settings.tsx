import { useState } from 'react';
import { useQuery, useQueryClient } from '@tanstack/react-query';
import { colors, font, radius, ACCENTS, type AccentName } from '../../theme';
import { Toggle } from '../../components/Toggle';
import {
  saveStreaming, triggerScan, getPrefs,
  isDesktop, getLibraries, addMediaPath, removeMediaPath, pickMediaPath,
  getIptvSettings, saveIptvSettings, pickPlaylist, pickEpg,
  getArtworkConfig, saveArtworkKey,
  getErsatzSettings, saveErsatzSettings,
} from '../../api/settings';
import { LANGUAGES } from '../../api/languages';
import { useActiveProfile } from '../../state/activeProfile';

function Group({ title, subtitle, children }: { title: string; subtitle?: string; children: React.ReactNode }) {
  return (
    <div style={{ border: '1px solid rgba(236,239,247,.08)', background: 'rgba(236,239,247,.02)', borderRadius: 18, marginBottom: 18 }}>
      <div style={{ padding: '18px 22px 14px' }}>
        <div style={{ font: `600 16px ${font.ui}`, color: colors.ink1 }}>{title}</div>
        {subtitle && <div style={{ font: `400 13px ${font.ui}`, color: colors.ink4, marginTop: 2 }}>{subtitle}</div>}
      </div>
      <div style={{ borderTop: '1px solid rgba(236,239,247,.06)' }}>{children}</div>
    </div>
  );
}

function Row({ label, children }: { label: React.ReactNode; children: React.ReactNode }) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', gap: 16, padding: '14px 22px', borderBottom: '1px solid rgba(236,239,247,.05)' }}>
      <span style={{ font: `500 14px ${font.ui}`, color: colors.ink2, minWidth: 0, overflow: 'hidden', textOverflow: 'ellipsis' }}>{label}</span>
      {children}
    </div>
  );
}

const btn = (accent = false): React.CSSProperties => ({
  border: accent ? 0 : '1px solid rgba(236,239,247,.2)', borderRadius: 10, padding: '9px 16px', cursor: 'pointer',
  background: accent ? 'var(--ac)' : 'rgba(236,239,247,.06)', color: accent ? colors.acInk : colors.ink1, font: `600 13px ${font.ui}`,
});
const input: React.CSSProperties = { flex: 1, padding: '9px 12px', borderRadius: 10, border: '1px solid rgba(236,239,247,.14)', background: 'rgba(236,239,247,.04)', color: colors.ink0, font: `500 13px ${font.ui}`, outline: 'none' };
const select: React.CSSProperties = { padding: '8px 12px', borderRadius: 10, border: '1px solid rgba(236,239,247,.18)', background: '#191c27', color: colors.ink1, font: `500 13px ${font.ui}`, cursor: 'pointer', outline: 'none' };

export function Settings() {
  const qc = useQueryClient();
  const { profile, setProfile } = useActiveProfile();
  const desktop = isDesktop();

  const prefsQ = useQuery({ queryKey: ['prefs'], queryFn: getPrefs });
  const streaming = (prefsQ.data as Record<string, unknown>) || {};
  const [edits, setEdits] = useState<Record<string, unknown>>({});
  const val = (k: string, def = false) => (edits[k] ?? (streaming[k] as boolean) ?? def) as boolean;
  const set = (k: string, v: boolean) => { const next = { ...edits, [k]: v }; setEdits(next); saveStreaming({ ...streaming, ...next }); };
  const lang = (k: string, def: string) => (edits[k] ?? (streaming[k] as string) ?? def) as string;
  const setLang = (k: string, v: string) => { const next = { ...edits, [k]: v }; setEdits(next); saveStreaming({ ...streaming, ...next }); };

  // Libraries
  const libs = useQuery({ queryKey: ['libraries'], queryFn: getLibraries });
  const [newPath, setNewPath] = useState('');
  const refreshLibs = (data: unknown) => qc.setQueryData(['libraries'], data);
  const addPath = async () => { if (!newPath.trim()) return; try { refreshLibs(await addMediaPath(newPath.trim())); setNewPath(''); } catch { alert('Could not add — folder not found'); } };
  const pickPath = async () => { try { refreshLibs(await pickMediaPath()); } catch { /* ignore */ } };
  const removePath = async (p: string) => { refreshLibs(await removeMediaPath(p)); };

  // IPTV
  const iptv = useQuery({ queryKey: ['iptv-settings'], queryFn: getIptvSettings, enabled: desktop });
  const [m3u, setM3u] = useState<string | null>(null);
  const [xmltv, setXmltv] = useState<string | null>(null);
  const m3uVal = m3u ?? iptv.data?.playlistUrl ?? '';
  const xmltvVal = xmltv ?? iptv.data?.epgUrl ?? '';
  const [iptvMsg, setIptvMsg] = useState('');
  const saveIptv = async () => { try { await saveIptvSettings({ playlistUrl: m3uVal, epgUrl: xmltvVal }); setIptvMsg('Saved'); } catch { setIptvMsg('Save failed'); } };
  const browseM3u = async () => { try { const s = await pickPlaylist(); if (s.playlistUrl != null) setM3u(s.playlistUrl); qc.setQueryData(['iptv-settings'], s); setIptvMsg('Imported playlist'); } catch { /* canceled */ } };
  const browseEpg = async () => { try { const s = await pickEpg(); if (s.epgUrl != null) setXmltv(s.epgUrl); qc.setQueryData(['iptv-settings'], s); setIptvMsg('Imported EPG'); } catch { /* canceled */ } };

  const [scanMsg, setScanMsg] = useState('');
  const scan = async () => { setScanMsg('Scanning…'); try { await triggerScan(); setScanMsg('Scan started'); } catch { setScanMsg('Scan unavailable (handled per-folder on desktop)'); } };

  // "Your own TV" — built-in local channels, or an external ErsatzTV server.
  const ersatz = useQuery({ queryKey: ['ersatz-settings'], queryFn: getErsatzSettings, enabled: desktop });
  const [ersatzUrl, setErsatzUrl] = useState<string | null>(null);
  const ersatzUrlVal = ersatzUrl ?? ersatz.data?.url ?? '';
  const ersatzMode = (ersatz.data?.mode ?? 'off') as 'off' | 'local' | 'server';
  const saveErsatz = async (patch: { url?: string; mode?: 'off' | 'local' | 'server' }) => {
    await saveErsatzSettings({ url: ersatzUrlVal, mode: ersatzMode, ...patch });
    qc.invalidateQueries({ queryKey: ['ersatz-settings'] });
  };

  // Artwork (TMDB)
  const art = useQuery({ queryKey: ['artwork-config'], queryFn: getArtworkConfig, enabled: desktop });
  const [tmdbKey, setTmdbKey] = useState('');
  const [artMsg, setArtMsg] = useState('');
  const saveArt = async () => { try { await saveArtworkKey(tmdbKey.trim()); setArtMsg(tmdbKey.trim() ? 'Saved — posters will load shortly' : 'Cleared'); qc.invalidateQueries({ queryKey: ['artwork-config'] }); qc.invalidateQueries({ queryKey: ['titles'] }); } catch { setArtMsg('Save failed'); } };

  return (
    <div className="nf-rise" style={{ padding: '48px 40px', maxWidth: 880 }}>
      <h1 style={{ margin: '0 0 26px', font: `700 40px ${font.ui}`, letterSpacing: '-.02em', color: colors.ink0 }}>Settings</h1>

      <Group title="Library" subtitle="Folders Marquee scans for media">
        {(libs.data ?? []).map((l) => (
          <Row key={l.path} label={<span><span style={{ color: colors.ink1 }}>{l.name}</span> <span style={{ font: `400 12px ${font.mono}`, color: colors.ink4 }}>{l.path}</span></span>}>
            <button data-focusable tabIndex={0} onClick={() => removePath(l.path)} style={btn()}>Remove</button>
          </Row>
        ))}
        {!libs.data?.length && <Row label={<span style={{ color: colors.ink4 }}>No libraries yet — add a folder to get started.</span>}>{null}</Row>}
        <div style={{ display: 'flex', gap: 10, padding: '14px 22px' }}>
          <input data-focusable value={newPath} onChange={(e) => setNewPath(e.target.value)} placeholder="Paste a folder path…" style={input} onKeyDown={(e) => e.key === 'Enter' && addPath()} />
          <button data-focusable tabIndex={0} onClick={addPath} style={btn(true)}>Add</button>
          {desktop && <button data-focusable tabIndex={0} onClick={pickPath} style={btn()}>Browse…</button>}
        </div>
        <Row label="Re-scan / refresh index">
          <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
            {scanMsg && <span style={{ font: `400 12px ${font.ui}`, color: colors.ink3 }}>{scanMsg}</span>}
            <button data-focusable tabIndex={0} onClick={scan} style={btn(true)}>Scan</button>
          </div>
        </Row>
      </Group>

      {desktop && (
        <Group title="Live TV" subtitle="IPTV playlist (M3U) + guide (XMLTV)">
          <div style={{ padding: '14px 22px', display: 'flex', flexDirection: 'column', gap: 14 }}>
            <div>
              <div style={{ font: `500 12px ${font.mono}`, color: colors.ink4, marginBottom: 6 }}>M3U PLAYLIST — URL or local file</div>
              <div style={{ display: 'flex', gap: 10 }}>
                <input data-focusable value={m3uVal} onChange={(e) => setM3u(e.target.value)} placeholder="http://…/playlist.m3u  or  C:\\path\\to\\playlist.m3u" style={input} />
                <button data-focusable tabIndex={0} onClick={browseM3u} style={btn()}>Browse…</button>
              </div>
            </div>
            <div>
              <div style={{ font: `500 12px ${font.mono}`, color: colors.ink4, marginBottom: 6 }}>XMLTV EPG — URL or local file</div>
              <div style={{ display: 'flex', gap: 10 }}>
                <input data-focusable value={xmltvVal} onChange={(e) => setXmltv(e.target.value)} placeholder="http://…/xmltv.xml  or  C:\\path\\to\\guide.xml" style={input} />
                <button data-focusable tabIndex={0} onClick={browseEpg} style={btn()}>Browse…</button>
              </div>
            </div>
            <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
              <button data-focusable tabIndex={0} onClick={saveIptv} style={btn(true)}>Save Live TV settings</button>
              {iptvMsg && <span style={{ font: `400 12px ${font.ui}`, color: colors.ink3 }}>{iptvMsg}</span>}
            </div>
          </div>
        </Group>
      )}

      {desktop && (
        <Group title="Your own TV" subtitle="24/7 channels shown as a second view in Live TV">
          <Row label="Source">
            <div style={{ display: 'inline-flex', gap: 4, padding: 4, borderRadius: 999, background: 'rgba(236,239,247,.05)', border: '1px solid rgba(236,239,247,.08)' }}>
              {([['off', 'Off'], ['local', 'Built-in'], ['server', 'Server']] as ['off' | 'local' | 'server', string][]).map(([m, label]) => (
                <button key={m} data-focusable tabIndex={0} onClick={() => saveErsatz({ mode: m })} style={{ border: 0, cursor: 'pointer', padding: '7px 14px', borderRadius: 999, font: `600 12px ${font.ui}`, background: ersatzMode === m ? 'var(--ac)' : 'transparent', color: ersatzMode === m ? colors.acInk : colors.ink2 }}>{label}</button>
              ))}
            </div>
          </Row>
          {ersatzMode === 'local' && <Row label={<span style={{ color: colors.ink4 }}>Built-in channels are generated from your library — no extra software needed.</span>}>{null}</Row>}
          {ersatzMode === 'server' && (
            <div style={{ padding: '14px 22px' }}>
              <div style={{ font: `500 12px ${font.mono}`, color: colors.ink4, marginBottom: 6 }}>ERSATZTV SERVER URL</div>
              <div style={{ display: 'flex', gap: 10 }}>
                <input data-focusable value={ersatzUrlVal} onChange={(e) => setErsatzUrl(e.target.value)} placeholder="http://192.168.1.100:8409" style={input} />
                <button data-focusable tabIndex={0} onClick={() => saveErsatz({})} style={btn(true)}>Save</button>
              </div>
            </div>
          )}
        </Group>
      )}

      {desktop && (
        <Group title="Artwork" subtitle="Real posters & art come from local image files; add a free TMDB key for movie/TV posters">
          <div style={{ padding: '14px 22px' }}>
            <div style={{ font: `500 12px ${font.mono}`, color: colors.ink4, marginBottom: 6 }}>TMDB API KEY {art.data?.hasKey ? '· configured' : ''}</div>
            <div style={{ display: 'flex', gap: 10 }}>
              <input data-focusable value={tmdbKey} onChange={(e) => setTmdbKey(e.target.value)} placeholder={art.data?.hasKey ? '•••••••• (saved) — paste to replace' : 'Paste your TMDB API key'} style={input} />
              <button data-focusable tabIndex={0} onClick={saveArt} style={btn(true)}>Save</button>
            </div>
            <div style={{ font: `400 12px ${font.ui}`, color: colors.ink4, marginTop: 8 }}>
              {artMsg || 'Get a free key at themoviedb.org → Settings → API. Without it, Marquee uses local poster.jpg/folder.jpg/cover.jpg files next to your media, falling back to gradients.'}
            </div>
          </div>
        </Group>
      )}

      <Group title="Playback & Subtitles" subtitle="Defaults applied to every player">
        <Row label="Default audio language">
          <select data-focusable value={lang('audioLanguage', 'eng')} onChange={(e) => setLang('audioLanguage', e.target.value)} style={select}>
            {LANGUAGES.map((l) => <option key={l.code} value={l.code}>{l.label}</option>)}
          </select>
        </Row>
        <Row label="Default subtitle language">
          <select data-focusable value={lang('subtitleLanguage', 'off')} onChange={(e) => setLang('subtitleLanguage', e.target.value)} style={select}>
            <option value="off">Off</option>
            {LANGUAGES.map((l) => <option key={l.code} value={l.code}>{l.label}</option>)}
          </select>
        </Row>
        <Row label="Autoplay next"><Toggle on={val('autoplay')} onChange={(v) => set('autoplay', v)} /></Row>
        <Row label="HDR passthrough"><Toggle on={val('hdr', true)} onChange={(v) => set('hdr', v)} /></Row>
        <Row label="Hardware decoding"><Toggle on={val('hwdecode', true)} onChange={(v) => set('hwdecode', v)} /></Row>
      </Group>

      <Group title="Appearance" subtitle="Accent color (per profile)">
        <div style={{ display: 'flex', gap: 12, padding: '18px 22px', flexWrap: 'wrap' }}>
          {(Object.keys(ACCENTS) as AccentName[]).map((a) => (
            <button key={a} data-focusable tabIndex={0} onClick={() => setProfile({ ...profile, accent: a })} aria-label={a}
              style={{ width: 36, height: 36, borderRadius: radius.pill, cursor: 'pointer', background: ACCENTS[a], border: profile.accent === a ? '3px solid #fff' : '3px solid transparent', boxShadow: profile.accent === a ? '0 0 0 2px var(--ac)' : 'none' }} />
          ))}
        </div>
      </Group>
    </div>
  );
}
