import { api } from './http';
import { orFallback } from './fallback';
import type { Profile } from './types';

export interface UserResponse {
  authenticated: boolean;
  user?: { id: string; displayName?: string; libraryAccess?: string[]; profilePicture?: string };
  settings?: Record<string, unknown> & { streaming?: Record<string, unknown> };
}

export const getUser = () => api.get<UserResponse>('/api/user');

// True inside the Electron desktop shell (exposes folder picker + media-path management).
export const isDesktop = (): boolean => typeof window !== 'undefined' && 'nedflixDesktop' in window;

export interface Library { path: string; name: string }
export const getLibraries = () => api.get<Library[]>('/api/libraries');
export const addMediaPath = (p: string) => api.post<Library[]>('/api/media-paths', { path: p });
export const removeMediaPath = (p: string) => api.del<Library[]>(`/api/media-paths?path=${encodeURIComponent(p)}`);
export const pickMediaPath = () => api.post<Library[]>('/api/media-paths/pick');

export interface IptvSettings { playlistUrl?: string; epgUrl?: string }
export const getIptvSettings = () => api.get<IptvSettings>('/api/iptv/settings').catch(() => ({} as IptvSettings));
export const saveIptvSettings = (s: IptvSettings) => api.post('/api/iptv/settings', s);
// Native file pickers (desktop only) — return the updated IPTV settings.
export const pickPlaylist = () => api.post<IptvSettings>('/api/iptv/pick-playlist');
export const pickEpg = () => api.post<IptvSettings>('/api/iptv/pick-epg');

// Artwork provider (desktop): a TMDB API key enables real movie/TV posters.
export const getArtworkConfig = () => api.get<{ hasKey: boolean }>('/api/artwork/config').catch(() => ({ hasKey: false }));
export const saveArtworkKey = (tmdbKey: string) => api.put('/api/artwork/config', { tmdbKey });

export const saveStreaming = (streaming: Record<string, unknown>) =>
  api.post('/api/settings', { streaming }).catch(() => {});

export interface PlaybackPrefs {
  audioLanguage?: string;
  subtitleLanguage?: string; // 'off' to disable by default
  [k: string]: unknown;
}

// Current playback prefs (the `streaming` settings object). Works on desktop (GET
// /api/settings) and the server build (/api/user.settings.streaming).
export async function getPrefs(): Promise<PlaybackPrefs> {
  try {
    const s = await api.get<{ streaming?: PlaybackPrefs } & PlaybackPrefs>('/api/settings');
    return (s.streaming ?? s) || {};
  } catch {
    try {
      const u = await api.get<UserResponse>('/api/user');
      return (u.settings?.streaming as PlaybackPrefs) ?? {};
    } catch {
      return {};
    }
  }
}

export const triggerScan = () => api.post('/api/admin/scan', {});

// Profiles (also used by the Profiles gate).
const MOCK_PROFILES: Profile[] = [
  { id: 'ned', name: 'Ned', initial: 'N', color: 'var(--ac)', accent: 'coral' },
  { id: 'sam', name: 'Sam', initial: 'S', color: '#7fb2d9', accent: 'blue' },
  { id: 'kids', name: 'Kids', initial: 'K', color: '#8fce9b', accent: 'mint' },
];

export const listProfiles = () => orFallback(api.get<Profile[]>('/api/profiles'), () => MOCK_PROFILES);
export const createProfile = (name: string, accent: Profile['accent']) => api.post<Profile>('/api/profiles', { name, accent });
export const updateProfile = (id: string, patch: Partial<Profile>) => api.put<Profile>(`/api/profiles/${id}`, patch);
