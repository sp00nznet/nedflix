# marquee-native (Capacitor plugin scaffold)

The single native bridge for Marquee. The React UI talks **only** to `MarqueeNative`; the web build
uses `<video>`/`<audio>` + Media Session, and inside Capacitor the iOS/Android implementations
forward to **AVPlayer** / **ExoPlayer-Media3** and emit OS transport events back so the UI stays in
sync. See `../MOBILE_HANDOFF.md` for the strategy.

> **Status: scaffold.** Transport + lock-screen Now Playing are wired; downloads, casting, and PiP
> are stubbed with `TODO`s and complete signatures so you can fill them in incrementally.

## Layout
```
marquee-native/
  package.json
  src/
    definitions.ts   # the MarqueeNative TypeScript interface + types  ← source of truth
    index.ts         # registerPlugin (lazy-loads web impl)
    web.ts           # functional web/PWA implementation
  ios/Sources/MarqueeNativePlugin/MarqueeNativePlugin.swift   # AVPlayer + MPRemoteCommandCenter
  android/src/main/java/com/marquee/native/MarqueeNativePlugin.kt  # ExoPlayer/Media3 + MediaSession
```

## Install (in your app)
```bash
npm i ./marquee-native        # or publish privately and install by name
npx cap sync
```
iOS: add background mode **Audio, AirPlay, and Picture in Picture** to `Info.plist`
(`UIBackgroundModes` → `audio`). Android: add the Media3 deps and a foreground-service permission.

## Usage — wire it to your player store
```ts
import { MarqueeNative } from 'marquee-native';

// keep the OS transport in sync with the UI
await MarqueeNative.addListener('remotePlay',  () => playerStore.play());
await MarqueeNative.addListener('remotePause', () => playerStore.pause());
await MarqueeNative.addListener('remoteSeek',  e => playerStore.seek(e.positionSec));
await MarqueeNative.addListener('stateChange', s => playerStore.sync(s));   // position/duration
await MarqueeNative.addListener('ended',       () => playerStore.next());

// start playback
await MarqueeNative.load({
  id: title.id, url: title.hlsUrl, type: 'video',
  title: title.name, artist: title.year?.toString(), artworkUrl: title.artworkUrl,
  startAtSec: title.resume?.positionSec ?? 0,
});
await MarqueeNative.play();
await MarqueeNative.setNowPlaying({ title: title.name, artist: title.genre, artworkUrl: title.artworkUrl, durationSec: title.runtimeSec });
```

The **same calls** run on web (Media Session drives the lock screen / headphones) and on native
(AVPlayer/ExoPlayer drive the real OS transport) — no UI branching.

## What's wired vs. TODO
| Area | Web | iOS | Android |
|---|---|---|---|
| load/play/pause/seek/rate/getState | ✅ | ✅ AVPlayer | ✅ ExoPlayer |
| lock-screen Now Playing + remote events | ✅ Media Session | ✅ MPNowPlayingInfoCenter / MPRemoteCommandCenter | ⚠️ MediaSession (TODO: service + notification) |
| background audio | n/a | ✅ AVAudioSession `.playback` | ⚠️ TODO foreground MediaSessionService |
| Picture-in-Picture | ✅ requestPictureInPicture | ⚠️ TODO AVPlayerLayer + controller | ⚠️ TODO PiP mode |
| offline downloads | ✅ Cache API | ⚠️ TODO background URLSession | ⚠️ TODO Media3 DownloadService |
| casting (AirPlay/Cast) | ⚠️ Remote Playback prompt | ⚠️ TODO AVRoutePickerView / Cast SDK | ⚠️ TODO Cast SDK / MediaRouter |

Fill the TODOs in priority order: background-audio service (Android) → PiP → downloads → cast.
