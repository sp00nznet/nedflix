/*
 * Nedflix for Android TV - Player Screen
 * ExoPlayer-based media playback with TV-optimized controls
 */

package com.nedflix.tv.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.focusable
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.media3.common.MediaItem as ExoMediaItem
import androidx.media3.common.Player
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.ui.PlayerView
import com.nedflix.tv.data.ApiClient
import com.nedflix.tv.data.MediaItem
import kotlinx.coroutines.delay
import java.util.concurrent.TimeUnit

@Composable
fun PlayerScreen(
    item: MediaItem,
    onClose: () -> Unit
) {
    val context = LocalContext.current

    // Build stream URL
    val streamUrl = remember(item) {
        ApiClient.instance.getStreamUrl(item)
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
    var isBuffering by remember { mutableStateOf(true) }

    // Listen to player state
    DisposableEffect(player) {
        val listener = object : Player.Listener {
            override fun onPlaybackStateChanged(state: Int) {
                isBuffering = state == Player.STATE_BUFFERING
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
            delay(500)
        }
    }

    // Auto-hide controls
    LaunchedEffect(showControls) {
        if (showControls && isPlaying) {
            delay(5000)
            showControls = false
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

        // Buffering indicator
        if (isBuffering) {
            CircularProgressIndicator(
                modifier = Modifier.align(Alignment.Center),
                color = Color(0xFFE50914)
            )
        }

        // Controls overlay
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
            .background(Color.Black.copy(alpha = 0.5f))
    ) {
        // Top bar with gradient
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .align(Alignment.TopCenter)
                .background(
                    Brush.verticalGradient(
                        colors = listOf(
                            Color.Black.copy(alpha = 0.8f),
                            Color.Transparent
                        )
                    )
                )
                .padding(24.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                IconButton(onClick = onClose) {
                    Icon(
                        Icons.Default.ArrowBack,
                        contentDescription = "Close",
                        tint = Color.White,
                        modifier = Modifier.size(32.dp)
                    )
                }

                Text(
                    text = title,
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.SemiBold,
                    color = Color.White
                )

                Spacer(modifier = Modifier.width(32.dp))
            }
        }

        // Center controls
        Row(
            modifier = Modifier.align(Alignment.Center),
            horizontalArrangement = Arrangement.spacedBy(48.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Seek back
            IconButton(
                onClick = onSeekBack,
                modifier = Modifier.focusable()
            ) {
                Icon(
                    Icons.Default.Replay10,
                    contentDescription = "Seek back 10s",
                    tint = Color.White,
                    modifier = Modifier.size(56.dp)
                )
            }

            // Play/Pause
            IconButton(
                onClick = onPlayPause,
                modifier = Modifier.focusable()
            ) {
                Icon(
                    if (isPlaying) Icons.Default.Pause else Icons.Default.PlayArrow,
                    contentDescription = if (isPlaying) "Pause" else "Play",
                    tint = Color.White,
                    modifier = Modifier.size(80.dp)
                )
            }

            // Seek forward
            IconButton(
                onClick = onSeekForward,
                modifier = Modifier.focusable()
            ) {
                Icon(
                    Icons.Default.Forward10,
                    contentDescription = "Seek forward 10s",
                    tint = Color.White,
                    modifier = Modifier.size(56.dp)
                )
            }
        }

        // Bottom progress bar with gradient
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .align(Alignment.BottomCenter)
                .background(
                    Brush.verticalGradient(
                        colors = listOf(
                            Color.Transparent,
                            Color.Black.copy(alpha = 0.8f)
                        )
                    )
                )
                .padding(24.dp)
        ) {
            // Progress slider
            Slider(
                value = if (duration > 0) currentPosition.toFloat() / duration else 0f,
                onValueChange = { value ->
                    onSeek((value * duration).toLong())
                },
                colors = SliderDefaults.colors(
                    thumbColor = Color(0xFFE50914),
                    activeTrackColor = Color(0xFFE50914),
                    inactiveTrackColor = Color.Gray.copy(alpha = 0.5f)
                ),
                modifier = Modifier
                    .fillMaxWidth()
                    .focusable()
            )

            // Time display
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = formatDuration(currentPosition),
                    color = Color.White,
                    style = MaterialTheme.typography.bodyMedium
                )
                Text(
                    text = formatDuration(duration),
                    color = Color.White,
                    style = MaterialTheme.typography.bodyMedium
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
