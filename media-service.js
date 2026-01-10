/**
 * Media Service for Nedflix
 * Handles file indexing, scanning, and search functionality
 */

const path = require('path');
const fs = require('fs');

// Database reference (shared with user-service)
let db = null;

// Scan state
let currentScan = null;

// Supported file extensions
const VIDEO_EXTENSIONS = ['.mp4', '.mkv', '.avi', '.mov', '.wmv', '.webm', '.m4v', '.flv'];
const AUDIO_EXTENSIONS = ['.mp3', '.flac', '.wav', '.m4a', '.aac', '.ogg', '.wma', '.opus', '.aiff'];

/**
 * Initialize media service with database connection
 */
function init(database) {
    db = database;

    // Create tables for file indexing and scan logs
    db.exec(`
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

        CREATE INDEX IF NOT EXISTS idx_file_name ON file_index(name);
        CREATE INDEX IF NOT EXISTS idx_file_type ON file_index(file_type);
        CREATE INDEX IF NOT EXISTS idx_file_library ON file_index(library);
        CREATE INDEX IF NOT EXISTS idx_file_parent ON file_index(parent_path);
        CREATE INDEX IF NOT EXISTS idx_scan_status ON scan_logs(status);
    `);

    console.log('📁 Media index database initialized');
    return true;
}

/**
 * Get file type from extension
 */
function getFileType(ext) {
    const lowerExt = ext.toLowerCase();
    if (VIDEO_EXTENSIONS.includes(lowerExt)) return 'video';
    if (AUDIO_EXTENSIONS.includes(lowerExt)) return 'audio';
    return 'folder';
}

/**
 * Determine library from path
 */
function getLibraryFromPath(filePath, libraries) {
    for (const lib of libraries) {
        if (filePath.startsWith(lib.path)) {
            return lib.name;
        }
    }
    return 'unknown';
}

/**
 * Recursively scan directory and collect files
 */
function scanDirectory(dirPath, files = [], errors = []) {
    try {
        const entries = fs.readdirSync(dirPath, { withFileTypes: true });

        for (const entry of entries) {
            const fullPath = path.join(dirPath, entry.name);

            try {
                if (entry.isDirectory()) {
                    // Add folder to index
                    files.push({
                        path: fullPath,
                        name: entry.name,
                        parentPath: dirPath,
                        fileType: 'folder',
                        extension: null,
                        size: 0,
                        modifiedAt: null
                    });
                    // Recurse into subdirectory
                    scanDirectory(fullPath, files, errors);
                } else if (entry.isFile()) {
                    const ext = path.extname(entry.name);
                    const fileType = getFileType(ext);

                    // Only index video and audio files
                    if (fileType === 'video' || fileType === 'audio') {
                        const stats = fs.statSync(fullPath);
                        files.push({
                            path: fullPath,
                            name: entry.name,
                            parentPath: dirPath,
                            fileType: fileType,
                            extension: ext.toLowerCase(),
                            size: stats.size,
                            modifiedAt: Math.floor(stats.mtime.getTime() / 1000)
                        });
                    }
                }
            } catch (err) {
                errors.push({ path: fullPath, error: err.message });
            }
        }
    } catch (err) {
        errors.push({ path: dirPath, error: err.message });
    }

    return { files, errors };
}

/**
 * Start a media scan
 */
async function startScan(scanPath, libraries, triggeredBy = 'system') {
    // Check if scan is already running
    if (currentScan && currentScan.status === 'running') {
        return { success: false, error: 'Scan already in progress', scanId: currentScan.id };
    }

    // Create scan log entry
    const result = db.prepare(`
        INSERT INTO scan_logs (started_at, status, scan_path, triggered_by)
        VALUES (strftime('%s', 'now'), 'running', ?, ?)
    `).run(scanPath, triggeredBy);

    const scanId = result.lastInsertRowid;

    currentScan = {
        id: scanId,
        status: 'running',
        filesFound: 0,
        filesIndexed: 0,
        errors: 0
    };

    // Run scan asynchronously
    setImmediate(async () => {
        try {
            console.log(`📂 Starting media scan: ${scanPath}`);

            // Scan the directory
            const { files, errors } = scanDirectory(scanPath);
            currentScan.filesFound = files.length;
            currentScan.errors = errors.length;

            // Clear existing index for this path
            db.prepare('DELETE FROM file_index WHERE path LIKE ?').run(scanPath + '%');

            // Insert files in batches
            const insertStmt = db.prepare(`
                INSERT OR REPLACE INTO file_index
                (path, name, parent_path, file_type, extension, size, modified_at, library, indexed_at)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, strftime('%s', 'now'))
            `);

            const insertMany = db.transaction((files) => {
                for (const file of files) {
                    const library = getLibraryFromPath(file.path, libraries);
                    insertStmt.run(
                        file.path,
                        file.name,
                        file.parentPath,
                        file.fileType,
                        file.extension,
                        file.size,
                        file.modifiedAt,
                        library
                    );
                    currentScan.filesIndexed++;
                }
            });

            insertMany(files);

            // Update scan log
            db.prepare(`
                UPDATE scan_logs
                SET completed_at = strftime('%s', 'now'),
                    status = 'completed',
                    files_found = ?,
                    files_indexed = ?,
                    errors = ?,
                    error_details = ?
                WHERE id = ?
            `).run(
                currentScan.filesFound,
                currentScan.filesIndexed,
                currentScan.errors,
                errors.length > 0 ? JSON.stringify(errors.slice(0, 100)) : null, // Limit error details
                scanId
            );

            currentScan.status = 'completed';
            console.log(`✅ Media scan complete: ${currentScan.filesIndexed} files indexed`);

        } catch (err) {
            console.error('Scan error:', err);

            db.prepare(`
                UPDATE scan_logs
                SET completed_at = strftime('%s', 'now'),
                    status = 'failed',
                    error_details = ?
                WHERE id = ?
            `).run(JSON.stringify({ fatal: err.message }), scanId);

            currentScan.status = 'failed';
        }
    });

    return { success: true, scanId };
}

/**
 * Get current scan status
 */
function getScanStatus() {
    if (!currentScan) {
        // Check for most recent scan
        const lastScan = db.prepare(`
            SELECT * FROM scan_logs ORDER BY started_at DESC LIMIT 1
        `).get();

        if (lastScan) {
            return {
                id: lastScan.id,
                status: lastScan.status,
                startedAt: lastScan.started_at,
                completedAt: lastScan.completed_at,
                filesFound: lastScan.files_found,
                filesIndexed: lastScan.files_indexed,
                errors: lastScan.errors
            };
        }
        return null;
    }

    return {
        id: currentScan.id,
        status: currentScan.status,
        filesFound: currentScan.filesFound,
        filesIndexed: currentScan.filesIndexed,
        errors: currentScan.errors
    };
}

/**
 * Get scan logs
 */
function getScanLogs(limit = 50) {
    return db.prepare(`
        SELECT * FROM scan_logs ORDER BY started_at DESC LIMIT ?
    `).all(limit);
}

/**
 * Search files in index
 */
function search(query, options = {}) {
    const { fileType, library, limit = 100 } = options;

    let sql = `
        SELECT * FROM file_index
        WHERE name LIKE ?
    `;
    const params = [`%${query}%`];

    if (fileType) {
        sql += ' AND file_type = ?';
        params.push(fileType);
    }

    if (library) {
        sql += ' AND library = ?';
        params.push(library);
    }

    sql += ' ORDER BY name ASC LIMIT ?';
    params.push(limit);

    return db.prepare(sql).all(...params);
}

/**
 * Get index statistics
 */
function getIndexStats() {
    const stats = db.prepare(`
        SELECT
            file_type,
            COUNT(*) as count,
            SUM(size) as total_size
        FROM file_index
        GROUP BY file_type
    `).all();

    const lastScan = db.prepare(`
        SELECT * FROM scan_logs WHERE status = 'completed' ORDER BY completed_at DESC LIMIT 1
    `).get();

    return {
        byType: stats,
        lastScan: lastScan ? {
            completedAt: lastScan.completed_at,
            filesIndexed: lastScan.files_indexed
        } : null,
        totalFiles: stats.reduce((sum, s) => sum + s.count, 0)
    };
}

/**
 * Check if index exists and has data
 */
function hasIndex() {
    const count = db.prepare('SELECT COUNT(*) as count FROM file_index').get();
    return count.count > 0;
}

module.exports = {
    init,
    startScan,
    getScanStatus,
    getScanLogs,
    search,
    getIndexStats,
    hasIndex,
    VIDEO_EXTENSIONS,
    AUDIO_EXTENSIONS
};
