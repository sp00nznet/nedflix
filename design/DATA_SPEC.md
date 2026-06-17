# DATA_SPEC — prototype data shapes → nedflix backend

This maps every data structure used in the prototype (`Nedflix.dc.html`, in the `Component` logic
class) to the corresponding nedflix backend concept, plus a suggested API surface. Field names are
the prototype's; rename to match your real schema.

> The prototype hardcodes catalogs (`C` = video titles, `BK` = audiobooks, plus inline album/artist/
> channel arrays) and derives view-model objects in `renderVals()`. In production, replace the
> hardcoded maps with fetched data and keep the derivation logic.

---

## 1. Video title (films + series) — prototype `C[id]`

```ts
interface Title {
  id: string;
  title: string;
  year: number;
  type: 'Film' | 'Series';
  genre: string;
  runtime: string;        // "2h 08m"  (films)  | "S2 · 16 eps" (series)
  rating: string;         // "TV-MA"
  tags: string[];         // ["4K","Dolby Vision","Atmos"]  ← derive from media probe
  synopsis: string;
  // production additions:
  backdropUrl?: string; posterUrl?: string;
  resume?: { positionSec: number; durationSec: number };  // → Continue watching
  technical?: TechnicalInfo;   // see §5
  cast?: { name: string; photoUrl?: string }[];
}
```
- **Source**: nedflix library scan + metadata provider (TMDB/TVDB). `tags` (4K/HDR/Atmos) come from
  the **media probe** (ffprobe), not the metadata provider.
- **Continue watching** = titles with a `resume` row (per profile). Context line ("S2 E11 · 18 min
  left") is computed from resume + episode.
- Suggested: `GET /api/library/titles?type=film|series`, `GET /api/titles/:id`,
  `GET /api/profiles/:pid/continue`.

### Episodes (series detail) — prototype `episodes[]`
```ts
interface Episode { id:string; season:number; number:number; title:string;
  durationSec:number; synopsis:string; thumbUrl?:string;
  resume?:{positionSec:number;durationSec:number}; }
```
`GET /api/titles/:id/seasons/:s/episodes`.

---

## 2. Audiobook — prototype `BK[id]`

```ts
interface Audiobook {
  id:string; title:string; author:string; narrator:string;
  genre:string; len:string /* "9h 12m" */;
  progressPct:string /* "46%" — derive from resume */;
  coverUrl?:string;
  chapters: Chapter[];
}
interface Chapter { number:number; title:string; durationSec:number;
  state:'done'|'current'|'todo' /* derive from resume */; }
```
- **Continue listening** = audiobooks with progress > 0. **Library** = all.
- Source: audiobook library scan (m4b chapters / per-file). Resume stored per profile.
- `GET /api/audiobooks`, `GET /api/audiobooks/:id`, `PUT /api/audiobooks/:id/progress`.

---

## 3. Music — prototype `albums` / `allAlbums` / `artists` / `queue` / `nowMusic`

```ts
interface Album  { id:string; title:string; artist:string; trackCount:number; artUrl?:string; }
interface Artist { id:string; name:string; albumCount:number; imageUrl?:string; }
interface Track  { id:string; title:string; artist:string; album:string; durationSec:number; }
interface NowPlaying { track:Track; positionSec:number; }   // drives the 3 visualizer modes
```
- Source: music library scan + tags (MusicBrainz optional). **Now Playing / Albums / Artists** are
  three views over the same collection.
- The **visualizer** (`vizMode: bars|mirror|pulse`) is pure front-end animation in the prototype;
  wire to real audio-analyser data (Web Audio `AnalyserNode`) if desired, else keep decorative.
- `GET /api/music/albums`, `/artists`, `/albums/:id/tracks`, plus the player queue.

---

## 4. Live TV / EPG — prototype `channels[]`, `favChannels`, `liveTimes`

```ts
interface Channel {
  id:string; num:string; name:string; logoUrl?:string;
  favorite:boolean;                       // → Favorites row
  nowPlaying:{ title:string; progressPct:string };
  programs: Program[];                    // EPG schedule
}
interface Program { title:string; startTime:string; endTime:string;
  isLive:boolean; widthWeight:number /* span across guide */; }
```
- **Source**: **M3U** playlist (channels) + **XMLTV** (EPG/programs) — both already in nedflix
  Settings. `favorite` is a per-user flag you store.
- **ErsatzTV** auto-channels appear here as ordinary channels (24/7 schedule from your library).
- `widthWeight` in the prototype is illustrative; in production compute block width from
  `start/end` vs. the visible time window.
- `GET /api/livetv/channels` (merged M3U+XMLTV), `GET /api/livetv/epg?from&to`,
  `PUT /api/livetv/channels/:id/favorite`.

---

## 5. Technical / File info — prototype `meta`

```ts
interface TechnicalInfo {
  codec:string;      // "HEVC · 10-bit"
  container:string;  // "MKV"
  size:string;       // "14.2 GB"
  bitrate:string;    // "24.6 Mb/s"
  audioTracks:string;// "2 · Atmos / 5.1"
  subs:string;       // "8 · OpenSubtitles"
}
```
Straight from **ffprobe** on scan. Subtitle count includes embedded + OpenSubtitles fetches.

---

## 6. Playback / player

- **Video**: the prototype fakes playback with a 1s `cur++` ticker. Replace with your real player
  (hls.js / `<video>` against your **HLS/transcode** endpoint). Honor **HW decoding / HDR
  passthrough** settings. Persist resume position on pause/exit (`positionSec` on Title/Episode).
- **Seek**: the scrubber's click handler maps clientX→fraction→`positionSec`. The lighter fill
  behind the progress fill is the **buffered** range — wire to `video.buffered`.
- **Subtitle / audio track** chips (SUBS · EN / AUDIO · 5.1) → real track selection + OpenSubtitles
  download-on-demand.

---

## 7. Profiles & Settings

```ts
interface Profile { id:string; name:string; initial:string; color:string; }
interface Settings {
  mediaPath:string; nfsMount:string;
  m3uUrl:string; xmltvUrl:string; ersatzTvEnabled:boolean;     // ← state.toggles.ersatz
  hardwareDecoding:boolean; hdrPassthrough:boolean;            // ← hwdecode / hdr
  subtitlesByDefault:boolean; skipIntroAutoplay:boolean;       // ← subsDefault / skipintro
  accent:'coral'|'iris'|'violet'|'indigo'|'mint'|'blue';       // ← --ac / accent prop
}
```
- Resume points, favorites, and `accent` are **per-profile**; library paths, M3U/XMLTV, ErsatzTV,
  decode/HDR are **server-global**.
- The **accent** is already a first-class tweakable (`accent` prop → `--ac` CSS var). Persist it and
  apply on load.

---

## 8. Suggested implementation order
1. Shared chrome + nav rail + **spatial focus engine** (the reusable foundation; everything depends
   on it for gamepad support).
2. Library list/detail/player for **video** (largest surface; resume + technical info).
3. **Music** and **Audiobooks** collections + players.
4. **Live TV** (M3U/XMLTV merge + EPG grid + favorites).
5. **Settings** wired to real server config; **Profiles**; **Search**.

Keep the gradient artwork placeholders as loading/empty states throughout.
