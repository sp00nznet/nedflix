import { api } from './http';
import { orFallback } from './fallback';
import { decodePathId } from './ids';
import type { Title } from './types';
import { TITLES, FILM_IDS, SERIES_IDS, CONTINUE } from '../mock/catalog';

export type TitleCard = Title & { progress?: number; context?: string; episodes?: number };

export function listTitles(type: 'film' | 'series'): Promise<TitleCard[]> {
  return orFallback(
    api.get<TitleCard[]>(`/api/library/titles?type=${type}`),
    () => (type === 'film' ? FILM_IDS : SERIES_IDS).map((id) => ({ ...TITLES[id] })),
  );
}

export function continueWatching(profileId = 'default'): Promise<TitleCard[]> {
  return orFallback(
    api.get<TitleCard[]>(`/api/profiles/${profileId}/continue`),
    () => CONTINUE.map((c) => ({ ...TITLES[c.id], progress: c.progress, context: c.context })),
  );
}

export function getTitle(id: string): Promise<Title> {
  // Detail uses the file path encoded in the id → /api/metadata, enriched.
  return orFallback<Title>(
    (async () => {
      const path = decodePathId(id);
      const r = await api.get<{ found: boolean; metadata: Record<string, unknown> }>(`/api/metadata?path=${encodeURIComponent(path)}`);
      const m = r.metadata;
      return {
        id,
        path,
        title: (m.cleanTitle as string) || 'Untitled',
        year: (m.year as number) || 0,
        type: m.type === 'tv' ? 'Series' : 'Film',
        genre: (m.genre as string) || '',
        runtime: (m.runtime as string) || '',
        rating: (m.rating as string) || '',
        tags: [],
        synopsis: (m.plot as string) || '',
        posterUrl: (m.poster as string) || undefined,
        cast: m.actors ? String(m.actors).split(',').map((n) => ({ name: n.trim() })) : [],
      } satisfies Title;
    })(),
    () => ({ ...(TITLES[id] ?? TITLES.meridian) }),
  );
}

