-- Nedflix Database Schema for PostgreSQL
-- This script runs automatically when the PostgreSQL container first starts

-- Users table
CREATE TABLE IF NOT EXISTS users (
    id VARCHAR(255) PRIMARY KEY,
    username VARCHAR(255) UNIQUE,
    password_hash TEXT,
    provider VARCHAR(50) NOT NULL DEFAULT 'local',
    display_name VARCHAR(255),
    email VARCHAR(255),
    avatar VARCHAR(50) DEFAULT 'cat',
    is_admin BOOLEAN DEFAULT FALSE,
    is_allowed BOOLEAN DEFAULT TRUE,
    library_access TEXT DEFAULT '["movies","tv","music","audiobooks"]',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP
);

-- User settings table
CREATE TABLE IF NOT EXISTS user_settings (
    user_id VARCHAR(255) PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    quality VARCHAR(20) DEFAULT 'auto',
    autoplay BOOLEAN DEFAULT FALSE,
    volume INTEGER DEFAULT 80,
    playback_speed DECIMAL(3,2) DEFAULT 1.0,
    subtitles BOOLEAN DEFAULT FALSE,
    audio_language VARCHAR(10) DEFAULT 'eng',
    subtitle_language VARCHAR(10) DEFAULT 'en',
    profile_picture VARCHAR(50) DEFAULT 'cat'
);

-- File index table for search
CREATE TABLE IF NOT EXISTS file_index (
    id SERIAL PRIMARY KEY,
    path TEXT UNIQUE NOT NULL,
    name TEXT NOT NULL,
    parent_path TEXT,
    file_type VARCHAR(20) NOT NULL,
    extension VARCHAR(20),
    size BIGINT DEFAULT 0,
    modified_at TIMESTAMP,
    library VARCHAR(50),
    indexed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Scan logs table
CREATE TABLE IF NOT EXISTS scan_logs (
    id SERIAL PRIMARY KEY,
    started_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP,
    status VARCHAR(20) DEFAULT 'running',
    files_found INTEGER DEFAULT 0,
    files_indexed INTEGER DEFAULT 0,
    errors INTEGER DEFAULT 0,
    error_details TEXT,
    scan_path TEXT,
    triggered_by VARCHAR(255)
);

-- Indexes for better query performance
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_provider ON users(provider);
CREATE INDEX IF NOT EXISTS idx_file_name ON file_index(name);
CREATE INDEX IF NOT EXISTS idx_file_type ON file_index(file_type);
CREATE INDEX IF NOT EXISTS idx_file_library ON file_index(library);
CREATE INDEX IF NOT EXISTS idx_file_parent ON file_index(parent_path);
CREATE INDEX IF NOT EXISTS idx_scan_status ON scan_logs(status);

-- Create default admin user (will be updated by the app if needed)
INSERT INTO users (id, provider, display_name, avatar, is_admin, is_allowed)
VALUES ('local-admin', 'local', 'Admin', 'bear', TRUE, TRUE)
ON CONFLICT (id) DO NOTHING;

INSERT INTO user_settings (user_id)
VALUES ('local-admin')
ON CONFLICT (user_id) DO NOTHING;
