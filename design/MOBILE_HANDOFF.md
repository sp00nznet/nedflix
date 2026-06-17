# MOBILE_HANDOFF — reproduce on web, then ship to iOS/Android

Covers two things: **(A)** how to reproduce `Marquee Mobile.dc.html` as a real responsive web /
PWA, and **(B)** the native hooks needed so iOS and Android run the **same interface** instead of a
separate rebuild.

## Strategy: one web UI, thin native shells
The prototype is a touch web UI. The lowest-effort way to get "the same interface" on phones is to
ship the **web app inside a native shell** (Capacitor) and call out to native code only for the
things browsers can't do well (background audio, downloads, casting, PiP). You keep **one codebase
and one design system** across web + iOS + Android.

```
            ┌──────────────── React web UI (this prototype) ────────────────┐
            │  responsive: phone layout < 768px · desktop rail ≥ 768px       │
            └───────────────┬───────────────────────────┬───────────────────┘
                            │                            │
                    Browser / PWA              Capacitor WebView (iOS + Android)
                    (Media Session,            + native plugins:
                     PiP, Fullscreen)            audio · player · downloads · cast · push
```

> Alternative considered: **React Native / Expo**. It gives more "native" feel but uses a *different*
> render layer — you'd re-implement every screen and lose the shared web UI. Choose RN only if you
> want fully native views and are willing to maintain two front-ends. For "same interface," Capacitor
> (or Tauri Mobile) wins.

---

## A) Reproduce on the web

### Responsive shell
- One app, two layouts at a `768px` breakpoint: **< 768** = this mobile design (bottom tab bar,
  single column, full-bleed hero); **≥ 768** = the desktop rail layout from the main handoff. Same
  routes, same data, same components — only the chrome (NavRail ↔ BottomTabBar) and grid density swap.
- The prototype's **mock status bar, device bezel, and home indicator are presentation only** —
  delete them in production and render full-bleed into the real viewport.

### Viewport, safe areas, sizing
```html
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
```
- Use `100dvh` (not `100vh`) so the layout survives the mobile URL bar show/hide.
- Pad the bottom tab bar and any bottom sheet with the safe-area inset:
  `padding-bottom: calc(8px + env(safe-area-inset-bottom));` (the prototype hardcodes 26px — replace
  with `env()`). Pad the top app bar with `env(safe-area-inset-top)`.
- Touch targets ≥ 44×44 (already true in the prototype). Disable double-tap zoom on controls
  (`touch-action: manipulation`).

### PWA / installable
- Ship `brand/manifest.webmanifest` (already built: standalone, theme `#0b0c11`, maskable icon) +
  the `<head>` tags in `brand/README.md`.
- **Service worker** (Workbox): precache the app shell + fonts; runtime-cache artwork
  (stale-while-revalidate) and API GETs (network-first). Gives offline browse + instant relaunch.
- `display: standalone`, `theme-color #0b0c11`, apple-touch-icon, splash from `og`/icon assets.

### Media + the bits people forget
- Video/audio: `<video>`/`<audio>` + **hls.js** against your transcode endpoint.
- **Media Session API** → lock-screen / headphone controls on web *and* inside the Capacitor WebView:
  ```js
  navigator.mediaSession.metadata = new MediaMetadata({ title, artist, album, artwork:[{src,sizes,type}] });
  navigator.mediaSession.setActionHandler('play',  onPlay);
  navigator.mediaSession.setActionHandler('pause', onPause);
  navigator.mediaSession.setActionHandler('seekto', e => seek(e.seekTime));
  navigator.mediaSession.setActionHandler('previoustrack'/'nexttrack', …);
  ```
- **Picture-in-Picture**: `videoEl.requestPictureInPicture()` (web/Android); iOS needs the native
  hook (below).
- **Fullscreen** player: `requestFullscreen()` + lock to landscape via Screen Orientation API.
- **Wake Lock** during playback: `navigator.wakeLock.request('screen')`.
- **Visualizer**: Web Audio `AnalyserNode` → drive the equalizer bars (decorative in the prototype).
- Gestures: native momentum scroll; add swipe-back on overlays (Detail/Player/Now-Playing) and an
  optional pull-to-refresh on tab roots.

---

## B) Native hooks for iOS / Android (Capacitor)

Each capability below is something the web UI **calls into**; the native side implements it and
**emits events back** (e.g. a lock-screen "pause" must update the web UI). Define one bridge
interface and implement it per platform.

| Capability | Why native | iOS API | Android API | Plugin |
|---|---|---|---|---|
| **Background audio** (keeps playing when screen locks / app backgrounded) | browsers suspend audio in background | `AVAudioSession` category `.playback` | foreground `MediaSessionService` + media notification | `@capacitor-community/audio` or custom |
| **Lock screen / Control Center / Now Playing** | OS transport UI | `MPNowPlayingInfoCenter` + `MPRemoteCommandCenter` | `MediaSessionCompat` + `MediaStyle` notification | media-session plugin |
| **Native video / HLS / DRM** (optional, for perf/offline/HDR) | `<video>` can't do offline HLS or FairPlay | `AVPlayer` / `AVPlayerViewController` | `ExoPlayer` / **Media3** | custom player plugin |
| **Picture-in-Picture** | iOS web has no PiP | `AVPictureInPictureController` | native PiP mode (`enterPictureInPictureMode`) | custom |
| **Offline downloads** | background, resumable, encrypted | `URLSession` background config | `WorkManager` / `DownloadManager` | `@capacitor/filesystem` + custom |
| **Casting** | living-room handoff | **AirPlay** (`AVRoutePickerView`) | **Google Cast** SDK | cast plugin |
| **Hardware decode / HDR / Atmos passthrough** | battery + quality | handled by `AVPlayer` | handled by `ExoPlayer` | (native player) |
| **Push** (new episodes, resume nudges) | APNs/FCM | APNs | FCM | `@capacitor/push-notifications` |
| **Deep / universal links** (`marquee.mov/title/123` opens app) | OS link routing | Associated Domains + `apple-app-site-association` | App Links + `assetlinks.json` | `@capacitor/app` |
| **Secure auth storage** | tokens off JS | Keychain | Keystore/EncryptedSharedPrefs | `@capacitor/preferences` + secure-storage |
| **Status bar / safe areas / orientation** | native chrome | `UIStatusBar`, orientation lock | window insets, orientation | `@capacitor/status-bar`, `@capacitor/screen-orientation` |
| **Haptics, network status, keep-awake** | polish | `UIFeedbackGenerator`, Reachability | `Vibrator`, `ConnectivityManager` | `@capacitor/haptics`, `@capacitor/network` |
| **App icons / splash** | home screen | iOS AppIcon set (use `brand/` PNGs) | adaptive icon: foreground = `icon.svg` mark, background `#0b0c11`; splash from icon | `@capacitor/splash-screen` |

### The bridge contract (one interface, two implementations)
> ✅ **Scaffolded for you** in `marquee-native/` — the `MarqueeNative` TypeScript interface
> (`src/definitions.ts`), a working web implementation (`src/web.ts`), and iOS (Swift/AVPlayer) +
> Android (Kotlin/Media3) stubs with complete method signatures. Transport + lock-screen Now Playing
> are wired; downloads/cast/PiP are marked `TODO`.

Expose a single object the React app talks to; swap web ⇄ native at runtime:
```ts
interface MarqueeNative {
  load(track: {id; url; title; artist; artworkUrl; type:'video'|'audio'}): Promise<void>;
  play(): void;  pause(): void;  seek(sec: number): void;
  setNowPlaying(meta): void;                 // lock-screen metadata
  enterPiP(): void;
  startDownload(id: string): Promise<void>;  removeDownload(id): void;
  listRoutes(): Promise<CastRoute[]>;  cast(routeId: string): void;
  on(evt: 'remote-play'|'remote-pause'|'remote-seek'|'ended'|'download-progress', cb): void;
}
```
- **Web impl**: `<video>`/`<audio>` + Media Session + PiP + Web download.
- **Native impl**: a Capacitor plugin forwarding to AVPlayer/ExoPlayer; the OS transport events
  (lock-screen pause, headphone next) come back through `on(...)` so the **web UI stays in sync** —
  this is the part that makes it feel native rather than a website in a box.

### Orientation rules
- Lock the app to **portrait** everywhere except the **video player**, which allows **landscape**
  and offers fullscreen + PiP. The prototype's player overlay is the screen that flips.

---

## Recommended stack & build order
1. **React + TS responsive web** (mobile layout < 768, desktop ≥ 768) — reuse the component layer
   from `COMPONENT_STRUCTURE.md`; add `BottomTabBar`, `MiniPlayer`, and the Detail/Player/NowPlaying
   overlays from this prototype.
2. **PWA**: manifest + service worker + Media Session + safe-area insets. Ships to web immediately.
3. **Capacitor wrap**: add iOS + Android projects; wire `@capacitor/status-bar`, `splash-screen`,
   `app` (deep links), `push-notifications`.
4. **Media plugin**: background audio + lock-screen Now Playing (highest-value native hook).
5. **Downloads, Cast/AirPlay, PiP, native player** as needed.

> Net: build the interface **once** as responsive web (this prototype), ship it as a PWA, then wrap
> it with Capacitor and implement the `MarqueeNative` bridge so iOS/Android reuse the exact same UI
> with real background audio, lock-screen controls, downloads, and casting.

## Files
- `Marquee Mobile.dc.html` — the navigable mobile prototype (Home, Search, Listen, Live, You +
  Detail / Video player / Now-Playing overlays, mini-player, bottom tab bar).
