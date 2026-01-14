/*
 * Nedflix Android - API Client
 */

package com.nedflix.android.data

import io.ktor.client.*
import io.ktor.client.call.*
import io.ktor.client.engine.okhttp.*
import io.ktor.client.plugins.contentnegotiation.*
import io.ktor.client.request.*
import io.ktor.http.*
import io.ktor.serialization.kotlinx.json.*
import kotlinx.serialization.json.Json

class ApiClient {

    private val client = HttpClient(OkHttp) {
        install(ContentNegotiation) {
            json(Json {
                ignoreUnknownKeys = true
                isLenient = true
            })
        }
    }

    private val settings: AppSettings
        get() = SettingsManager.instance.settings

    private val baseUrl: String
        get() = settings.serverUrl

    suspend fun browse(library: Library, path: String): List<MediaItem> {
        if (baseUrl.isEmpty()) {
            throw Exception("Server not configured")
        }

        val response: BrowseResponse = client.get("$baseUrl/api/browse/${library.apiPath}") {
            parameter("path", path)
            parameter("token", settings.token ?: "")
        }.body()

        return response.items
    }

    suspend fun login(username: String, password: String): String {
        if (baseUrl.isEmpty()) {
            throw Exception("Server not configured")
        }

        val response: LoginResponse = client.post("$baseUrl/api/auth/login") {
            contentType(ContentType.Application.Json)
            setBody(mapOf("username" to username, "password" to password))
        }.body()

        return response.token
    }

    suspend fun search(query: String): List<MediaItem> {
        if (baseUrl.isEmpty()) {
            throw Exception("Server not configured")
        }

        val response: BrowseResponse = client.get("$baseUrl/api/search") {
            parameter("q", query)
            parameter("token", settings.token ?: "")
        }.body()

        return response.items
    }

    fun getStreamUrl(item: MediaItem, quality: VideoQuality = settings.videoQuality): String {
        return "$baseUrl/api/stream?path=${item.path}&quality=${quality.value}&token=${settings.token ?: ""}"
    }
}
