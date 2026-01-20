/*
 * Nedflix for Android TV - Data Models
 */

package com.nedflix.tv.data

import kotlinx.serialization.Serializable

@Serializable
data class MediaItem(
    val id: String,
    val name: String,
    val path: String,
    val type: String,
    val duration: Double? = null,
    val thumbnail: String? = null,
    val description: String? = null,
    val year: Int? = null,
    val rating: Double? = null
)

@Serializable
data class MediaCategory(
    val id: String,
    val name: String,
    val items: List<MediaItem>
)

@Serializable
data class AuthResponse(
    val token: String
)

@Serializable
data class ProgressResponse(
    val position: Double
)

data class Settings(
    val serverUrl: String = "",
    val token: String? = null,
    val videoQuality: VideoQuality = VideoQuality.AUTO
)

enum class VideoQuality(val value: String) {
    AUTO("auto"),
    HIGH("high"),
    MEDIUM("medium"),
    LOW("low")
}
