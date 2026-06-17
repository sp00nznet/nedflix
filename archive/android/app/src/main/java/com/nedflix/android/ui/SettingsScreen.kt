/*
 * Nedflix Android - Settings Screen
 */

package com.nedflix.android.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.nedflix.android.data.AppSettings
import com.nedflix.android.data.VideoQuality
import com.nedflix.android.ui.theme.NedflixRed

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    settings: AppSettings,
    onSettingsChanged: (AppSettings) -> Unit,
    onBack: () -> Unit
) {
    var currentSettings by remember { mutableStateOf(settings) }
    var showQualityDialog by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xFF0A0A0A))
    ) {
        TopAppBar(
            title = { Text("Settings") },
            navigationIcon = {
                IconButton(onClick = onBack) {
                    Icon(Icons.Default.ArrowBack, contentDescription = "Back", tint = Color.White)
                }
            },
            colors = TopAppBarDefaults.topAppBarColors(
                containerColor = Color(0xFF0A0A0A),
                titleContentColor = Color.White
            )
        )

        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(16.dp)
        ) {
            // Server Section
            SettingsSection(title = "Server") {
                OutlinedTextField(
                    value = currentSettings.serverUrl,
                    onValueChange = {
                        currentSettings = currentSettings.copy(serverUrl = it)
                        onSettingsChanged(currentSettings)
                    },
                    label = { Text("Server URL") },
                    placeholder = { Text("http://192.168.1.100:8080") },
                    modifier = Modifier.fillMaxWidth(),
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedTextColor = Color.White,
                        unfocusedTextColor = Color.White,
                        focusedBorderColor = NedflixRed,
                        unfocusedBorderColor = Color.Gray
                    )
                )
            }

            Spacer(modifier = Modifier.height(24.dp))

            // Playback Section
            SettingsSection(title = "Playback") {
                // Video Quality
                SettingsRow(
                    title = "Video Quality",
                    subtitle = currentSettings.videoQuality.displayName,
                    onClick = { showQualityDialog = true }
                )

                // Auto-play
                SettingsSwitch(
                    title = "Auto-play",
                    checked = currentSettings.autoPlay,
                    onCheckedChange = {
                        currentSettings = currentSettings.copy(autoPlay = it)
                        onSettingsChanged(currentSettings)
                    }
                )

                // Subtitles
                SettingsSwitch(
                    title = "Show Subtitles",
                    checked = currentSettings.showSubtitles,
                    onCheckedChange = {
                        currentSettings = currentSettings.copy(showSubtitles = it)
                        onSettingsChanged(currentSettings)
                    }
                )
            }

            Spacer(modifier = Modifier.height(24.dp))

            // About Section
            SettingsSection(title = "About") {
                SettingsRow(title = "Version", subtitle = "1.0.0")
                SettingsRow(title = "Platform", subtitle = "Android")
            }
        }
    }

    // Quality Selection Dialog
    if (showQualityDialog) {
        AlertDialog(
            onDismissRequest = { showQualityDialog = false },
            title = { Text("Video Quality") },
            text = {
                Column {
                    VideoQuality.values().forEach { quality ->
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 8.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            RadioButton(
                                selected = quality == currentSettings.videoQuality,
                                onClick = {
                                    currentSettings = currentSettings.copy(videoQuality = quality)
                                    onSettingsChanged(currentSettings)
                                    showQualityDialog = false
                                },
                                colors = RadioButtonDefaults.colors(
                                    selectedColor = NedflixRed
                                )
                            )
                            Spacer(modifier = Modifier.width(8.dp))
                            Text(quality.displayName)
                        }
                    }
                }
            },
            confirmButton = {
                TextButton(onClick = { showQualityDialog = false }) {
                    Text("Cancel")
                }
            }
        )
    }
}

@Composable
fun SettingsSection(
    title: String,
    content: @Composable ColumnScope.() -> Unit
) {
    Column {
        Text(
            text = title,
            color = NedflixRed,
            style = MaterialTheme.typography.titleMedium
        )
        Spacer(modifier = Modifier.height(8.dp))
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = Color(0xFF1A1A1A))
        ) {
            Column(
                modifier = Modifier.padding(16.dp),
                content = content
            )
        }
    }
}

@Composable
fun SettingsRow(
    title: String,
    subtitle: String,
    onClick: (() -> Unit)? = null
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .then(if (onClick != null) Modifier.padding(vertical = 8.dp) else Modifier)
            .let { if (onClick != null) it else it.padding(vertical = 4.dp) },
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(title, color = Color.White)
        Text(subtitle, color = Color.Gray)
    }
}

@Composable
fun SettingsSwitch(
    title: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(title, color = Color.White)
        Switch(
            checked = checked,
            onCheckedChange = onCheckedChange,
            colors = SwitchDefaults.colors(
                checkedThumbColor = Color.White,
                checkedTrackColor = NedflixRed,
                uncheckedThumbColor = Color.Gray,
                uncheckedTrackColor = Color.DarkGray
            )
        )
    }
}
