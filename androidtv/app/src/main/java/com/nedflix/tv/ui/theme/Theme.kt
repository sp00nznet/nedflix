/*
 * Nedflix for Android TV - Theme
 */

package com.nedflix.tv.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

val NedflixRed = Color(0xFFE50914)
val NedflixDark = Color(0xFF121212)
val NedflixDarkGray = Color(0xFF1A1A1A)
val NedflixGray = Color(0xFF2A2A2A)

private val DarkColorScheme = darkColorScheme(
    primary = NedflixRed,
    secondary = NedflixRed,
    tertiary = NedflixRed,
    background = NedflixDark,
    surface = NedflixDarkGray,
    onPrimary = Color.White,
    onSecondary = Color.White,
    onTertiary = Color.White,
    onBackground = Color.White,
    onSurface = Color.White,
)

@Composable
fun NedflixTVTheme(
    content: @Composable () -> Unit
) {
    MaterialTheme(
        colorScheme = DarkColorScheme,
        content = content
    )
}
