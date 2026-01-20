/*
 * Nedflix for Android TV - Main Activity
 * Full-featured media streaming app using Leanback and ExoPlayer
 */

package com.nedflix.tv

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import com.nedflix.tv.data.SettingsManager
import com.nedflix.tv.ui.NedflixTVApp
import com.nedflix.tv.ui.theme.NedflixTVTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Initialize settings
        SettingsManager.init(applicationContext)

        setContent {
            NedflixTVTheme {
                NedflixTVApp(
                    modifier = Modifier
                        .fillMaxSize()
                        .background(Color(0xFF121212))
                )
            }
        }
    }
}
