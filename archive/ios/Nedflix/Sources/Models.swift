/*
 * Nedflix iOS - Data Models
 */

import Foundation

// MARK: - Library

enum Library: String, CaseIterable, Codable {
    case music
    case audiobooks
    case movies
    case tvshows

    var displayName: String {
        switch self {
        case .music: return "Music"
        case .audiobooks: return "Audiobooks"
        case .movies: return "Movies"
        case .tvshows: return "TV Shows"
        }
    }

    var apiPath: String {
        return rawValue
    }
}

// MARK: - Media Item

struct MediaItem: Codable, Identifiable {
    let id: String
    let name: String
    let path: String
    let type: MediaType
    let isDirectory: Bool
    let duration: Int?
    let size: Int64?
    let year: Int?
    let rating: Double?
    let description: String?
    let thumbnailURL: String?

    enum CodingKeys: String, CodingKey {
        case id, name, path, type, duration, size, year, rating, description
        case isDirectory = "is_directory"
        case thumbnailURL = "thumbnail_url"
    }

    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        id = try container.decodeIfPresent(String.self, forKey: .id) ?? UUID().uuidString
        name = try container.decode(String.self, forKey: .name)
        path = try container.decode(String.self, forKey: .path)
        type = try container.decodeIfPresent(MediaType.self, forKey: .type) ?? .unknown
        isDirectory = try container.decodeIfPresent(Bool.self, forKey: .isDirectory) ?? false
        duration = try container.decodeIfPresent(Int.self, forKey: .duration)
        size = try container.decodeIfPresent(Int64.self, forKey: .size)
        year = try container.decodeIfPresent(Int.self, forKey: .year)
        rating = try container.decodeIfPresent(Double.self, forKey: .rating)
        description = try container.decodeIfPresent(String.self, forKey: .description)
        thumbnailURL = try container.decodeIfPresent(String.self, forKey: .thumbnailURL)
    }
}

// MARK: - Media Type

enum MediaType: String, Codable {
    case audio
    case video
    case directory
    case unknown

    var iconName: String {
        switch self {
        case .audio: return "music.note"
        case .video: return "film"
        case .directory: return "folder"
        case .unknown: return "doc"
        }
    }
}

// MARK: - API Response

struct BrowseResponse: Codable {
    let items: [MediaItem]
    let path: String
    let totalCount: Int?

    enum CodingKeys: String, CodingKey {
        case items, path
        case totalCount = "total_count"
    }
}

struct LoginResponse: Codable {
    let token: String
    let expiresAt: Date?

    enum CodingKeys: String, CodingKey {
        case token
        case expiresAt = "expires_at"
    }
}

// MARK: - Settings

struct AppSettings: Codable {
    var serverURL: String
    var token: String?
    var username: String?
    var volume: Int
    var videoQuality: VideoQuality
    var autoPlay: Bool
    var showSubtitles: Bool
    var subtitleLanguage: String
    var audioLanguage: String

    static var `default`: AppSettings {
        AppSettings(
            serverURL: "",
            token: nil,
            username: nil,
            volume: 80,
            videoQuality: .hd,
            autoPlay: true,
            showSubtitles: true,
            subtitleLanguage: "en",
            audioLanguage: "en"
        )
    }
}

enum VideoQuality: String, Codable, CaseIterable {
    case sd = "sd"
    case hd = "hd"
    case fhd = "fhd"

    var displayName: String {
        switch self {
        case .sd: return "SD (480p)"
        case .hd: return "HD (720p)"
        case .fhd: return "Full HD (1080p)"
        }
    }
}
