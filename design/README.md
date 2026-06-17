# Handoff: Marquee — front-end redesign (nedflix media server)

## Brand
The front-end is being rebranded **Marquee** (the cinema-sign metaphor: "now showing"). The signature
mark is a coral **bulb dot**: a monogram `m` in a dark rounded square with a coral dot at the top-right
(app icon / nav rail), and the wordmark `marquee` followed by a glowing coral bulb dot (topbar/hero).
The backend project/repo remains **nedflix** unless you rename it. Accent color ships as **coral
`#fb7159`** and is a configurable token (see Design Tokens).

## Overview
A full redesign of the **Marquee** self-hosted media server front-end (backend project: nedflix). It is
a **single navigable prototype** covering every primary surface: Home, Films/Series browse, Title detail, Video player,
Music (now-playing + collection), Audiobooks (library + player), Live TV (EPG guide), Profiles,
Search, and Settings.

The design goal was to move **off the Plex/Netflix look** (gold-on-charcoal and red respectively)
into its own identity — a cool cinematic "ink" base with a single configurable accent (shipping
default: **coral**). Every surface is built to be operable both up close (mouse) and at a distance
(**gamepad / D-pad / Xbox controller**) via a spatial focus engine.

## About the Design Files
The file in this bundle (`Nedflix.dc.html`) is a **design reference created in HTML** — a working
prototype that demonstrates the intended look, layout, motion, and navigation model. **It is not
production code to ship directly.**

> It is authored in a bespoke "Design Component" format (a `<x-dc>` template + a `DCLogic` class,
> rendered by an in-house runtime called `support.js`). Do **not** try to vendor that runtime into
> the product. Treat the file as an executable spec: open it in a browser to see behavior, read the
> template for structure/styling and the logic class for state/data shapes.

The task is to **recreate these designs in nedflix's existing front-end environment**, using its
established component patterns, router, and data layer. If nedflix's web client has no established
framework yet, implement in **React + TypeScript** (the data shapes below are written to map cleanly
onto React component props/state).

## Fidelity
**High-fidelity.** Final colors, typography, spacing, radii, motion, and interaction states are all
specified and should be reproduced precisely. The one deliberate abstraction: **artwork is
represented by gradient placeholders** (posters, backdrops, album/cover art, channel logos all show
a labeled gradient). In production these map to real artwork URLs from your metadata providers —
see `DATA_SPEC.md`. Keep the gradient as the loading/empty state.

---

## Global Layout & Shared Chrome

```
┌──────┬─────────────────────────────────────────────┐
│ nav  │  main (scrolls)                              │
│ rail │   - per-screen content                       │
│ 98px │                                              │
│fixed │                                              │
└──────┴─────────────────────────────────────────────┘
            (fixed D-pad hint pill, bottom-right, hidden on player)
```

- **Root**: `display:flex; height:100vh; overflow:hidden; background:#0b0c11`. Carries CSS var
  `--ac` (accent) and `--ac-ink` (on-accent text, `#ffffff`).
- **Nav rail** — `flex:0 0 98px`, full height, vertical, `linear-gradient(180deg,#14161f,#0e0f15)`,
  right border `1px solid rgba(236,239,247,.07)`.
  - Brand glyph at top: 30×30, radius 9, `background:var(--ac)`, white "n", glow shadow.
  - Primary items (each 74×60, radius 14, icon + 9px mono caption): **HOME, FILMS, SERIES, MUSIC,
    BOOKS, LIVE**.
  - Footer items: **SEARCH, SETUP** (settings), and a circular **profile avatar** (40×40).
  - Active item: accent color + a 3×24 accent bar pinned to the rail's left edge. Inactive icon
    color `#7c8193`. Focus ring: `box-shadow:0 0 0 2px var(--ac)` over a faint accent wash.
- **D-pad hint pill** — fixed bottom-right, mono 10px, shows `◄ ▲ ▼ ► NAVIGATE · ↵ SELECT · esc BACK`.
  Hidden on the video player screen.

---

## Screens / Views

### 1. Home (`screen='home'`)
- **Purpose**: Curated landing — a featured hero + horizontal rails. Deliberately *not* an infinite
  recommendation grid.
- **Layout**: 580px hero, then stacked rails.
  - **Hero**: full-bleed backdrop gradient with 3 stacked scrims (left-to-right + bottom fade +
    radial highlight). Content block bottom-left, max-width 620: eyebrow (`FEATURED FILM · IN YOUR
    LIBRARY`), 72px title, meta row (year · genre · runtime + tag chips), 16px synopsis,
    actions: **Resume · 42%** (filled accent), **More info** (glass), **+** icon (glass).
  - **Rails**: "Continue watching" (16:9 cards, 300px, progress bar + resume overlay + context line
    like "S2 E11 · 18 min left"), "Films" and "Series" (2:3 poster cards, 186px). Each card title
    overlaid bottom-left; meta line under the card. Horizontal scroll, 40px side padding, 18px gap.
- **Card focus/hover**: `translateY(-5px) scale(1.04)` + accent ring `0 0 0 4px var(--ac)`.

### 2. Browse — Films / Series (`screen='films' | 'series'`)
- **Purpose**: Full library grid for one media type.
- **Layout**: Header (eyebrow `LIBRARY`, 40px title, "Sort · Recently added" control, count) then a
  responsive grid `repeat(auto-fill, minmax(176px,1fr))`, 20px gap, of 2:3 poster cards (same card
  as Home rails).

### 3. Title Detail (`screen='detail'`)
- **Purpose**: Everything about one title; entry point to playback.
- **Layout**: 470px backdrop with a **Back** button (top-left) and title block (kind eyebrow, 58px
  title, meta row with rating chip + tag chips). Below: two-column `1fr / 300px`.
  - **Left**: action row (**Play** filled, **My List**, download icon, cast/audio icon), 17px
    synopsis. For **series**: an **Episodes** list (season dropdown + rows: number, 16:9 thumb with
    play affordance + progress, title, duration, 2-line synopsis). Then a **More like this** rail.
  - **Right (sticky)**: **Cast** card (avatar + name rows) and a **File · Technical** card
    (mono key/value: VIDEO codec, CONTAINER, BITRATE, SIZE, AUDIO tracks, SUBS).

### 4. Video Player (`screen='player'`)
- **Purpose**: Playback surface. D-pad hint hidden here.
- **Layout**: Full-viewport black stage (dim backdrop gradient + vignette, center mono label
  `VIDEO · 3840 × 2160 · HEVC`).
  - **Top bar**: Back + "NOW PLAYING / title", quality chips (4K HDR, ATMOS).
  - **Center**: large circular play/pause toggle (88px).
  - **Bottom controls** (gradient scrim): scrubber row (current time, **clickable seek track** with
    a lighter "buffered" fill at 78% behind the accent progress fill + draggable knob, remaining
    time) then transport row: −10s, **play/pause** (filled accent 52px), +30s, volume slider, and
    right cluster **SUBS · EN**, **AUDIO · 5.1**, fullscreen.
- **Live behavior**: a 1s interval advances `cur` while `playing`. Space toggles play/pause.

### 5. Music (`screen='music'`)
- **Sub-nav** (pill tabs): **Now Playing / Albums / Artists** (`musicTab`).
  - **Now Playing**: two-column `1.25fr / 1fr`. Left = now-playing card (16:9 album art with an
    **animated equalizer visualizer** at the bottom, 3 modes — BARS / MIRROR / PULSE — switchable
    via small mono buttons; track title/artist/album; seek bar; transport: shuffle, prev, **play
    62px circle**, next, repeat). Right = **Up next** queue list (now-playing row shows a live
    3-bar equalizer, others a ♪).
  - **Albums**: grid `minmax(180px,1fr)`, 1:1 art tiles + title/artist·tracks.
  - **Artists**: grid `minmax(150px,1fr)`, circular avatars + name + album count.

### 6. Audiobooks (`screen='audiobooks'`)
- **Sub-nav** (pill tabs): **Library / Listening** (`bookTab`).
  - **Library** (default): "Continue listening" shelf (330px horizontal cards: 80×120 cover,
    title/author, progress bar, "% · RESUME") + "All audiobooks" grid `minmax(150px,1fr)` of 2:3
    covers (progress bar on in-progress, author + "genre · length" beneath). Tapping a cover calls
    `openBook(id)` → switches to Listening.
  - **Listening**: two-column `380px / 1fr`. Left = player card (2:3 cover, narrator line, chapter
    title, chapter seek bar, transport −15s / **play 64px** / +30s, SPEED / SLEEP / bookmark row).
    Right = **Chapters** list (book % complete bar; rows: number, title, duration; states
    done/current/todo color-coded with the accent for current).

### 7. Live TV (`screen='live'`)
- **Purpose**: IPTV/EPG guide.
- **Layout**: Header ("ON AIR NOW" pulse + 40px title; clock + "SAT · 14 JUN · M3U + XMLTV").
  - **Favorites** row (pinned above guide): horizontal cards (channel number badge, name,
    `● now-playing` in live-red, progress bar).
  - **Guide grid**: a header row (CHANNEL | time columns) then per-channel rows: left cell =
    logo badge + name + `● now`; right cell = program blocks sized by `flex:weight`, the **LIVE**
    block gets accent border/badge + progress, others are neutral. Times in mono.

### 8. Profiles (`screen='profiles'`)
- Centered "Who's watching?" with 124×124 rounded avatar tiles (initial on a per-profile color) +
  an "Add profile" dashed tile. Selecting sets `profile` and returns Home.

### 9. Search (`screen='search'`)
- Big search field (focusable, "/ TO FOCUS" hint), quick-suggestion chips, then a "Browse your
  library" responsive poster grid.

### 10. Settings (`screen='settings'`)
- Max-width 880 stack of grouped cards, each `border:1px solid rgba(236,239,247,.08)` on
  `rgba(236,239,247,.02)`, radius 18:
  - **Library**: media path (editable field + **Scan**), NFS mount.
  - **Live TV & Auto-channels**: M3U playlist URL, XMLTV EPG URL, **ErsatzTV** toggle.
  - **Playback & Subtitles**: Hardware decoding, HDR passthrough, Subtitles-on-by-default
    (OpenSubtitles), Skip intro & autoplay-next — all **toggles**.
  - **Appearance**: **Accent** swatch row (maps to the `--ac` variable / `accent` prop).
- **Toggle spec**: 44×26 track, radius 999; off `rgba(236,239,247,.18)`, on `var(--ac)`; 20px knob
  (`#0b0c11`) translates 2px→20px; 0.2s transition.

---

## Interactions & Behavior

- **Navigation model**: single-component screen router via `state.screen`. Nav rail buttons call
  `go(screen)`; cards call `openDetail(id)`; Play buttons call `play()`; Back/Esc call `back()`
  (player → detail, audiobook-listening → library, anything else → home).
- **Spatial focus engine (the gamepad story)** — implemented in the logic class:
  - All interactive elements carry `data-focusable` + `tabindex="0"`.
  - Arrow keys (`keydown`) call `moveFocus(dir)`, which scores every visible focusable by geometric
    distance from the current one in that direction (primary-axis distance + 1.8× cross-axis
    penalty, with a 50px tolerance cone) and focuses the best candidate.
  - `Enter` clicks the focused element; `Esc`/`Backspace` = back; `Space` = play/pause on player.
  - On every screen change, focus resets to the first non-nav focusable (double-rAF).
  - **Map controller buttons to these keys**: D-pad → arrows, A → Enter, B → Esc.
- **Focus visuals**: every focusable defines a `style-focus` accent ring; cards add lift+scale.
- **Motion**: screens enter with `nf-rise` (0.3–0.5s, opacity+8–10px translate). Cards lift on
  hover/focus (0.2s `cubic-bezier(.2,.7,.3,1)`). Equalizer bars animate `nf-eq` (scaleY) with
  randomized per-bar duration/delay; "ON AIR" dot pulses (`nf-pulse`).
- **Player ticker**: `setInterval` 1s increments `cur` toward `dur` while `playing`.
- **Persistence**: `screen/section/detailId/profile` saved to `localStorage['nedflix-state']` on
  update and restored on mount. **Replace with your real router/URL state** in production.

## State Management
Local component state in the prototype (lift into router params + data store in production):

| State | Meaning |
|---|---|
| `screen` | active top-level surface (`home/films/series/detail/player/music/audiobooks/live/profiles/search/settings`) |
| `section` | which nav item is highlighted |
| `detailId` | currently opened title id |
| `playing` / `cur` / `dur` | video transport |
| `profile` | active profile index |
| `musicTab` | `now/albums/artists` |
| `musicPlaying` / `vizMode` | music transport + visualizer mode |
| `bookTab` / `bookId` / `bookPlaying` | audiobook collection + player |
| `toggles` | settings booleans (autoplay, subsDefault, hdr, hwdecode, skipintro, ersatz) |

Data needs are described in **`DATA_SPEC.md`** — it maps every prototype data shape to the
corresponding nedflix backend concept (library scan, metadata, M3U/XMLTV, ErsatzTV, OpenSubtitles,
HLS/transcode, resume points).

## Design Tokens

**Color — base (cool ink)**
| Token | Hex | Use |
|---|---|---|
| bg-0 | `#0b0c11` | app background, knob fill |
| bg-1 | `#101220` | raised panel (player cards) |
| bg-2 | `#14161f` / `#0e0f15` | nav rail gradient |
| panel | `#191c27` | now-playing/player card top |
| ink-0 | `#f6f8fd` | brightest headings |
| ink-1 | `#eceef4` | primary text |
| ink-2 | `#c1c6d3` | secondary text |
| ink-3 | `#8c91a2` | tertiary |
| ink-4 | `#626878` | muted / mono labels |
| hairline | `rgba(236,239,247,.07–.12)` | borders |
| overlay | `rgba(11,12,17,*)` | scrims |
| live-red | `#ec5c4c` | on-air indicator only |

**Color — accent (CSS `--ac`, tweakable prop `accent`)**
default `coral #fb7159`; presets: iris `#7378f2`, violet `#9a6cf0`, indigo `#5a63e0`,
mint `#23b187`, blue `#4f7cf5`. On-accent text/icons: `#ffffff`. Accent is used for: nav active,
focus rings, primary buttons, progress/seek fills, visualizer bars, LIVE markers, current chapter.

**Poster/art placeholder gradients** (8, `155deg`, dark): indigo, plum, rust, forest, teal, wine,
sand, slate — see the `GRAD` map in the logic class. Replace with real artwork; keep as fallback.

**Typography**
- Display/UI: **Schibsted Grotesk** (400/500/600/700). Headings 600–700, tight letter-spacing
  (−.02 to −.03em on large sizes). Sizes in use: 72 (hero), 58 (detail), 40 (section title),
  30 (now-playing), 21 (card title), 14–17 (body), down to 13.
- Mono / technical: **JetBrains Mono** (400/500). Used for eyebrows, timecodes, EPG times, codec
  metadata, captions — typically 9–13px with .08–.24em letter-spacing.

**Radius**: chips/badges 6–8 · buttons 10–13 · cards 11–16 · panels 18–24 · pills/knobs 999.
**Shadows**: card `0 12px 30px -14px rgba(0,0,0,.75)`; accent button glow
`0 12px 30px -12px var(--ac @ ~.55)`. **Spacing**: 40px screen gutters, 18–22px grid gaps.

## Assets
No binary assets ship in this prototype. All "images" are CSS gradient placeholders labeled
ARTWORK / BACKDROP / ALBUM ART / COVER. Icons are inline SVG (feather-style, 1.7 stroke). Fonts load
from Google Fonts (Schibsted Grotesk, JetBrains Mono) — swap for your self-hosted copies. In
production, source real artwork from your existing metadata pipeline (TMDB/MusicBrainz/etc.).

## Files
- `Marquee.dc.html` — the complete prototype (all 10 surfaces, focus engine, data shapes).
- `DATA_SPEC.md` — data shapes → nedflix backend mapping and suggested API surface.
- `COMPONENT_STRUCTURE.md` — suggested React + TypeScript file/component tree to build this in.
- `MOBILE_HANDOFF.md` — how to reproduce the mobile UI on web (PWA) + native iOS/Android hooks.
- `marquee-native/` — Capacitor plugin scaffold for the native bridge: `MarqueeNative` TS interface,
  a working web implementation, and iOS (Swift/AVPlayer) + Android (Kotlin/Media3) stubs.
- `Marquee Mobile.dc.html` — the navigable mobile prototype (bottom tab bar, mini-player, overlays).
- **Additional screens (gap-fill, storyboard sheets):**
  - `Marquee Onboarding.dc.html` — connect server → sign in → choose libraries → who's watching → PIN.
  - `Marquee Detail Pages.dc.html` — album (tracklist), artist (discography), audiobook (chapters).
  - `Marquee States.dc.html` — search results + loading / empty / no-results / offline / error / scanning.
  - `Marquee Settings.dc.html` — settings sub-pages: users & parental PINs, server & storage, transcoding.
  - `Marquee Player Extras.dc.html` — subtitle/audio track picker, "Play on" cast picker, Up Next queue.
  - `Marquee Mobile Extras.dc.html` — My List and Downloads / offline (mobile).
  - `Marquee Browse & Collections.dc.html` — genre/year/rating filtered browse + boxsets/collection detail.
- `brand/` — app icons (favicon / PWA / maskable / apple-touch), `icon.svg` master,
  `manifest.webmanifest`, `og-image.png`, and a brand `README.md` with drop-in `<head>` tags.
- `Marquee Brand.dc.html` — the brand & app-icon lockup sheet (construction, sizes, variants, lockups).
- `screenshots/` — reference captures of every screen (01–12). **Note:** the capture pipeline
  substitutes a serif fallback for the display face; the live prototype renders in **Schibsted
  Grotesk** (see Typography). Captures are authoritative for layout/color/composition; the token
  spec + live file are authoritative for type.

> To run the prototype: open `Marquee.dc.html` in a browser. Use arrow keys + Enter + Esc, or a
> gamepad mapped to those keys, to navigate.
