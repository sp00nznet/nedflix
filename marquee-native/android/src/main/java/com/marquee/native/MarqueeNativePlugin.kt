package com.marquee.native

import androidx.media3.common.MediaItem
import androidx.media3.common.Player
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.session.MediaSession
import com.getcapacitor.JSObject
import com.getcapacitor.Plugin
import com.getcapacitor.PluginCall
import com.getcapacitor.PluginMethod
import com.getcapacitor.annotation.CapacitorPlugin

/**
 * MarqueeNative — Android implementation (Capacitor 6 + Media3/ExoPlayer).
 *
 * Core wired: ExoPlayer transport (HLS + hardware decode + HDR), MediaSession for
 * lock-screen / notification controls.
 * Stubbed with TODOs: foreground MediaSessionService + PlayerNotificationManager,
 * DownloadManager/WorkManager downloads, Google Cast, PiP.
 *
 * Run the player on the main thread; emit events with notifyListeners(...).
 */
@CapacitorPlugin(name = "MarqueeNative")
class MarqueeNativePlugin : Plugin() {

    private var player: ExoPlayer? = null
    private var mediaSession: MediaSession? = null
    private var currentId: String? = null

    override fun load() {
        bridge.activity.runOnUiThread {
            val p = ExoPlayer.Builder(context).build()
            // TODO: start a MediaSessionService so audio survives backgrounding, and attach
            //       a PlayerNotificationManager for the media-style notification.
            mediaSession = MediaSession.Builder(context, p).build()
            p.addListener(object : Player.Listener {
                override fun onIsPlayingChanged(isPlaying: Boolean) = emitState()
                override fun onPlaybackStateChanged(state: Int) {
                    if (state == Player.STATE_ENDED) {
                        notifyListeners("ended", JSObject().put("id", currentId))
                    }
                }
            })
            player = p
            startTicker()
        }
    }

    @PluginMethod
    fun load(call: PluginCall) {
        val url = call.getString("url") ?: return call.reject("url required")
        currentId = call.getString("id")
        val startAt = call.getDouble("startAtSec") ?: 0.0
        bridge.activity.runOnUiThread {
            player?.apply {
                setMediaItem(MediaItem.fromUri(url))
                prepare()
                if (startAt > 0) seekTo((startAt * 1000).toLong())
            }
            call.resolve()
        }
    }

    @PluginMethod fun play(call: PluginCall) = ui { player?.play(); call.resolve() }
    @PluginMethod fun pause(call: PluginCall) = ui { player?.pause(); call.resolve() }
    @PluginMethod fun stop(call: PluginCall) = ui { player?.stop(); call.resolve() }
    @PluginMethod fun seek(call: PluginCall) = ui {
        player?.seekTo(((call.getDouble("positionSec") ?: 0.0) * 1000).toLong()); call.resolve()
    }
    @PluginMethod fun setRate(call: PluginCall) = ui {
        player?.setPlaybackSpeed((call.getDouble("rate") ?: 1.0).toFloat()); call.resolve()
    }

    @PluginMethod
    fun getState(call: PluginCall) = ui {
        val p = player
        call.resolve(JSObject()
            .put("id", currentId)
            .put("playing", p?.isPlaying ?: false)
            .put("positionSec", (p?.currentPosition ?: 0) / 1000.0)
            .put("durationSec", ((p?.duration ?: 0).coerceAtLeast(0)) / 1000.0)
            .put("buffered", (p?.bufferedPercentage ?: 0) / 100.0))
    }

    @PluginMethod
    fun setNowPlaying(call: PluginCall) {
        // MediaSession + MediaMetadata drive the lock-screen UI; set MediaItem.MediaMetadata
        // (title/artist/artworkUri) when you load(). TODO: update metadata here for live changes.
        call.resolve()
    }

    @PluginMethod fun enterPiP(call: PluginCall) { /* TODO: activity.enterPictureInPictureMode(...) */ call.resolve() }
    @PluginMethod fun exitPiP(call: PluginCall) { call.resolve() }

    // TODO: DownloadManager / WorkManager + Media3 DownloadService; emit "downloadProgress".
    @PluginMethod fun startDownload(call: PluginCall) { call.resolve() }
    @PluginMethod fun removeDownload(call: PluginCall) { call.resolve() }
    @PluginMethod fun listDownloads(call: PluginCall) { call.resolve(JSObject().put("items", org.json.JSONArray())) }

    // TODO: Google Cast (CastContext) + MediaRouter; emit "routesChanged".
    @PluginMethod fun listRoutes(call: PluginCall) { call.resolve(JSObject().put("routes", org.json.JSONArray())) }
    @PluginMethod fun showRoutePicker(call: PluginCall) { call.resolve() }
    @PluginMethod fun selectRoute(call: PluginCall) { call.resolve() }

    private fun emitState() {
        val p = player ?: return
        notifyListeners("stateChange", JSObject()
            .put("id", currentId)
            .put("playing", p.isPlaying)
            .put("positionSec", p.currentPosition / 1000.0)
            .put("durationSec", p.duration.coerceAtLeast(0) / 1000.0)
            .put("buffered", p.bufferedPercentage / 100.0))
    }

    private val tick = object : Runnable {
        override fun run() {
            if (player?.isPlaying == true) emitState()
            bridge.activity.window.decorView.postDelayed(this, 500)
        }
    }
    private fun startTicker() { bridge.activity.window.decorView.postDelayed(tick, 500) }

    private inline fun ui(crossinline block: () -> Unit) { bridge.activity.runOnUiThread { block() } }

    override fun handleOnDestroy() {
        mediaSession?.release(); player?.release(); player = null
    }
}
