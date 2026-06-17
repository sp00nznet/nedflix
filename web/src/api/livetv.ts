import { api, API_BASE } from './http';
import { ALLOW_MOCK } from './fallback';
import type { Channel, Program } from './types';
import { CHANNELS, FAV_CHANNEL_NUMS } from '../mock/catalog';

interface RawChannel { id: string; name: string; tvgId?: string; tvgLogo?: string; group?: string; url?: string }
interface RawProgram { start: number; stop: number; title: string }
interface RawEpg { programs: Record<string, RawProgram[]> }

const hhmm = (ms: number) => {
  const d = new Date(ms);
  return `${String(d.getHours()).padStart(2, '0')}:${String(d.getMinutes()).padStart(2, '0')}`;
};

// Merge M3U channels + XMLTV EPG into the design's Channel shape, over a visible window
// starting "now". widthWeight = program duration in minutes.
function merge(channels: RawChannel[], epg: RawEpg, favs: Set<string>): (Channel & { logoUrl?: string; url?: string })[] {
  const now = Date.now();
  const windowEnd = now + 3 * 3600_000;
  return channels.map((c, i) => {
    const progs = (epg.programs[c.tvgId || c.id] || []).filter((p) => p.stop > now && p.start < windowEnd);
    const cur = progs.find((p) => p.start <= now && p.stop > now) || progs[0];
    const programs: Program[] = progs.map((p) => ({
      title: p.title,
      startTime: hhmm(p.start),
      endTime: hhmm(p.stop),
      isLive: p.start <= now && p.stop > now,
      widthWeight: Math.max(1, Math.round((p.stop - p.start) / 60000)),
    }));
    const curPct = cur && cur.stop > cur.start ? `${Math.round(((now - cur.start) / (cur.stop - cur.start)) * 100)}%` : '0%';
    return {
      id: c.id,
      num: String(i + 1).padStart(2, '0'),
      name: c.name,
      logoUrl: c.tvgLogo,
      url: c.url,
      favorite: favs.has(c.id),
      nowPlaying: { title: cur?.title || 'No guide data', progressPct: curPct },
      programs,
    };
  });
}

export async function getGuide(): Promise<(Channel & { logoUrl?: string; url?: string; grad?: string })[]> {
  try {
    const [ch, epg, favs] = await Promise.all([
      api.get<{ configured: boolean; channels: RawChannel[] }>('/api/iptv/channels'),
      api.get<RawEpg>('/api/iptv/epg'),
      api.get<string[]>('/api/livetv/favorites').catch(() => [] as string[]),
    ]);
    if (!ch.configured || !ch.channels?.length) throw new Error('not configured');
    return merge(ch.channels, epg, new Set(favs));
  } catch (e) {
    if (ALLOW_MOCK) return CHANNELS.map((c) => ({ ...c, favorite: FAV_CHANNEL_NUMS.includes(c.num) }));
    throw e;
  }
}

export const toggleFavorite = (channelId: string, favorite: boolean) =>
  api.put(`/api/livetv/channels/${channelId}/favorite`, { favorite }).catch(() => {});

export const iptvStreamUrl = (url: string) => `${API_BASE}/api/iptv/stream?url=${encodeURIComponent(url)}`;
