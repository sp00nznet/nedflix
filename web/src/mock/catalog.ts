// Fixtures ported from the prototype (design/Marquee.dc.html logic class). These drive
// the UI until each surface is wired to the real nedflix API (Phases 2–5). Keep around
// as Storybook/test fixtures after wiring.
import { GRAD } from '../theme';
import type { Album, Artist, Audiobook, Channel, Title } from '../api/types';

export const TITLES: Record<string, Title> = {
  meridian: { id: 'meridian', title: 'Meridian', year: 2024, type: 'Film', genre: 'Sci-Fi Thriller', runtime: '2h 08m', rating: 'TV-MA', tags: ['4K', 'Dolby Vision', 'Atmos'], synopsis: 'A salvage crew chasing a derelict freighter to the edge of charted space finds the ship’s last broadcast still looping — and someone, impossibly, still answering.' },
  nightjar: { id: 'nightjar', title: 'Nightjar', year: 2023, type: 'Series', genre: 'Mystery', runtime: 'S2 · 16 eps', rating: 'TV-14', tags: ['4K', '5.1'], synopsis: 'A small coastal town wakes to find the tide has gone out and never come back. A returning detective works the case the locals would rather forget.' },
  saltpath: { id: 'saltpath', title: 'The Salt Path', year: 2022, type: 'Film', genre: 'Drama', runtime: '1h 54m', rating: 'PG-13', tags: ['4K', '5.1'], synopsis: 'After losing everything, a couple walks six hundred miles of coastline with only what they can carry.' },
  slowlight: { id: 'slowlight', title: 'Slow Light', year: 2025, type: 'Film', genre: 'Romance', runtime: '1h 47m', rating: 'R', tags: ['4K', 'HDR'], synopsis: 'Two strangers keep meeting in the same all-night diner, certain they have met before but never able to say where.' },
  foxglove: { id: 'foxglove', title: 'Foxglove', year: 2024, type: 'Series', genre: 'Crime', runtime: 'S1 · 8 eps', rating: 'TV-MA', tags: ['4K', 'Atmos'], synopsis: 'A botanist with a perfect memory is pulled into a string of poisonings that all trace back to her own garden.' },
  harbor: { id: 'harbor', title: 'Harbor Lights', year: 2021, type: 'Film', genre: 'Drama', runtime: '2h 02m', rating: 'PG-13', tags: ['4K', '5.1'], synopsis: 'A lighthouse keeper’s daughter returns to sell the family station and finds the lamp still burning.' },
  ember: { id: 'ember', title: 'Ember', year: 2023, type: 'Film', genre: 'Thriller', runtime: '1h 58m', rating: 'R', tags: ['4K', 'HDR', 'Atmos'], synopsis: 'A wildfire crew is cut off from the line as the wind turns. The only way down is through.' },
  staticbloom: { id: 'staticbloom', title: 'Static Bloom', year: 2025, type: 'Series', genre: 'Sci-Fi', runtime: 'S1 · 10 eps', rating: 'TV-MA', tags: ['4K', 'Dolby Vision'], synopsis: 'In a city where memories can be grown like flowers, a black-market gardener starts losing her own.' },
  palefire: { id: 'palefire', title: 'Pale Fire', year: 2020, type: 'Film', genre: 'Mystery', runtime: '2h 14m', rating: 'R', tags: ['4K', '5.1'], synopsis: 'An aging poet and his obsessive neighbor argue over a manuscript that may not exist.' },
  northwood: { id: 'northwood', title: 'Northwood', year: 2024, type: 'Series', genre: 'Adventure', runtime: 'S3 · 24 eps', rating: 'TV-14', tags: ['4K', '5.1'], synopsis: 'Three siblings inherit a failing fishing lodge at the end of the only road north.' },
  quietcoast: { id: 'quietcoast', title: 'The Quiet Coast', year: 2022, type: 'Film', genre: 'Documentary', runtime: '1h 31m', rating: 'TV-G', tags: ['4K'], synopsis: 'A year on the most remote shoreline in the country, told by the four people who still live there.' },
  coastal: { id: 'coastal', title: 'Coastal', year: 2023, type: 'Film', genre: 'Drama', runtime: '1h 49m', rating: 'PG-13', tags: ['4K', 'HDR'], synopsis: 'A retired ferry captain teaches his estranged granddaughter to read the water.' },
};

export const CONTINUE: { id: string; progress: number; context: string }[] = [
  { id: 'meridian', progress: 0.42, context: '42 min left' },
  { id: 'nightjar', progress: 0.68, context: 'S2 E11 · 18 min left' },
  { id: 'foxglove', progress: 0.15, context: 'S1 E2 · 41 min left' },
  { id: 'harbor', progress: 0.9, context: '9 min left' },
  { id: 'northwood', progress: 0.33, context: 'S3 E7 · 32 min left' },
];

export const FILM_IDS = ['slowlight', 'saltpath', 'ember', 'palefire', 'coastal', 'quietcoast', 'harbor', 'meridian'];
export const SERIES_IDS = ['nightjar', 'foxglove', 'staticbloom', 'northwood'];
export const FEATURED_ID = 'meridian';

export const ALBUMS: Album[] = [
  ['Low Tide', 'Hale', 11], ['Paper Moons', 'The Veldt', 9], ['Northern Soil', 'Bram Ode', 12],
  ['Glasshouse', 'Mira Sound', 10], ['Driftwood', 'Hale', 8], ['The Keeper', 'Lune', 13],
  ['Tidewater', 'The Veldt', 9], ['Long Water', 'Bram Ode', 11], ['Coastlines', 'Lune', 7],
].map(([title, artist, trackCount], i) => ({ id: `al${i}`, title: title as string, artist: artist as string, trackCount: trackCount as number }));

export const ARTISTS: Artist[] = [
  ['Hale', 4], ['The Veldt', 4], ['Bram Ode', 3], ['Mira Sound', 2], ['Lune', 3], ['Nia Holt', 2], ['Sela Okonkwo', 1], ['Vela', 2],
].map(([name, albumCount], i) => ({ id: `ar${i}`, name: name as string, albumCount: albumCount as number }));

const ch = (num: string, name: string, grad: string, now: string, nowPct: string, programs: Channel['programs']): Channel => ({
  id: num, num, name, logoUrl: undefined, favorite: false, nowPlaying: { title: now, progressPct: nowPct }, programs,
  // grad kept as a non-typed hint for placeholder rendering
  ...(({ grad } as unknown) as object),
});

export const CHANNELS: (Channel & { grad?: string })[] = [
  ch('01', 'Marquee Classics', GRAD.rust, 'Harbor Lights', '62%', [{ title: 'Harbor Lights', startTime: '19:30', endTime: '21:34', isLive: true, widthWeight: 3 }, { title: 'The Quiet Coast', startTime: '21:34', endTime: '23:05', isLive: false, widthWeight: 2 }, { title: 'Coastal', startTime: '23:05', endTime: '00:54', isLive: false, widthWeight: 3 }]),
  ch('02', 'Noir 24/7', GRAD.slate, 'Pale Fire', '40%', [{ title: 'Pale Fire', startTime: '18:50', endTime: '21:04', isLive: true, widthWeight: 4 }, { title: 'Ember', startTime: '21:04', endTime: '23:02', isLive: false, widthWeight: 4 }]),
  ch('03', 'Cartoon Vault', GRAD.teal, 'Morning Loops', '55%', [{ title: 'Morning Loops', startTime: '20:00', endTime: '20:30', isLive: true, widthWeight: 2 }, { title: 'Shorts Block', startTime: '20:30', endTime: '21:00', isLive: false, widthWeight: 2 }, { title: 'Northwood', startTime: '21:00', endTime: '22:30', isLive: false, widthWeight: 4 }]),
  ch('04', 'Concert Hall', GRAD.plum, 'Hale — Live', '70%', [{ title: 'Hale — Live at Low Tide', startTime: '19:00', endTime: '22:00', isLive: true, widthWeight: 6 }, { title: 'Encore', startTime: '22:00', endTime: '23:00', isLive: false, widthWeight: 2 }]),
  ch('05', 'Nature Loop', GRAD.forest, 'Coastlines', '48%', [{ title: 'Coastlines', startTime: '19:45', endTime: '22:00', isLive: true, widthWeight: 3 }, { title: 'Tidepools', startTime: '22:00', endTime: '00:30', isLive: false, widthWeight: 5 }]),
  ch('06', 'The Salt Channel', GRAD.sand, 'The Salt Path', '80%', [{ title: 'The Salt Path', startTime: '18:30', endTime: '21:10', isLive: true, widthWeight: 4 }, { title: 'Slow Light', startTime: '21:10', endTime: '23:00', isLive: false, widthWeight: 4 }]),
];
export const FAV_CHANNEL_NUMS = ['01', '04', '03', '05'];

export const BOOKS: Record<string, Audiobook> = Object.fromEntries(
  ([
    ['quiet', 'A History of Quiet', 'Edith Crane', 'Tom Vale', 'Memoir', '9h 12m', '46%'],
    ['nightgarden', 'The Night Garden', 'Sela Okonkwo', 'Tom Vale', 'Mystery', '8h 50m', '78%'],
    ['saltroads', 'Salt Roads', 'Idris Cole', 'Idris Cole', 'History', '7h 38m', '12%'],
    ['lighthouse', 'The Lighthouse Year', 'Edith Crane', 'Lena Park', 'Memoir', '10h 02m', '34%'],
    ['tideline', 'Tideline', 'Mara Vance', 'Tom Vale', 'Fiction', '7h 11m', '5%'],
    ['farfield', 'The Far Field', 'Mara Vance', 'Lena Park', 'Fiction', '11h 04m', '0%'],
    ['driftwood', 'Driftwood', 'Bram Ode', 'Nia Holt', 'Fiction', '6h 20m', '0%'],
    ['coldwater', 'Cold Water', 'Lune', 'Lena Park', 'Thriller', '9h 47m', '0%'],
  ] as const).map(([id, title, author, narrator, genre, len, pct]) => [
    id,
    {
      id, title, author, narrator, genre, len, progressPct: pct,
      chapters: Array.from({ length: 8 }, (_, i) => ({ number: i + 1, title: `Chapter ${i + 1}`, durationSec: (40 + i) * 60, state: i < 3 ? 'done' : i === 3 ? 'current' : 'todo' })),
    } satisfies Audiobook,
  ]),
);
