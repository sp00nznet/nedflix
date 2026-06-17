/**
 * Marquee Service — backend for the surfaces the redesign added on top of the original
 * video/IPTV file-browser: resume/continue-watching, profiles, music (albums/artists/
 * tracks derived from the audio file index), and audiobooks (folders + chapters).
 *
 * Music and audiobooks are DERIVED from `file_index` (no separate scan): audio files are
 * partitioned by path — anything under an "Audiobooks" folder is a book, everything else
 * is music. Albums/artists come from the folder layout (Artist/Album/track.ext).
 *
 * Tables use only TEXT/INTEGER/REAL + composite PKs so one DDL works on both SQLite and
 * PostgreSQL via the db abstraction. Timestamps are app-set epoch seconds (no dialect fns).
 */

const path = require('path');
const crypto = require('crypto');
const db = require('./db');

const AUDIOBOOK_HINT = /audiobook/i; // path segment that marks the audiobooks library

const now = () => Math.floor(Date.now() / 1000);
const encId = (p) => Buffer.from(p, 'utf8').toString('base64url');
const decId = (id) => Buffer.from(String(id), 'base64url').toString('utf8');
const seg = (p) => p.split(/[\\/]/).filter(Boolean);
const baseName = (p) => seg(p).pop() || '';
const parentName = (p) => {
  const s = seg(p);
  return s.length >= 2 ? s[s.length - 2] : '';
};

// ---------------------------------------------------------------------------
// Schema
// ---------------------------------------------------------------------------
async function ensureTables() {
  const stmts = [
    `CREATE TABLE IF NOT EXISTS profiles (
       id TEXT PRIMARY KEY,
       user_id TEXT NOT NULL,
       name TEXT NOT NULL,
       initial TEXT,
       color TEXT DEFAULT 'var(--ac)',
       accent TEXT DEFAULT 'coral',
       created_at INTEGER
     )`,
    `CREATE TABLE IF NOT EXISTS watch_progress (
       user_id TEXT NOT NULL,
       profile_id TEXT DEFAULT '',
       file_path TEXT NOT NULL,
       title_id TEXT,
       kind TEXT DEFAULT 'video',
       position_sec REAL DEFAULT 0,
       duration_sec REAL DEFAULT 0,
       updated_at INTEGER,
       PRIMARY KEY (user_id, profile_id, file_path)
     )`,
    `CREATE TABLE IF NOT EXISTS channel_favorites (
       user_id TEXT NOT NULL,
       channel_id TEXT NOT NULL,
       PRIMARY KEY (user_id, channel_id)
     )`,
    `CREATE INDEX IF NOT EXISTS idx_progress_user ON watch_progress(user_id, updated_at)`,
    `CREATE INDEX IF NOT EXISTS idx_profiles_user ON profiles(user_id)`,
  ];
  for (const s of stmts) await db.run(s);
  console.log('🎬 Marquee tables verified');
}

// ---------------------------------------------------------------------------
// Profiles
// ---------------------------------------------------------------------------
const COLORS = ['var(--ac)', '#7fb2d9', '#8fce9b', '#c8a0c8', '#e0b07a', '#d98f8f'];

async function listProfiles(userId) {
  const rows = await db.all('SELECT * FROM profiles WHERE user_id = ? ORDER BY created_at ASC', [userId]);
  return rows.map((r) => ({ id: r.id, name: r.name, initial: r.initial, color: r.color, accent: r.accent }));
}

async function createProfile(userId, { name, accent }) {
  const id = crypto.randomUUID();
  const clean = String(name || 'Profile').trim().slice(0, 40) || 'Profile';
  const existing = await db.all('SELECT id FROM profiles WHERE user_id = ?', [userId]);
  const color = COLORS[existing.length % COLORS.length];
  await db.run(
    'INSERT INTO profiles (id, user_id, name, initial, color, accent, created_at) VALUES (?,?,?,?,?,?,?)',
    [id, userId, clean, clean[0].toUpperCase(), color, accent || 'coral', now()],
  );
  return { id, name: clean, initial: clean[0].toUpperCase(), color, accent: accent || 'coral' };
}

async function updateProfile(userId, id, { name, accent }) {
  const p = await db.get('SELECT * FROM profiles WHERE id = ? AND user_id = ?', [id, userId]);
  if (!p) return null;
  const name2 = name != null ? String(name).trim().slice(0, 40) || p.name : p.name;
  const accent2 = accent || p.accent;
  await db.run('UPDATE profiles SET name = ?, initial = ?, accent = ? WHERE id = ? AND user_id = ?', [
    name2, name2[0].toUpperCase(), accent2, id, userId,
  ]);
  return { id, name: name2, initial: name2[0].toUpperCase(), color: p.color, accent: accent2 };
}

async function deleteProfile(userId, id) {
  const r = await db.run('DELETE FROM profiles WHERE id = ? AND user_id = ?', [id, userId]);
  return (r.changes || 0) > 0;
}

// ---------------------------------------------------------------------------
// Resume / continue-watching
// ---------------------------------------------------------------------------
async function saveProgress(userId, profileId, { filePath, titleId, kind, positionSec, durationSec }) {
  const pid = profileId || '';
  await db.run(
    `INSERT INTO watch_progress (user_id, profile_id, file_path, title_id, kind, position_sec, duration_sec, updated_at)
     VALUES (?,?,?,?,?,?,?,?)
     ON CONFLICT (user_id, profile_id, file_path) DO UPDATE SET
       position_sec = ?, duration_sec = ?, title_id = ?, kind = ?, updated_at = ?`,
    [userId, pid, filePath, titleId || null, kind || 'video', positionSec, durationSec, now(),
     positionSec, durationSec, titleId || null, kind || 'video', now()],
  );
  return { ok: true };
}

async function getProgress(userId, profileId, filePath) {
  const r = await db.get('SELECT position_sec, duration_sec FROM watch_progress WHERE user_id = ? AND profile_id = ? AND file_path = ?', [userId, profileId || '', filePath]);
  return r ? { positionSec: r.position_sec, durationSec: r.duration_sec } : null;
}

// Continue watching: recent video progress that isn't (nearly) finished, enriched with
// cached metadata. Returns design-shaped items.
async function continueWatching(userId, profileId, limit = 20) {
  const rows = await db.all(
    `SELECT wp.file_path, wp.position_sec, wp.duration_sec, wp.title_id,
            fi.name, mm.clean_title, mm.year, mm.type, mm.poster_path, mm.genre, mm.runtime, mm.rating, mm.season, mm.episode
       FROM watch_progress wp
       LEFT JOIN file_index fi ON fi.path = wp.file_path
       LEFT JOIN media_metadata mm ON mm.file_path = wp.file_path
      WHERE wp.user_id = ? AND wp.profile_id = ? AND wp.kind = 'video'
        AND (wp.duration_sec = 0 OR wp.position_sec < wp.duration_sec * 0.95)
      ORDER BY wp.updated_at DESC
      LIMIT ?`,
    [userId, profileId || '', limit],
  );
  return rows.map((r) => {
    const dur = r.duration_sec || 0;
    const pos = r.position_sec || 0;
    const remainMin = dur > 0 ? Math.max(0, Math.round((dur - pos) / 60)) : null;
    const epPrefix = r.season && r.episode ? `S${r.season} E${r.episode} · ` : '';
    return {
      id: r.title_id || encId(r.file_path),
      path: r.file_path,
      title: r.clean_title || (r.name ? r.name.replace(/\.[^.]+$/, '') : 'Untitled'),
      year: r.year || undefined,
      type: r.type === 'tv' ? 'Series' : 'Film',
      genre: r.genre || '',
      runtime: r.runtime || '',
      rating: r.rating || '',
      posterUrl: r.poster_path || undefined,
      resume: { positionSec: pos, durationSec: dur },
      progress: dur > 0 ? pos / dur : 0,
      context: remainMin != null ? `${epPrefix}${remainMin} min left` : epPrefix.trim(),
    };
  });
}

// ---------------------------------------------------------------------------
// Music (derived from the audio file index)
// ---------------------------------------------------------------------------
// Audio that is NOT under an audiobooks folder.
const MUSIC_WHERE = `file_type = 'audio' AND path NOT LIKE '%Audiobook%' AND path NOT LIKE '%audiobook%'`;
const BOOK_WHERE = `file_type = 'audio' AND (path LIKE '%Audiobook%' OR path LIKE '%audiobook%')`;

async function musicAlbums() {
  const rows = await db.all(
    `SELECT parent_path, COUNT(*) AS cnt FROM file_index WHERE ${MUSIC_WHERE} GROUP BY parent_path ORDER BY parent_path ASC`,
  );
  return rows.map((r) => ({
    id: encId(r.parent_path),
    title: baseName(r.parent_path),
    artist: parentName(r.parent_path) || 'Unknown Artist',
    trackCount: parseInt(r.cnt, 10),
  }));
}

async function musicArtists() {
  const albums = await musicAlbums();
  const byArtist = new Map();
  for (const a of albums) {
    const cur = byArtist.get(a.artist) || 0;
    byArtist.set(a.artist, cur + 1);
  }
  return Array.from(byArtist.entries()).map(([name, albumCount]) => ({ id: encId('artist:' + name), name, albumCount }));
}

async function albumTracks(albumId) {
  const parent = decId(albumId);
  const rows = await db.all(
    `SELECT path, name FROM file_index WHERE parent_path = ? AND file_type = 'audio' ORDER BY name ASC`,
    [parent],
  );
  return rows.map((r) => ({
    id: encId(r.path),
    path: r.path,
    title: r.name.replace(/\.[^.]+$/, ''),
    artist: parentName(parent) || 'Unknown Artist',
    album: baseName(parent),
    durationSec: 0, // filled lazily by the player / ffprobe if needed
  }));
}

// ---------------------------------------------------------------------------
// Audiobooks (derived: each folder with audio under the audiobooks library = a book)
// ---------------------------------------------------------------------------
async function audiobooks(userId, profileId) {
  const rows = await db.all(
    `SELECT parent_path, COUNT(*) AS cnt FROM file_index WHERE ${BOOK_WHERE} GROUP BY parent_path ORDER BY parent_path ASC`,
  );
  const out = [];
  for (const r of rows) {
    const id = encId(r.parent_path);
    const prog = await getProgress(userId, profileId, r.parent_path);
    const pct = prog && prog.durationSec > 0 ? Math.round((prog.positionSec / prog.durationSec) * 100) : 0;
    out.push({
      id,
      path: r.parent_path,
      title: baseName(r.parent_path),
      author: parentName(r.parent_path) || 'Unknown Author',
      narrator: '',
      genre: '',
      len: '',
      progressPct: `${pct}%`,
      chapters: [],
    });
  }
  return out;
}

async function audiobookDetail(userId, profileId, id) {
  const parent = decId(id);
  const files = await db.all(
    `SELECT path, name, size FROM file_index WHERE parent_path = ? AND file_type = 'audio' ORDER BY name ASC`,
    [parent],
  );
  if (!files.length) return null;
  const prog = await getProgress(userId, profileId, parent);
  const pct = prog && prog.durationSec > 0 ? Math.round((prog.positionSec / prog.durationSec) * 100) : 0;
  const currentIdx = prog && prog.durationSec > 0 ? Math.floor((prog.positionSec / prog.durationSec) * files.length) : 0;
  return {
    id,
    path: parent,
    title: baseName(parent),
    author: parentName(parent) || 'Unknown Author',
    narrator: '',
    genre: '',
    len: '',
    progressPct: `${pct}%`,
    chapters: files.map((f, i) => ({
      number: i + 1,
      title: f.name.replace(/\.[^.]+$/, ''),
      durationSec: 0,
      state: i < currentIdx ? 'done' : i === currentIdx ? 'current' : 'todo',
      path: f.path,
    })),
  };
}

// ---------------------------------------------------------------------------
// Library titles (films / series grids) — grouped from the video file index
// ---------------------------------------------------------------------------
function metaToTitle(row, id, fallbackTitle) {
  return {
    id,
    path: row.path,
    title: row.clean_title || fallbackTitle,
    year: row.year || undefined,
    type: row.type === 'tv' ? 'Series' : 'Film',
    genre: row.genre || '',
    runtime: row.runtime || '',
    rating: row.rating || '',
    posterUrl: row.poster_path || undefined,
    tags: [],
    synopsis: row.plot || '',
  };
}

// kind: 'film' | 'series'. Films map one row→one title; series collapse to the show
// folder (the segment directly under the "TV Shows" library dir).
async function libraryTitles(kind) {
  const like = kind === 'series' ? '%TV Show%' : '%Movie%';
  const rows = await db.all(
    `SELECT fi.path, fi.name, fi.parent_path,
            mm.clean_title, mm.year, mm.type, mm.poster_path, mm.genre, mm.rating, mm.runtime, mm.plot
       FROM file_index fi
       LEFT JOIN media_metadata mm ON mm.file_path = fi.path
      WHERE fi.file_type = 'video' AND fi.path LIKE ?
      ORDER BY fi.name ASC
      LIMIT 2000`,
    [like],
  );
  if (kind !== 'series') {
    return rows.map((r) => metaToTitle(r, encId(r.path), (r.name || '').replace(/\.[^.]+$/, '')));
  }
  // Collapse episodes to their show folder.
  const shows = new Map();
  for (const r of rows) {
    const parts = seg(r.parent_path);
    const tvIdx = parts.findIndex((p) => /tv show/i.test(p));
    const show = tvIdx >= 0 && parts[tvIdx + 1] ? parts[tvIdx + 1] : parentName(r.path) || baseName(r.parent_path);
    if (!shows.has(show)) {
      const folder = tvIdx >= 0 ? '/' + parts.slice(0, tvIdx + 2).join('/') : r.parent_path;
      shows.set(show, { ...metaToTitle(r, encId('show:' + folder), show), title: r.clean_title || show, type: 'Series', path: folder, episodes: 0 });
    }
    shows.get(show).episodes++;
  }
  return Array.from(shows.values());
}

// ---------------------------------------------------------------------------
// Channel favorites (per user)
// ---------------------------------------------------------------------------
async function listFavorites(userId) {
  const rows = await db.all('SELECT channel_id FROM channel_favorites WHERE user_id = ?', [userId]);
  return rows.map((r) => r.channel_id);
}
async function setFavorite(userId, channelId, fav) {
  if (fav) {
    await db.run('INSERT INTO channel_favorites (user_id, channel_id) VALUES (?,?) ON CONFLICT (user_id, channel_id) DO NOTHING', [userId, channelId]);
  } else {
    await db.run('DELETE FROM channel_favorites WHERE user_id = ? AND channel_id = ?', [userId, channelId]);
  }
  return { ok: true };
}

// ---------------------------------------------------------------------------
// Routes
// ---------------------------------------------------------------------------
function registerRoutes(app, { ensureAuthenticated }) {
  const pid = (req) => req.query.profile || req.body?.profileId || '';

  // Profiles
  app.get('/api/profiles', ensureAuthenticated, async (req, res) => {
    try { res.json(await listProfiles(req.user.id)); } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.post('/api/profiles', ensureAuthenticated, async (req, res) => {
    try { res.json(await createProfile(req.user.id, req.body || {})); } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.put('/api/profiles/:id', ensureAuthenticated, async (req, res) => {
    try {
      const r = await updateProfile(req.user.id, req.params.id, req.body || {});
      if (!r) return res.status(404).json({ error: 'Not found' });
      res.json(r);
    } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.delete('/api/profiles/:id', ensureAuthenticated, async (req, res) => {
    try { res.json({ ok: await deleteProfile(req.user.id, req.params.id) }); } catch (e) { res.status(500).json({ error: e.message }); }
  });

  // Resume / progress
  app.get('/api/profiles/:pid/continue', ensureAuthenticated, async (req, res) => {
    try { res.json(await continueWatching(req.user.id, req.params.pid === 'default' ? '' : req.params.pid)); } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.put('/api/progress', ensureAuthenticated, async (req, res) => {
    try {
      const b = req.body || {};
      if (!b.filePath) return res.status(400).json({ error: 'filePath required' });
      res.json(await saveProgress(req.user.id, b.profileId || '', b));
    } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.get('/api/progress', ensureAuthenticated, async (req, res) => {
    try { res.json(await getProgress(req.user.id, pid(req), req.query.path) || {}); } catch (e) { res.status(500).json({ error: e.message }); }
  });

  // Music
  app.get('/api/music/albums', ensureAuthenticated, async (_req, res) => {
    try { res.json(await musicAlbums()); } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.get('/api/music/artists', ensureAuthenticated, async (_req, res) => {
    try { res.json(await musicArtists()); } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.get('/api/music/albums/:id/tracks', ensureAuthenticated, async (req, res) => {
    try { res.json(await albumTracks(req.params.id)); } catch (e) { res.status(500).json({ error: e.message }); }
  });

  // Audiobooks
  app.get('/api/audiobooks', ensureAuthenticated, async (req, res) => {
    try { res.json(await audiobooks(req.user.id, pid(req))); } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.get('/api/audiobooks/:id', ensureAuthenticated, async (req, res) => {
    try {
      const b = await audiobookDetail(req.user.id, pid(req), req.params.id);
      if (!b) return res.status(404).json({ error: 'Not found' });
      res.json(b);
    } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.put('/api/audiobooks/:id/progress', ensureAuthenticated, async (req, res) => {
    try {
      const b = req.body || {};
      res.json(await saveProgress(req.user.id, b.profileId || '', { filePath: decId(req.params.id), kind: 'audiobook', positionSec: b.positionSec, durationSec: b.durationSec }));
    } catch (e) { res.status(500).json({ error: e.message }); }
  });

  // Library grids
  app.get('/api/library/titles', ensureAuthenticated, async (req, res) => {
    try { res.json(await libraryTitles(req.query.type === 'series' ? 'series' : 'film')); } catch (e) { res.status(500).json({ error: e.message }); }
  });

  // Live TV favorites
  app.get('/api/livetv/favorites', ensureAuthenticated, async (req, res) => {
    try { res.json(await listFavorites(req.user.id)); } catch (e) { res.status(500).json({ error: e.message }); }
  });
  app.put('/api/livetv/channels/:id/favorite', ensureAuthenticated, async (req, res) => {
    try { res.json(await setFavorite(req.user.id, req.params.id, !!(req.body && req.body.favorite))); } catch (e) { res.status(500).json({ error: e.message }); }
  });

  console.log('🎬 Marquee routes registered');
}

module.exports = {
  ensureTables,
  registerRoutes,
  // exported for tests
  _enc: encId,
  _dec: decId,
};
