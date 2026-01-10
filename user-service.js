/**
 * User Service for Nedflix
 * Manages users and their settings in SQLite database
 */

const path = require('path');
const crypto = require('crypto');

// Database setup
let db = null;
const DB_PATH = path.join(__dirname, 'users.db');

/**
 * Hash a password using scrypt
 */
function hashPassword(password) {
    const salt = crypto.randomBytes(16).toString('hex');
    const hash = crypto.scryptSync(password, salt, 64).toString('hex');
    return `${salt}:${hash}`;
}

/**
 * Verify a password against a hash
 */
function verifyPassword(password, storedHash) {
    const [salt, hash] = storedHash.split(':');
    const verifyHash = crypto.scryptSync(password, salt, 64).toString('hex');
    return hash === verifyHash;
}

/**
 * Initialize the user service
 */
function init() {
    try {
        const Database = require('better-sqlite3');
        db = new Database(DB_PATH);

        // Create tables
        db.exec(`
            CREATE TABLE IF NOT EXISTS users (
                id TEXT PRIMARY KEY,
                username TEXT UNIQUE,
                password_hash TEXT,
                provider TEXT NOT NULL DEFAULT 'local',
                display_name TEXT,
                email TEXT,
                avatar TEXT DEFAULT 'cat',
                is_admin INTEGER DEFAULT 0,
                is_allowed INTEGER DEFAULT 1,
                created_at INTEGER DEFAULT (strftime('%s', 'now')),
                last_login INTEGER
            );

            CREATE TABLE IF NOT EXISTS user_settings (
                user_id TEXT PRIMARY KEY,
                quality TEXT DEFAULT 'auto',
                autoplay INTEGER DEFAULT 0,
                volume INTEGER DEFAULT 80,
                playback_speed REAL DEFAULT 1.0,
                subtitles INTEGER DEFAULT 0,
                audio_language TEXT DEFAULT 'eng',
                subtitle_language TEXT DEFAULT 'en',
                profile_picture TEXT DEFAULT 'cat',
                FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
            );

            CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
            CREATE INDEX IF NOT EXISTS idx_users_provider ON users(provider);
        `);

        // Migrate: Add username column if it doesn't exist
        try {
            db.exec('ALTER TABLE users ADD COLUMN username TEXT UNIQUE');
        } catch (e) {
            // Column already exists
        }

        // Migrate: Add password_hash column if it doesn't exist
        try {
            db.exec('ALTER TABLE users ADD COLUMN password_hash TEXT');
        } catch (e) {
            // Column already exists
        }

        // Ensure local-admin exists and is admin + allowed
        ensureAdminExists();

        console.log('👥 User database initialized');
        return true;
    } catch (error) {
        console.error('Failed to initialize user database:', error.message);
        return false;
    }
}

/**
 * Ensure the local admin user exists
 */
function ensureAdminExists() {
    const admin = db.prepare('SELECT * FROM users WHERE id = ?').get('local-admin');
    if (!admin) {
        db.prepare(`
            INSERT INTO users (id, provider, display_name, avatar, is_admin, is_allowed)
            VALUES (?, ?, ?, ?, ?, ?)
        `).run('local-admin', 'local', 'Admin', 'bear', 1, 1);

        db.prepare(`
            INSERT INTO user_settings (user_id)
            VALUES (?)
        `).run('local-admin');
    }
}

/**
 * Authenticate a user with username and password
 */
function authenticateUser(username, password) {
    if (!username || !password) return null;

    const row = db.prepare('SELECT * FROM users WHERE username = ?').get(username.toLowerCase());
    if (!row || !row.password_hash) return null;

    if (!verifyPassword(password, row.password_hash)) return null;

    // Update last login
    db.prepare('UPDATE users SET last_login = strftime(\'%s\', \'now\') WHERE id = ?').run(row.id);

    return {
        id: row.id,
        username: row.username,
        provider: row.provider,
        displayName: row.display_name,
        email: row.email,
        avatar: row.avatar,
        isAdmin: row.is_admin === 1,
        isAllowed: row.is_allowed === 1
    };
}

/**
 * Get user by ID
 */
function getUser(id) {
    const row = db.prepare(`
        SELECT u.*, s.quality, s.autoplay, s.volume, s.playback_speed,
               s.subtitles, s.audio_language, s.subtitle_language, s.profile_picture
        FROM users u
        LEFT JOIN user_settings s ON u.id = s.user_id
        WHERE u.id = ?
    `).get(id);

    if (!row) return null;

    return {
        user: {
            id: row.id,
            username: row.username,
            provider: row.provider,
            displayName: row.display_name,
            email: row.email,
            avatar: row.avatar,
            isAdmin: row.is_admin === 1,
            isAllowed: row.is_allowed === 1,
            createdAt: row.created_at,
            lastLogin: row.last_login
        },
        settings: {
            streaming: {
                quality: row.quality || 'auto',
                autoplay: row.autoplay === 1,
                volume: row.volume || 80,
                playbackSpeed: row.playback_speed || 1.0,
                subtitles: row.subtitles === 1,
                audioLanguage: row.audio_language || 'eng',
                subtitleLanguage: row.subtitle_language || 'en'
            },
            profilePicture: row.profile_picture || 'cat'
        }
    };
}

/**
 * Get user by username
 */
function getUserByUsername(username) {
    if (!username) return null;
    const row = db.prepare('SELECT * FROM users WHERE username = ?').get(username.toLowerCase());
    return row ? {
        id: row.id,
        username: row.username,
        provider: row.provider,
        displayName: row.display_name,
        email: row.email,
        avatar: row.avatar,
        isAdmin: row.is_admin === 1,
        isAllowed: row.is_allowed === 1
    } : null;
}

/**
 * Add a new user with username and password (admin only)
 */
function addUser(username, password, displayName, isAdmin = false) {
    if (!username) throw new Error('Username is required');
    if (!password) throw new Error('Password is required');
    if (password.length < 4) throw new Error('Password must be at least 4 characters');

    const normalizedUsername = username.toLowerCase().trim();

    // Validate username format (alphanumeric, underscores, hyphens)
    if (!/^[a-z0-9_-]+$/.test(normalizedUsername)) {
        throw new Error('Username can only contain letters, numbers, underscores, and hyphens');
    }

    const existing = db.prepare('SELECT id FROM users WHERE username = ?').get(normalizedUsername);
    if (existing) {
        throw new Error('Username already exists');
    }

    const userId = `user-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
    const passwordHash = hashPassword(password);

    db.prepare(`
        INSERT INTO users (id, username, password_hash, provider, display_name, is_admin, is_allowed)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    `).run(userId, normalizedUsername, passwordHash, 'local', displayName || username, isAdmin ? 1 : 0, 1);

    db.prepare('INSERT INTO user_settings (user_id) VALUES (?)').run(userId);

    return {
        id: userId,
        username: normalizedUsername,
        displayName: displayName || username,
        isAdmin,
        isAllowed: true,
        provider: 'local'
    };
}

/**
 * Update user (admin only)
 */
function updateUser(id, updates) {
    const user = db.prepare('SELECT * FROM users WHERE id = ?').get(id);
    if (!user) throw new Error('User not found');

    // Don't allow modifying the local-admin's admin status
    if (id === 'local-admin' && updates.isAdmin === false) {
        throw new Error('Cannot remove admin status from local admin');
    }

    const fields = [];
    const values = [];

    if (updates.displayName !== undefined) {
        fields.push('display_name = ?');
        values.push(updates.displayName);
    }
    if (updates.password !== undefined && updates.password.length >= 4) {
        fields.push('password_hash = ?');
        values.push(hashPassword(updates.password));
    }
    if (updates.isAdmin !== undefined) {
        fields.push('is_admin = ?');
        values.push(updates.isAdmin ? 1 : 0);
    }
    if (updates.isAllowed !== undefined) {
        fields.push('is_allowed = ?');
        values.push(updates.isAllowed ? 1 : 0);
    }

    if (fields.length > 0) {
        values.push(id);
        db.prepare(`UPDATE users SET ${fields.join(', ')} WHERE id = ?`).run(...values);
    }

    return getUser(id);
}

/**
 * Delete user (admin only)
 */
function deleteUser(id) {
    if (id === 'local-admin') {
        throw new Error('Cannot delete local admin');
    }

    const user = db.prepare('SELECT * FROM users WHERE id = ?').get(id);
    if (!user) throw new Error('User not found');

    db.prepare('DELETE FROM user_settings WHERE user_id = ?').run(id);
    db.prepare('DELETE FROM users WHERE id = ?').run(id);

    return true;
}

/**
 * Get all users (admin only)
 */
function getAllUsers() {
    const rows = db.prepare(`
        SELECT id, username, provider, display_name, email, avatar, is_admin, is_allowed, created_at, last_login
        FROM users
        ORDER BY created_at DESC
    `).all();

    return rows.map(row => ({
        id: row.id,
        username: row.username,
        provider: row.provider,
        displayName: row.display_name,
        email: row.email,
        avatar: row.avatar,
        isAdmin: row.is_admin === 1,
        isAllowed: row.is_allowed === 1,
        createdAt: row.created_at,
        lastLogin: row.last_login
    }));
}

/**
 * Update user settings
 */
function updateSettings(userId, settings) {
    const existing = db.prepare('SELECT * FROM user_settings WHERE user_id = ?').get(userId);

    if (!existing) {
        // Create settings for this user
        db.prepare('INSERT INTO user_settings (user_id) VALUES (?)').run(userId);
    }

    const updates = [];
    const values = [];

    if (settings.streaming) {
        if (settings.streaming.quality !== undefined) {
            updates.push('quality = ?');
            values.push(settings.streaming.quality);
        }
        if (settings.streaming.autoplay !== undefined) {
            updates.push('autoplay = ?');
            values.push(settings.streaming.autoplay ? 1 : 0);
        }
        if (settings.streaming.volume !== undefined) {
            updates.push('volume = ?');
            values.push(settings.streaming.volume);
        }
        if (settings.streaming.playbackSpeed !== undefined) {
            updates.push('playback_speed = ?');
            values.push(settings.streaming.playbackSpeed);
        }
        if (settings.streaming.subtitles !== undefined) {
            updates.push('subtitles = ?');
            values.push(settings.streaming.subtitles ? 1 : 0);
        }
        if (settings.streaming.audioLanguage !== undefined) {
            updates.push('audio_language = ?');
            values.push(settings.streaming.audioLanguage);
        }
        if (settings.streaming.subtitleLanguage !== undefined) {
            updates.push('subtitle_language = ?');
            values.push(settings.streaming.subtitleLanguage);
        }
    }

    if (settings.profilePicture !== undefined) {
        updates.push('profile_picture = ?');
        values.push(settings.profilePicture);
    }

    if (updates.length > 0) {
        values.push(userId);
        db.prepare(`UPDATE user_settings SET ${updates.join(', ')} WHERE user_id = ?`).run(...values);
    }

    return getUser(userId)?.settings;
}

/**
 * Get user settings
 */
function getSettings(userId) {
    const data = getUser(userId);
    return data?.settings || null;
}

module.exports = {
    init,
    getUser,
    getUserByUsername,
    authenticateUser,
    addUser,
    updateUser,
    deleteUser,
    getAllUsers,
    updateSettings,
    getSettings
};
