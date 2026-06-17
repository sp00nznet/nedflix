// Shared audio player — one persistent <audio> element behind a context, used by Music
// and Audiobooks. Holds a queue, exposes transport, drives Media Session, and reports
// progress to an optional onProgress sink (audiobooks persist via it).
import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useRef,
  useState,
  type ReactNode,
} from 'react';
import { audioUrl } from '../api/playback';
import { isNativeShell, nativeMedia } from '../native/bridge';

export interface QueueItem {
  id: string;
  path: string;
  title: string;
  artist?: string;
  album?: string;
  artUrl?: string;
}

interface PlayerState {
  queue: QueueItem[];
  index: number;
  playing: boolean;
  positionSec: number;
  durationSec: number;
  current: QueueItem | null;
  load: (items: QueueItem[], startIndex?: number, meta?: { kind?: string; onProgress?: ProgressSink }) => void;
  toggle: () => void;
  play: () => void;
  pause: () => void;
  next: () => void;
  prev: () => void;
  seek: (frac: number) => void;
}

type ProgressSink = (positionSec: number, durationSec: number) => void;

const Ctx = createContext<PlayerState | null>(null);

export function AudioPlayerProvider({ children }: { children: ReactNode }) {
  const ref = useRef<HTMLAudioElement | null>(null);
  const [queue, setQueue] = useState<QueueItem[]>([]);
  const [index, setIndex] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [positionSec, setPos] = useState(0);
  const [durationSec, setDur] = useState(0);
  const onProgress = useRef<ProgressSink | undefined>(undefined);

  const current = queue[index] ?? null;

  const load = useCallback((items: QueueItem[], startIndex = 0, meta?: { onProgress?: ProgressSink }) => {
    onProgress.current = meta?.onProgress;
    setQueue(items);
    setIndex(startIndex);
    setPlaying(true);
  }, []);

  const native = isNativeShell() ? nativeMedia() : null;

  // Point the player at the current track. Native shell → hand off to the plugin so audio
  // keeps playing in the background with lock-screen controls; web → the <audio> element.
  useEffect(() => {
    if (!current) return;
    if (native) {
      native
        .load({ id: current.id, url: audioUrl(current.path), type: 'audio', title: current.title, artist: current.artist, album: current.album, artworkUrl: current.artUrl })
        .then(() => native.play())
        .then(() => setPlaying(true))
        .catch(() => setPlaying(false));
      return;
    }
    const a = ref.current;
    if (!a) return;
    a.src = audioUrl(current.path);
    a.play().then(() => setPlaying(true)).catch(() => setPlaying(false));
  }, [current, native]);

  // Native transport events (lock screen / headphones) → keep web UI in sync.
  useEffect(() => {
    if (!native) return;
    const handles: { remove: () => void }[] = [];
    native.addListener('remotePlay', () => setPlaying(true)).then((h) => handles.push(h));
    native.addListener('remotePause', () => setPlaying(false)).then((h) => handles.push(h));
    native.addListener('stateChange', (e) => {
      const s = e as { positionSec?: number; durationSec?: number; playing?: boolean } | undefined;
      if (!s) return;
      if (s.positionSec != null) setPos(s.positionSec);
      if (s.durationSec) setDur(s.durationSec);
      if (s.playing != null) setPlaying(s.playing);
      onProgress.current?.(s.positionSec ?? 0, s.durationSec ?? 0);
    }).then((h) => handles.push(h));
    return () => handles.forEach((h) => h.remove());
  }, [native]);

  const play = useCallback(() => (native ? native.play() : ref.current?.play()), [native]);
  const pause = useCallback(() => (native ? native.pause() : ref.current?.pause()), [native]);
  const toggle = useCallback(() => {
    if (native) {
      if (playing) native.pause();
      else native.play();
      return;
    }
    const a = ref.current;
    if (!a) return;
    if (a.paused) a.play();
    else a.pause();
  }, [native, playing]);
  const next = useCallback(() => setIndex((i) => Math.min(queue.length - 1, i + 1)), [queue.length]);
  const prev = useCallback(() => setIndex((i) => Math.max(0, i - 1)), []);
  const seek = useCallback((frac: number) => {
    if (native) {
      native.seek({ positionSec: frac * (durationSec || 0) });
      return;
    }
    const a = ref.current;
    if (a && a.duration) a.currentTime = frac * a.duration;
  }, [native, durationSec]);

  // Media Session metadata for lock-screen / headphone controls.
  useEffect(() => {
    if (!('mediaSession' in navigator) || !current) return;
    navigator.mediaSession.metadata = new MediaMetadata({
      title: current.title,
      artist: current.artist || '',
      album: current.album || '',
      artwork: current.artUrl ? [{ src: current.artUrl }] : [],
    });
    navigator.mediaSession.setActionHandler('play', play);
    navigator.mediaSession.setActionHandler('pause', pause);
    navigator.mediaSession.setActionHandler('previoustrack', prev);
    navigator.mediaSession.setActionHandler('nexttrack', next);
  }, [current, play, pause, prev, next]);

  const value = useMemo<PlayerState>(
    () => ({ queue, index, playing, positionSec, durationSec, current, load, toggle, play, pause, next, prev, seek }),
    [queue, index, playing, positionSec, durationSec, current, load, toggle, play, pause, next, prev, seek],
  );

  return (
    <Ctx.Provider value={value}>
      {children}
      <audio
        ref={ref}
        onPlay={() => setPlaying(true)}
        onPause={() => setPlaying(false)}
        onTimeUpdate={(e) => {
          const a = e.currentTarget;
          setPos(a.currentTime);
          onProgress.current?.(a.currentTime, a.duration || 0);
        }}
        onLoadedMetadata={(e) => setDur(e.currentTarget.duration || 0)}
        onEnded={next}
      />
    </Ctx.Provider>
  );
}

export function useAudioPlayer(): PlayerState {
  const c = useContext(Ctx);
  if (!c) throw new Error('useAudioPlayer must be used within AudioPlayerProvider');
  return c;
}
