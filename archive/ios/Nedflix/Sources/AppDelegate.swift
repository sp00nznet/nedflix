/*
 * Nedflix iOS - App Delegate
 * Entry point for the iOS application
 */

import UIKit
import AVFoundation

@main
class AppDelegate: UIResponder, UIApplicationDelegate {

    var window: UIWindow?

    func application(_ application: UIApplication,
                     didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {

        // Configure audio session for media playback
        configureAudioSession()

        // Setup appearance
        configureAppearance()

        // Initialize window for iOS 12 and earlier
        if #available(iOS 13.0, *) {
            // SceneDelegate handles window
        } else {
            window = UIWindow(frame: UIScreen.main.bounds)
            let mainVC = MainViewController()
            let navController = UINavigationController(rootViewController: mainVC)
            window?.rootViewController = navController
            window?.makeKeyAndVisible()
        }

        return true
    }

    // MARK: - Audio Session

    private func configureAudioSession() {
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playback, mode: .moviePlayback, options: [.allowAirPlay, .allowBluetooth])
            try session.setActive(true)
        } catch {
            print("Failed to configure audio session: \(error)")
        }
    }

    // MARK: - Appearance

    private func configureAppearance() {
        // Netflix-style dark theme
        if #available(iOS 13.0, *) {
            // Dark mode by default
        }

        UINavigationBar.appearance().barTintColor = UIColor(red: 0.04, green: 0.04, blue: 0.04, alpha: 1.0)
        UINavigationBar.appearance().tintColor = .white
        UINavigationBar.appearance().titleTextAttributes = [.foregroundColor: UIColor.white]
        UINavigationBar.appearance().isTranslucent = false
    }

    // MARK: - UISceneSession Lifecycle (iOS 13+)

    @available(iOS 13.0, *)
    func application(_ application: UIApplication,
                     configurationForConnecting connectingSceneSession: UISceneSession,
                     options: UIScene.ConnectionOptions) -> UISceneConfiguration {
        return UISceneConfiguration(name: "Default Configuration", sessionRole: connectingSceneSession.role)
    }

    @available(iOS 13.0, *)
    func application(_ application: UIApplication,
                     didDiscardSceneSessions sceneSessions: Set<UISceneSession>) {
    }

    // MARK: - Background Tasks

    func applicationDidEnterBackground(_ application: UIApplication) {
        // Continue audio playback in background
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        // Resume video playback
    }
}
