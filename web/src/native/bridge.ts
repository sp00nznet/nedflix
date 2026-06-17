// Thin accessor for the MarqueeNative Capacitor plugin. In a native shell the plugin is
// registered on window.Capacitor.Plugins, so the web app uses it WITHOUT a build-time
// Capacitor dependency. On plain web these return null and callers fall back to
// <video>/<audio> + Media Session.

export interface NativeTrack {
  id: string;
  url: string;
  type: 'video' | 'audio';
  title: string;
  artist?: string;
  album?: string;
  artworkUrl?: string;
  startAtSec?: number;
}
export interface NativeState {
  id?: string;
  playing: boolean;
  positionSec: number;
  durationSec: number;
  buffered: number;
}

export interface MarqueeNativeLike {
  load(o: NativeTrack): Promise<void>;
  play(): Promise<void>;
  pause(): Promise<void>;
  seek(o: { positionSec: number }): Promise<void>;
  setNowPlaying(o: { title: string; artist?: string; album?: string; positionSec?: number; durationSec?: number }): Promise<void>;
  enterPiP(): Promise<void>;
  addListener(event: string, cb: (e?: unknown) => void): Promise<{ remove: () => void }>;
  removeAllListeners(): Promise<void>;
}

type Cap = { isNativePlatform?: () => boolean; Plugins?: { MarqueeNative?: MarqueeNativeLike } };
const cap = (): Cap | undefined => (window as unknown as { Capacitor?: Cap }).Capacitor;

export const isNativeShell = (): boolean => !!cap()?.isNativePlatform?.();
export const nativeMedia = (): MarqueeNativeLike | null => cap()?.Plugins?.MarqueeNative ?? null;
