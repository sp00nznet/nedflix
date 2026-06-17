/*
 * Nedflix iOS - API Client
 * Network communication with the server
 */

import Foundation

class APIClient {

    // MARK: - Singleton

    static let shared = APIClient()

    // MARK: - Properties

    private let session: URLSession
    private var baseURL: URL?
    private var token: String?

    // MARK: - Errors

    enum APIError: LocalizedError {
        case invalidURL
        case notConfigured
        case networkError(Error)
        case httpError(Int)
        case decodingError(Error)
        case unauthorized

        var errorDescription: String? {
            switch self {
            case .invalidURL: return "Invalid server URL"
            case .notConfigured: return "Server not configured"
            case .networkError(let error): return "Network error: \(error.localizedDescription)"
            case .httpError(let code): return "Server error: \(code)"
            case .decodingError: return "Invalid response from server"
            case .unauthorized: return "Authentication required"
            }
        }
    }

    // MARK: - Init

    private init() {
        let config = URLSessionConfiguration.default
        config.timeoutIntervalForRequest = 30
        config.timeoutIntervalForResource = 300
        session = URLSession(configuration: config)

        loadSettings()
    }

    // MARK: - Configuration

    func configure(serverURL: String, token: String? = nil) {
        guard let url = URL(string: serverURL) else { return }
        self.baseURL = url
        self.token = token
        saveSettings()
    }

    private func loadSettings() {
        let settings = SettingsManager.shared.settings
        if !settings.serverURL.isEmpty {
            baseURL = URL(string: settings.serverURL)
        }
        token = settings.token
    }

    private func saveSettings() {
        var settings = SettingsManager.shared.settings
        settings.serverURL = baseURL?.absoluteString ?? ""
        settings.token = token
        SettingsManager.shared.settings = settings
    }

    // MARK: - API Methods

    func browse(library: Library, path: String, completion: @escaping (Result<[MediaItem], Error>) -> Void) {
        guard let baseURL = baseURL else {
            completion(.failure(APIError.notConfigured))
            return
        }

        var components = URLComponents(url: baseURL.appendingPathComponent("/api/browse/\(library.apiPath)"), resolvingAgainstBaseURL: true)
        components?.queryItems = [
            URLQueryItem(name: "path", value: path),
            URLQueryItem(name: "token", value: token ?? "")
        ]

        guard let url = components?.url else {
            completion(.failure(APIError.invalidURL))
            return
        }

        var request = URLRequest(url: url)
        request.httpMethod = "GET"
        addHeaders(to: &request)

        session.dataTask(with: request) { data, response, error in
            if let error = error {
                completion(.failure(APIError.networkError(error)))
                return
            }

            guard let httpResponse = response as? HTTPURLResponse else {
                completion(.failure(APIError.networkError(NSError(domain: "Invalid response", code: -1))))
                return
            }

            guard (200...299).contains(httpResponse.statusCode) else {
                if httpResponse.statusCode == 401 {
                    completion(.failure(APIError.unauthorized))
                } else {
                    completion(.failure(APIError.httpError(httpResponse.statusCode)))
                }
                return
            }

            guard let data = data else {
                completion(.success([]))
                return
            }

            do {
                let decoder = JSONDecoder()
                let response = try decoder.decode(BrowseResponse.self, from: data)
                completion(.success(response.items))
            } catch {
                completion(.failure(APIError.decodingError(error)))
            }
        }.resume()
    }

    func login(username: String, password: String, completion: @escaping (Result<String, Error>) -> Void) {
        guard let baseURL = baseURL else {
            completion(.failure(APIError.notConfigured))
            return
        }

        let url = baseURL.appendingPathComponent("/api/auth/login")
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        addHeaders(to: &request)

        let body = ["username": username, "password": password]
        request.httpBody = try? JSONSerialization.data(withJSONObject: body)

        session.dataTask(with: request) { [weak self] data, response, error in
            if let error = error {
                completion(.failure(APIError.networkError(error)))
                return
            }

            guard let data = data else {
                completion(.failure(APIError.decodingError(NSError(domain: "No data", code: -1))))
                return
            }

            do {
                let decoder = JSONDecoder()
                let loginResponse = try decoder.decode(LoginResponse.self, from: data)
                self?.token = loginResponse.token
                self?.saveSettings()
                completion(.success(loginResponse.token))
            } catch {
                completion(.failure(APIError.decodingError(error)))
            }
        }.resume()
    }

    func getStreamURL(for item: MediaItem, quality: VideoQuality) -> URL? {
        guard let baseURL = baseURL else { return nil }

        var components = URLComponents(url: baseURL.appendingPathComponent("/api/stream"), resolvingAgainstBaseURL: true)
        components?.queryItems = [
            URLQueryItem(name: "path", value: item.path),
            URLQueryItem(name: "quality", value: quality.rawValue),
            URLQueryItem(name: "token", value: token ?? "")
        ]

        return components?.url
    }

    func search(query: String, completion: @escaping (Result<[MediaItem], Error>) -> Void) {
        guard let baseURL = baseURL else {
            completion(.failure(APIError.notConfigured))
            return
        }

        var components = URLComponents(url: baseURL.appendingPathComponent("/api/search"), resolvingAgainstBaseURL: true)
        components?.queryItems = [
            URLQueryItem(name: "q", value: query),
            URLQueryItem(name: "token", value: token ?? "")
        ]

        guard let url = components?.url else {
            completion(.failure(APIError.invalidURL))
            return
        }

        var request = URLRequest(url: url)
        request.httpMethod = "GET"
        addHeaders(to: &request)

        session.dataTask(with: request) { data, response, error in
            if let error = error {
                completion(.failure(APIError.networkError(error)))
                return
            }

            guard let data = data else {
                completion(.success([]))
                return
            }

            do {
                let decoder = JSONDecoder()
                let response = try decoder.decode(BrowseResponse.self, from: data)
                completion(.success(response.items))
            } catch {
                completion(.failure(APIError.decodingError(error)))
            }
        }.resume()
    }

    // MARK: - Helpers

    private func addHeaders(to request: inout URLRequest) {
        request.setValue("Nedflix-iOS/1.0", forHTTPHeaderField: "User-Agent")
        if let token = token {
            request.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
        }
    }
}
