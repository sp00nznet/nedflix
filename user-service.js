/**
 * User Service for Nedflix
 * Manages users and their settings in SQLite database
 */

const path = require('path');

// Database setup
let db = null;
const DB_PATH = path.join(__dirname, 'users.db');

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
                provider TEXT NOT NULL,
                display_name TEXT,
                email TEXT,
                avatar TEXT DEFAULT 'cat',
                is_admin INTEGER DEFAULT 0,
                is_allowed INTEGER DEFAULT 0,
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

            CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
            CREATE INDEX IF NOT EXISTS idx_users_provider ON users(provider);
        `);

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
 * Get user by email (for checking if OAuth user is allowed)
 */
function getUserByEmail(email) {
    if (!email) return null;
    const row = db.prepare('SELECT * FROM users WHERE email = ?').get(email.toLowerCase());
    return row ? {
        id: row.id,
        provider: row.provider,
        displayName: row.display_name,
        email: row.email,
        avatar: row.avatar,
        isAdmin: row.is_admin === 1,
        isAllowed: row.is_allowed === 1
    } : null;
}

/**
 * Check if a user is allowed to access the system
 */
function isUserAllowed(id, email) {
    // Check by ID first
    const byId = db.prepare('SELECT is_allowed FROM users WHERE id = ?').get(id);
    if (byId) return byId.is_allowed === 1;

    // Check by email (for pre-registered users logging in via OAuth)
    if (email) {
        const byEmail = db.prepare('SELECT is_allowed FROM users WHERE email = ?').get(email.toLowerCase());
        if (byEmail) return byEmail.is_allowed === 1;
    }

    return false;
}

/**
 * Create or update user from OAuth login
 */
function upsertOAuthUser(profile, provider) {
    const id = profile.id;
    const email = profile.emails?.[0]?.value?.toLowerCase() || null;
    const displayName = profile.displayName || profile.username || 'User';
    const avatar = profile.photos?.[0]?.value || (provider === 'google' ? 'cat' : 'dog');

    // Check if user exists by ID
    let existing = db.prepare('SELECT * FROM users WHERE id = ?').get(id);

    // If not found by ID, check by email (user might have been pre-registered)
    if (!existing && email) {
        existing = db.prepare('SELECT * FROM users WHERE email = ? AND provider = ?').get(email, 'pending');
        if (existing) {
            // Update the pre-registered user with OAuth details
            db.prepare(`
                UPDATE users
                SET id = ?, provider = ?, display_name = ?, avatar = ?, last_login = strftime('%s', 'now')
                WHERE email = ? AND provider = 'pending'
            `).run(id, provider, displayName, avatar, email);

            // Update settings foreign key
            db.prepare('UPDATE user_settings SET user_id = ? WHERE user_id = ?').run(id, existing.id);

            existing = db.prepare('SELECT * FROM users WHERE id = ?').get(id);
        }
    }

    if (existing) {
        // Update last login
        db.prepare('UPDATE users SET last_login = strftime(\'%s\', \'now\') WHERE id = ?').run(id);

        return {
            id: existing.id,
            provider: existing.provider,
            displayName: existing.display_name,
            email: existing.email,
            avatar: existing.avatar,
            isAdmin: existing.is_admin === 1,
            isAllowed: existing.is_allowed === 1
        };
    }

    // User doesn't exist - they're not allowed
    return {
        id,
        provider,
        displayName,
        email,
        avatar,
        isAdmin: false,
        isAllowed: false
    };
}

/**
 * Add a new user (admin only)
 */
function addUser(email, displayName, isAdmin = false) {
    if (!email) throw new Error('Email is required');

    const existingByEmail = db.prepare('SELECT id FROM users WHERE email = ?').get(email.toLowerCase());
    if (existingByEmail) {
        throw new Error('User with this email already exists');
    }

    // Generate a temporary ID for pre-registered users
    const tempId = `pending-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;

    db.prepare(`
        INSERT INTO users (id, provider, display_name, email, is_admin, is_allowed)
        VALUES (?, ?, ?, ?, ?, ?)
    `).run(tempId, 'pending', displayName || email.split('@')[0], email.toLowerCase(), isAdmin ? 1 : 0, 1);

    db.prepare('INSERT INTO user_settings (user_id) VALUES (?)').run(tempId);

    return {
        id: tempId,
        email: email.toLowerCase(),
        displayName: displayName || email.split('@')[0],
        isAdmin,
        isAllowed: true,
        provider: 'pending'
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
    if (updates.email !== undefined) {
        fields.push('email = ?');
        values.push(updates.email?.toLowerCase() || null);
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
        SELECT id, provider, display_name, email, avatar, is_admin, is_allowed, created_at, last_login
        FROM users
        ORDER BY created_at DESC
    `).all();

    return rows.map(row => ({
        id: row.id,
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
    getUserByEmail,
    isUserAllowed,
    upsertOAuthUser,
    addUser,
    updateUser,
    deleteUser,
    getAllUsers,
    updateSettings,
    getSettings
};
