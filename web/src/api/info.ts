import { api } from './http';

export interface FileInfo {
  path: string;
  name: string;
  container: string;
  size: string;
  video?: string;
  resolution?: string;
  fps?: string;
  bitrate?: string;
  duration?: string;
  audio?: string[];
  subtitles?: string[];
  note?: string;
}

export const getFileInfo = (path: string) =>
  api.get<FileInfo>(`/api/fileinfo?path=${encodeURIComponent(path)}`).catch(() => null);

export interface ArtCandidate { title: string; year?: string; thumb: string; url: string }

export const searchArtwork = (provider: 'itunes' | 'tmdb', kind: string, q: string) =>
  api.get<ArtCandidate[]>(`/api/artwork/search?provider=${provider}&kind=${kind}&q=${encodeURIComponent(q)}`).catch(() => [] as ArtCandidate[]);

export const refreshArtwork = (path: string) => api.post('/api/artwork/refresh', { path }).catch(() => {});

export const getDescription = (kind: string, title: string, year?: string | number) =>
  api.get<{ description: string }>(`/api/description?kind=${kind}&title=${encodeURIComponent(title)}${year ? `&year=${year}` : ''}`).then((r) => r.description).catch(() => '');
export const setArtworkOverride = (path: string, url: string) => api.put('/api/artwork/override', { path, url }).catch(() => {});

// Whether the manual-search provider (TMDB) is configured.
export const artworkHasTmdb = () => api.get<{ hasKey: boolean }>('/api/artwork/config').then((r) => r.hasKey).catch(() => false);
