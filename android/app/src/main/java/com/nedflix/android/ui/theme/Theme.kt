/*
 * Nedflix Android - Theme
 */

package com.nedflix.android.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

val NedflixRed = Color(0xFFE50914)
val DarkBackground = Color(0xFF0A0A0A)
val SurfaceDark = Color(0xFF1A1A1A)

private val DarkColorScheme = darkColorScheme(
    primary = NedflixRed,
    secondary = NedflixRed,
    tertiary = NedflixRed,
    background = DarkBackground,
    surface = SurfaceDark,
    onPrimary = Color.White,
    onSecondary = Color.White,
    onTertiary = Color.White,
    onBackground = Color.White,
    onSurface = Color.White
)

@Composable
fun NedflixTheme(
    content: @Composable () -> Unit
) {
    MaterialTheme(
        colorScheme = DarkColorScheme,
        content = content
    )
}
