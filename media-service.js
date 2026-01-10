/**
 * Media Service for Nedflix
 * Handles file indexing, scanning, and search functionality
 * Uses the database abstraction layer for SQLite/PostgreSQL support
 */

const path = require('path');
const fs = require('fs');
const db = require('./db');

// Scan state
let currentScan = null;

// Supported file extensions
const VIDEO_EXTENSIONS = ['.mp4', '.mkv', '.avi', '.mov', '.wmv', '.webm', '.m4v', '.flv'];
const AUDIO_EXTENSIONS = ['.mp3', '.flac', '.wav', '.m4a', '.aac', '.ogg', '.wma', '.opus', '.aiff'];

/**
 * Initialize media service (db.js handles table creation)
 */
async function init() {
    console.log('📁 Media index service initialized');
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
    let scanId;
    if (db.isUsingPostgres()) {
        const result = await db.get(`
            INSERT INTO scan_logs (started_at, status, scan_path, triggered_by)
            VALUES (CURRENT_TIMESTAMP, 'running', ?, ?)
            RETURNING id
        `, [scanPath, triggeredBy]);
        scanId = result.id;
    } else {
        const result = await db.run(`
            INSERT INTO scan_logs (started_at, status, scan_path, triggered_by)
            VALUES (strftime('%s', 'now'), 'running', ?, ?)
        `, [scanPath, triggeredBy]);
        scanId = result.lastInsertRowid;
    }

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
            await db.run('DELETE FROM file_index WHERE path LIKE ?', [scanPath + '%']);

            // Insert files using transaction for PostgreSQL or batch for SQLite
            if (db.isUsingPostgres()) {
                await db.transaction(async (tx) => {
                    for (const file of files) {
                        const library = getLibraryFromPath(file.path, libraries);
                        await tx.run(`
                            INSERT INTO file_index (path, name, parent_path, file_type, extension, size, modified_at, library, indexed_at)
                            VALUES (?, ?, ?, ?, ?, ?, to_timestamp(?), ?, CURRENT_TIMESTAMP)
                            ON CONFLICT (path) DO UPDATE SET
                                name = EXCLUDED.name,
                                parent_path = EXCLUDED.parent_path,
                                file_type = EXCLUDED.file_type,
                                extension = EXCLUDED.extension,
                                size = EXCLUDED.size,
                                modified_at = EXCLUDED.modified_at,
                                library = EXCLUDED.library,
                                indexed_at = CURRENT_TIMESTAMP
                        `, [
                            file.path,
                            file.name,
                            file.parentPath,
                            file.fileType,
                            file.extension,
                            file.size,
                            file.modifiedAt,
                            library
                        ]);
                        currentScan.filesIndexed++;
                    }
                });
            } else {
                // SQLite batch insert
                for (const file of files) {
                    const library = getLibraryFromPath(file.path, libraries);
                    await db.run(`
                        INSERT OR REPLACE INTO file_index
                        (path, name, parent_path, file_type, extension, size, modified_at, library, indexed_at)
                        VALUES (?, ?, ?, ?, ?, ?, ?, ?, strftime('%s', 'now'))
                    `, [
                        file.path,
                        file.name,
                        file.parentPath,
                        file.fileType,
                        file.extension,
                        file.size,
                        file.modifiedAt,
                        library
                    ]);
                    currentScan.filesIndexed++;
                }
            }

            // Update scan log
            if (db.isUsingPostgres()) {
                await db.run(`
                    UPDATE scan_logs
                    SET completed_at = CURRENT_TIMESTAMP,
                        status = 'completed',
                        files_found = ?,
                        files_indexed = ?,
                        errors = ?,
                        error_details = ?
                    WHERE id = ?
                `, [
                    currentScan.filesFound,
                    currentScan.filesIndexed,
                    currentScan.errors,
                    errors.length > 0 ? JSON.stringify(errors.slice(0, 100)) : null,
                    scanId
                ]);
            } else {
                await db.run(`
                    UPDATE scan_logs
                    SET completed_at = strftime('%s', 'now'),
                        status = 'completed',
                        files_found = ?,
                        files_indexed = ?,
                        errors = ?,
                        error_details = ?
                    WHERE id = ?
                `, [
                    currentScan.filesFound,
                    currentScan.filesIndexed,
                    currentScan.errors,
                    errors.length > 0 ? JSON.stringify(errors.slice(0, 100)) : null,
                    scanId
                ]);
            }

            currentScan.status = 'completed';
            console.log(`✅ Media scan complete: ${currentScan.filesIndexed} files indexed`);

        } catch (err) {
            console.error('Scan error:', err);

            if (db.isUsingPostgres()) {
                await db.run(`
                    UPDATE scan_logs
                    SET completed_at = CURRENT_TIMESTAMP,
                        status = 'failed',
                        error_details = ?
                    WHERE id = ?
                `, [JSON.stringify({ fatal: err.message }), scanId]);
            } else {
                await db.run(`
                    UPDATE scan_logs
                    SET completed_at = strftime('%s', 'now'),
                        status = 'failed',
                        error_details = ?
                    WHERE id = ?
                `, [JSON.stringify({ fatal: err.message }), scanId]);
            }

            currentScan.status = 'failed';
        }
    });

    return { success: true, scanId };
}

/**
 * Get current scan status
 */
async function getScanStatus() {
    if (!currentScan) {
        // Check for most recent scan
        const lastScan = await db.get(`
            SELECT * FROM scan_logs ORDER BY started_at DESC LIMIT 1
        `);

        if (lastScan) {
            // Handle timestamp conversion for PostgreSQL
            const startedAt = lastScan.started_at instanceof Date
                ? Math.floor(lastScan.started_at.getTime() / 1000)
                : lastScan.started_at;
            const completedAt = lastScan.completed_at instanceof Date
                ? Math.floor(lastScan.completed_at.getTime() / 1000)
                : lastScan.completed_at;

            return {
                id: lastScan.id,
                status: lastScan.status,
                startedAt,
                completedAt,
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
async function getScanLogs(limit = 50) {
    const logs = await db.all(`
        SELECT * FROM scan_logs ORDER BY started_at DESC LIMIT ?
    `, [limit]);

    // Convert timestamps for PostgreSQL
    return logs.map(log => ({
        ...log,
        started_at: log.started_at instanceof Date
            ? Math.floor(log.started_at.getTime() / 1000)
            : log.started_at,
        completed_at: log.completed_at instanceof Date
            ? Math.floor(log.completed_at.getTime() / 1000)
            : log.completed_at
    }));
}

/**
 * Search files in index
 */
async function search(query, options = {}) {
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

    return await db.all(sql, params);
}

/**
 * Get index statistics
 */
async function getIndexStats() {
    const stats = await db.all(`
        SELECT
            file_type,
            COUNT(*) as count,
            SUM(size) as total_size
        FROM file_index
        GROUP BY file_type
    `);

    const lastScan = await db.get(`
        SELECT * FROM scan_logs WHERE status = 'completed' ORDER BY completed_at DESC LIMIT 1
    `);

    let lastScanInfo = null;
    if (lastScan) {
        const completedAt = lastScan.completed_at instanceof Date
            ? Math.floor(lastScan.completed_at.getTime() / 1000)
            : lastScan.completed_at;
        lastScanInfo = {
            completedAt,
            filesIndexed: lastScan.files_indexed
        };
    }

    return {
        byType: stats,
        lastScan: lastScanInfo,
        totalFiles: stats.reduce((sum, s) => sum + parseInt(s.count), 0)
    };
}

/**
 * Check if index exists and has data
 */
async function hasIndex() {
    const count = await db.get('SELECT COUNT(*) as count FROM file_index');
    return parseInt(count.count) > 0;
}

/**
 * Browse files from the database index
 * Returns files and folders in the specified directory path
 */
async function browse(directoryPath) {
    // Get all items where parent_path matches the directory
    const items = await db.all(`
        SELECT path, name, parent_path, file_type, extension, size, modified_at, library
        FROM file_index
        WHERE parent_path = ?
        ORDER BY
            CASE WHEN file_type = 'folder' THEN 0 ELSE 1 END,
            name ASC
    `, [directoryPath]);

    return items.map(item => ({
        name: item.name,
        path: item.path,
        isDirectory: item.file_type === 'folder',
        isVideo: item.file_type === 'video',
        isAudio: item.file_type === 'audio',
        size: item.size || 0,
        library: item.library
    }));
}

/**
 * Check if a specific path has indexed children
 */
async function hasIndexedChildren(directoryPath) {
    const count = await db.get(
        'SELECT COUNT(*) as count FROM file_index WHERE parent_path = ?',
        [directoryPath]
    );
    return parseInt(count.count) > 0;
}

module.exports = {
    init,
    startScan,
    getScanStatus,
    getScanLogs,
    search,
    getIndexStats,
    hasIndex,
    browse,
    hasIndexedChildren,
    VIDEO_EXTENSIONS,
    AUDIO_EXTENSIONS
};
