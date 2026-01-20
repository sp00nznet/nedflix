/*
 * Nedflix for Android TV - API Client
 * Handles communication with Nedflix server
 */

package com.nedflix.tv.data

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import java.net.URLEncoder
import java.util.concurrent.TimeUnit

class ApiClient private constructor() {
    private var baseUrl: String = ""
    private var authToken: String? = null

    private val json = Json { ignoreUnknownKeys = true }

    private val client = OkHttpClient.Builder()
        .connectTimeout(30, TimeUnit.SECONDS)
        .readTimeout(60, TimeUnit.SECONDS)
        .writeTimeout(30, TimeUnit.SECONDS)
        .build()

    fun configure(serverUrl: String) {
        baseUrl = serverUrl.trimEnd('/')
    }

    fun setToken(token: String?) {
        authToken = token
    }

    // Authentication
    suspend fun authenticate(username: String, password: String): String = withContext(Dispatchers.IO) {
        val requestBody = """{"username":"$username","password":"$password"}"""
            .toRequestBody("application/json".toMediaType())

        val request = Request.Builder()
            .url("$baseUrl/api/auth/login")
            .post(requestBody)
            .build()

        val response = client.newCall(request).execute()
        if (!response.isSuccessful) {
            throw Exception("Authentication failed: ${response.code}")
        }

        val body = response.body?.string() ?: throw Exception("Empty response")
        val authResponse = json.decodeFromString<AuthResponse>(body)
        authToken = authResponse.token

        // Save token
        SettingsManager.instance.updateToken(authResponse.token)

        authResponse.token
    }

    // Fetch library
    suspend fun fetchLibrary(): List<MediaCategory> = withContext(Dispatchers.IO) {
        val requestBuilder = Request.Builder()
            .url("$baseUrl/api/library")
            .get()

        authToken?.let {
            requestBuilder.addHeader("Authorization", "Bearer $it")
        }

        val response = client.newCall(requestBuilder.build()).execute()

        if (response.code == 401) {
            throw Exception("Authentication required")
        }

        if (!response.isSuccessful) {
            throw Exception("Server error: ${response.code}")
        }

        val body = response.body?.string() ?: "[]"

        // Try parsing as categories first
        try {
            return@withContext json.decodeFromString<List<MediaCategory>>(body)
        } catch (e: Exception) {
            // Try parsing as flat list
            try {
                val items = json.decodeFromString<List<MediaItem>>(body)
                return@withContext groupItemsByType(items)
            } catch (e2: Exception) {
                return@withContext emptyList()
            }
        }
    }

    private fun groupItemsByType(items: List<MediaItem>): List<MediaCategory> {
        val videos = mutableListOf<MediaItem>()
        val audio = mutableListOf<MediaItem>()
        val other = mutableListOf<MediaItem>()

        items.forEach { item ->
            when (item.type.lowercase()) {
                "video", "movie", "tv" -> videos.add(item)
                "audio", "music" -> audio.add(item)
                else -> other.add(item)
            }
        }

        val categories = mutableListOf<MediaCategory>()

        if (videos.isNotEmpty()) {
            categories.add(MediaCategory("videos", "Videos", videos))
        }
        if (audio.isNotEmpty()) {
            categories.add(MediaCategory("audio", "Music", audio))
        }
        if (other.isNotEmpty()) {
            categories.add(MediaCategory("other", "Other", other))
        }

        return categories
    }

    // Get stream URL for media item
    fun getStreamUrl(item: MediaItem, quality: String = "auto"): String {
        val encodedPath = URLEncoder.encode(item.path, "UTF-8")
        var url = "$baseUrl/api/stream?path=$encodedPath&quality=$quality"

        authToken?.let {
            url += "&token=$it"
        }

        return url
    }

    // Get thumbnail URL
    fun getThumbnailUrl(item: MediaItem): String? {
        val thumbnail = item.thumbnail ?: return null

        return if (thumbnail.startsWith("http")) {
            thumbnail
        } else {
            "$baseUrl$thumbnail"
        }
    }

    // Search
    suspend fun search(query: String): List<MediaItem> = withContext(Dispatchers.IO) {
        val encodedQuery = URLEncoder.encode(query, "UTF-8")

        val requestBuilder = Request.Builder()
            .url("$baseUrl/api/search?q=$encodedQuery")
            .get()

        authToken?.let {
            requestBuilder.addHeader("Authorization", "Bearer $it")
        }

        val response = client.newCall(requestBuilder.build()).execute()
        val body = response.body?.string() ?: "[]"

        json.decodeFromString(body)
    }

    // Update playback progress
    suspend fun updateProgress(itemId: String, position: Double) = withContext(Dispatchers.IO) {
        val requestBody = """{"id":"$itemId","position":$position}"""
            .toRequestBody("application/json".toMediaType())

        val requestBuilder = Request.Builder()
            .url("$baseUrl/api/progress")
            .post(requestBody)

        authToken?.let {
            requestBuilder.addHeader("Authorization", "Bearer $it")
        }

        client.newCall(requestBuilder.build()).execute()
    }

    // Get playback progress
    suspend fun getProgress(itemId: String): Double = withContext(Dispatchers.IO) {
        val requestBuilder = Request.Builder()
            .url("$baseUrl/api/progress/$itemId")
            .get()

        authToken?.let {
            requestBuilder.addHeader("Authorization", "Bearer $it")
        }

        val response = client.newCall(requestBuilder.build()).execute()
        val body = response.body?.string() ?: """{"position":0}"""

        val progressResponse = json.decodeFromString<ProgressResponse>(body)
        progressResponse.position
    }

    companion object {
        val instance: ApiClient by lazy { ApiClient() }
    }
}
