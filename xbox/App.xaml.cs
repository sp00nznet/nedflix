using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using System;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.Gaming.Input;
using Windows.System;

namespace Nedflix.Xbox
{
    /// <summary>
    /// Nedflix Xbox Application - Personal Video Streaming Platform
    /// Optimized for Xbox Series S/X with gamepad navigation
    /// </summary>
    public partial class App : Application
    {
        private Window m_window;
        private bool isXboxDevice = false;

        public App()
        {
            this.InitializeComponent();

            // Check if running on Xbox
            isXboxDevice = DetectXboxDevice();

            // Configure for Xbox if detected
            if (isXboxDevice)
            {
                ConfigureForXbox();
            }
        }

        /// <summary>
        /// Detects if the app is running on an Xbox console
        /// </summary>
        private bool DetectXboxDevice()
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
        /// Configures the application for Xbox-specific behavior
        /// </summary>
        private void ConfigureForXbox()
        {
            // In WinUI 3 / Windows App SDK, gamepad focus navigation is enabled
            // by default when running on Xbox. Focus visuals and pointer mode
            // are configured at the element level rather than the application level.
            // Xbox-specific UI optimizations are applied in MainWindow.
        }

        /// <summary>
        /// Invoked when the application is launched
        /// </summary>
        protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
        {
            m_window = new MainWindow();

            // Configure window for Xbox (fullscreen)
            if (isXboxDevice)
            {
                // Xbox apps run fullscreen by default
                m_window.Title = "Nedflix";
            }
            else
            {
                m_window.Title = "Nedflix - Xbox Client";
            }

            m_window.Activate();
        }

        /// <summary>
        /// Gets whether the app is running on Xbox
        /// </summary>
        public bool IsXbox => isXboxDevice;

        /// <summary>
        /// Gets the main application window
        /// </summary>
        public Window MainWindow => m_window;
    }
}
