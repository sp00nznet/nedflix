// Data shapes from design/DATA_SPEC.md. Field names mirror the prototype; map the real
// nedflix API onto these in the api/* modules.

export interface Resume {
  positionSec: number;
  durationSec: number;
}

export interface TechnicalInfo {
  codec: string; // "HEVC · 10-bit"
  container: string; // "MKV"
  size: string; // "14.2 GB"
  bitrate: string; // "24.6 Mb/s"
  audioTracks: string; // "2 · Atmos / 5.1"
  subs: string; // "8 · OpenSubtitles"
}

export interface CastMember {
  name: string;
  photoUrl?: string;
}

export interface Title {
  id: string;
  title: string;
  year: number;
  type: 'Film' | 'Series';
  genre: string;
  runtime: string; // "2h 08m" | "S2 · 16 eps"
  rating: string; // "TV-MA"
  tags: string[]; // ["4K","Dolby Vision","Atmos"]
  synopsis: string;
  backdropUrl?: string;
  posterUrl?: string;
  resume?: Resume;
  technical?: TechnicalInfo;
  cast?: CastMember[];
  // production helper: server path used for playback endpoints
  path?: string;
}

export interface Episode {
  id: string;
  season: number;
  number: number;
  title: string;
  durationSec: number;
  synopsis: string;
  thumbUrl?: string;
  resume?: Resume;
  path?: string;
}

export interface ContinueItem {
  title: Title;
  resume: Resume;
  context: string; // "S2 E11 · 18 min left"
}

export interface Chapter {
  number: number;
  title: string;
  durationSec: number;
  state: 'done' | 'current' | 'todo';
}
export interface Audiobook {
  id: string;
  title: string;
  author: string;
  narrator: string;
  genre: string;
  len: string; // "9h 12m"
  progressPct: string; // "46%"
  coverUrl?: string;
  chapters: Chapter[];
  path?: string;
}

export interface Album {
  id: string;
  title: string;
  artist: string;
  trackCount: number;
  artUrl?: string;
}
export interface Artist {
  id: string;
  name: string;
  albumCount: number;
  imageUrl?: string;
}
export interface Track {
  id: string;
  title: string;
  artist: string;
  album: string;
  durationSec: number;
  path?: string;
}

export interface Program {
  title: string;
  startTime: string;
  endTime: string;
  isLive: boolean;
  widthWeight: number;
  titleId?: string; // built-in local channels: the library title to play
}
export interface Channel {
  id: string;
  num: string;
  name: string;
  logoUrl?: string;
  favorite: boolean;
  nowPlaying: { title: string; progressPct: string };
  programs: Program[];
}

export interface Profile {
  id: string;
  name: string;
  initial: string;
  color: string;
  accent: 'coral' | 'iris' | 'violet' | 'indigo' | 'mint' | 'blue';
}

export interface Settings {
  mediaPath: string;
  nfsMount: string;
  m3uUrl: string;
  xmltvUrl: string;
  ersatzTvEnabled: boolean;
  hardwareDecoding: boolean;
  hdrPassthrough: boolean;
  subtitlesByDefault: boolean;
  skipIntroAutoplay: boolean;
  accent: Profile['accent'];
}
