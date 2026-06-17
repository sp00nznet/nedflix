import { api } from './http';
import { orFallback } from './fallback';
import { decodePathId } from './ids';
import type { Audiobook } from './types';
import { BOOKS } from '../mock/catalog';

export const listBooks = () => orFallback(api.get<Audiobook[]>('/api/audiobooks'), () => Object.values(BOOKS));
export const getBook = (id: string) =>
  orFallback(api.get<Audiobook>(`/api/audiobooks/${id}`), () => BOOKS[id] ?? Object.values(BOOKS)[0]);

export function saveBookProgress(id: string, positionSec: number, durationSec: number, profileId = '') {
  return api.put(`/api/audiobooks/${id}/progress`, { positionSec, durationSec, profileId }).catch(() => {});
}

export const bookChapterPath = (id: string, chapterPath?: string) => chapterPath ?? decodePathId(id);
