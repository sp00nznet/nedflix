/*
 * Nedflix Android - Data Models
 */

package com.nedflix.android.data

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

enum class Library(val displayName: String, val apiPath: String) {
    MUSIC("Music", "music"),
    AUDIOBOOKS("Audiobooks", "audiobooks"),
    MOVIES("Movies", "movies"),
    TVSHOWS("TV Shows", "tvshows")
}

enum class VideoQuality(val value: String, val displayName: String) {
    SD("sd", "SD (480p)"),
    HD("hd", "HD (720p)"),
    FHD("fhd", "Full HD (1080p)")
}

@Serializable
data class MediaItem(
    val id: String = "",
    val name: String,
    val path: String,
    val type: String = "unknown",
    @SerialName("is_directory")
    val isDirectory: Boolean = false,
    val duration: Int? = null,
    val size: Long? = null,
    val year: Int? = null,
    val rating: Double? = null,
    val description: String? = null,
    @SerialName("thumbnail_url")
    val thumbnailUrl: String? = null
)

@Serializable
data class BrowseResponse(
    val items: List<MediaItem>,
    val path: String,
    @SerialName("total_count")
    val totalCount: Int? = null
)

@Serializable
data class LoginResponse(
    val token: String,
    @SerialName("expires_at")
    val expiresAt: String? = null
)

data class AppSettings(
    val serverUrl: String = "",
    val token: String? = null,
    val username: String? = null,
    val volume: Int = 80,
    val videoQuality: VideoQuality = VideoQuality.HD,
    val autoPlay: Boolean = true,
    val showSubtitles: Boolean = true,
    val subtitleLanguage: String = "en",
    val audioLanguage: String = "en"
)
