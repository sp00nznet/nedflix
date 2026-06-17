import { api } from './http';
import { orFallback } from './fallback';
import type { Album, Artist, Track } from './types';
import { ALBUMS, ARTISTS } from '../mock/catalog';

export const listAlbums = () => orFallback(api.get<Album[]>('/api/music/albums'), () => ALBUMS);
export const listArtists = () => orFallback(api.get<Artist[]>('/api/music/artists'), () => ARTISTS);
export const albumTracks = (id: string) =>
  orFallback(api.get<Track[]>(`/api/music/albums/${id}/tracks`), () =>
    Array.from({ length: 8 }, (_, i) => ({ id: `${id}-${i}`, path: '', title: `Track ${i + 1}`, artist: '', album: '', durationSec: 0 })),
  );
