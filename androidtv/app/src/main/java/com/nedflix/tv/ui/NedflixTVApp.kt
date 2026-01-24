/*
 * Nedflix for Android TV - Main App UI
 * Compose for TV with Leanback-style navigation
 */

package com.nedflix.tv.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.focusable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import coil.compose.AsyncImage
import com.nedflix.tv.data.*
import kotlinx.coroutines.launch

// App state
class AppState {
    var isAuthenticated by mutableStateOf(false)
    var isLoading by mutableStateOf(false)
    var error by mutableStateOf<String?>(null)
    var categories by mutableStateOf<List<MediaCategory>>(emptyList())
}

@Composable
fun NedflixTVApp(modifier: Modifier = Modifier) {
    val appState = remember { AppState() }
    val settings = SettingsManager.instance.settings

    // Check if already configured
    LaunchedEffect(Unit) {
        appState.isAuthenticated = settings.serverUrl.isNotEmpty()
    }

    Box(modifier = modifier) {
        if (!appState.isAuthenticated) {
            SetupScreen(
                onConnected = { appState.isAuthenticated = true }
            )
        } else {
            MediaBrowserScreen(appState = appState)
        }
    }
}

@Composable
fun SetupScreen(onConnected: () -> Unit) {
    var serverUrl by remember { mutableStateOf("") }
    var username by remember { mutableStateOf("") }
    var password by remember { mutableStateOf("") }
    var isConnecting by remember { mutableStateOf(false) }
    var errorMessage by remember { mutableStateOf<String?>(null) }
    val scope = rememberCoroutineScope()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(80.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        // Logo
        Icon(
            imageVector = Icons.Default.PlayArrow,
            contentDescription = "Nedflix",
            modifier = Modifier.size(120.dp),
            tint = Color(0xFFE50914)
        )

        Spacer(modifier = Modifier.height(16.dp))

        Text(
            text = "Nedflix",
            style = MaterialTheme.typography.displayLarge,
            fontWeight = FontWeight.Bold,
            color = Color.White
        )

        Spacer(modifier = Modifier.height(8.dp))

        Text(
            text = "Connect to your media server",
            style = MaterialTheme.typography.bodyLarge,
            color = Color.Gray
        )

        Spacer(modifier = Modifier.height(48.dp))

        // Server URL
        OutlinedTextField(
            value = serverUrl,
            onValueChange = { serverUrl = it },
            label = { Text("Server URL") },
            placeholder = { Text("http://192.168.1.100:3000") },
            modifier = Modifier
                .width(500.dp)
                .focusable(),
            singleLine = true,
            colors = OutlinedTextFieldDefaults.colors(
                focusedTextColor = Color.White,
                unfocusedTextColor = Color.White,
                focusedBorderColor = Color(0xFFE50914),
                unfocusedBorderColor = Color.Gray
            )
        )

        Spacer(modifier = Modifier.height(16.dp))

        // Username (optional)
        OutlinedTextField(
            value = username,
            onValueChange = { username = it },
            label = { Text("Username (optional)") },
            modifier = Modifier
                .width(500.dp)
                .focusable(),
            singleLine = true,
            colors = OutlinedTextFieldDefaults.colors(
                focusedTextColor = Color.White,
                unfocusedTextColor = Color.White,
                focusedBorderColor = Color(0xFFE50914),
                unfocusedBorderColor = Color.Gray
            )
        )

        Spacer(modifier = Modifier.height(16.dp))

        // Password (optional)
        OutlinedTextField(
            value = password,
            onValueChange = { password = it },
            label = { Text("Password (optional)") },
            modifier = Modifier
                .width(500.dp)
                .focusable(),
            singleLine = true,
            colors = OutlinedTextFieldDefaults.colors(
                focusedTextColor = Color.White,
                unfocusedTextColor = Color.White,
                focusedBorderColor = Color(0xFFE50914),
                unfocusedBorderColor = Color.Gray
            )
        )

        Spacer(modifier = Modifier.height(32.dp))

        // Connect button
        Button(
            onClick = {
                isConnecting = true
                errorMessage = null
                scope.launch {
                    try {
                        ApiClient.instance.configure(serverUrl)

                        if (username.isNotEmpty()) {
                            ApiClient.instance.authenticate(username, password)
                        }

                        // Test connection
                        ApiClient.instance.fetchLibrary()

                        // Save settings
                        SettingsManager.instance.updateServerUrl(serverUrl)
                        isConnecting = false
                        onConnected()
                    } catch (e: Exception) {
                        errorMessage = "Connection failed: ${e.message}"
                        isConnecting = false
                    }
                }
            },
            enabled = serverUrl.isNotEmpty() && !isConnecting,
            modifier = Modifier
                .width(200.dp)
                .height(56.dp),
            colors = ButtonDefaults.buttonColors(
                containerColor = Color(0xFFE50914)
            )
        ) {
            if (isConnecting) {
                CircularProgressIndicator(
                    modifier = Modifier.size(24.dp),
                    color = Color.White
                )
            } else {
                Text("Connect", style = MaterialTheme.typography.titleMedium)
            }
        }

        errorMessage?.let { error ->
            Spacer(modifier = Modifier.height(16.dp))
            Text(
                text = error,
                color = Color(0xFFE50914),
                style = MaterialTheme.typography.bodyMedium
            )
        }
    }
}

@Composable
fun MediaBrowserScreen(appState: AppState) {
    var selectedItem by remember { mutableStateOf<MediaItem?>(null) }
    val scope = rememberCoroutineScope()

    // Load library
    LaunchedEffect(Unit) {
        appState.isLoading = true
        try {
            appState.categories = ApiClient.instance.fetchLibrary()
        } catch (e: Exception) {
            appState.error = e.message
        }
        appState.isLoading = false
    }

    Box(modifier = Modifier.fillMaxSize()) {
        if (appState.isLoading) {
            CircularProgressIndicator(
                modifier = Modifier.align(Alignment.Center),
                color = Color(0xFFE50914)
            )
        } else {
            LazyColumn(
                modifier = Modifier.fillMaxSize(),
                contentPadding = PaddingValues(vertical = 40.dp)
            ) {
                // Header
                item {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 48.dp, vertical = 24.dp),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(
                                imageVector = Icons.Default.PlayArrow,
                                contentDescription = null,
                                tint = Color(0xFFE50914),
                                modifier = Modifier.size(48.dp)
                            )
                            Spacer(modifier = Modifier.width(12.dp))
                            Text(
                                text = "Nedflix",
                                style = MaterialTheme.typography.headlineLarge,
                                fontWeight = FontWeight.Bold,
                                color = Color.White
                            )
                        }

                        Row {
                            IconButton(
                                onClick = {
                                    scope.launch {
                                        appState.isLoading = true
                                        try {
                                            appState.categories = ApiClient.instance.fetchLibrary()
                                        } catch (e: Exception) {
                                            appState.error = e.message
                                        }
                                        appState.isLoading = false
                                    }
                                }
                            ) {
                                Icon(
                                    Icons.Default.Refresh,
                                    contentDescription = "Refresh",
                                    tint = Color.White
                                )
                            }
                            IconButton(onClick = { /* Settings */ }) {
                                Icon(
                                    Icons.Default.Settings,
                                    contentDescription = "Settings",
                                    tint = Color.White
                                )
                            }
                        }
                    }
                }

                // Categories
                if (appState.categories.isEmpty()) {
                    item {
                        Column(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(100.dp),
                            horizontalAlignment = Alignment.CenterHorizontally
                        ) {
                            Icon(
                                Icons.Default.Movie,
                                contentDescription = null,
                                modifier = Modifier.size(80.dp),
                                tint = Color.Gray
                            )
                            Spacer(modifier = Modifier.height(16.dp))
                            Text(
                                text = "No media found",
                                style = MaterialTheme.typography.titleLarge,
                                color = Color.Gray
                            )
                        }
                    }
                } else {
                    items(appState.categories) { category ->
                        MediaCategoryRow(
                            category = category,
                            onItemSelected = { selectedItem = it }
                        )
                    }
                }
            }
        }

        // Player overlay
        selectedItem?.let { item ->
            PlayerScreen(
                item = item,
                onClose = { selectedItem = null }
            )
        }
    }
}

@Composable
fun MediaCategoryRow(
    category: MediaCategory,
    onItemSelected: (MediaItem) -> Unit
) {
    Column(modifier = Modifier.padding(vertical = 16.dp)) {
        Text(
            text = category.name,
            style = MaterialTheme.typography.titleLarge,
            fontWeight = FontWeight.SemiBold,
            color = Color.White,
            modifier = Modifier.padding(horizontal = 48.dp, vertical = 12.dp)
        )

        LazyRow(
            contentPadding = PaddingValues(horizontal = 48.dp),
            horizontalArrangement = Arrangement.spacedBy(20.dp)
        ) {
            items(category.items) { item ->
                MediaCard(
                    item = item,
                    onClick = { onItemSelected(item) }
                )
            }
        }
    }
}

@Composable
fun MediaCard(
    item: MediaItem,
    onClick: () -> Unit
) {
    var isFocused by remember { mutableStateOf(false) }

    Card(
        modifier = Modifier
            .width(260.dp)
            .onFocusChanged { isFocused = it.isFocused }
            .focusable()
            .clickable { onClick() },
        colors = CardDefaults.cardColors(
            containerColor = if (isFocused) Color(0xFF2A2A2A) else Color(0xFF1A1A1A)
        )
    ) {
        Column {
            // Thumbnail
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(150.dp)
                    .background(Color(0xFF2A2A2A))
            ) {
                if (item.thumbnail != null) {
                    AsyncImage(
                        model = item.thumbnail,
                        contentDescription = item.name,
                        modifier = Modifier.fillMaxSize(),
                        contentScale = ContentScale.Crop
                    )
                } else {
                    Icon(
                        imageVector = if (item.type == "video") Icons.Default.Movie else Icons.Default.MusicNote,
                        contentDescription = null,
                        modifier = Modifier
                            .size(48.dp)
                            .align(Alignment.Center),
                        tint = Color.Gray
                    )
                }

                // Play icon overlay on focus
                if (isFocused) {
                    Box(
                        modifier = Modifier
                            .fillMaxSize()
                            .background(Color.Black.copy(alpha = 0.4f)),
                        contentAlignment = Alignment.Center
                    ) {
                        Icon(
                            imageVector = Icons.Default.PlayArrow,
                            contentDescription = "Play",
                            modifier = Modifier.size(48.dp),
                            tint = Color.White
                        )
                    }
                }
            }

            // Info
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(12.dp)
            ) {
                Text(
                    text = item.name,
                    style = MaterialTheme.typography.bodyMedium,
                    fontWeight = FontWeight.Medium,
                    color = Color.White,
                    maxLines = 2,
                    overflow = TextOverflow.Ellipsis
                )

                item.duration?.let { duration ->
                    Text(
                        text = formatDuration(duration),
                        style = MaterialTheme.typography.bodySmall,
                        color = Color.Gray
                    )
                }
            }
        }
    }
}

private fun formatDuration(seconds: Double): String {
    val mins = (seconds / 60).toInt()
    val secs = (seconds % 60).toInt()
    return String.format("%d:%02d", mins, secs)
}
