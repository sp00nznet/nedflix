import { api } from './http';
import { ALLOW_MOCK } from './fallback';
import { encodePathId } from './ids';
import { TITLES } from '../mock/catalog';

interface RawFile { path: string; name: string; library?: string; file_type?: string }

export interface SearchHit {
  id: string;
  title: string;
  meta?: string;
  posterUrl?: string;
}

export async function search(q: string): Promise<SearchHit[]> {
  if (q.trim().length < 2) return [];
  try {
    const r = await api.get<{ results: RawFile[] }>(`/api/search?q=${encodeURIComponent(q)}`);
    return r.results.map((f) => ({
      id: encodePathId(f.path),
      title: f.name.replace(/\.[^.]+$/, ''),
      meta: f.library,
    }));
  } catch (e) {
    if (ALLOW_MOCK) {
      const ql = q.toLowerCase();
      return Object.values(TITLES)
        .filter((t) => t.title.toLowerCase().includes(ql))
        .map((t) => ({ id: t.id, title: t.title, meta: `${t.year} · ${t.genre}` }));
    }
    throw e;
  }
}
