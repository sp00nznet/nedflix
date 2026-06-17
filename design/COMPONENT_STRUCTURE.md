# COMPONENT_STRUCTURE — suggested React + TypeScript build

A concrete file/component tree for recreating the Marquee prototype in a real front-end. This is a
**recommendation**, not a mandate — if your web client already has conventions (router, data layer,
styling), follow those and use this as a mapping from prototype → components. Framework-agnostic
readers: the component boundaries and the focus-engine contract translate directly to Vue/Svelte/
SwiftUI.

## Stack assumptions
- **React 18 + TypeScript**, Vite.
- **Routing**: React Router (URL = source of truth; replace the prototype's `localStorage` screen
  state). Each top-level surface is a route.
- **Data**: TanStack Query against the nedflix API (shapes in `DATA_SPEC.md`).
- **Styling**: your choice. The prototype is inline-styled; in production use CSS Modules or your
  existing system. Keep the **accent as a CSS variable** `--ac` set on the app root from the active
  profile's `accent` token (don't hardcode coral).
- **Playback**: `hls.js` (or native HLS) for video/audio against your transcode endpoint.

## Routes → screens
```
/                         Home
/films  /series           Browse (BrowseGrid, type-param)
/title/:id                TitleDetail
/watch/:id                VideoPlayer        (fullscreen, hides chrome)
/music                    Music   (?tab=now|albums|artists)
/audiobooks               Audiobooks (?tab=library|listening, ?book=:id)
/live                     LiveTV
/search                   Search
/settings                 Settings
/profiles                 Profiles  (gate before app, like the prototype)
```

## File tree
```
src/
  main.tsx
  App.tsx                       # router + <AppShell>
  theme.ts                      # tokens: colors, accent presets, type scale, radii, shadows

  shell/
    AppShell.tsx                # flex layout: <NavRail/> + <main> + <DpadHint/>
    NavRail.tsx                 # brand bulb mark + nav items + profile avatar
    NavItem.tsx                 # icon + caption + active bar + focus ring
    DpadHint.tsx                # bottom-right key legend (hidden on /watch)
    BrandMark.tsx               # the coral bulb monogram (app icon) + wordmark variants

  focus/                        # ★ the gamepad/10-foot foundation — build first
    SpatialFocusProvider.tsx    # keydown listener; arrows->moveFocus, Enter->click, Esc->back
    useFocusable.ts             # hook: registers element, sets tabIndex + data-focusable
    spatialNav.ts               # geometry scoring (port of prototype moveFocus): primary-axis
                                #   distance + 1.8x cross-axis penalty, ~50px tolerance cone
    gamepad.ts                  # navigator.getGamepads polling -> synthesizes arrow/Enter/Esc

  components/
    PosterCard.tsx              # 2:3 card (Home rails, Browse, Search) + focus lift/scale
    WideCard.tsx                # 16:9 continue-watching card w/ progress + resume overlay
    Rail.tsx                    # horizontal scroller w/ heading + "view all"
    Hero.tsx                    # backdrop + scrims + title block + action buttons
    Chip.tsx  TagChip.tsx
    Toggle.tsx                  # 44x26 settings switch (off->on = accent)
    PillTabs.tsx                # sub-nav for Music/Audiobooks
    ProgressBar.tsx  SeekBar.tsx# SeekBar: click/drag -> fraction; buffered underlay
    ArtPlaceholder.tsx          # gradient + label fallback (loading/empty state for all art)
    Equalizer.tsx               # animated bars; modes: bars | mirror | pulse

  screens/
    Home/Home.tsx
    Browse/BrowseGrid.tsx
    Detail/TitleDetail.tsx  EpisodeRow.tsx  CastCard.tsx  TechnicalCard.tsx
    Player/VideoPlayer.tsx  PlayerControls.tsx
    Music/Music.tsx  NowPlaying.tsx  AlbumGrid.tsx  ArtistGrid.tsx  Queue.tsx
    Audiobooks/Audiobooks.tsx  BookLibrary.tsx  BookPlayer.tsx  ChapterList.tsx
    Live/LiveTV.tsx  FavoritesRow.tsx  EpgGrid.tsx  ProgramBlock.tsx
    Search/Search.tsx
    Settings/Settings.tsx  SettingGroup.tsx
    Profiles/Profiles.tsx

  api/                          # one module per DATA_SPEC section
    titles.ts  audiobooks.ts  music.ts  livetv.ts  playback.ts  profiles.ts  settings.ts
    types.ts                    # the interfaces from DATA_SPEC.md

  state/
    useActiveProfile.ts         # current profile -> sets --ac accent on root
    usePlayer.ts                # transport state; real <video>/hls wiring (replaces 1s ticker)
```

## The focus engine (build this first)
Everything depends on it for controller support. Contract:
- Any focusable renders with `useFocusable()` → applies `tabIndex={0}` + `data-focusable`.
- `SpatialFocusProvider` listens for `keydown`: arrows call `spatialNav(direction)` which queries
  `document.querySelectorAll('[data-focusable]')` (visible only), scores by geometry, focuses best.
- `Enter` → `document.activeElement.click()`; `Esc`/`Backspace` → router back; `Space` → play/pause
  on the player route.
- On route change, focus the first non-rail focusable (double-`requestAnimationFrame`).
- `gamepad.ts` polls `navigator.getGamepads()` and maps **D-pad → arrows, A → Enter, B → Esc**,
  dispatching synthetic key events so the same code path serves remote, keyboard, and controller.
- Keep the visible **focus ring** (`box-shadow: 0 0 0 2px var(--ac)`) on every focusable — it is the
  primary affordance at 10 feet.

## Notes
- Replace all gradient `ArtPlaceholder`s with real artwork URLs; keep the gradient as the
  loading/empty fallback.
- The equalizer is decorative in the prototype; optionally drive it from a Web Audio `AnalyserNode`.
- Persist resume positions, favorites, and the `accent` token **per profile**; library paths,
  M3U/XMLTV, ErsatzTV, and decode/HDR are **server-global** (see DATA_SPEC §7).
- Suggested build order: focus engine + shell → video library/detail/player → music + audiobooks →
  live TV → settings/profiles/search.
```
