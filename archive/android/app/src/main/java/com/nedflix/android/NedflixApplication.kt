/*
 * Nedflix Android - Application class
 */

package com.nedflix.android

import android.app.Application
import com.nedflix.android.data.SettingsManager

class NedflixApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        SettingsManager.initialize(this)
    }
}
