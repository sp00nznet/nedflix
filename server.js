require('dotenv').config();

const express = require('express');
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');
const session = require('express-session');
const passport = require('passport');
const GoogleStrategy = require('passport-google-oauth20').Strategy;
const GitHubStrategy = require('passport-github2').Strategy;

const app = express();

// Configuration
const PORT = process.env.PORT || 3000;
const HTTPS_PORT = process.env.HTTPS_PORT || 3443;
const NFS_MOUNT_PATH = process.env.NFS_MOUNT_PATH || '/mnt/nfs';

// Default profile pictures
const DEFAULT_AVATARS = ['cat', 'dog', 'cow', 'fox', 'owl', 'bear', 'rabbit', 'penguin'];

// In-memory user store (use a database in production)
const users = new Map();
const userSettings = new Map();

// Middleware
app.use(express.json());
app.use(express.static('public'));

// Session configuration
app.use(session({
    secret: process.env.SESSION_SECRET || 'change-this-secret-in-production',
    resave: false,
    saveUninitialized: false,
    cookie: {
        secure: true,
        httpOnly: true,
        maxAge: 24 * 60 * 60 * 1000 // 24 hours
    }
}));

// Initialize Passport
app.use(passport.initialize());
app.use(passport.session());

// Passport serialization
passport.serializeUser((user, done) => {
    done(null, user.id);
});

passport.deserializeUser((id, done) => {
    const user = users.get(id);
    done(null, user || null);
});

// Google OAuth Strategy
if (process.env.GOOGLE_CLIENT_ID && process.env.GOOGLE_CLIENT_SECRET) {
    passport.use(new GoogleStrategy({
        clientID: process.env.GOOGLE_CLIENT_ID,
        clientSecret: process.env.GOOGLE_CLIENT_SECRET,
        callbackURL: `${process.env.CALLBACK_BASE_URL || 'https://localhost:' + HTTPS_PORT}/auth/google/callback`
    }, (accessToken, refreshToken, profile, done) => {
        let user = users.get(profile.id);
        if (!user) {
            user = {
                id: profile.id,
                provider: 'google',
                displayName: profile.displayName,
                email: profile.emails?.[0]?.value,
                avatar: profile.photos?.[0]?.value || 'cat'
            };
            users.set(profile.id, user);
            userSettings.set(profile.id, getDefaultSettings());
        }
        return done(null, user);
    }));
}

// GitHub OAuth Strategy
if (process.env.GITHUB_CLIENT_ID && process.env.GITHUB_CLIENT_SECRET) {
    passport.use(new GitHubStrategy({
        clientID: process.env.GITHUB_CLIENT_ID,
        clientSecret: process.env.GITHUB_CLIENT_SECRET,
        callbackURL: `${process.env.CALLBACK_BASE_URL || 'https://localhost:' + HTTPS_PORT}/auth/github/callback`
    }, (accessToken, refreshToken, profile, done) => {
        let user = users.get(profile.id);
        if (!user) {
            user = {
                id: profile.id,
                provider: 'github',
                displayName: profile.displayName || profile.username,
                email: profile.emails?.[0]?.value,
                avatar: profile.photos?.[0]?.value || 'dog'
            };
            users.set(profile.id, user);
            userSettings.set(profile.id, getDefaultSettings());
        }
        return done(null, user);
    }));
}

// Default user settings
function getDefaultSettings() {
    return {
        streaming: {
            quality: 'auto',
            autoplay: false,
            volume: 80,
            playbackSpeed: 1.0,
            subtitles: false
        },
        profilePicture: 'cat'
    };
}

// Authentication middleware
function ensureAuthenticated(req, res, next) {
    if (req.isAuthenticated()) {
        return next();
    }
    res.status(401).json({ error: 'Not authenticated' });
}

// Auth routes
app.get('/auth/google', passport.authenticate('google', {
    scope: ['profile', 'email']
}));

app.get('/auth/google/callback',
    passport.authenticate('google', { failureRedirect: '/login.html' }),
    (req, res) => {
        res.redirect('/');
    }
);

app.get('/auth/github', passport.authenticate('github', {
    scope: ['user:email']
}));

app.get('/auth/github/callback',
    passport.authenticate('github', { failureRedirect: '/login.html' }),
    (req, res) => {
        res.redirect('/');
    }
);

app.get('/auth/logout', (req, res) => {
    req.logout((err) => {
        if (err) {
            return res.status(500).json({ error: 'Logout failed' });
        }
        res.redirect('/login.html');
    });
});

// API: Get current user
app.get('/api/user', (req, res) => {
    if (req.isAuthenticated()) {
        const settings = userSettings.get(req.user.id) || getDefaultSettings();
        res.json({
            authenticated: true,
            user: {
                ...req.user,
                profilePicture: settings.profilePicture
            },
            settings: settings
        });
    } else {
        res.json({ authenticated: false });
    }
});

// API: Get available avatars
app.get('/api/avatars', (req, res) => {
    res.json(DEFAULT_AVATARS);
});

// API: Update user settings
app.post('/api/settings', ensureAuthenticated, (req, res) => {
    const { streaming, profilePicture } = req.body;
    let settings = userSettings.get(req.user.id) || getDefaultSettings();
    
    if (streaming) {
        settings.streaming = { ...settings.streaming, ...streaming };
    }
    
    if (profilePicture && DEFAULT_AVATARS.includes(profilePicture)) {
        settings.profilePicture = profilePicture;
    }
    
    userSettings.set(req.user.id, settings);
    res.json({ success: true, settings });
});

// API: Browse directories (protected)
app.get('/api/browse', ensureAuthenticated, (req, res) => {
    const requestedPath = req.query.path || NFS_MOUNT_PATH;
    
    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(requestedPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }
    
    try {
        const items = fs.readdirSync(normalizedPath, { withFileTypes: true });
        
        const fileList = items.map(item => {
            const fullPath = path.join(normalizedPath, item.name);
            let stats;
            try {
                stats = fs.statSync(fullPath);
            } catch {
                return null;
            }
            
            return {
                name: item.name,
                path: fullPath,
                isDirectory: item.isDirectory(),
                isVideo: !item.isDirectory() && /\.(mp4|webm|ogg|avi|mkv|mov|m4v)$/i.test(item.name),
                size: stats.size
            };
        }).filter(Boolean);
        
        fileList.sort((a, b) => {
            if (a.isDirectory && !b.isDirectory) return -1;
            if (!a.isDirectory && b.isDirectory) return 1;
            return a.name.localeCompare(b.name);
        });
        
        // Calculate parent path (but don't go above mount point)
        let parentPath = path.dirname(normalizedPath);
        if (!parentPath.startsWith(NFS_MOUNT_PATH)) {
            parentPath = NFS_MOUNT_PATH;
        }
        
        res.json({
            currentPath: normalizedPath,
            parentPath: parentPath,
            canGoUp: normalizedPath !== NFS_MOUNT_PATH,
            items: fileList
        });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Stream video (protected)
app.get('/api/video', ensureAuthenticated, (req, res) => {
    const videoPath = req.query.path;
    
    if (!videoPath) {
        return res.status(400).send('Video path is required');
    }
    
    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(videoPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }
    
    try {
        const stat = fs.statSync(normalizedPath);
        const fileSize = stat.size;
        const range = req.headers.range;
        
        // Get user's quality settings
        const settings = userSettings.get(req.user.id) || getDefaultSettings();
        
        // Determine content type
        const ext = path.extname(normalizedPath).toLowerCase();
        const contentTypes = {
            '.mp4': 'video/mp4',
            '.webm': 'video/webm',
            '.ogg': 'video/ogg',
            '.m4v': 'video/mp4',
            '.mkv': 'video/x-matroska',
            '.avi': 'video/x-msvideo',
            '.mov': 'video/quicktime'
        };
        const contentType = contentTypes[ext] || 'video/mp4';
        
        if (range) {
            const parts = range.replace(/bytes=/, "").split("-");
            const start = parseInt(parts[0], 10);
            const end = parts[1] ? parseInt(parts[1], 10) : fileSize - 1;
            const chunksize = (end - start) + 1;
            const file = fs.createReadStream(normalizedPath, { start, end });
            
            const head = {
                'Content-Range': `bytes ${start}-${end}/${fileSize}`,
                'Accept-Ranges': 'bytes',
                'Content-Length': chunksize,
                'Content-Type': contentType,
            };
            
            res.writeHead(206, head);
            file.pipe(res);
        } else {
            const head = {
                'Content-Length': fileSize,
                'Content-Type': contentType,
            };
            
            res.writeHead(200, head);
            fs.createReadStream(normalizedPath).pipe(res);
        }
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Check for available OAuth providers
app.get('/api/auth-providers', (req, res) => {
    const providers = [];
    if (process.env.GOOGLE_CLIENT_ID) providers.push('google');
    if (process.env.GITHUB_CLIENT_ID) providers.push('github');
    res.json(providers);
});

// Redirect root to login if not authenticated
app.get('/', (req, res, next) => {
    if (!req.isAuthenticated()) {
        return res.redirect('/login.html');
    }
    next();
});

// Start servers
function startServer() {
    const certsPath = path.join(__dirname, 'certs');
    const keyPath = path.join(certsPath, 'server.key');
    const certPath = path.join(certsPath, 'server.cert');
    
    // Check for SSL certificates
    if (fs.existsSync(keyPath) && fs.existsSync(certPath)) {
        const httpsOptions = {
            key: fs.readFileSync(keyPath),
            cert: fs.readFileSync(certPath)
        };
        
        // HTTPS server
        https.createServer(httpsOptions, app).listen(HTTPS_PORT, () => {
            console.log(`🔒 HTTPS server running at https://localhost:${HTTPS_PORT}`);
        });
        
        // HTTP redirect server
        const redirectApp = express();
        redirectApp.all('*', (req, res) => {
            res.redirect(`https://${req.hostname}:${HTTPS_PORT}${req.url}`);
        });
        http.createServer(redirectApp).listen(PORT, () => {
            console.log(`🔀 HTTP redirect server running at http://localhost:${PORT}`);
        });
    } else {
        console.log('⚠️  SSL certificates not found. Running HTTP only (not recommended).');
        console.log('   Run "npm run generate-certs" to create self-signed certificates.');
        
        // HTTP only (not secure)
        app.listen(PORT, () => {
            console.log(`⚠️  HTTP server running at http://localhost:${PORT}`);
        });
    }
    
    console.log(`\n📁 NFS mount path: ${NFS_MOUNT_PATH}`);
    
    // Check OAuth configuration
    const providers = [];
    if (process.env.GOOGLE_CLIENT_ID) providers.push('Google');
    if (process.env.GITHUB_CLIENT_ID) providers.push('GitHub');
    
    if (providers.length > 0) {
        console.log(`🔐 OAuth providers enabled: ${providers.join(', ')}`);
    } else {
        console.log('⚠️  No OAuth providers configured. Copy .env.example to .env and configure.');
    }
}

startServer();
