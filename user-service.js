/**
 * User Service for Nedflix
 * Manages users and their settings using the database abstraction layer
 * Supports both SQLite (local) and PostgreSQL (Docker)
 */

const crypto = require('crypto');
const db = require('./db');

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
 * Initialize the user service (now just logs - db.js handles initialization)
 */
async function init() {
    console.log('👥 User service initialized');
    return true;
}

/**
 * Authenticate a user with username and password
 */
async function authenticateUser(username, password) {
    if (!username || !password) return null;

    const row = await db.get('SELECT * FROM users WHERE username = ?', [username.toLowerCase()]);
    if (!row || !row.password_hash) return null;

    if (!verifyPassword(password, row.password_hash)) return null;

    // Update last login
    if (db.isUsingPostgres()) {
        await db.run('UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = ?', [row.id]);
    } else {
        await db.run("UPDATE users SET last_login = strftime('%s', 'now') WHERE id = ?", [row.id]);
    }

    return {
        id: row.id,
        username: row.username,
        provider: row.provider,
        displayName: row.display_name,
        email: row.email,
        avatar: row.avatar,
        isAdmin: row.is_admin === 1 || row.is_admin === true,
        isAllowed: row.is_allowed === 1 || row.is_allowed === true
    };
}

/**
 * Get user by ID
 */
async function getUser(id) {
    const row = await db.get(`
        SELECT u.*, s.quality, s.autoplay, s.volume, s.playback_speed,
               s.subtitles, s.audio_language, s.subtitle_language, s.profile_picture
        FROM users u
        LEFT JOIN user_settings s ON u.id = s.user_id
        WHERE u.id = ?
    `, [id]);

    if (!row) return null;

    // Parse library access
    let libraryAccess = ['movies', 'tv', 'music', 'audiobooks'];
    try {
        if (row.library_access) {
            libraryAccess = JSON.parse(row.library_access);
        }
    } catch (e) {
        // Use default if parsing fails
    }

    // Handle boolean conversion for PostgreSQL vs SQLite
    const isAdmin = row.is_admin === 1 || row.is_admin === true;
    const isAllowed = row.is_allowed === 1 || row.is_allowed === true;
    const autoplay = row.autoplay === 1 || row.autoplay === true;
    const subtitles = row.subtitles === 1 || row.subtitles === true;

    return {
        user: {
            id: row.id,
            username: row.username,
            provider: row.provider,
            displayName: row.display_name,
            email: row.email,
            avatar: row.avatar,
            isAdmin,
            isAllowed,
            libraryAccess,
            createdAt: row.created_at,
            lastLogin: row.last_login
        },
        settings: {
            streaming: {
                quality: row.quality || 'auto',
                autoplay,
                volume: row.volume || 80,
                playbackSpeed: row.playback_speed || 1.0,
                subtitles,
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
async function getUserByUsername(username) {
    if (!username) return null;
    const row = await db.get('SELECT * FROM users WHERE username = ?', [username.toLowerCase()]);
    if (!row) return null;

    return {
        id: row.id,
        username: row.username,
        provider: row.provider,
        displayName: row.display_name,
        email: row.email,
        avatar: row.avatar,
        isAdmin: row.is_admin === 1 || row.is_admin === true,
        isAllowed: row.is_allowed === 1 || row.is_allowed === true
    };
}

/**
 * Upsert OAuth user (for Google/GitHub login)
 */
async function upsertOAuthUser(profile, provider) {
    const id = `${provider}-${profile.id}`;
    const displayName = profile.displayName || profile.username || 'User';
    const email = profile.emails?.[0]?.value || null;
    const avatar = 'cat';

    // Check if user exists
    const existing = await db.get('SELECT * FROM users WHERE id = ?', [id]);

    if (existing) {
        // Update last login
        if (db.isUsingPostgres()) {
            await db.run('UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = ?', [id]);
        } else {
            await db.run("UPDATE users SET last_login = strftime('%s', 'now') WHERE id = ?", [id]);
        }
        return {
            id: existing.id,
            provider: existing.provider,
            displayName: existing.display_name,
            email: existing.email,
            avatar: existing.avatar,
            isAdmin: existing.is_admin === 1 || existing.is_admin === true,
            isAllowed: existing.is_allowed === 1 || existing.is_allowed === true
        };
    }

    // Create new user
    if (db.isUsingPostgres()) {
        await db.run(`
            INSERT INTO users (id, provider, display_name, email, avatar, is_admin, is_allowed, created_at, last_login)
            VALUES (?, ?, ?, ?, ?, FALSE, TRUE, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
        `, [id, provider, displayName, email, avatar]);
    } else {
        await db.run(`
            INSERT INTO users (id, provider, display_name, email, avatar, is_admin, is_allowed, created_at, last_login)
            VALUES (?, ?, ?, ?, ?, 0, 1, strftime('%s', 'now'), strftime('%s', 'now'))
        `, [id, provider, displayName, email, avatar]);
    }

    await db.run('INSERT INTO user_settings (user_id) VALUES (?)', [id]);

    return {
        id,
        provider,
        displayName,
        email,
        avatar,
        isAdmin: false,
        isAllowed: true
    };
}

/**
 * Add a new user with username and password (admin only)
 */
async function addUser(username, password, displayName, isAdmin = false) {
    if (!username) throw new Error('Username is required');
    if (!password) throw new Error('Password is required');
    if (password.length < 4) throw new Error('Password must be at least 4 characters');

    const normalizedUsername = username.toLowerCase().trim();

    // Validate username format (alphanumeric, underscores, hyphens)
    if (!/^[a-z0-9_-]+$/.test(normalizedUsername)) {
        throw new Error('Username can only contain letters, numbers, underscores, and hyphens');
    }

    const existing = await db.get('SELECT id FROM users WHERE username = ?', [normalizedUsername]);
    if (existing) {
        throw new Error('Username already exists');
    }

    const userId = `user-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
    const passwordHash = hashPassword(password);

    if (db.isUsingPostgres()) {
        await db.run(`
            INSERT INTO users (id, username, password_hash, provider, display_name, is_admin, is_allowed)
            VALUES (?, ?, ?, ?, ?, ?, TRUE)
        `, [userId, normalizedUsername, passwordHash, 'local', displayName || username, isAdmin]);
    } else {
        await db.run(`
            INSERT INTO users (id, username, password_hash, provider, display_name, is_admin, is_allowed)
            VALUES (?, ?, ?, ?, ?, ?, 1)
        `, [userId, normalizedUsername, passwordHash, 'local', displayName || username, isAdmin ? 1 : 0]);
    }

    await db.run('INSERT INTO user_settings (user_id) VALUES (?)', [userId]);

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
async function updateUser(id, updates) {
    const user = await db.get('SELECT * FROM users WHERE id = ?', [id]);
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
        values.push(db.isUsingPostgres() ? updates.isAdmin : (updates.isAdmin ? 1 : 0));
    }
    if (updates.isAllowed !== undefined) {
        fields.push('is_allowed = ?');
        values.push(db.isUsingPostgres() ? updates.isAllowed : (updates.isAllowed ? 1 : 0));
    }
    if (updates.libraryAccess !== undefined && Array.isArray(updates.libraryAccess)) {
        fields.push('library_access = ?');
        values.push(JSON.stringify(updates.libraryAccess));
    }

    if (fields.length > 0) {
        values.push(id);
        await db.run(`UPDATE users SET ${fields.join(', ')} WHERE id = ?`, values);
    }

    return await getUser(id);
}

/**
 * Delete user (admin only)
 */
async function deleteUser(id) {
    if (id === 'local-admin') {
        throw new Error('Cannot delete local admin');
    }

    const user = await db.get('SELECT * FROM users WHERE id = ?', [id]);
    if (!user) throw new Error('User not found');

    await db.run('DELETE FROM user_settings WHERE user_id = ?', [id]);
    await db.run('DELETE FROM users WHERE id = ?', [id]);

    return true;
}

/**
 * Change user's own password (any user)
 */
async function changePassword(userId, currentPassword, newPassword) {
    const user = await db.get('SELECT * FROM users WHERE id = ?', [userId]);
    if (!user) {
        return { success: false, error: 'User not found' };
    }

    if (!user.password_hash) {
        return { success: false, error: 'Cannot change password for OAuth users' };
    }

    // Verify current password
    if (!verifyPassword(currentPassword, user.password_hash)) {
        return { success: false, error: 'Current password is incorrect' };
    }

    // Validate new password
    if (!newPassword || newPassword.length < 4) {
        return { success: false, error: 'New password must be at least 4 characters' };
    }

    // Update password
    const newHash = hashPassword(newPassword);
    await db.run('UPDATE users SET password_hash = ? WHERE id = ?', [newHash, userId]);

    return { success: true };
}

/**
 * Get all users (admin only)
 */
async function getAllUsers() {
    const rows = await db.all(`
        SELECT id, username, provider, display_name, email, avatar, is_admin, is_allowed, library_access, created_at, last_login
        FROM users
        ORDER BY created_at DESC
    `);

    return rows.map(row => {
        let libraryAccess = ['movies', 'tv', 'music', 'audiobooks'];
        try {
            if (row.library_access) {
                libraryAccess = JSON.parse(row.library_access);
            }
        } catch (e) {}

        return {
            id: row.id,
            username: row.username,
            provider: row.provider,
            displayName: row.display_name,
            email: row.email,
            avatar: row.avatar,
            isAdmin: row.is_admin === 1 || row.is_admin === true,
            isAllowed: row.is_allowed === 1 || row.is_allowed === true,
            libraryAccess,
            createdAt: row.created_at,
            lastLogin: row.last_login
        };
    });
}

/**
 * Update user settings
 */
async function updateSettings(userId, settings) {
    const existing = await db.get('SELECT * FROM user_settings WHERE user_id = ?', [userId]);

    if (!existing) {
        // Create settings for this user
        await db.run('INSERT INTO user_settings (user_id) VALUES (?)', [userId]);
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
            values.push(db.isUsingPostgres() ? settings.streaming.autoplay : (settings.streaming.autoplay ? 1 : 0));
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
            values.push(db.isUsingPostgres() ? settings.streaming.subtitles : (settings.streaming.subtitles ? 1 : 0));
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
        await db.run(`UPDATE user_settings SET ${updates.join(', ')} WHERE user_id = ?`, values);
    }

    const userData = await getUser(userId);
    return userData?.settings;
}

/**
 * Get user settings
 */
async function getSettings(userId) {
    const data = await getUser(userId);
    return data?.settings || null;
}

module.exports = {
    init,
    getUser,
    getUserByUsername,
    upsertOAuthUser,
    authenticateUser,
    addUser,
    updateUser,
    deleteUser,
    changePassword,
    getAllUsers,
    updateSettings,
    getSettings
};
