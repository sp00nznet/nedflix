# Marquee mobile (Capacitor)

iOS + Android shell that wraps the **same** Marquee web build (`../web/dist`) — one UI
across web, desktop, and mobile. Native capabilities (background audio, lock-screen Now
Playing, PiP, downloads, casting) come from the **`marquee-native`** Capacitor plugin
(`../marquee-native`); the web app talks to it through `web/src/native/bridge.ts` and
falls back to `<video>`/`<audio>` + Media Session on plain web.

## First-time setup
```bash
npm --prefix ../web run build     # produce web/dist
npm install                       # capacitor + marquee-native
npm run copy:web                  # web/dist -> www
npm run add:ios                   # creates ios/ (needs macOS + Xcode)
npm run add:android               # creates android/ (needs Android SDK)
npm run sync
```

## Iterate
```bash
npm run ios       # copy web -> www, cap sync, open Xcode
npm run android   # copy web -> www, cap sync, open Android Studio
```

## Auth
The bundled build calls the API same-origin, so for mobile either:
- set `server.url` in `capacitor.config.ts` to your nedflix host (the WebView holds the
  session cookie), **or**
- point the web build at a host with `VITE_API_BASE` before `npm run build`.

## Native bridge status
`marquee-native` ships a working **web** impl and iOS (AVPlayer) / Android (Media3) stubs
with transport + lock-screen Now Playing wired; **downloads / cast / PiP are TODO**
(`../marquee-native/README.md`). The audio player (`web/src/state/audioPlayer.tsx`) already
prefers the native plugin when running in the shell.

## App icons / splash
Generate from `../web/public/brand/icon.svg` (foreground) on `#0b0c11` (background) with
`@capacitor/assets`, or drop the `brand/` PNGs into the platform icon sets.

> The legacy native apps (Swift/Kotlin) were retired to `../archive/` when this shell
> replaced them.
