/*
 * Nedflix Android - Main Compose App
 */

package com.nedflix.android.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.nedflix.android.data.Library
import com.nedflix.android.data.MediaItem
import com.nedflix.android.ui.theme.NedflixRed

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun NedflixApp(
    viewModel: MainViewModel = viewModel()
) {
    val uiState by viewModel.uiState.collectAsState()

    var selectedLibrary by remember { mutableStateOf(Library.MUSIC) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Nedflix") },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = Color(0xFF0A0A0A),
                    titleContentColor = Color.White
                ),
                actions = {
                    IconButton(onClick = { viewModel.navigateToSettings() }) {
                        Icon(
                            Icons.Default.Settings,
                            contentDescription = "Settings",
                            tint = Color.White
                        )
                    }
                }
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .background(Color(0xFF0A0A0A))
        ) {
            // Library selector
            LibrarySelector(
                selectedLibrary = selectedLibrary,
                onLibrarySelected = {
                    selectedLibrary = it
                    viewModel.loadLibrary(it)
                }
            )

            // Content
            when {
                uiState.isLoading -> {
                    Box(
                        modifier = Modifier.fillMaxSize(),
                        contentAlignment = Alignment.Center
                    ) {
                        CircularProgressIndicator(color = NedflixRed)
                    }
                }
                uiState.error != null -> {
                    ErrorScreen(
                        message = uiState.error!!,
                        onRetry = { viewModel.loadLibrary(selectedLibrary) }
                    )
                }
                uiState.showSettings -> {
                    SettingsScreen(
                        settings = uiState.settings,
                        onSettingsChanged = { viewModel.updateSettings(it) },
                        onBack = { viewModel.closeSettings() }
                    )
                }
                uiState.playingItem != null -> {
                    PlayerScreen(
                        item = uiState.playingItem!!,
                        onClose = { viewModel.stopPlayback() }
                    )
                }
                else -> {
                    MediaList(
                        items = uiState.mediaItems,
                        onItemClick = { viewModel.onMediaItemClick(it) }
                    )
                }
            }
        }
    }
}

@Composable
fun LibrarySelector(
    selectedLibrary: Library,
    onLibrarySelected: (Library) -> Unit
) {
    ScrollableTabRow(
        selectedTabIndex = Library.values().indexOf(selectedLibrary),
        containerColor = Color(0xFF1A1A1A),
        contentColor = Color.White,
        edgePadding = 8.dp,
        indicator = { tabPositions ->
            TabRowDefaults.Indicator(
                Modifier.tabIndicatorOffset(tabPositions[Library.values().indexOf(selectedLibrary)]),
                color = NedflixRed
            )
        }
    ) {
        Library.values().forEach { library ->
            Tab(
                selected = library == selectedLibrary,
                onClick = { onLibrarySelected(library) },
                text = {
                    Text(
                        library.displayName,
                        color = if (library == selectedLibrary) Color.White else Color.Gray
                    )
                }
            )
        }
    }
}

@Composable
fun MediaList(
    items: List<MediaItem>,
    onItemClick: (MediaItem) -> Unit
) {
    if (items.isEmpty()) {
        Box(
            modifier = Modifier.fillMaxSize(),
            contentAlignment = Alignment.Center
        ) {
            Text("No items found", color = Color.Gray)
        }
    } else {
        LazyColumn(
            modifier = Modifier.fillMaxSize(),
            contentPadding = PaddingValues(16.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            items(items) { item ->
                MediaListItem(item = item, onClick = { onItemClick(item) })
            }
        }
    }
}

@Composable
fun MediaListItem(
    item: MediaItem,
    onClick: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable(onClick = onClick),
        colors = CardDefaults.cardColors(
            containerColor = Color(0xFF1A1A1A)
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Icon
            Icon(
                imageVector = when {
                    item.isDirectory -> Icons.Default.Folder
                    item.type == "audio" -> Icons.Default.MusicNote
                    else -> Icons.Default.Movie
                },
                contentDescription = null,
                tint = Color.White,
                modifier = Modifier.size(32.dp)
            )

            Spacer(modifier = Modifier.width(16.dp))

            // Title and subtitle
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = item.name,
                    color = Color.White,
                    fontWeight = FontWeight.Medium
                )
                item.duration?.let { duration ->
                    Text(
                        text = "${duration / 60} min",
                        color = Color.Gray,
                        style = MaterialTheme.typography.bodySmall
                    )
                }
            }

            // Chevron
            Icon(
                Icons.Default.ChevronRight,
                contentDescription = null,
                tint = Color.Gray
            )
        }
    }
}

@Composable
fun ErrorScreen(
    message: String,
    onRetry: () -> Unit
) {
    Column(
        modifier = Modifier.fillMaxSize(),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Icon(
            Icons.Default.Error,
            contentDescription = null,
            tint = NedflixRed,
            modifier = Modifier.size(64.dp)
        )
        Spacer(modifier = Modifier.height(16.dp))
        Text(message, color = Color.White)
        Spacer(modifier = Modifier.height(16.dp))
        Button(
            onClick = onRetry,
            colors = ButtonDefaults.buttonColors(containerColor = NedflixRed)
        ) {
            Text("Retry")
        }
    }
}
