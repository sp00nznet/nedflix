import { api, API_BASE } from './http';

// Stream URLs against the existing nedflix endpoints (progressive HTTP, range-capable).
export const videoUrl = (path: string, startSec = 0) =>
  `${API_BASE}/api/video-transcode?path=${encodeURIComponent(path)}${startSec ? `&start=${startSec}` : ''}`;
export const videoDirectUrl = (path: string) => `${API_BASE}/api/video?path=${encodeURIComponent(path)}`;
export const audioUrl = (path: string) => `${API_BASE}/api/audio?path=${encodeURIComponent(path)}`;

export interface ProgressBody {
  filePath: string;
  titleId?: string;
  kind?: 'video' | 'audiobook';
  positionSec: number;
  durationSec: number;
  profileId?: string;
}

export function saveProgress(body: ProgressBody) {
  return api.put('/api/progress', body).catch(() => {});
}

export function getProgress(path: string, profileId = ''): Promise<{ positionSec?: number; durationSec?: number }> {
  return api
    .get<{ positionSec?: number; durationSec?: number }>(`/api/progress?path=${encodeURIComponent(path)}&profile=${profileId}`)
    .catch(() => ({}));
}
