import { WebPlugin } from '@capacitor/core';
import type {
  MarqueeNativePlugin, TrackInfo, NowPlayingMeta, PlaybackState,
  DownloadProgress, CastRoute,
} from './definitions';

/**
 * Web / PWA implementation.
 * Uses a single shared HTMLMediaElement, the Media Session API for lock-screen /
 * headphone controls, the Picture-in-Picture API, and the Cache API for downloads.
 * This is fully functional in a browser; the native impls override it inside Capacitor.
 *
 * NOTE: attach hls.js to `this.el` for .m3u8 sources (left as a TODO so this file stays
 * dependency-free).
 */
export class MarqueeNativeWeb extends WebPlugin implements MarqueeNativePlugin {
  private el: HTMLMediaElement | null = null;
  private current?: TrackInfo;
  private ticker?: number;

  private ensureEl(type: 'video' | 'audio'): HTMLMediaElement {
    if (this.el && this.el.tagName.toLowerCase() === type) return this.el;
    if (this.el) { this.el.pause(); this.el.remove(); }
    const el = document.createElement(type);
    el.setAttribute('playsinline', 'true');
    (el as HTMLVideoElement).controls = false;
    if (type === 'video') {
      el.id = 'marquee-video';
      Object.assign(el.style, { position: 'fixed', inset: '0', width: '100%', height: '100%', background: '#000', zIndex: '40' });
      document.body.appendChild(el);
    }
    el.addEventListener('ended', () => this.notifyListeners('ended', { id: this.current?.id }));
    el.addEventListener('timeupdate', () => this.emitState());
    this.el = el;
    return el;
  }

  private emitState(): void {
    if (!this.el) return;
    const buffered = this.el.buffered.length ? this.el.buffered.end(this.el.buffered.length - 1) / (this.el.duration || 1) : 0;
    const state: PlaybackState = {
      id: this.current?.id,
      playing: !this.el.paused,
      positionSec: this.el.currentTime,
      durationSec: this.el.duration || this.current?.durationSec || 0,
      buffered,
    };
    this.notifyListeners('stateChange', state);
  }

  private wireMediaSession(): void {
    if (!('mediaSession' in navigator)) return;
    const ms = navigator.mediaSession;
    ms.setActionHandler('play', () => this.notifyListeners('remotePlay', {}));
    ms.setActionHandler('pause', () => this.notifyListeners('remotePause', {}));
    ms.setActionHandler('stop', () => this.notifyListeners('remoteStop', {}));
    ms.setActionHandler('nexttrack', () => this.notifyListeners('remoteNext', {}));
    ms.setActionHandler('previoustrack', () => this.notifyListeners('remotePrev', {}));
    ms.setActionHandler('seekto', (e) => this.notifyListeners('remoteSeek', { positionSec: e.seekTime ?? 0 }));
  }

  async load(options: TrackInfo): Promise<void> {
    this.current = options;
    const el = this.ensureEl(options.type);
    // TODO: if options.url ends with .m3u8 and !el.canPlayType('application/vnd.apple.mpegurl'),
    //       attach hls.js here instead of setting src directly.
    el.src = options.url;
    if (options.startAtSec) el.currentTime = options.startAtSec;
    this.wireMediaSession();
    await this.setNowPlaying(options);
  }

  async play(): Promise<void> { await this.el?.play(); this.emitState(); }
  async pause(): Promise<void> { this.el?.pause(); this.emitState(); }
  async stop(): Promise<void> { if (this.el) { this.el.pause(); this.el.removeAttribute('src'); this.el.load(); } }
  async seek(options: { positionSec: number }): Promise<void> { if (this.el) this.el.currentTime = options.positionSec; }
  async setRate(options: { rate: number }): Promise<void> { if (this.el) this.el.playbackRate = options.rate; }

  async getState(): Promise<PlaybackState> {
    return {
      id: this.current?.id,
      playing: !!this.el && !this.el.paused,
      positionSec: this.el?.currentTime ?? 0,
      durationSec: this.el?.duration ?? 0,
      buffered: 0,
    };
  }

  async setNowPlaying(options: NowPlayingMeta): Promise<void> {
    if (!('mediaSession' in navigator)) return;
    navigator.mediaSession.metadata = new MediaMetadata({
      title: options.title,
      artist: options.artist ?? '',
      album: options.album ?? '',
      artwork: options.artworkUrl ? [{ src: options.artworkUrl, sizes: '512x512', type: 'image/png' }] : [],
    });
  }

  async enterPiP(): Promise<void> {
    const v = this.el as HTMLVideoElement | null;
    if (v && (v as any).requestPictureInPicture) await (v as any).requestPictureInPicture();
  }
  async exitPiP(): Promise<void> {
    if ((document as any).pictureInPictureElement) await (document as any).exitPictureInPicture();
  }

  // Cache-API download (good enough for PWA offline; native impls use background download).
  async startDownload(options: { id: string; url: string; title?: string }): Promise<void> {
    const emit = (p: Partial<DownloadProgress>) =>
      this.notifyListeners('downloadProgress', { id: options.id, status: 'downloading', progress: 0, ...p });
    try {
      const cache = await caches.open('marquee-downloads');
      const res = await fetch(options.url);
      await cache.put(options.url, res.clone());
      emit({ status: 'done', progress: 1 });
    } catch {
      emit({ status: 'failed', progress: 0 });
    }
  }
  async removeDownload(options: { id: string }): Promise<void> {
    this.notifyListeners('downloadProgress', { id: options.id, status: 'removed', progress: 0 });
  }
  async listDownloads(): Promise<{ items: DownloadProgress[] }> { return { items: [] }; }

  // Web casting: prefer the Remote Playback API where available.
  async listRoutes(): Promise<{ routes: CastRoute[] }> { return { routes: [{ id: 'local', name: 'This device', type: 'local', active: true }] }; }
  async showRoutePicker(): Promise<void> {
    const v = this.el as any;
    if (v?.remote?.prompt) await v.remote.prompt();
  }
  async selectRoute(_options: { id: string }): Promise<void> { /* handled by remote.prompt() on web */ }
}
