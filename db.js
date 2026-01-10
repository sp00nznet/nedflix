/**
 * Database Abstraction Layer for Nedflix
 * Supports both SQLite (local development) and PostgreSQL (Docker production)
 */

const path = require('path');

let db = null;
let isPostgres = false;

/**
 * Initialize database connection
 * Automatically detects whether to use PostgreSQL or SQLite based on environment
 */
async function init() {
    // Check if PostgreSQL is configured
    const dbHost = process.env.DB_HOST;
    const dbUrl = process.env.DATABASE_URL;

    if (dbHost || dbUrl) {
        // Use PostgreSQL
        isPostgres = true;
        const { Pool } = require('pg');

        const config = dbUrl ? {
            connectionString: dbUrl,
            ssl: process.env.DB_SSL === 'true' ? { rejectUnauthorized: false } : false
        } : {
            host: process.env.DB_HOST,
            port: parseInt(process.env.DB_PORT) || 5432,
            user: process.env.DB_USER,
            password: process.env.DB_PASSWORD,
            database: process.env.DB_NAME,
        };

        db = new Pool(config);

        // Test connection
        try {
            const client = await db.connect();
            client.release();
            console.log('🐘 Connected to PostgreSQL database');
        } catch (err) {
            console.error('PostgreSQL connection error:', err.message);
            throw err;
        }
    } else {
        // Fall back to SQLite
        isPostgres = false;
        const Database = require('better-sqlite3');
        const DB_PATH = path.join(__dirname, 'users.db');
        db = new Database(DB_PATH);
        console.log('📦 Using SQLite database');

        // Create tables for SQLite
        initSqliteTables();
    }

    return db;
}

/**
 * Initialize SQLite tables (only called when using SQLite)
 */
function initSqliteTables() {
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
            library_access TEXT DEFAULT '["movies","tv","music","audiobooks"]',
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

        CREATE TABLE IF NOT EXISTS file_index (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            parent_path TEXT,
            file_type TEXT NOT NULL,
            extension TEXT,
            size INTEGER DEFAULT 0,
            modified_at INTEGER,
            library TEXT,
            indexed_at INTEGER DEFAULT (strftime('%s', 'now'))
        );

        CREATE TABLE IF NOT EXISTS scan_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            started_at INTEGER NOT NULL,
            completed_at INTEGER,
            status TEXT DEFAULT 'running',
            files_found INTEGER DEFAULT 0,
            files_indexed INTEGER DEFAULT 0,
            errors INTEGER DEFAULT 0,
            error_details TEXT,
            scan_path TEXT,
            triggered_by TEXT
        );

        CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
        CREATE INDEX IF NOT EXISTS idx_users_provider ON users(provider);
        CREATE INDEX IF NOT EXISTS idx_file_name ON file_index(name);
        CREATE INDEX IF NOT EXISTS idx_file_type ON file_index(file_type);
        CREATE INDEX IF NOT EXISTS idx_file_library ON file_index(library);
        CREATE INDEX IF NOT EXISTS idx_file_parent ON file_index(parent_path);
        CREATE INDEX IF NOT EXISTS idx_scan_status ON scan_logs(status);
    `);

    // Migrations for SQLite
    try { db.exec('ALTER TABLE users ADD COLUMN username TEXT UNIQUE'); } catch (e) {}
    try { db.exec('ALTER TABLE users ADD COLUMN password_hash TEXT'); } catch (e) {}
    try { db.exec("ALTER TABLE users ADD COLUMN library_access TEXT DEFAULT '[\"movies\",\"tv\",\"music\",\"audiobooks\"]'"); } catch (e) {}

    // Ensure admin exists
    const admin = db.prepare('SELECT * FROM users WHERE id = ?').get('local-admin');
    if (!admin) {
        db.prepare(`
            INSERT INTO users (id, provider, display_name, avatar, is_admin, is_allowed)
            VALUES (?, ?, ?, ?, ?, ?)
        `).run('local-admin', 'local', 'Admin', 'bear', 1, 1);

        db.prepare('INSERT INTO user_settings (user_id) VALUES (?)').run('local-admin');
    }
}

/**
 * Execute a query (unified interface for both databases)
 */
async function query(sql, params = []) {
    if (isPostgres) {
        // PostgreSQL uses $1, $2, etc. for placeholders
        // Convert ? to $1, $2, etc.
        let paramIndex = 0;
        const pgSql = sql.replace(/\?/g, () => `$${++paramIndex}`);
        const result = await db.query(pgSql, params);
        return result.rows;
    } else {
        // SQLite
        const stmt = db.prepare(sql);
        if (sql.trim().toUpperCase().startsWith('SELECT')) {
            return stmt.all(...params);
        } else {
            return stmt.run(...params);
        }
    }
}

/**
 * Get a single row
 */
async function get(sql, params = []) {
    if (isPostgres) {
        let paramIndex = 0;
        const pgSql = sql.replace(/\?/g, () => `$${++paramIndex}`);
        const result = await db.query(pgSql, params);
        return result.rows[0] || null;
    } else {
        return db.prepare(sql).get(...params) || null;
    }
}

/**
 * Get all rows
 */
async function all(sql, params = []) {
    if (isPostgres) {
        let paramIndex = 0;
        const pgSql = sql.replace(/\?/g, () => `$${++paramIndex}`);
        const result = await db.query(pgSql, params);
        return result.rows;
    } else {
        return db.prepare(sql).all(...params);
    }
}

/**
 * Run a statement (INSERT, UPDATE, DELETE)
 */
async function run(sql, params = []) {
    if (isPostgres) {
        let paramIndex = 0;
        const pgSql = sql.replace(/\?/g, () => `$${++paramIndex}`);
        const result = await db.query(pgSql, params);
        return {
            changes: result.rowCount,
            lastInsertRowid: result.rows[0]?.id || null
        };
    } else {
        return db.prepare(sql).run(...params);
    }
}

/**
 * Run multiple statements in a transaction
 */
async function transaction(callback) {
    if (isPostgres) {
        const client = await db.connect();
        try {
            await client.query('BEGIN');
            const result = await callback({
                query: async (sql, params) => {
                    let paramIndex = 0;
                    const pgSql = sql.replace(/\?/g, () => `$${++paramIndex}`);
                    return (await client.query(pgSql, params)).rows;
                },
                run: async (sql, params) => {
                    let paramIndex = 0;
                    const pgSql = sql.replace(/\?/g, () => `$${++paramIndex}`);
                    const res = await client.query(pgSql, params);
                    return { changes: res.rowCount };
                }
            });
            await client.query('COMMIT');
            return result;
        } catch (e) {
            await client.query('ROLLBACK');
            throw e;
        } finally {
            client.release();
        }
    } else {
        // SQLite transaction
        const sqliteTransaction = db.transaction(callback);
        return sqliteTransaction({
            query: (sql, params) => db.prepare(sql).all(...(params || [])),
            run: (sql, params) => db.prepare(sql).run(...(params || []))
        });
    }
}

/**
 * Check if using PostgreSQL
 */
function isUsingPostgres() {
    return isPostgres;
}

/**
 * Get the raw database connection (use sparingly)
 */
function getConnection() {
    return db;
}

/**
 * Close database connection
 */
async function close() {
    if (isPostgres) {
        await db.end();
    } else {
        db.close();
    }
}

module.exports = {
    init,
    query,
    get,
    all,
    run,
    transaction,
    isUsingPostgres,
    getConnection,
    close
};
