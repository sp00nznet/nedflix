/**
 * Marquee API for the standalone desktop server. Implements the same endpoints the
 * Marquee SPA expects (library titles, metadata, music, audiobooks, profiles, resume,
 * favorites) against the real local filesystem under the configured `mediaPaths` — no DB.
 * Everything is empty until media paths are configured and contain files.
 *
 * Library typing is by folder name: a media path (or an immediate child of one) named
 * like Movies/Films -> films, TV/Shows/Series -> series, Music -> music,
 * Audiobooks -> audiobooks. A plain video folder with no typed name is treated as films.
 *
 * Profiles / resume / favorites persist to marquee-desktop.json in the userData dir.
 */
const fs = require('fs');
const path = require('path');
const { execFile } = require('child_process');
const { XMLParser } = require('fast-xml-parser');
const ffprobePath = (() => { try { return require('ffprobe-static').path; } catch { return null; } })();
const xml = new XMLParser({ ignoreAttributes: false, attributeNamePrefix: '@_' });

// --- podcast helpers (RSS) ---
const stripHtml = (s) => String(s || '').replace(/<[^>]+>/g, '').replace(/\s+/g, ' ').trim();
function podcastDur(d) {
  if (d == null) return 0;
  const s = String(d);
  if (/^\d+$/.test(s)) return parseInt(s, 10);
  const parts = s.split(':').map(Number);
  return parts.reduce((acc, n) => acc * 60 + (n || 0), 0);
}
function parseFeed(content) {
  let doc;
  try { doc = xml.parse(content); } catch { return { title: '', episodes: [] }; }
  const channel = (doc && doc.rss && doc.rss.channel) || {};
  let items = channel.item || [];
  if (!Array.isArray(items)) items = [items];
  const showArt = (channel['itunes:image'] && channel['itunes:image']['@_href']) || (channel.image && channel.image.url);
  const episodes = items.slice(0, 300).map((it) => {
    const enc = Array.isArray(it.enclosure) ? it.enclosure[0] : it.enclosure;
    return {
      guid: typeof it.guid === 'object' ? it.guid['#text'] : it.guid,
      title: typeof it.title === 'object' ? it.title['#text'] : it.title,
      audioUrl: enc && enc['@_url'],
      date: it.pubDate || '',
      durationSec: podcastDur(it['itunes:duration']),
      description: stripHtml(it['itunes:summary'] || it.description || ''),
      artUrl: (it['itunes:image'] && it['itunes:image']['@_href']) || showArt,
    };
  }).filter((e) => e.audioUrl);
  return { title: channel.title || '', artUrl: showArt, episodes };
}

const VIDEO = /\.(mp4|webm|ogg|avi|mkv|mov|m4v|wmv)$/i;
const AUDIO = /\.(mp3|m4a|flac|wav|aac|ogg|wma|opus|aiff)$/i;

const encId = (p) => Buffer.from(p, 'utf8').toString('base64url');
const decId = (id) => Buffer.from(String(id), 'base64url').toString('utf8');
const seg = (p) => p.split(/[\\/]/).filter(Boolean);
const baseName = (p) => seg(p).pop() || '';
const parentName = (p) => { const s = seg(p); return s.length >= 2 ? s[s.length - 2] : ''; };
const now = () => Math.floor(Date.now() / 1000);

function cleanTitle(filename) {
  let name = filename.replace(/\.[^.]+$/, '');
  const yearM = name.match(/\b(19\d{2}|20\d{2})\b/);
  const year = yearM ? parseInt(yearM[1], 10) : undefined;
  name = name.replace(/[._]+/g, ' ').replace(/\s*[([]?(19\d{2}|20\d{2})[)\]]?.*$/, '').replace(/\b(1080p|720p|2160p|4k|x264|x265|hevc|web-?dl|bluray)\b.*$/i, '').trim();
  return { title: name || filename, year };
}

// Recursively collect files matching `re` under `dir` up to `maxDepth`.
function walk(dir, re, maxDepth, out = [], depth = 0) {
  let entries;
  try { entries = fs.readdirSync(dir, { withFileTypes: true }); } catch { return out; }
  for (const e of entries) {
    const full = path.join(dir, e.name);
    if (e.isDirectory()) { if (depth < maxDepth) walk(full, re, maxDepth, out, depth + 1); }
    else if (re.test(e.name)) out.push(full);
    if (out.length > 5000) break;
  }
  return out;
}

function immediateDirs(dir) {
  try { return fs.readdirSync(dir, { withFileTypes: true }).filter((e) => e.isDirectory()).map((e) => path.join(dir, e.name)); }
  catch { return []; }
}

const typeOf = (name) => {
  if (/audiobook/i.test(name)) return 'book';
  if (/music/i.test(name)) return 'music';
  if (/\b(tv|shows?|series)\b/i.test(name)) return 'series';
  if (/\b(movies?|films?)\b/i.test(name)) return 'film';
  return null;
};

// Resolve typed roots from configured media paths + their immediate typed children.
function typedRoots(mediaPaths) {
  const roots = { film: [], series: [], music: [], book: [] };
  for (const mp of mediaPaths) {
    if (!fs.existsSync(mp)) continue;
    const t = typeOf(baseName(mp));
    const children = immediateDirs(mp);
    const typedChildren = children.filter((c) => typeOf(baseName(c)));
    if (t) roots[t].push(mp);
    else if (typedChildren.length) { for (const c of typedChildren) { const ct = typeOf(baseName(c)); if (ct) roots[ct].push(c); } }
    else roots.film.push(mp); // plain video folder
  }
  return roots;
}

function metaTitle(p, id, title, year, type) {
  return { id, path: p, title, year, type, genre: '', runtime: '', rating: '', tags: [], synopsis: '', posterUrl: undefined };
}

// ffprobe a media file -> normalized technical info (or basic info if probe unavailable).
const humanSize = (b) => (b >= 1073741824 ? (b / 1073741824).toFixed(2) + ' GB' : (b / 1048576).toFixed(0) + ' MB');
const fmtDur = (s) => { s = Math.round(s); const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60); return (h ? h + 'h ' : '') + m + 'm'; };
const chLayout = (n) => ({ 1: 'Mono', 2: 'Stereo', 6: '5.1', 8: '7.1' }[n] || n + 'ch');

function ffprobe(file) {
  return new Promise((resolve) => {
    if (!ffprobePath) return resolve(null);
    execFile(ffprobePath, ['-v', 'quiet', '-print_format', 'json', '-show_format', '-show_streams', file], { maxBuffer: 16 * 1024 * 1024 }, (err, stdout) => {
      if (err) return resolve(null);
      try { resolve(JSON.parse(stdout)); } catch { resolve(null); }
    });
  });
}

const MEDIA_RE = /\.(mp4|webm|ogg|avi|mkv|mov|m4v|wmv|mp3|m4a|m4b|flac|wav|aac|opus|aiff)$/i;
async function fileInfo(p) {
  let stat;
  try { stat = fs.statSync(p); } catch { return null; }
  // Albums/audiobooks are folders — probe the first media file inside, but keep the name.
  let probeTarget = p;
  const folderName = path.basename(p);
  if (stat.isDirectory()) {
    let first = null;
    try { first = (fs.readdirSync(p).filter((n) => MEDIA_RE.test(n)).sort())[0]; } catch { /* ignore */ }
    if (!first) return { path: p, name: folderName, container: 'Folder', size: '', note: 'No media files' };
    probeTarget = path.join(p, first);
    stat = fs.statSync(probeTarget);
  }
  const info = { path: p, name: folderName, container: path.extname(probeTarget).replace('.', '').toUpperCase(), size: humanSize(stat.size) };
  const data = await ffprobe(probeTarget);
  if (data) {
    const streams = data.streams || [];
    const v = streams.find((s) => s.codec_type === 'video');
    const audios = streams.filter((s) => s.codec_type === 'audio');
    const subs = streams.filter((s) => s.codec_type === 'subtitle');
    const dur = parseFloat(data.format && data.format.duration) || 0;
    const br = parseInt(data.format && data.format.bit_rate, 10) || 0;
    if (v) {
      info.video = `${(v.codec_name || '').toUpperCase()}${v.bits_per_raw_sample ? ' · ' + v.bits_per_raw_sample + '-bit' : ''}`;
      if (v.width && v.height) info.resolution = `${v.width} × ${v.height}`;
      const fr = (v.avg_frame_rate || '').split('/');
      if (fr.length === 2 && +fr[1]) info.fps = (+fr[0] / +fr[1]).toFixed(0) + ' fps';
    }
    if (br) info.bitrate = (br / 1e6).toFixed(1) + ' Mb/s';
    if (dur) info.duration = fmtDur(dur);
    info.audio = audios.map((a) => `${(a.codec_name || '').toUpperCase()}${a.channels ? ' · ' + chLayout(a.channels) : ''}${a.tags && a.tags.language ? ' (' + a.tags.language + ')' : ''}`);
    info.subtitles = subs.map((s) => (s.tags && s.tags.language) || s.codec_name || 'sub');
  } else {
    info.note = ffprobePath ? 'Could not read media details' : 'ffprobe unavailable';
  }
  return info;
}

module.exports = function registerMarqueeDesktop(expressApp, { getMediaPaths, dataDir }) {
  const STORE = path.join(dataDir, 'marquee-desktop.json');
  let store = { profiles: [], progress: {}, favorites: [], tmdbKey: '', artCache: {}, mylist: [], descCache: {}, podcasts: [] };
  try { if (fs.existsSync(STORE)) store = { ...store, ...JSON.parse(fs.readFileSync(STORE, 'utf8')) }; } catch { /* ignore */ }
  const save = () => { try { fs.writeFileSync(STORE, JSON.stringify(store, null, 2)); } catch { /* ignore */ } };

  // ---- artwork: local sidecar images, then TMDB (if a key is configured) ----
  const IMG_EXT = ['.jpg', '.jpeg', '.png', '.webp'];
  const artUrl = (kind, p, title, year) =>
    `/api/artwork?kind=${kind}&path=${encodeURIComponent(p)}` + (title ? `&title=${encodeURIComponent(title)}` : '') + (year ? `&year=${year}` : '');

  function findSidecar(p) {
    let dir, base;
    try { const st = fs.statSync(p); if (st.isDirectory()) { dir = p; base = null; } else { dir = path.dirname(p); base = path.basename(p).replace(/\.[^.]+$/, ''); } } catch { return null; }
    const names = [];
    if (base) for (const e of IMG_EXT) { names.push(base + e); names.push(base + '-poster' + e); }
    for (const stem of ['poster', 'folder', 'cover', 'fanart', 'thumb', 'front']) for (const e of IMG_EXT) names.push(stem + e);
    for (const n of names) { const f = path.join(dir, n); try { if (fs.existsSync(f)) return f; } catch { /* ignore */ } }
    return null;
  }

  async function tmdbPoster(kind, title, year) {
    if (!store.tmdbKey || !title) return null;
    const type = kind === 'tv' || kind === 'series' ? 'tv' : 'movie';
    const yq = year ? (type === 'tv' ? `&first_air_date_year=${year}` : `&year=${year}`) : '';
    const url = `https://api.themoviedb.org/3/search/${type}?api_key=${encodeURIComponent(store.tmdbKey)}&query=${encodeURIComponent(title)}${yq}`;
    try {
      const r = await fetch(url);
      if (!r.ok) return null;
      const j = await r.json();
      const hit = (j.results || []).find((x) => x.poster_path);
      return hit ? `https://image.tmdb.org/t/p/w500${hit.poster_path}` : null;
    } catch { return null; }
  }

  // iTunes Search API — keyless. Great for music/audiobooks/TV; best-effort for movies
  // (Apple's movie entity filter is unreliable, so we search broadly and keep only
  // feature-movie results). Returns a 600px artwork URL or null.
  const upscale = (u) => u.replace(/\/[0-9]+x[0-9]+bb\./, '/600x600bb.');
  async function itunesArt(kind, title) {
    if (!title) return null;
    try {
      if (kind === 'movie' || kind === 'film') {
        const r = await fetch(`https://itunes.apple.com/search?term=${encodeURIComponent(title)}&limit=25`);
        if (!r.ok) return null;
        const j = await r.json();
        const movies = (j.results || []).filter((x) => x.kind === 'feature-movie' && x.artworkUrl100);
        if (!movies.length) return null;
        const norm = (s) => (s || '').toLowerCase().replace(/[^a-z0-9]+/g, ' ').trim();
        const tn = norm(title);
        const hit = movies.find((m) => norm(m.trackName) === tn) || movies.find((m) => norm(m.trackName).includes(tn)) || movies[0];
        return upscale(hit.artworkUrl100);
      }
      const entity = kind === 'tv' || kind === 'series' ? 'tvSeason' : kind === 'music' ? 'album' : kind === 'book' ? 'audiobook' : 'movie';
      const r = await fetch(`https://itunes.apple.com/search?term=${encodeURIComponent(title)}&entity=${entity}&limit=1`);
      if (!r.ok) return null;
      const j = await r.json();
      const hit = (j.results || [])[0];
      return hit && hit.artworkUrl100 ? upscale(hit.artworkUrl100) : null;
    } catch { return null; }
  }

  // Manual artwork search -> candidate list the user picks from (TMDB or iTunes).
  async function searchArtwork(provider, kind, q) {
    if (!q) return [];
    try {
      if (provider === 'tmdb') {
        if (!store.tmdbKey) return [];
        const type = kind === 'tv' || kind === 'series' ? 'tv' : 'movie';
        const r = await fetch(`https://api.themoviedb.org/3/search/${type}?api_key=${encodeURIComponent(store.tmdbKey)}&query=${encodeURIComponent(q)}`);
        if (!r.ok) return [];
        const j = await r.json();
        return (j.results || []).filter((x) => x.poster_path).slice(0, 12).map((x) => ({
          title: x.title || x.name, year: (x.release_date || x.first_air_date || '').slice(0, 4),
          thumb: `https://image.tmdb.org/t/p/w185${x.poster_path}`, url: `https://image.tmdb.org/t/p/w500${x.poster_path}`,
        }));
      }
      // iTunes
      const entity = kind === 'tv' || kind === 'series' ? 'tvSeason' : kind === 'music' ? 'album' : kind === 'book' ? 'audiobook' : 'movie';
      const isMovie = entity === 'movie';
      const r = await fetch(`https://itunes.apple.com/search?term=${encodeURIComponent(q)}&${isMovie ? '' : 'entity=' + entity + '&'}limit=12`);
      if (!r.ok) return [];
      const j = await r.json();
      let results = j.results || [];
      if (isMovie) results = results.filter((x) => x.kind === 'feature-movie');
      return results.filter((x) => x.artworkUrl100).slice(0, 12).map((x) => ({
        title: x.trackName || x.collectionName || '', year: (x.releaseDate || '').slice(0, 4),
        thumb: x.artworkUrl100, url: upscale(x.artworkUrl100),
      }));
    } catch { return []; }
  }

  // Plot/overview from whatever provider is configured (TMDB preferred, then iTunes).
  async function fetchDescription(kind, title, year) {
    if (!title) return '';
    const key = `v2|${kind}|${title}|${year || ''}`;
    if (store.descCache[key] !== undefined) return store.descCache[key];
    let desc = '';
    try {
      if (store.tmdbKey) {
        const type = kind === 'tv' || kind === 'series' ? 'tv' : 'movie';
        const yq = year ? (type === 'tv' ? `&first_air_date_year=${year}` : `&year=${year}`) : '';
        const r = await fetch(`https://api.themoviedb.org/3/search/${type}?api_key=${encodeURIComponent(store.tmdbKey)}&query=${encodeURIComponent(title)}${yq}`);
        if (r.ok) { const j = await r.json(); desc = (j.results || []).map((x) => x.overview).find(Boolean) || ''; }
      }
      if (!desc && (kind === 'movie' || kind === 'film' || kind === 'tv' || kind === 'series')) {
        // Only trust an iTunes match whose title actually matches (its movie filter is flaky).
        const r = await fetch(`https://itunes.apple.com/search?term=${encodeURIComponent(title)}&limit=15`);
        if (r.ok) {
          const j = await r.json();
          const norm = (s) => (s || '').toLowerCase().replace(/[^a-z0-9]+/g, ' ').trim();
          const tn = norm(title);
          const hit = (j.results || []).find((x) => x.kind === 'feature-movie' && x.longDescription && (norm(x.trackName) === tn || norm(x.trackName).includes(tn)));
          desc = hit ? hit.longDescription : '';
        }
      }
    } catch { /* ignore */ }
    store.descCache[key] = desc;
    save();
    return desc;
  }

  // Embedded cover art from audio tags (mp3/flac/m4a/m4b) via music-metadata (ESM, lazy).
  let mmPromise = null;
  const getMM = () => (mmPromise || (mmPromise = import('music-metadata')));
  const ART_DIR = path.join(dataDir, 'artcache');
  async function embeddedArt(folder) {
    let files = [];
    try { files = fs.readdirSync(folder).filter((n) => AUDIO.test(n)).sort(); } catch { return null; }
    if (!files.length) return null;
    try {
      const mm = await getMM();
      const meta = await mm.parseFile(path.join(folder, files[0]), { duration: false, skipCovers: false });
      const pic = meta.common && meta.common.picture && meta.common.picture[0];
      if (!pic) return null;
      if (!fs.existsSync(ART_DIR)) fs.mkdirSync(ART_DIR, { recursive: true });
      const ext = (pic.format || '').includes('png') ? '.png' : '.jpg';
      const out = path.join(ART_DIR, encId(folder).slice(0, 48) + ext);
      fs.writeFileSync(out, Buffer.from(pic.data));
      return out;
    } catch { return null; }
  }

  // ---- library grids ----
  function libraryTitles(kind) {
    const roots = typedRoots(getMediaPaths());
    if (kind === 'series') {
      const out = [];
      for (const root of roots.series) {
        for (const show of immediateDirs(root)) {
          const eps = walk(show, VIDEO, 4);
          if (!eps.length) continue;
          out.push({ ...metaTitle(eps[0], encId(eps[0]), baseName(show), undefined, 'Series'), episodes: eps.length, posterUrl: artUrl('tv', show, baseName(show)) });
        }
      }
      return out;
    }
    const out = [];
    for (const root of roots.film) {
      for (const f of walk(root, VIDEO, 4)) {
        const { title, year } = cleanTitle(baseName(f));
        out.push({ ...metaTitle(f, encId(f), title, year, 'Film'), posterUrl: artUrl('movie', f, title, year) });
      }
    }
    return out;
  }

  expressApp.get('/api/library/titles', (req, res) => {
    try { res.json(libraryTitles(req.query.type === 'series' ? 'series' : 'film')); }
    catch (e) { res.status(500).json({ error: e.message }); }
  });

  // ---- metadata (filename-derived; desktop has no metadata provider) ----
  expressApp.get('/api/metadata', (req, res) => {
    const p = req.query.path;
    if (!p) return res.status(400).json({ error: 'path required' });
    const { title, year } = cleanTitle(baseName(p));
    const isVideo = VIDEO.test(p);
    res.json({ found: false, metadata: { cleanTitle: title, year, type: isVideo ? 'movie' : 'audio', poster: isVideo ? artUrl('movie', p, title, year) : undefined, source: 'filename' } });
  });

  // ---- music ----
  function albumsUnder(roots) {
    const albums = new Map();
    for (const root of roots) for (const f of walk(root, AUDIO, 4)) {
      const dir = path.dirname(f);
      albums.set(dir, (albums.get(dir) || 0) + 1);
    }
    return albums;
  }
  expressApp.get('/api/music/albums', (req, res) => {
    const roots = typedRoots(getMediaPaths()).music;
    const albums = albumsUnder(roots);
    res.json(Array.from(albums.entries()).map(([dir, cnt]) => ({ id: encId(dir), title: baseName(dir), artist: parentName(dir) || 'Unknown Artist', trackCount: cnt, artUrl: artUrl('music', dir) })));
  });
  expressApp.get('/api/music/artists', (req, res) => {
    const roots = typedRoots(getMediaPaths()).music;
    const albums = albumsUnder(roots);
    const byArtist = new Map();
    for (const dir of albums.keys()) { const a = parentName(dir) || 'Unknown Artist'; byArtist.set(a, (byArtist.get(a) || 0) + 1); }
    res.json(Array.from(byArtist.entries()).map(([name, c]) => ({ id: encId('artist:' + name), name, albumCount: c })));
  });
  expressApp.get('/api/music/albums/:id/tracks', (req, res) => {
    const dir = decId(req.params.id);
    let files = [];
    try { files = fs.readdirSync(dir).filter((n) => AUDIO.test(n)).sort(); } catch { /* empty */ }
    res.json(files.map((n) => ({ id: encId(path.join(dir, n)), path: path.join(dir, n), title: n.replace(/\.[^.]+$/, ''), artist: parentName(dir) || 'Unknown Artist', album: baseName(dir), durationSec: 0 })));
  });

  // ---- audiobooks ----
  function bookFolders(roots) {
    const books = new Map();
    for (const root of roots) for (const f of walk(root, AUDIO, 4)) {
      const dir = path.dirname(f);
      books.set(dir, (books.get(dir) || 0) + 1);
    }
    return books;
  }
  const bookPct = (dir) => { const pr = store.progress[dir]; return pr && pr.durationSec > 0 ? Math.round((pr.positionSec / pr.durationSec) * 100) : 0; };
  expressApp.get('/api/audiobooks', (req, res) => {
    const roots = typedRoots(getMediaPaths()).book;
    const books = bookFolders(roots);
    res.json(Array.from(books.keys()).map((dir) => ({ id: encId(dir), path: dir, title: baseName(dir), author: parentName(dir) || 'Unknown Author', narrator: '', genre: '', len: '', progressPct: `${bookPct(dir)}%`, coverUrl: artUrl('book', dir), chapters: [] })));
  });
  expressApp.get('/api/audiobooks/:id', (req, res) => {
    const dir = decId(req.params.id);
    let files = [];
    try { files = fs.readdirSync(dir).filter((n) => AUDIO.test(n)).sort(); } catch { /* empty */ }
    if (!files.length) return res.status(404).json({ error: 'Not found' });
    const pr = store.progress[dir];
    const curIdx = pr && pr.durationSec > 0 ? Math.floor((pr.positionSec / pr.durationSec) * files.length) : 0;
    res.json({ id: req.params.id, path: dir, title: baseName(dir), author: parentName(dir) || 'Unknown Author', narrator: '', genre: '', len: '', progressPct: `${bookPct(dir)}%`,
      chapters: files.map((n, i) => ({ number: i + 1, title: n.replace(/\.[^.]+$/, ''), durationSec: 0, state: i < curIdx ? 'done' : i === curIdx ? 'current' : 'todo', path: path.join(dir, n) })) });
  });
  expressApp.put('/api/audiobooks/:id/progress', (req, res) => {
    const dir = decId(req.params.id);
    const b = req.body || {};
    store.progress[dir] = { positionSec: b.positionSec || 0, durationSec: b.durationSec || 0, kind: 'audiobook', titleId: req.params.id, updatedAt: now() };
    save();
    res.json({ ok: true });
  });

  // ---- resume / continue ----
  expressApp.put('/api/progress', (req, res) => {
    const b = req.body || {};
    if (!b.filePath) return res.status(400).json({ error: 'filePath required' });
    store.progress[b.filePath] = { positionSec: b.positionSec || 0, durationSec: b.durationSec || 0, kind: b.kind || 'video', titleId: b.titleId, updatedAt: now() };
    save();
    res.json({ ok: true });
  });
  expressApp.get('/api/progress', (req, res) => {
    const pr = store.progress[req.query.path];
    res.json(pr ? { positionSec: pr.positionSec, durationSec: pr.durationSec } : {});
  });
  expressApp.get('/api/profiles/:pid/continue', (req, res) => {
    const items = Object.entries(store.progress)
      .filter(([, pr]) => pr.kind === 'video' && (pr.durationSec === 0 || pr.positionSec < pr.durationSec * 0.95))
      .sort((a, b) => (b[1].updatedAt || 0) - (a[1].updatedAt || 0))
      .map(([p, pr]) => {
        const { title, year } = cleanTitle(baseName(p));
        const remain = pr.durationSec > 0 ? Math.max(0, Math.round((pr.durationSec - pr.positionSec) / 60)) : null;
        return { id: pr.titleId || encId(p), path: p, title, year, type: 'Film', genre: '', runtime: '', rating: '', resume: { positionSec: pr.positionSec, durationSec: pr.durationSec }, progress: pr.durationSec > 0 ? pr.positionSec / pr.durationSec : 0, context: remain != null ? `${remain} min left` : '' };
      });
    res.json(items);
  });

  // ---- profiles ----
  const COLORS = ['var(--ac)', '#7fb2d9', '#8fce9b', '#c8a0c8', '#e0b07a'];
  expressApp.get('/api/profiles', (req, res) => res.json(store.profiles));
  expressApp.post('/api/profiles', (req, res) => {
    const b = req.body || {};
    const name = String(b.name || 'Profile').trim().slice(0, 40) || 'Profile';
    const p = { id: 'p' + now() + Math.floor(Math.random() * 1000), name, initial: name[0].toUpperCase(), color: COLORS[store.profiles.length % COLORS.length], accent: b.accent || 'coral' };
    store.profiles.push(p); save(); res.json(p);
  });
  expressApp.put('/api/profiles/:id', (req, res) => {
    const p = store.profiles.find((x) => x.id === req.params.id);
    if (!p) return res.status(404).json({ error: 'Not found' });
    const b = req.body || {};
    if (b.name) { p.name = String(b.name).trim().slice(0, 40) || p.name; p.initial = p.name[0].toUpperCase(); }
    if (b.accent) p.accent = b.accent;
    save(); res.json(p);
  });
  expressApp.delete('/api/profiles/:id', (req, res) => {
    store.profiles = store.profiles.filter((x) => x.id !== req.params.id); save(); res.json({ ok: true });
  });

  // ---- search (filename match across media paths) ----
  const MEDIA = /\.(mp4|webm|ogg|avi|mkv|mov|m4v|wmv|mp3|m4a|flac|wav|aac|wma|opus|aiff)$/i;
  expressApp.get('/api/search', (req, res) => {
    const q = String(req.query.q || '').toLowerCase();
    if (q.length < 2) return res.json({ query: q, results: [] });
    const results = [];
    for (const root of getMediaPaths()) {
      for (const f of walk(root, MEDIA, 5)) {
        if (path.basename(f).toLowerCase().includes(q)) results.push({ path: f, name: path.basename(f), library: baseName(root) });
        if (results.length >= 200) break;
      }
      if (results.length >= 200) break;
    }
    res.json({ query: q, count: results.length, results });
  });

  // ---- artwork ----
  expressApp.get('/api/artwork/config', (req, res) => res.json({ hasKey: !!store.tmdbKey }));
  expressApp.put('/api/artwork/config', (req, res) => {
    store.tmdbKey = (req.body && req.body.tmdbKey) || '';
    store.artCache = {}; // re-resolve everything with the new key
    save();
    res.json({ hasKey: !!store.tmdbKey });
  });
  expressApp.get('/api/artwork', async (req, res) => {
    const p = req.query.path;
    if (!p) return res.status(404).end();
    const kind = req.query.kind || 'movie';
    const cached = store.artCache[p];
    if (cached === 'none') return res.status(404).end();
    if (cached) {
      if (/^https?:/.test(cached)) return res.redirect(cached);
      if (fs.existsSync(cached)) return res.sendFile(cached);
    }
    const isVideo = ['movie', 'film', 'tv', 'series'].includes(kind);
    const title = req.query.title || baseName(p);

    // 1) local sidecar image next to the media / in the folder
    const side = findSidecar(p);
    if (side) { store.artCache[p] = side; save(); return res.sendFile(side); }

    // 2) embedded cover art (music + audiobooks)
    if (!isVideo) {
      const emb = await embeddedArt(p);
      if (emb) { store.artCache[p] = emb; save(); return res.sendFile(emb); }
    }

    // 3) TMDB for video if a key is configured (higher quality than iTunes)
    if (isVideo && store.tmdbKey) {
      const poster = await tmdbPoster(kind, req.query.title, req.query.year);
      if (poster) { store.artCache[p] = poster; save(); return res.redirect(poster); }
    }

    // 4) iTunes Search (keyless) — movies / TV / music / audiobooks
    const it = await itunesArt(kind, title);
    if (it) { store.artCache[p] = it; save(); return res.redirect(it); }

    store.artCache[p] = 'none'; save();
    res.status(404).end();
  });

  // ---- file info (technical details) ----
  expressApp.get('/api/fileinfo', async (req, res) => {
    const p = req.query.path;
    if (!p) return res.status(400).json({ error: 'path required' });
    const info = await fileInfo(p);
    if (!info) return res.status(404).json({ error: 'not found' });
    res.json(info);
  });

  // ---- artwork management ----
  expressApp.post('/api/artwork/refresh', (req, res) => {
    const p = req.body && req.body.path;
    if (p) { delete store.artCache[p]; save(); }
    res.json({ ok: true });
  });
  expressApp.get('/api/artwork/search', async (req, res) => {
    res.json(await searchArtwork(req.query.provider || 'itunes', req.query.kind || 'movie', req.query.q || ''));
  });
  expressApp.put('/api/artwork/override', (req, res) => {
    const b = req.body || {};
    if (b.path && b.url) { store.artCache[b.path] = b.url; save(); }
    res.json({ ok: true });
  });

  // ---- description (plot/overview) ----
  expressApp.get('/api/description', async (req, res) => {
    res.json({ description: await fetchDescription(req.query.kind || 'movie', req.query.title || '', req.query.year) });
  });

  // ---- My List (watchlist, per install) ----
  expressApp.get('/api/mylist', (req, res) => res.json(store.mylist));
  expressApp.post('/api/mylist', (req, res) => {
    const it = req.body || {};
    if (it.id && !store.mylist.some((x) => x.id === it.id)) {
      store.mylist.unshift({ id: it.id, path: it.path, kind: it.kind, title: it.title, subtitle: it.subtitle, artUrl: it.artUrl });
      save();
    }
    res.json(store.mylist);
  });
  expressApp.delete('/api/mylist', (req, res) => {
    store.mylist = store.mylist.filter((x) => x.id !== req.query.id);
    save();
    res.json(store.mylist);
  });

  // ---- Built-in local channels (standalone "your TV" — no external server) ----
  const hhmm = (ms) => { const d = new Date(ms); return `${String(d.getHours()).padStart(2, '0')}:${String(d.getMinutes()).padStart(2, '0')}`; };
  expressApp.get('/api/channels/local-guide', (req, res) => {
    const films = libraryTitles('film');
    if (!films.length) return res.json([]);
    const SLOT = 90 * 60 * 1000; // 90-minute slots
    const now = Date.now();
    const base = Math.floor(now / SLOT);
    const defs = [
      { num: '01', name: 'Marquee Movies', off: 0 },
      { num: '02', name: 'Late Night', off: 7 },
      { num: '03', name: 'Random Reels', off: 13 },
      { num: '04', name: 'The Marathon', off: 23 },
    ];
    const guide = defs.map((ch) => {
      const programs = [];
      for (let i = 0; i < 5; i++) {
        const slot = base + i;
        const film = films[(slot + ch.off) % films.length];
        programs.push({ title: film.title, startTime: hhmm(slot * SLOT), endTime: hhmm((slot + 1) * SLOT), isLive: i === 0, widthWeight: 90, titleId: film.id });
      }
      const livePct = Math.round(((now - base * SLOT) / SLOT) * 100);
      return { id: 'local-' + ch.num, num: ch.num, name: ch.name, favorite: false, nowPlaying: { title: programs[0].title, progressPct: livePct + '%' }, programs };
    });
    res.json(guide);
  });

  // ---- podcasts ----
  expressApp.get('/api/podcasts/search', async (req, res) => {
    const q = req.query.q;
    if (!q) return res.json([]);
    try {
      const r = await fetch(`https://itunes.apple.com/search?term=${encodeURIComponent(q)}&entity=podcast&limit=24`);
      const j = await r.json();
      res.json((j.results || []).filter((x) => x.feedUrl).map((x) => ({
        id: String(x.collectionId), title: x.collectionName, author: x.artistName, feedUrl: x.feedUrl,
        artUrl: x.artworkUrl600 || x.artworkUrl100, episodeCount: x.trackCount,
      })));
    } catch (e) { res.status(500).json({ error: e.message }); }
  });
  expressApp.get('/api/podcasts', (req, res) => res.json(store.podcasts));
  expressApp.post('/api/podcasts/subscribe', (req, res) => {
    const p = req.body || {};
    if (p.feedUrl && !store.podcasts.some((x) => x.feedUrl === p.feedUrl)) {
      store.podcasts.unshift({ id: p.id || p.feedUrl, feedUrl: p.feedUrl, title: p.title, author: p.author, artUrl: p.artUrl });
      save();
    }
    res.json(store.podcasts);
  });
  expressApp.delete('/api/podcasts', (req, res) => {
    store.podcasts = store.podcasts.filter((x) => x.feedUrl !== req.query.feedUrl);
    save();
    res.json(store.podcasts);
  });
  expressApp.get('/api/podcasts/episodes', async (req, res) => {
    const feedUrl = req.query.feedUrl;
    if (!feedUrl) return res.status(400).json({ error: 'feedUrl required' });
    try {
      const r = await fetch(feedUrl, { redirect: 'follow' });
      if (!r.ok) return res.status(502).json({ error: 'feed fetch failed' });
      const text = await r.text();
      const feed = parseFeed(text);
      // attach resume progress per episode (keyed by audioUrl)
      feed.episodes = feed.episodes.map((e) => {
        const pr = store.progress[e.audioUrl];
        return { ...e, resume: pr ? { positionSec: pr.positionSec, durationSec: pr.durationSec } : undefined };
      });
      res.json(feed);
    } catch (e) { res.status(500).json({ error: e.message }); }
  });

  // ---- live tv favorites ----
  expressApp.get('/api/livetv/favorites', (req, res) => res.json(store.favorites));
  expressApp.put('/api/livetv/channels/:id/favorite', (req, res) => {
    const fav = !!(req.body && req.body.favorite);
    store.favorites = store.favorites.filter((c) => c !== req.params.id);
    if (fav) store.favorites.push(req.params.id);
    save(); res.json({ ok: true });
  });

  // ---- video-transcode alias (desktop has no ffmpeg; stream directly with range) ----
  // The SPA player requests /api/video-transcode; map it to a ranged direct stream.
  const MIME = { '.mp4': 'video/mp4', '.webm': 'video/webm', '.mkv': 'video/x-matroska', '.avi': 'video/x-msvideo', '.mov': 'video/quicktime', '.m4v': 'video/mp4', '.wmv': 'video/x-ms-wmv' };
  expressApp.get('/api/video-transcode', (req, res) => {
    const p = req.query.path && path.normalize(req.query.path);
    if (!p) return res.status(400).send('path required');
    if (!getMediaPaths().some((mp) => p.startsWith(path.normalize(mp)))) return res.status(403).send('Access denied');
    let stat;
    try { stat = fs.statSync(p); } catch { return res.status(404).send('Not found'); }
    const type = MIME[path.extname(p).toLowerCase()] || 'video/mp4';
    const range = req.headers.range;
    if (range) {
      const [s, e] = range.replace(/bytes=/, '').split('-');
      const start = parseInt(s, 10);
      const end = e ? parseInt(e, 10) : stat.size - 1;
      res.writeHead(206, { 'Content-Range': `bytes ${start}-${end}/${stat.size}`, 'Accept-Ranges': 'bytes', 'Content-Length': end - start + 1, 'Content-Type': type });
      fs.createReadStream(p, { start, end }).pipe(res);
    } else {
      res.writeHead(200, { 'Content-Length': stat.size, 'Content-Type': type });
      fs.createReadStream(p).pipe(res);
    }
  });

  console.log('🎬 Marquee desktop endpoints registered');
};
