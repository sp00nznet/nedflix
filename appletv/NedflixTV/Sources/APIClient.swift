/*
 * Nedflix for Apple TV - API Client
 * Handles communication with Nedflix server
 */

import Foundation

class APIClient {
    static let shared = APIClient()

    private var baseURL: String = ""
    private var authToken: String?
    private let session: URLSession

    private init() {
        let config = URLSessionConfiguration.default
        config.timeoutIntervalForRequest = 30
        config.timeoutIntervalForResource = 300
        session = URLSession(configuration: config)

        // Load saved settings
        baseURL = UserDefaults.standard.string(forKey: "serverURL") ?? ""
        authToken = UserDefaults.standard.string(forKey: "authToken")
    }

    func configure(serverURL: String) {
        baseURL = serverURL.trimmingCharacters(in: .whitespacesAndNewlines)
        if baseURL.hasSuffix("/") {
            baseURL = String(baseURL.dropLast())
        }
    }

    func setToken(_ token: String?) {
        authToken = token
    }

    // MARK: - Authentication

    func authenticate(username: String, password: String) async throws -> String {
        let url = URL(string: "\(baseURL)/api/auth/login")!
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")

        let body = ["username": username, "password": password]
        request.httpBody = try JSONEncoder().encode(body)

        let (data, response) = try await session.data(for: request)

        guard let httpResponse = response as? HTTPURLResponse,
              httpResponse.statusCode == 200 else {
            throw APIError.authenticationFailed
        }

        struct AuthResponse: Codable {
            let token: String
        }

        let authResponse = try JSONDecoder().decode(AuthResponse.self, from: data)
        authToken = authResponse.token
        return authResponse.token
    }

    // MARK: - Library

    func fetchLibrary() async throws -> [MediaCategory] {
        let url = URL(string: "\(baseURL)/api/library")!
        var request = URLRequest(url: url)

        if let token = authToken {
            request.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
        }

        let (data, response) = try await session.data(for: request)

        guard let httpResponse = response as? HTTPURLResponse else {
            throw APIError.invalidResponse
        }

        if httpResponse.statusCode == 401 {
            throw APIError.authenticationFailed
        }

        guard httpResponse.statusCode == 200 else {
            throw APIError.serverError(httpResponse.statusCode)
        }

        // Try to decode as array of categories
        do {
            return try JSONDecoder().decode([MediaCategory].self, from: data)
        } catch {
            // Try to decode as flat list and group by type
            do {
                let items = try JSONDecoder().decode([MediaItem].self, from: data)
                return groupItemsByType(items)
            } catch {
                // Return empty if cannot parse
                return []
            }
        }
    }

    private func groupItemsByType(_ items: [MediaItem]) -> [MediaCategory] {
        var videos: [MediaItem] = []
        var audio: [MediaItem] = []
        var other: [MediaItem] = []

        for item in items {
            switch item.type.lowercased() {
            case "video", "movie", "tv":
                videos.append(item)
            case "audio", "music":
                audio.append(item)
            default:
                other.append(item)
            }
        }

        var categories: [MediaCategory] = []

        if !videos.isEmpty {
            categories.append(MediaCategory(id: "videos", name: "Videos", items: videos))
        }
        if !audio.isEmpty {
            categories.append(MediaCategory(id: "audio", name: "Music", items: audio))
        }
        if !other.isEmpty {
            categories.append(MediaCategory(id: "other", name: "Other", items: other))
        }

        return categories
    }

    // MARK: - Streaming

    func getStreamURL(for item: MediaItem, quality: String = "auto") -> URL? {
        var urlString = "\(baseURL)/api/stream?path=\(item.path.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed) ?? item.path)"

        urlString += "&quality=\(quality)"

        if let token = authToken {
            urlString += "&token=\(token)"
        }

        return URL(string: urlString)
    }

    func getThumbnailURL(for item: MediaItem) -> URL? {
        if let thumbnail = item.thumbnail, !thumbnail.isEmpty {
            if thumbnail.hasPrefix("http") {
                return URL(string: thumbnail)
            } else {
                return URL(string: "\(baseURL)\(thumbnail)")
            }
        }
        return nil
    }

    // MARK: - Search

    func search(query: String) async throws -> [MediaItem] {
        let encoded = query.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed) ?? query
        let url = URL(string: "\(baseURL)/api/search?q=\(encoded)")!

        var request = URLRequest(url: url)
        if let token = authToken {
            request.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
        }

        let (data, _) = try await session.data(for: request)
        return try JSONDecoder().decode([MediaItem].self, from: data)
    }

    // MARK: - Playback State

    func updateProgress(itemId: String, position: Double) async throws {
        let url = URL(string: "\(baseURL)/api/progress")!
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")

        if let token = authToken {
            request.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
        }

        let body: [String: Any] = [
            "id": itemId,
            "position": position
        ]
        request.httpBody = try JSONSerialization.data(withJSONObject: body)

        _ = try await session.data(for: request)
    }

    func getProgress(itemId: String) async throws -> Double {
        let url = URL(string: "\(baseURL)/api/progress/\(itemId)")!
        var request = URLRequest(url: url)

        if let token = authToken {
            request.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
        }

        let (data, _) = try await session.data(for: request)

        struct ProgressResponse: Codable {
            let position: Double
        }

        let response = try JSONDecoder().decode(ProgressResponse.self, from: data)
        return response.position
    }
}

// MARK: - Errors

enum APIError: LocalizedError {
    case invalidURL
    case invalidResponse
    case authenticationFailed
    case serverError(Int)
    case networkError(Error)

    var errorDescription: String? {
        switch self {
        case .invalidURL:
            return "Invalid server URL"
        case .invalidResponse:
            return "Invalid server response"
        case .authenticationFailed:
            return "Authentication failed"
        case .serverError(let code):
            return "Server error: \(code)"
        case .networkError(let error):
            return error.localizedDescription
        }
    }
}
