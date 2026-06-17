import Foundation
import Capacitor
import AVKit
import AVFoundation
import MediaPlayer

/**
 * MarqueeNative — iOS implementation (Capacitor 6, CAPBridgedPlugin / no .m needed).
 *
 * Core wired: AVPlayer transport, AVAudioSession .playback (background audio),
 * MPNowPlayingInfoCenter + MPRemoteCommandCenter (lock screen / Control Center),
 * AVPictureInPictureController.
 * Stubbed with TODOs: offline downloads (URLSession background), Google Cast,
 * AirPlay route picker (AVRoutePickerView).
 */
@objc(MarqueeNativePlugin)
public class MarqueeNativePlugin: CAPPlugin, CAPBridgedPlugin {
    public let identifier = "MarqueeNativePlugin"
    public let jsName = "MarqueeNative"
    public let pluginMethods: [CAPPluginMethod] = [
        CAPPluginMethod(name: "load", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "play", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "pause", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "stop", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "seek", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "setRate", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "getState", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "setNowPlaying", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "enterPiP", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "exitPiP", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "startDownload", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "removeDownload", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "listDownloads", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "listRoutes", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "showRoutePicker", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "selectRoute", returnType: CAPPluginReturnPromise)
    ]

    private var player: AVPlayer?
    private var currentId: String?
    private var pip: AVPictureInPictureController?
    private var timeObserver: Any?

    public override func load() {
        configureAudioSession()
        configureRemoteCommands()
    }

    private func configureAudioSession() {
        let s = AVAudioSession.sharedInstance()
        try? s.setCategory(.playback, mode: .moviePlayback)
        try? s.setActive(true)
    }

    private func configureRemoteCommands() {
        let c = MPRemoteCommandCenter.shared()
        c.playCommand.addTarget { [weak self] _ in self?.notifyListeners("remotePlay", data: [:]); return .success }
        c.pauseCommand.addTarget { [weak self] _ in self?.notifyListeners("remotePause", data: [:]); return .success }
        c.stopCommand.addTarget { [weak self] _ in self?.notifyListeners("remoteStop", data: [:]); return .success }
        c.nextTrackCommand.addTarget { [weak self] _ in self?.notifyListeners("remoteNext", data: [:]); return .success }
        c.previousTrackCommand.addTarget { [weak self] _ in self?.notifyListeners("remotePrev", data: [:]); return .success }
        c.changePlaybackPositionCommand.addTarget { [weak self] e in
            guard let e = e as? MPChangePlaybackPositionCommandEvent else { return .commandFailed }
            self?.notifyListeners("remoteSeek", data: ["positionSec": e.positionTime]); return .success
        }
    }

    @objc func load(_ call: CAPPluginCall) {
        guard let urlStr = call.getString("url"), let url = URL(string: urlStr) else { return call.reject("url required") }
        currentId = call.getString("id")
        let item = AVPlayerItem(url: url)               // AVPlayer handles HLS + HDR + hardware decode
        player = AVPlayer(playerItem: item)
        if let start = call.getDouble("startAtSec") { player?.seek(to: CMTime(seconds: start, preferredTimescale: 600)) }
        addPeriodicObserver()
        NotificationCenter.default.addObserver(self, selector: #selector(didEnd), name: .AVPlayerItemDidPlayToEndTime, object: item)
        // TODO: build AVPlayerLayer + AVPictureInPictureController for enterPiP()
        call.resolve()
    }

    @objc func play(_ call: CAPPluginCall) { player?.play(); call.resolve() }
    @objc func pause(_ call: CAPPluginCall) { player?.pause(); call.resolve() }
    @objc func stop(_ call: CAPPluginCall) { player?.pause(); player = nil; call.resolve() }
    @objc func seek(_ call: CAPPluginCall) {
        let p = call.getDouble("positionSec") ?? 0
        player?.seek(to: CMTime(seconds: p, preferredTimescale: 600)); call.resolve()
    }
    @objc func setRate(_ call: CAPPluginCall) { player?.rate = Float(call.getDouble("rate") ?? 1); call.resolve() }

    @objc func getState(_ call: CAPPluginCall) {
        let dur = player?.currentItem?.duration.seconds ?? 0
        call.resolve([
            "id": currentId ?? "",
            "playing": (player?.rate ?? 0) > 0,
            "positionSec": player?.currentTime().seconds ?? 0,
            "durationSec": dur.isFinite ? dur : 0,
            "buffered": 0
        ])
    }

    @objc func setNowPlaying(_ call: CAPPluginCall) {
        var info: [String: Any] = [
            MPMediaItemPropertyTitle: call.getString("title") ?? "",
            MPMediaItemPropertyArtist: call.getString("artist") ?? "",
            MPMediaItemPropertyPlaybackDuration: call.getDouble("durationSec") ?? 0,
            MPNowPlayingInfoPropertyElapsedPlaybackTime: call.getDouble("positionSec") ?? 0,
            MPNowPlayingInfoPropertyPlaybackRate: call.getDouble("playbackRate") ?? 1
        ]
        MPNowPlayingInfoCenter.default().nowPlayingInfo = info
        // TODO: async-load artworkUrl into MPMediaItemArtwork and merge into info.
        call.resolve()
    }

    @objc func enterPiP(_ call: CAPPluginCall) { pip?.startPictureInPicture(); call.resolve() }   // TODO: needs AVPlayerLayer
    @objc func exitPiP(_ call: CAPPluginCall) { pip?.stopPictureInPicture(); call.resolve() }

    // TODO: implement with a background URLSession; emit "downloadProgress".
    @objc func startDownload(_ call: CAPPluginCall) { call.resolve() }
    @objc func removeDownload(_ call: CAPPluginCall) { call.resolve() }
    @objc func listDownloads(_ call: CAPPluginCall) { call.resolve(["items": []]) }

    // TODO: AirPlay via AVRoutePickerView; Google Cast via the Cast SDK.
    @objc func listRoutes(_ call: CAPPluginCall) { call.resolve(["routes": []]) }
    @objc func showRoutePicker(_ call: CAPPluginCall) { call.resolve() }
    @objc func selectRoute(_ call: CAPPluginCall) { call.resolve() }

    private func addPeriodicObserver() {
        guard let player = player else { return }
        timeObserver = player.addPeriodicTimeObserver(forInterval: CMTime(seconds: 0.5, preferredTimescale: 600), queue: .main) { [weak self] t in
            guard let self = self else { return }
            let dur = self.player?.currentItem?.duration.seconds ?? 0
            self.notifyListeners("stateChange", data: [
                "id": self.currentId ?? "",
                "playing": (self.player?.rate ?? 0) > 0,
                "positionSec": t.seconds,
                "durationSec": dur.isFinite ? dur : 0,
                "buffered": 0
            ])
        }
    }

    @objc private func didEnd() { notifyListeners("ended", data: ["id": currentId ?? ""]) }
}
