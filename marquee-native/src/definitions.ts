import type { PluginListenerHandle } from '@capacitor/core';

/**
 * MarqueeNative — one bridge, two implementations (web + native).
 * The React UI talks ONLY to this interface; the web impl uses <video>/<audio> +
 * Media Session, the native impl forwards to AVPlayer (iOS) / ExoPlayer-Media3 (Android)
 * and emits OS transport events back so the web UI stays in sync.
 */

export type MediaType = 'video' | 'audio';

export interface TrackInfo {
  id: string;
  /** HLS (.m3u8) or progressive URL from your transcode endpoint */
  url: string;
  type: MediaType;
  title: string;
  artist?: string;
  album?: string;
  artworkUrl?: string;
  durationSec?: number;
  /** resume position to start at */
  startAtSec?: number;
}

export interface NowPlayingMeta {
  title: string;
  artist?: string;
  album?: string;
  artworkUrl?: string;
  durationSec?: number;
  positionSec?: number;
  playbackRate?: number;
}

export interface PlaybackState {
  id?: string;
  playing: boolean;
  positionSec: number;
  durationSec: number;
  /** 0..1 buffered ahead */
  buffered: number;
}

export type DownloadStatus = 'queued' | 'downloading' | 'done' | 'failed' | 'removed';

export interface DownloadProgress {
  id: string;
  status: DownloadStatus;
  /** 0..1 */
  progress: number;
  bytes?: number;
  totalBytes?: number;
}

export interface CastRoute {
  id: string;
  name: string;
  type: 'airplay' | 'cast' | 'local';
  active: boolean;
}

export interface MarqueeNativePlugin {
  // ---- transport ----
  load(options: TrackInfo): Promise<void>;
  play(): Promise<void>;
  pause(): Promise<void>;
  stop(): Promise<void>;
  seek(options: { positionSec: number }): Promise<void>;
  setRate(options: { rate: number }): Promise<void>;
  getState(): Promise<PlaybackState>;

  // ---- lock screen / control center ----
  setNowPlaying(options: NowPlayingMeta): Promise<void>;

  // ---- picture in picture ----
  enterPiP(): Promise<void>;
  exitPiP(): Promise<void>;

  // ---- offline downloads ----
  startDownload(options: { id: string; url: string; title?: string }): Promise<void>;
  removeDownload(options: { id: string }): Promise<void>;
  listDownloads(): Promise<{ items: DownloadProgress[] }>;

  // ---- casting (AirPlay / Google Cast) ----
  listRoutes(): Promise<{ routes: CastRoute[] }>;
  showRoutePicker(): Promise<void>;
  selectRoute(options: { id: string }): Promise<void>;

  // ---- events (native -> web) ----
  /** OS transport commands from lock screen / headphones / car */
  addListener(eventName: 'remotePlay' | 'remotePause' | 'remoteStop' | 'remoteNext' | 'remotePrev', listenerFunc: () => void): Promise<PluginListenerHandle>;
  addListener(eventName: 'remoteSeek', listenerFunc: (e: { positionSec: number }) => void): Promise<PluginListenerHandle>;
  /** periodic playback state (≈ every 500ms while playing) */
  addListener(eventName: 'stateChange', listenerFunc: (e: PlaybackState) => void): Promise<PluginListenerHandle>;
  addListener(eventName: 'ended', listenerFunc: (e: { id: string }) => void): Promise<PluginListenerHandle>;
  addListener(eventName: 'downloadProgress', listenerFunc: (e: DownloadProgress) => void): Promise<PluginListenerHandle>;
  addListener(eventName: 'routesChanged', listenerFunc: (e: { routes: CastRoute[] }) => void): Promise<PluginListenerHandle>;
  removeAllListeners(): Promise<void>;
}
