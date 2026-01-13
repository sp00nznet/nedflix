using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.Web.WebView2.Core;
using System;
using System.Threading.Tasks;
using Windows.Gaming.Input;
using Windows.Storage;
using Windows.System;

namespace Nedflix.Xbox
{
    /// <summary>
    /// Main window hosting the Nedflix WebView2 content
    /// Optimized for Xbox Series S/X with gamepad navigation
    /// </summary>
    public sealed partial class MainWindow : Window
    {
        private const string DEFAULT_SERVER_URL = "http://localhost:3000";
        private const string SETTINGS_KEY_SERVER_URL = "nedflix_server_url";

        private string serverUrl;
        private bool isXbox = false;
        private DispatcherTimer gamepadTimer;
        private Gamepad currentGamepad;

        public MainWindow()
        {
            this.InitializeComponent();

            // Detect Xbox device
            isXbox = DetectXbox();

            // Load saved settings
            LoadSettings();

            // Initialize WebView2
            InitializeWebViewAsync();

            // Setup gamepad monitoring for Xbox
            if (isXbox)
            {
                SetupGamepadSupport();
                ControllerHints.Visibility = Visibility.Visible;
            }

            // Handle keyboard for PC testing
            this.Content.KeyDown += MainWindow_KeyDown;
        }

        /// <summary>
        /// Detects if running on Xbox console
        /// </summary>
        private bool DetectXbox()
        {
            try
            {
                var deviceFamily = Windows.System.Profile.AnalyticsInfo.VersionInfo.DeviceFamily;
                return deviceFamily.Equals("Windows.Xbox", StringComparison.OrdinalIgnoreCase);
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// Loads saved application settings
        /// </summary>
        private void LoadSettings()
        {
            try
            {
                var localSettings = ApplicationData.Current.LocalSettings;
                serverUrl = localSettings.Values[SETTINGS_KEY_SERVER_URL] as string ?? DEFAULT_SERVER_URL;
                ServerUrlInput.Text = serverUrl;
            }
            catch
            {
                serverUrl = DEFAULT_SERVER_URL;
            }
        }

        /// <summary>
        /// Saves application settings
        /// </summary>
        private void SaveSettings()
        {
            try
            {
                var localSettings = ApplicationData.Current.LocalSettings;
                localSettings.Values[SETTINGS_KEY_SERVER_URL] = serverUrl;
            }
            catch
            {
                // Settings save failed, continue anyway
            }
        }

        /// <summary>
        /// Initializes WebView2 and navigates to Nedflix server
        /// </summary>
        private async void InitializeWebViewAsync()
        {
            try
            {
                LoadingText.Text = "Initializing...";

                // Ensure WebView2 is initialized
                await NedflixWebView.EnsureCoreWebView2Async();

                // Configure WebView2 settings for media playback
                var settings = NedflixWebView.CoreWebView2.Settings;
                settings.IsScriptEnabled = true;
                settings.AreDefaultScriptDialogsEnabled = true;
                settings.IsWebMessageEnabled = true;
                settings.AreHostObjectsAllowed = true;
                settings.IsStatusBarEnabled = false;
                settings.AreDefaultContextMenusEnabled = false;

                // Enable media autoplay (important for video streaming)
                settings.IsGeneralAutofillEnabled = false;
                settings.IsPasswordAutosaveEnabled = false;

                // Handle navigation events
                NedflixWebView.CoreWebView2.NavigationStarting += WebView_NavigationStarting;
                NedflixWebView.CoreWebView2.NavigationCompleted += WebView_NavigationCompleted;
                NedflixWebView.CoreWebView2.WebMessageReceived += WebView_WebMessageReceived;

                // Inject Xbox-specific JavaScript
                await InjectXboxScripts();

                // Navigate to Nedflix server
                LoadingText.Text = "Connecting to server...";
                NedflixWebView.CoreWebView2.Navigate(serverUrl);
            }
            catch (Exception ex)
            {
                ShowError($"Failed to initialize: {ex.Message}");
            }
        }

        /// <summary>
        /// Injects Xbox-specific JavaScript for enhanced gamepad support
        /// </summary>
        private async Task InjectXboxScripts()
        {
            string xboxScript = @"
                // Xbox-specific enhancements for Nedflix
                window.isXboxApp = true;

                // Enhanced focus management for gamepad navigation
                document.addEventListener('DOMContentLoaded', function() {
                    // Add xbox-app class for CSS targeting
                    document.body.classList.add('xbox-app');

                    // Ensure focusable elements are properly marked
                    const focusableSelectors = 'a, button, input, select, [tabindex]:not([tabindex=\"-1\"]), .media-card, .library-item';
                    document.querySelectorAll(focusableSelectors).forEach(el => {
                        if (!el.hasAttribute('tabindex')) {
                            el.setAttribute('tabindex', '0');
                        }
                    });
                });

                // Expose method to receive messages from C# host
                window.xboxBridge = {
                    navigate: function(direction) {
                        const event = new KeyboardEvent('keydown', {
                            key: direction === 'up' ? 'ArrowUp' :
                                 direction === 'down' ? 'ArrowDown' :
                                 direction === 'left' ? 'ArrowLeft' : 'ArrowRight',
                            bubbles: true
                        });
                        document.activeElement.dispatchEvent(event);
                    },
                    select: function() {
                        if (document.activeElement) {
                            document.activeElement.click();
                        }
                    },
                    back: function() {
                        window.history.back();
                    },
                    togglePlayPause: function() {
                        const video = document.querySelector('video');
                        if (video) {
                            if (video.paused) video.play();
                            else video.pause();
                        }
                    },
                    seek: function(seconds) {
                        const video = document.querySelector('video');
                        if (video) {
                            video.currentTime += seconds;
                        }
                    },
                    adjustVolume: function(delta) {
                        const video = document.querySelector('video');
                        if (video) {
                            video.volume = Math.max(0, Math.min(1, video.volume + delta));
                        }
                    }
                };

                console.log('Xbox bridge initialized');
            ";

            await NedflixWebView.CoreWebView2.AddScriptToExecuteOnDocumentCreatedAsync(xboxScript);
        }

        /// <summary>
        /// Handles WebView2 navigation starting
        /// </summary>
        private void WebView_NavigationStarting(CoreWebView2 sender, CoreWebView2NavigationStartingEventArgs args)
        {
            LoadingOverlay.Visibility = Visibility.Visible;
            LoadingText.Text = "Loading...";
        }

        /// <summary>
        /// Handles WebView2 navigation completed
        /// </summary>
        private void WebView_NavigationCompleted(CoreWebView2 sender, CoreWebView2NavigationCompletedEventArgs args)
        {
            if (args.IsSuccess)
            {
                LoadingOverlay.Visibility = Visibility.Collapsed;
                ErrorPanel.Visibility = Visibility.Collapsed;
                NedflixWebView.Visibility = Visibility.Visible;
            }
            else
            {
                ShowError($"Failed to load page. Error: {args.WebErrorStatus}");
            }
        }

        /// <summary>
        /// Handles messages from JavaScript
        /// </summary>
        private void WebView_WebMessageReceived(CoreWebView2 sender, CoreWebView2WebMessageReceivedEventArgs args)
        {
            try
            {
                string message = args.TryGetWebMessageAsString();
                // Handle messages from web app if needed
            }
            catch { }
        }

        /// <summary>
        /// Shows error panel with message
        /// </summary>
        private void ShowError(string message)
        {
            LoadingOverlay.Visibility = Visibility.Collapsed;
            NedflixWebView.Visibility = Visibility.Collapsed;
            ErrorMessage.Text = message;
            ErrorPanel.Visibility = Visibility.Visible;
        }

        /// <summary>
        /// Sets up Xbox gamepad monitoring
        /// </summary>
        private void SetupGamepadSupport()
        {
            // Monitor for gamepad connections
            Gamepad.GamepadAdded += Gamepad_GamepadAdded;
            Gamepad.GamepadRemoved += Gamepad_GamepadRemoved;

            // Check for existing gamepads
            if (Gamepad.Gamepads.Count > 0)
            {
                currentGamepad = Gamepad.Gamepads[0];
            }

            // Setup polling timer for gamepad input
            gamepadTimer = new DispatcherTimer();
            gamepadTimer.Interval = TimeSpan.FromMilliseconds(16); // ~60fps
            gamepadTimer.Tick += GamepadTimer_Tick;
            gamepadTimer.Start();
        }

        private void Gamepad_GamepadAdded(object sender, Gamepad e)
        {
            if (currentGamepad == null)
            {
                currentGamepad = e;
            }
        }

        private void Gamepad_GamepadRemoved(object sender, Gamepad e)
        {
            if (currentGamepad == e)
            {
                currentGamepad = Gamepad.Gamepads.Count > 0 ? Gamepad.Gamepads[0] : null;
            }
        }

        // Gamepad state tracking
        private GamepadReading lastReading;
        private DateTime lastDpadTime = DateTime.MinValue;
        private const int DPAD_REPEAT_DELAY_MS = 200;

        /// <summary>
        /// Polls gamepad input and sends commands to WebView
        /// </summary>
        private async void GamepadTimer_Tick(object sender, object e)
        {
            if (currentGamepad == null || NedflixWebView.Visibility != Visibility.Visible)
                return;

            try
            {
                var reading = currentGamepad.GetCurrentReading();
                var now = DateTime.Now;

                // Check buttons (only on press, not hold)
                if (reading.Buttons != lastReading.Buttons)
                {
                    // A Button - Select
                    if (reading.Buttons.HasFlag(GamepadButtons.A) && !lastReading.Buttons.HasFlag(GamepadButtons.A))
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.select()");
                    }

                    // B Button - Back
                    if (reading.Buttons.HasFlag(GamepadButtons.B) && !lastReading.Buttons.HasFlag(GamepadButtons.B))
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.back()");
                    }

                    // X Button - Play/Pause
                    if (reading.Buttons.HasFlag(GamepadButtons.X) && !lastReading.Buttons.HasFlag(GamepadButtons.X))
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.togglePlayPause()");
                    }

                    // Y Button - Toggle fullscreen (via JavaScript)
                    if (reading.Buttons.HasFlag(GamepadButtons.Y) && !lastReading.Buttons.HasFlag(GamepadButtons.Y))
                    {
                        await NedflixWebView.ExecuteScriptAsync("document.fullscreenElement ? document.exitFullscreen() : document.documentElement.requestFullscreen()");
                    }

                    // Menu button - Show settings
                    if (reading.Buttons.HasFlag(GamepadButtons.Menu) && !lastReading.Buttons.HasFlag(GamepadButtons.Menu))
                    {
                        ShowSettingsPanel();
                    }

                    // LB/RB - Skip back/forward
                    if (reading.Buttons.HasFlag(GamepadButtons.LeftShoulder) && !lastReading.Buttons.HasFlag(GamepadButtons.LeftShoulder))
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.seek(-10)");
                    }
                    if (reading.Buttons.HasFlag(GamepadButtons.RightShoulder) && !lastReading.Buttons.HasFlag(GamepadButtons.RightShoulder))
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.seek(10)");
                    }
                }

                // D-Pad navigation with repeat
                if ((now - lastDpadTime).TotalMilliseconds > DPAD_REPEAT_DELAY_MS)
                {
                    if (reading.Buttons.HasFlag(GamepadButtons.DPadUp))
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.navigate('up')");
                        lastDpadTime = now;
                    }
                    else if (reading.Buttons.HasFlag(GamepadButtons.DPadDown))
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.navigate('down')");
                        lastDpadTime = now;
                    }
                    else if (reading.Buttons.HasFlag(GamepadButtons.DPadLeft))
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.navigate('left')");
                        lastDpadTime = now;
                    }
                    else if (reading.Buttons.HasFlag(GamepadButtons.DPadRight))
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.navigate('right')");
                        lastDpadTime = now;
                    }
                }

                // Triggers - Volume control
                if (reading.LeftTrigger > 0.5 && lastReading.LeftTrigger <= 0.5)
                {
                    await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.adjustVolume(-0.1)");
                }
                if (reading.RightTrigger > 0.5 && lastReading.RightTrigger <= 0.5)
                {
                    await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.adjustVolume(0.1)");
                }

                // Left stick for navigation (with deadzone)
                const double DEADZONE = 0.3;
                if ((now - lastDpadTime).TotalMilliseconds > DPAD_REPEAT_DELAY_MS)
                {
                    if (reading.LeftThumbstickY > DEADZONE)
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.navigate('up')");
                        lastDpadTime = now;
                    }
                    else if (reading.LeftThumbstickY < -DEADZONE)
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.navigate('down')");
                        lastDpadTime = now;
                    }
                    else if (reading.LeftThumbstickX > DEADZONE)
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.navigate('right')");
                        lastDpadTime = now;
                    }
                    else if (reading.LeftThumbstickX < -DEADZONE)
                    {
                        await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.navigate('left')");
                        lastDpadTime = now;
                    }
                }

                lastReading = reading;
            }
            catch { }
        }

        /// <summary>
        /// Handles keyboard input for PC testing
        /// </summary>
        private async void MainWindow_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (NedflixWebView.Visibility != Visibility.Visible)
                return;

            switch (e.Key)
            {
                case VirtualKey.Space:
                    await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.togglePlayPause()");
                    break;
                case VirtualKey.Escape:
                    await NedflixWebView.ExecuteScriptAsync("window.xboxBridge?.back()");
                    break;
                case VirtualKey.F11:
                    await NedflixWebView.ExecuteScriptAsync("document.fullscreenElement ? document.exitFullscreen() : document.documentElement.requestFullscreen()");
                    break;
            }
        }

        /// <summary>
        /// Shows the settings panel
        /// </summary>
        private void ShowSettingsPanel()
        {
            NedflixWebView.Visibility = Visibility.Collapsed;
            ErrorPanel.Visibility = Visibility.Collapsed;
            SettingsPanel.Visibility = Visibility.Visible;
            ServerUrlInput.Focus(FocusState.Programmatic);
        }

        // Button click handlers
        private void RetryButton_Click(object sender, RoutedEventArgs e)
        {
            ErrorPanel.Visibility = Visibility.Collapsed;
            LoadingOverlay.Visibility = Visibility.Visible;
            NedflixWebView.CoreWebView2?.Navigate(serverUrl);
        }

        private void SettingsButton_Click(object sender, RoutedEventArgs e)
        {
            ShowSettingsPanel();
        }

        private void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            serverUrl = ServerUrlInput.Text.Trim();

            if (string.IsNullOrEmpty(serverUrl))
            {
                serverUrl = DEFAULT_SERVER_URL;
            }

            // Ensure URL has protocol
            if (!serverUrl.StartsWith("http://") && !serverUrl.StartsWith("https://"))
            {
                serverUrl = "http://" + serverUrl;
            }

            if (RememberServerCheckbox.IsChecked == true)
            {
                SaveSettings();
            }

            SettingsPanel.Visibility = Visibility.Collapsed;
            LoadingOverlay.Visibility = Visibility.Visible;
            NedflixWebView.CoreWebView2?.Navigate(serverUrl);
        }

        private void CancelSettingsButton_Click(object sender, RoutedEventArgs e)
        {
            SettingsPanel.Visibility = Visibility.Collapsed;

            if (NedflixWebView.CoreWebView2 != null)
            {
                NedflixWebView.Visibility = Visibility.Visible;
            }
            else
            {
                ErrorPanel.Visibility = Visibility.Visible;
            }
        }
    }
}
