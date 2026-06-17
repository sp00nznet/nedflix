/*
 * Nedflix Android - Settings Manager
 */

package com.nedflix.android.data

import android.content.Context
import android.content.SharedPreferences
import androidx.core.content.edit

class SettingsManager private constructor(context: Context) {

    private val prefs: SharedPreferences =
        context.getSharedPreferences("nedflix_settings", Context.MODE_PRIVATE)

    var settings: AppSettings
        get() = AppSettings(
            serverUrl = prefs.getString(KEY_SERVER_URL, "") ?: "",
            token = prefs.getString(KEY_TOKEN, null),
            username = prefs.getString(KEY_USERNAME, null),
            volume = prefs.getInt(KEY_VOLUME, 80),
            videoQuality = VideoQuality.values().find {
                it.value == prefs.getString(KEY_VIDEO_QUALITY, "hd")
            } ?: VideoQuality.HD,
            autoPlay = prefs.getBoolean(KEY_AUTO_PLAY, true),
            showSubtitles = prefs.getBoolean(KEY_SHOW_SUBTITLES, true),
            subtitleLanguage = prefs.getString(KEY_SUBTITLE_LANGUAGE, "en") ?: "en",
            audioLanguage = prefs.getString(KEY_AUDIO_LANGUAGE, "en") ?: "en"
        )
        set(value) {
            prefs.edit {
                putString(KEY_SERVER_URL, value.serverUrl)
                putString(KEY_TOKEN, value.token)
                putString(KEY_USERNAME, value.username)
                putInt(KEY_VOLUME, value.volume)
                putString(KEY_VIDEO_QUALITY, value.videoQuality.value)
                putBoolean(KEY_AUTO_PLAY, value.autoPlay)
                putBoolean(KEY_SHOW_SUBTITLES, value.showSubtitles)
                putString(KEY_SUBTITLE_LANGUAGE, value.subtitleLanguage)
                putString(KEY_AUDIO_LANGUAGE, value.audioLanguage)
            }
        }

    companion object {
        private const val KEY_SERVER_URL = "server_url"
        private const val KEY_TOKEN = "token"
        private const val KEY_USERNAME = "username"
        private const val KEY_VOLUME = "volume"
        private const val KEY_VIDEO_QUALITY = "video_quality"
        private const val KEY_AUTO_PLAY = "auto_play"
        private const val KEY_SHOW_SUBTITLES = "show_subtitles"
        private const val KEY_SUBTITLE_LANGUAGE = "subtitle_language"
        private const val KEY_AUDIO_LANGUAGE = "audio_language"

        @Volatile
        private var INSTANCE: SettingsManager? = null

        val instance: SettingsManager
            get() = INSTANCE ?: throw IllegalStateException("SettingsManager not initialized")

        fun initialize(context: Context) {
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
