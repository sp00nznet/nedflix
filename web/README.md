# Marquee web — the unified nedflix front-end

React + TypeScript + Vite app implementing the **Marquee** redesign (see `../design/`).
This single build is the shared UI across **web, desktop (Electron), and mobile (Capacitor)**.

## Dev
```bash
npm install
npm run dev        # http://localhost:5173 — proxies /api + /auth to the backend
```
Point the proxy at a running nedflix server with `VITE_PROXY_TARGET` (default
`http://localhost:3000`). The app rides the existing passport **session cookie**
(`credentials: 'include'`); a 401 bounces to the server-rendered `/login.html`.

```bash
npm run build      # -> dist/ ; server.js serves this at the site root (Phase 6)
```

## Layout (per design/COMPONENT_STRUCTURE.md)
```
src/
  theme.ts            design tokens (colors, ACCENTS, GRAD, radii, shadows, fonts)
  focus/              the spatial-focus engine (gamepad / 10-foot)
    spatialNav.ts       moveFocus geometry scoring (ported from the prototype)
    SpatialFocusProvider.tsx  keydown: arrows/Enter/Esc/Space; refocus on route change
    gamepad.ts          getGamepads polling -> synthetic arrow/Enter/Esc
    useFocusable.ts     marks an element [data-focusable] + tabIndex
  shell/              AppShell, NavRail (>=768) / BottomTabBar (<768), DpadHint, BrandMark
  components/         PosterCard, WideCard, Rail, Hero, SeekBar, ProgressBar, Toggle,
                      PillTabs, ArtPlaceholder, Equalizer
  state/activeProfile.tsx   active profile -> sets --ac accent on the root
  api/                types.ts (DATA_SPEC shapes) + http.ts (same-origin fetch)
  screens/            one folder per surface (filled in over Phases 2-5)
  mock/catalog.ts     prototype fixtures driving the UI until each surface is API-wired
```

## Focus engine (the foundation)
Every interactive element spreads `useFocusable()` (or sets `data-focusable tabIndex={0}`).
`SpatialFocusProvider` handles arrows -> `moveFocus`, Enter -> click, Esc -> back,
Space -> play/pause on `/watch`, and gamepad mapping (D-pad->arrows, A->Enter, B->Esc). The
accent focus ring in `index.css` is the primary affordance at 10 feet.

## Backend it talks to
The Express server (`../server.js`) exposes the original video/IPTV API plus the new
Marquee endpoints in `../marquee-service.js`: `/api/profiles*`, `/api/progress`,
`/api/profiles/:pid/continue`, `/api/music/*`, `/api/audiobooks*`,
`/api/livetv/favorites`. Music/audiobooks are derived from the audio file index.
