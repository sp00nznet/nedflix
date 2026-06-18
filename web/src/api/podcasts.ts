import { api } from './http';

export interface Podcast {
  id: string;
  feedUrl: string;
  title: string;
  author?: string;
  artUrl?: string;
  episodeCount?: number;
}

export interface PodcastEpisode {
  guid?: string;
  title: string;
  audioUrl: string;
  date?: string;
  durationSec?: number;
  description?: string;
  artUrl?: string;
  resume?: { positionSec: number; durationSec: number };
}

export const searchPodcasts = (q: string) => api.get<Podcast[]>(`/api/podcasts/search?q=${encodeURIComponent(q)}`).catch(() => [] as Podcast[]);
export const listPodcasts = () => api.get<Podcast[]>('/api/podcasts').catch(() => [] as Podcast[]);
export const subscribePodcast = (p: Podcast) => api.post<Podcast[]>('/api/podcasts/subscribe', p);
export const unsubscribePodcast = (feedUrl: string) => api.del<Podcast[]>(`/api/podcasts?feedUrl=${encodeURIComponent(feedUrl)}`);
export interface PodcastFeed { title: string; artUrl?: string; episodes: PodcastEpisode[] }
export const podcastEpisodes = (feedUrl: string): Promise<PodcastFeed> =>
  api.get<PodcastFeed>(`/api/podcasts/episodes?feedUrl=${encodeURIComponent(feedUrl)}`).catch(() => ({ title: '', episodes: [] }));
