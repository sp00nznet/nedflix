/*
 * Nedflix iOS - Settings Manager
 * Persistent storage using UserDefaults
 */

import Foundation

class SettingsManager {

    // MARK: - Singleton

    static let shared = SettingsManager()

    // MARK: - Keys

    private enum Keys {
        static let settings = "nedflix.settings"
    }

    // MARK: - Properties

    var settings: AppSettings {
        get {
            guard let data = UserDefaults.standard.data(forKey: Keys.settings),
                  let settings = try? JSONDecoder().decode(AppSettings.self, from: data) else {
                return .default
            }
            return settings
        }
        set {
            if let data = try? JSONEncoder().encode(newValue) {
                UserDefaults.standard.set(data, forKey: Keys.settings)
            }
        }
    }

    // MARK: - Init

    private init() {}

    // MARK: - Methods

    func reset() {
        settings = .default
    }
}
