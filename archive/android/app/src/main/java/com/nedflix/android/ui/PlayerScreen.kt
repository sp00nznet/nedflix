/*
 * Nedflix Android - Player Screen
 * ExoPlayer-based media playback
 */

package com.nedflix.android.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.media3.common.MediaItem as ExoMediaItem
import androidx.media3.common.Player
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.ui.PlayerView
import com.nedflix.android.data.MediaItem
import com.nedflix.android.data.SettingsManager
import com.nedflix.android.ui.theme.NedflixRed
import java.util.concurrent.TimeUnit

@Composable
fun PlayerScreen(
    item: MediaItem,
    onClose: () -> Unit
) {
    val context = LocalContext.current
    val settings = SettingsManager.instance.settings

    // Build stream URL
    val streamUrl = remember(item) {
        "${settings.serverUrl}/api/stream?path=${item.path}&quality=${settings.videoQuality.value}&token=${settings.token ?: ""}"
    }

    // Create ExoPlayer
    val player = remember {
        ExoPlayer.Builder(context).build().apply {
            val mediaItem = ExoMediaItem.fromUri(streamUrl)
            setMediaItem(mediaItem)
            prepare()
            playWhenReady = true
        }
    }

    // Player state
    var isPlaying by remember { mutableStateOf(true) }
    var currentPosition by remember { mutableStateOf(0L) }
    var duration by remember { mutableStateOf(0L) }
    var showControls by remember { mutableStateOf(true) }

    // Listen to player state
    DisposableEffect(player) {
        val listener = object : Player.Listener {
            override fun onPlaybackStateChanged(state: Int) {
                isPlaying = player.isPlaying
                duration = player.duration.coerceAtLeast(0)
            }

            override fun onIsPlayingChanged(playing: Boolean) {
                isPlaying = playing
            }
        }
        player.addListener(listener)

        onDispose {
            player.removeListener(listener)
            player.release()
        }
    }

    // Update position periodically
    LaunchedEffect(player) {
        while (true) {
            currentPosition = player.currentPosition.coerceAtLeast(0)
            kotlinx.coroutines.delay(500)
        }
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black)
    ) {
        // Video player
        AndroidView(
            factory = { ctx ->
                PlayerView(ctx).apply {
                    this.player = player
                    useController = false
                    setOnClickListener { showControls = !showControls }
                }
            },
            modifier = Modifier.fillMaxSize()
        )

        // Custom controls overlay
        if (showControls) {
            PlayerControls(
                title = item.name,
                isPlaying = isPlaying,
                currentPosition = currentPosition,
                duration = duration,
                onPlayPause = {
                    if (isPlaying) player.pause() else player.play()
                },
                onSeek = { position ->
                    player.seekTo(position)
                },
                onSeekBack = {
                    player.seekTo((player.currentPosition - 10000).coerceAtLeast(0))
                },
                onSeekForward = {
                    player.seekTo((player.currentPosition + 10000).coerceAtMost(player.duration))
                },
                onClose = onClose
            )
        }
    }
}

@Composable
fun PlayerControls(
    title: String,
    isPlaying: Boolean,
    currentPosition: Long,
    duration: Long,
    onPlayPause: () -> Unit,
    onSeek: (Long) -> Unit,
    onSeekBack: () -> Unit,
    onSeekForward: () -> Unit,
    onClose: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.6f))
    ) {
        // Close button
        IconButton(
            onClick = onClose,
            modifier = Modifier
                .align(Alignment.TopStart)
                .padding(16.dp)
        ) {
            Icon(Icons.Default.Close, contentDescription = "Close", tint = Color.White)
        }

        // Title
        Text(
            text = title,
            color = Color.White,
            style = MaterialTheme.typography.titleLarge,
            modifier = Modifier
                .align(Alignment.TopCenter)
                .padding(16.dp),
            textAlign = TextAlign.Center
        )

        // Center controls
        Row(
            modifier = Modifier.align(Alignment.Center),
            horizontalArrangement = Arrangement.spacedBy(32.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Seek back
            IconButton(onClick = onSeekBack) {
                Icon(
                    Icons.Default.Replay10,
                    contentDescription = "Seek back 10s",
                    tint = Color.White,
                    modifier = Modifier.size(48.dp)
                )
            }

            // Play/Pause
            IconButton(onClick = onPlayPause) {
                Icon(
                    if (isPlaying) Icons.Default.Pause else Icons.Default.PlayArrow,
                    contentDescription = if (isPlaying) "Pause" else "Play",
                    tint = Color.White,
                    modifier = Modifier.size(64.dp)
                )
            }

            // Seek forward
            IconButton(onClick = onSeekForward) {
                Icon(
                    Icons.Default.Forward10,
                    contentDescription = "Seek forward 10s",
                    tint = Color.White,
                    modifier = Modifier.size(48.dp)
                )
            }
        }

        // Bottom progress bar
        Column(
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .padding(16.dp)
        ) {
            // Progress slider
            Slider(
                value = if (duration > 0) currentPosition.toFloat() / duration else 0f,
                onValueChange = { value ->
                    onSeek((value * duration).toLong())
                },
                colors = SliderDefaults.colors(
                    thumbColor = NedflixRed,
                    activeTrackColor = NedflixRed,
                    inactiveTrackColor = Color.Gray
                ),
                modifier = Modifier.fillMaxWidth()
            )

            // Time display
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = formatDuration(currentPosition),
                    color = Color.White,
                    style = MaterialTheme.typography.bodySmall
                )
                Text(
                    text = formatDuration(duration),
                    color = Color.White,
                    style = MaterialTheme.typography.bodySmall
                )
            }
        }
    }
}

private fun formatDuration(millis: Long): String {
    val minutes = TimeUnit.MILLISECONDS.toMinutes(millis)
    val seconds = TimeUnit.MILLISECONDS.toSeconds(millis) % 60
    return String.format("%02d:%02d", minutes, seconds)
}
