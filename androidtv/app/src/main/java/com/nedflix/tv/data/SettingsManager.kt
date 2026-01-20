/*
 * Nedflix for Android TV - Settings Manager
 * Persistent storage for app settings
 */

package com.nedflix.tv.data

import android.content.Context
import android.content.SharedPreferences
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

class SettingsManager private constructor(context: Context) {
    private val prefs: SharedPreferences = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

    var settings by mutableStateOf(loadSettings())
        private set

    private fun loadSettings(): Settings {
        return Settings(
            serverUrl = prefs.getString(KEY_SERVER_URL, "") ?: "",
            token = prefs.getString(KEY_TOKEN, null),
            videoQuality = VideoQuality.valueOf(
                prefs.getString(KEY_VIDEO_QUALITY, VideoQuality.AUTO.name) ?: VideoQuality.AUTO.name
            )
        )
    }

    fun updateServerUrl(url: String) {
        prefs.edit().putString(KEY_SERVER_URL, url).apply()
        settings = settings.copy(serverUrl = url)
    }

    fun updateToken(token: String?) {
        if (token != null) {
            prefs.edit().putString(KEY_TOKEN, token).apply()
        } else {
            prefs.edit().remove(KEY_TOKEN).apply()
        }
        settings = settings.copy(token = token)
    }

    fun updateVideoQuality(quality: VideoQuality) {
        prefs.edit().putString(KEY_VIDEO_QUALITY, quality.name).apply()
        settings = settings.copy(videoQuality = quality)
    }

    fun clearAll() {
        prefs.edit().clear().apply()
        settings = Settings()
    }

    companion object {
        private const val PREFS_NAME = "nedflix_prefs"
        private const val KEY_SERVER_URL = "server_url"
        private const val KEY_TOKEN = "auth_token"
        private const val KEY_VIDEO_QUALITY = "video_quality"

        @Volatile
        private var INSTANCE: SettingsManager? = null

        val instance: SettingsManager
            get() = INSTANCE ?: throw IllegalStateException("SettingsManager not initialized")

        fun init(context: Context) {
            if (INSTANCE == null) {
                synchronized(this) {
                    if (INSTANCE == null) {
                        INSTANCE = SettingsManager(context.applicationContext)
                    }
                }
            }
        }
    }
}
