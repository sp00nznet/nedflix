#if DESKTOP_MODE
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Threading.Tasks;
using EmbedIO;
using EmbedIO.Actions;
using EmbedIO.Files;
using EmbedIO.Routing;
using EmbedIO.WebApi;
using Windows.Storage;

namespace Nedflix.Xbox.Services
{
    /// <summary>
    /// Embedded web server for Desktop mode
    /// Provides the same API as the Node.js server for compatibility
    /// </summary>
    public class EmbeddedServer : IDisposable
    {
        private WebServer _server;
        private readonly int _port;
        private readonly string _webRoot;
        private List<string> _mediaPaths = new();
        private readonly string _configPath;

        public string BaseUrl => $"http://localhost:{_port}";
        public bool IsRunning => _server?.State == WebServerState.Listening;

        public EmbeddedServer(int port = 3000)
        {
            _port = port;
            _webRoot = Path.Combine(AppContext.BaseDirectory, "WebUI");
            _configPath = Path.Combine(ApplicationData.Current.LocalFolder.Path, "nedflix-config.json");
            LoadConfig();
        }

        /// <summary>
        /// Starts the embedded web server
        /// </summary>
        public async Task StartAsync()
        {
            if (_server != null)
            {
                await StopAsync();
            }

            _server = CreateWebServer();
            await _server.RunAsync();
        }

        /// <summary>
        /// Stops the embedded web server
        /// </summary>
        public async Task StopAsync()
        {
            if (_server != null)
            {
                _server.Dispose();
                _server = null;
            }
            await Task.CompletedTask;
        }

        /// <summary>
        /// Creates and configures the web server
        /// </summary>
        private WebServer CreateWebServer()
        {
            var server = new WebServer(o => o
                .WithUrlPrefix($"http://+:{_port}/")
                .WithMode(HttpListenerMode.EmbedIO))
                .WithWebApi("/api", m => m.WithController(() => new NedflixApiController(this)))
                .WithStaticFolder("/", _webRoot, true, m => m
                    .WithContentCaching(true)
                    .WithDefaultDocument("index.html"));

            server.StateChanged += (s, e) =>
            {
                System.Diagnostics.Debug.WriteLine($"Server state: {e.NewState}");
            };

            return server;
        }

        /// <summary>
        /// Loads configuration from file
        /// </summary>
        private void LoadConfig()
        {
            try
            {
                if (File.Exists(_configPath))
                {
                    var json = File.ReadAllText(_configPath);
                    var config = JsonSerializer.Deserialize<ConfigData>(json);
                    _mediaPaths = config?.MediaPaths ?? new List<string>();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Failed to load config: {ex.Message}");
            }

            // Add default Xbox media paths if none configured
            if (_mediaPaths.Count == 0)
            {
                var videosPath = Environment.GetFolderPath(Environment.SpecialFolder.MyVideos);
                if (!string.IsNullOrEmpty(videosPath) && Directory.Exists(videosPath))
                {
                    _mediaPaths.Add(videosPath);
                }
            }
        }

        /// <summary>
        /// Saves configuration to file
        /// </summary>
        public void SaveConfig()
        {
            try
            {
                var config = new ConfigData { MediaPaths = _mediaPaths };
                var json = JsonSerializer.Serialize(config, new JsonSerializerOptions { WriteIndented = true });
                File.WriteAllText(_configPath, json);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Failed to save config: {ex.Message}");
            }
        }

        public List<string> MediaPaths => _mediaPaths;

        public void SetMediaPaths(List<string> paths)
        {
            _mediaPaths = paths ?? new List<string>();
            SaveConfig();
        }

        public void Dispose()
        {
            _server?.Dispose();
        }

        private class ConfigData
        {
            public List<string> MediaPaths { get; set; } = new();
        }
    }

    /// <summary>
    /// API Controller providing Nedflix-compatible endpoints
    /// </summary>
    public class NedflixApiController : WebApiController
    {
        private readonly EmbeddedServer _server;

        public NedflixApiController(EmbeddedServer server)
        {
            _server = server;
        }

        /// <summary>
        /// GET /api/user - Returns mock user (no auth in desktop mode)
        /// </summary>
        [Route(HttpVerbs.Get, "/user")]
        public object GetUser()
        {
            return new
            {
                authenticated = true,
                user = new
                {
                    id = "xbox-desktop-user",
                    displayName = "Xbox User",
                    provider = "local",
                    isAdmin = true
                }
            };
        }

        /// <summary>
        /// GET /api/libraries - Returns configured media paths
        /// </summary>
        [Route(HttpVerbs.Get, "/libraries")]
        public object GetLibraries()
        {
            var libraries = _server.MediaPaths
                .Where(p => Directory.Exists(p))
                .Select(p => new
                {
                    path = p,
                    name = Path.GetFileName(p) ?? p
                })
                .ToList();

            return libraries;
        }

        /// <summary>
        /// GET /api/browse - Browse directory contents
        /// </summary>
        [Route(HttpVerbs.Get, "/browse")]
        public object Browse([QueryField] string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                throw HttpException.BadRequest("Path required");
            }

            var normalizedPath = Path.GetFullPath(path);

            // Security check: Ensure path is within allowed media paths
            var isAllowed = _server.MediaPaths.Any(mp =>
                normalizedPath.StartsWith(Path.GetFullPath(mp), StringComparison.OrdinalIgnoreCase));

            if (!isAllowed)
            {
                throw HttpException.Forbidden("Access denied");
            }

            if (!Directory.Exists(normalizedPath))
            {
                throw HttpException.NotFound("Directory not found");
            }

            var items = new List<object>();
            var videoExtensions = new[] { ".mp4", ".webm", ".mkv", ".avi", ".mov", ".m4v", ".wmv" };
            var audioExtensions = new[] { ".mp3", ".m4a", ".flac", ".wav", ".aac", ".ogg", ".wma", ".opus" };

            foreach (var entry in Directory.EnumerateFileSystemEntries(normalizedPath))
            {
                try
                {
                    var name = Path.GetFileName(entry);
                    var isDir = Directory.Exists(entry);
                    var ext = Path.GetExtension(entry).ToLowerInvariant();

                    items.Add(new
                    {
                        name,
                        path = entry,
                        isDirectory = isDir,
                        isVideo = !isDir && videoExtensions.Contains(ext),
                        isAudio = !isDir && audioExtensions.Contains(ext),
                        size = isDir ? 0 : new FileInfo(entry).Length
                    });
                }
                catch
                {
                    // Skip inaccessible entries
                }
            }

            // Sort: folders first, then by name
            items = items
                .OrderByDescending(i => ((dynamic)i).isDirectory)
                .ThenBy(i => ((dynamic)i).name)
                .ToList();

            var parentPath = Path.GetDirectoryName(normalizedPath);
            var canGoUp = !string.IsNullOrEmpty(parentPath) &&
                _server.MediaPaths.Any(mp =>
                    parentPath.StartsWith(Path.GetFullPath(mp), StringComparison.OrdinalIgnoreCase));

            return new
            {
                currentPath = normalizedPath,
                parentPath = canGoUp ? parentPath : null,
                canGoUp,
                items
            };
        }

        /// <summary>
        /// GET /api/video - Stream video file
        /// </summary>
        [Route(HttpVerbs.Get, "/video")]
        public async Task StreamVideo([QueryField] string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                throw HttpException.BadRequest("Video path required");
            }

            var normalizedPath = Path.GetFullPath(path);

            // Security check
            var isAllowed = _server.MediaPaths.Any(mp =>
                normalizedPath.StartsWith(Path.GetFullPath(mp), StringComparison.OrdinalIgnoreCase));

            if (!isAllowed)
            {
                throw HttpException.Forbidden("Access denied");
            }

            if (!File.Exists(normalizedPath))
            {
                throw HttpException.NotFound("File not found");
            }

            var fileInfo = new FileInfo(normalizedPath);
            var fileSize = fileInfo.Length;
            var ext = fileInfo.Extension.ToLowerInvariant();

            var mimeTypes = new Dictionary<string, string>
            {
                { ".mp4", "video/mp4" },
                { ".webm", "video/webm" },
                { ".mkv", "video/x-matroska" },
                { ".avi", "video/x-msvideo" },
                { ".mov", "video/quicktime" },
                { ".m4v", "video/mp4" },
                { ".wmv", "video/x-ms-wmv" }
            };

            var contentType = mimeTypes.GetValueOrDefault(ext, "video/mp4");
            var rangeHeader = HttpContext.Request.Headers["Range"];

            if (!string.IsNullOrEmpty(rangeHeader))
            {
                // Handle range request for seeking
                var range = rangeHeader.Replace("bytes=", "").Split('-');
                var start = long.Parse(range[0]);
                var end = range.Length > 1 && !string.IsNullOrEmpty(range[1])
                    ? long.Parse(range[1])
                    : fileSize - 1;

                var chunkSize = end - start + 1;

                HttpContext.Response.StatusCode = 206;
                HttpContext.Response.Headers["Content-Range"] = $"bytes {start}-{end}/{fileSize}";
                HttpContext.Response.Headers["Accept-Ranges"] = "bytes";
                HttpContext.Response.ContentLength64 = chunkSize;
                HttpContext.Response.ContentType = contentType;

                using var fs = new FileStream(normalizedPath, FileMode.Open, FileAccess.Read, FileShare.Read);
                fs.Seek(start, SeekOrigin.Begin);

                var buffer = new byte[64 * 1024]; // 64KB chunks
                var remaining = chunkSize;

                while (remaining > 0)
                {
                    var toRead = (int)Math.Min(buffer.Length, remaining);
                    var read = await fs.ReadAsync(buffer, 0, toRead);
                    if (read == 0) break;

                    await HttpContext.Response.OutputStream.WriteAsync(buffer, 0, read);
                    remaining -= read;
                }
            }
            else
            {
                // Full file response
                HttpContext.Response.ContentLength64 = fileSize;
                HttpContext.Response.ContentType = contentType;
                HttpContext.Response.Headers["Accept-Ranges"] = "bytes";

                using var fs = new FileStream(normalizedPath, FileMode.Open, FileAccess.Read, FileShare.Read);
                await fs.CopyToAsync(HttpContext.Response.OutputStream);
            }
        }

        /// <summary>
        /// GET /api/audio - Stream audio file
        /// </summary>
        [Route(HttpVerbs.Get, "/audio")]
        public async Task StreamAudio([QueryField] string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                throw HttpException.BadRequest("Audio path required");
            }

            var normalizedPath = Path.GetFullPath(path);

            // Security check
            var isAllowed = _server.MediaPaths.Any(mp =>
                normalizedPath.StartsWith(Path.GetFullPath(mp), StringComparison.OrdinalIgnoreCase));

            if (!isAllowed)
            {
                throw HttpException.Forbidden("Access denied");
            }

            if (!File.Exists(normalizedPath))
            {
                throw HttpException.NotFound("File not found");
            }

            var fileInfo = new FileInfo(normalizedPath);
            var ext = fileInfo.Extension.ToLowerInvariant();

            var mimeTypes = new Dictionary<string, string>
            {
                { ".mp3", "audio/mpeg" },
                { ".m4a", "audio/mp4" },
                { ".flac", "audio/flac" },
                { ".wav", "audio/wav" },
                { ".aac", "audio/aac" },
                { ".ogg", "audio/ogg" },
                { ".wma", "audio/x-ms-wma" },
                { ".opus", "audio/opus" }
            };

            HttpContext.Response.ContentLength64 = fileInfo.Length;
            HttpContext.Response.ContentType = mimeTypes.GetValueOrDefault(ext, "audio/mpeg");

            using var fs = new FileStream(normalizedPath, FileMode.Open, FileAccess.Read, FileShare.Read);
            await fs.CopyToAsync(HttpContext.Response.OutputStream);
        }

        /// <summary>
        /// GET /api/settings - Get user settings
        /// </summary>
        [Route(HttpVerbs.Get, "/settings")]
        public object GetSettings()
        {
            return new
            {
                theme = "dark",
                streaming = new
                {
                    quality = "auto",
                    volume = 80,
                    playbackSpeed = 1,
                    autoplay = false,
                    subtitles = true
                }
            };
        }

        /// <summary>
        /// POST /api/settings - Save user settings
        /// </summary>
        [Route(HttpVerbs.Post, "/settings")]
        public object SaveSettings()
        {
            return new { success = true };
        }

        /// <summary>
        /// GET /api/media-paths - Get configured media paths (Desktop-specific)
        /// </summary>
        [Route(HttpVerbs.Get, "/media-paths")]
        public object GetMediaPaths()
        {
            return new
            {
                paths = _server.MediaPaths
            };
        }

        /// <summary>
        /// POST /api/media-paths - Set media paths (Desktop-specific)
        /// </summary>
        [Route(HttpVerbs.Post, "/media-paths")]
        public async Task<object> SetMediaPaths()
        {
            using var reader = new StreamReader(HttpContext.OpenRequestStream());
            var body = await reader.ReadToEndAsync();
            var data = JsonSerializer.Deserialize<MediaPathsData>(body);

            if (data?.Paths != null)
            {
                _server.SetMediaPaths(data.Paths);
            }

            return new
            {
                success = true,
                paths = _server.MediaPaths
            };
        }

        private class MediaPathsData
        {
            public List<string> Paths { get; set; }
        }
    }
}
#endif
