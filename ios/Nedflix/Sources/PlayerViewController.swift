/*
 * Nedflix iOS - Media Player View Controller
 * AVPlayer-based playback for audio and video
 */

import UIKit
import AVFoundation
import AVKit

class PlayerViewController: UIViewController {

    // MARK: - Properties

    private let mediaItem: MediaItem
    private var player: AVPlayer?
    private var playerLayer: AVPlayerLayer?
    private var timeObserver: Any?
    private var isPlaying = false

    // MARK: - UI Components

    private lazy var playerContainerView: UIView = {
        let view = UIView()
        view.backgroundColor = .black
        return view
    }()

    private lazy var controlsView: UIView = {
        let view = UIView()
        view.backgroundColor = UIColor.black.withAlphaComponent(0.6)
        return view
    }()

    private lazy var titleLabel: UILabel = {
        let label = UILabel()
        label.textColor = .white
        label.font = .systemFont(ofSize: 20, weight: .semibold)
        label.textAlignment = .center
        return label
    }()

    private lazy var playPauseButton: UIButton = {
        let button = UIButton(type: .system)
        button.tintColor = .white
        button.setImage(UIImage(systemName: "play.fill", withConfiguration: UIImage.SymbolConfiguration(pointSize: 44)), for: .normal)
        button.addTarget(self, action: #selector(togglePlayPause), for: .touchUpInside)
        return button
    }()

    private lazy var seekBackButton: UIButton = {
        let button = UIButton(type: .system)
        button.tintColor = .white
        button.setImage(UIImage(systemName: "gobackward.10"), for: .normal)
        button.addTarget(self, action: #selector(seekBackward), for: .touchUpInside)
        return button
    }()

    private lazy var seekForwardButton: UIButton = {
        let button = UIButton(type: .system)
        button.tintColor = .white
        button.setImage(UIImage(systemName: "goforward.10"), for: .normal)
        button.addTarget(self, action: #selector(seekForward), for: .touchUpInside)
        return button
    }()

    private lazy var progressSlider: UISlider = {
        let slider = UISlider()
        slider.minimumTrackTintColor = UIColor(red: 0.9, green: 0.04, blue: 0.08, alpha: 1.0)
        slider.maximumTrackTintColor = .gray
        slider.addTarget(self, action: #selector(sliderValueChanged), for: .valueChanged)
        slider.addTarget(self, action: #selector(sliderTouchUp), for: [.touchUpInside, .touchUpOutside])
        return slider
    }()

    private lazy var currentTimeLabel: UILabel = {
        let label = UILabel()
        label.textColor = .white
        label.font = .monospacedDigitSystemFont(ofSize: 12, weight: .regular)
        label.text = "00:00"
        return label
    }()

    private lazy var durationLabel: UILabel = {
        let label = UILabel()
        label.textColor = .white
        label.font = .monospacedDigitSystemFont(ofSize: 12, weight: .regular)
        label.text = "00:00"
        label.textAlignment = .right
        return label
    }()

    private lazy var closeButton: UIButton = {
        let button = UIButton(type: .system)
        button.tintColor = .white
        button.setImage(UIImage(systemName: "xmark"), for: .normal)
        button.addTarget(self, action: #selector(close), for: .touchUpInside)
        return button
    }()

    private lazy var loadingIndicator: UIActivityIndicatorView = {
        let indicator = UIActivityIndicatorView(style: .large)
        indicator.color = .white
        indicator.hidesWhenStopped = true
        return indicator
    }()

    // MARK: - Init

    init(item: MediaItem) {
        self.mediaItem = item
        super.init(nibName: nil, bundle: nil)
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    // MARK: - Lifecycle

    override func viewDidLoad() {
        super.viewDidLoad()
        setupUI()
        setupPlayer()
    }

    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        playerLayer?.frame = playerContainerView.bounds
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        player?.pause()
        if let observer = timeObserver {
            player?.removeTimeObserver(observer)
        }
    }

    override var prefersStatusBarHidden: Bool { true }
    override var prefersHomeIndicatorAutoHidden: Bool { true }

    // MARK: - UI Setup

    private func setupUI() {
        view.backgroundColor = .black
        titleLabel.text = mediaItem.name

        view.addSubview(playerContainerView)
        view.addSubview(controlsView)
        view.addSubview(loadingIndicator)

        controlsView.addSubview(closeButton)
        controlsView.addSubview(titleLabel)
        controlsView.addSubview(seekBackButton)
        controlsView.addSubview(playPauseButton)
        controlsView.addSubview(seekForwardButton)
        controlsView.addSubview(progressSlider)
        controlsView.addSubview(currentTimeLabel)
        controlsView.addSubview(durationLabel)

        // Layout
        playerContainerView.translatesAutoresizingMaskIntoConstraints = false
        controlsView.translatesAutoresizingMaskIntoConstraints = false
        loadingIndicator.translatesAutoresizingMaskIntoConstraints = false
        closeButton.translatesAutoresizingMaskIntoConstraints = false
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        seekBackButton.translatesAutoresizingMaskIntoConstraints = false
        playPauseButton.translatesAutoresizingMaskIntoConstraints = false
        seekForwardButton.translatesAutoresizingMaskIntoConstraints = false
        progressSlider.translatesAutoresizingMaskIntoConstraints = false
        currentTimeLabel.translatesAutoresizingMaskIntoConstraints = false
        durationLabel.translatesAutoresizingMaskIntoConstraints = false

        NSLayoutConstraint.activate([
            playerContainerView.topAnchor.constraint(equalTo: view.topAnchor),
            playerContainerView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            playerContainerView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            playerContainerView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

            controlsView.topAnchor.constraint(equalTo: view.topAnchor),
            controlsView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            controlsView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            controlsView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

            loadingIndicator.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            loadingIndicator.centerYAnchor.constraint(equalTo: view.centerYAnchor),

            closeButton.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 16),
            closeButton.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),

            titleLabel.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 16),
            titleLabel.leadingAnchor.constraint(equalTo: closeButton.trailingAnchor, constant: 16),
            titleLabel.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -50),

            playPauseButton.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            playPauseButton.centerYAnchor.constraint(equalTo: view.centerYAnchor),

            seekBackButton.trailingAnchor.constraint(equalTo: playPauseButton.leadingAnchor, constant: -40),
            seekBackButton.centerYAnchor.constraint(equalTo: playPauseButton.centerYAnchor),

            seekForwardButton.leadingAnchor.constraint(equalTo: playPauseButton.trailingAnchor, constant: 40),
            seekForwardButton.centerYAnchor.constraint(equalTo: playPauseButton.centerYAnchor),

            currentTimeLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
            currentTimeLabel.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -20),

            durationLabel.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),
            durationLabel.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -20),

            progressSlider.leadingAnchor.constraint(equalTo: currentTimeLabel.trailingAnchor, constant: 8),
            progressSlider.trailingAnchor.constraint(equalTo: durationLabel.leadingAnchor, constant: -8),
            progressSlider.centerYAnchor.constraint(equalTo: currentTimeLabel.centerYAnchor),
        ])

        // Tap to toggle controls
        let tapGesture = UITapGestureRecognizer(target: self, action: #selector(toggleControls))
        view.addGestureRecognizer(tapGesture)
    }

    // MARK: - Player Setup

    private func setupPlayer() {
        loadingIndicator.startAnimating()

        guard let url = APIClient.shared.getStreamURL(for: mediaItem, quality: SettingsManager.shared.settings.videoQuality) else {
            showError("Could not get stream URL")
            return
        }

        let playerItem = AVPlayerItem(url: url)
        player = AVPlayer(playerItem: playerItem)

        playerLayer = AVPlayerLayer(player: player)
        playerLayer?.videoGravity = .resizeAspect
        playerContainerView.layer.addSublayer(playerLayer!)

        // Observe playback state
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(playerDidFinishPlaying),
            name: .AVPlayerItemDidPlayToEndTime,
            object: playerItem
        )

        // Time observer
        let interval = CMTime(seconds: 0.5, preferredTimescale: CMTimeScale(NSEC_PER_SEC))
        timeObserver = player?.addPeriodicTimeObserver(forInterval: interval, queue: .main) { [weak self] time in
            self?.updateProgress(time: time)
        }

        // KVO for buffering state
        playerItem.addObserver(self, forKeyPath: "playbackBufferEmpty", options: .new, context: nil)
        playerItem.addObserver(self, forKeyPath: "playbackLikelyToKeepUp", options: .new, context: nil)

        // Start playback
        player?.play()
        isPlaying = true
        updatePlayPauseButton()
        loadingIndicator.stopAnimating()
    }

    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey : Any]?, context: UnsafeMutableRawPointer?) {
        if keyPath == "playbackBufferEmpty" {
            loadingIndicator.startAnimating()
        } else if keyPath == "playbackLikelyToKeepUp" {
            loadingIndicator.stopAnimating()
        }
    }

    // MARK: - Actions

    @objc private func togglePlayPause() {
        if isPlaying {
            player?.pause()
        } else {
            player?.play()
        }
        isPlaying.toggle()
        updatePlayPauseButton()
    }

    @objc private func seekBackward() {
        guard let player = player else { return }
        let currentTime = player.currentTime()
        let newTime = CMTimeSubtract(currentTime, CMTime(seconds: 10, preferredTimescale: 1))
        player.seek(to: newTime)
    }

    @objc private func seekForward() {
        guard let player = player else { return }
        let currentTime = player.currentTime()
        let newTime = CMTimeAdd(currentTime, CMTime(seconds: 10, preferredTimescale: 1))
        player.seek(to: newTime)
    }

    @objc private func sliderValueChanged() {
        guard let player = player,
              let duration = player.currentItem?.duration else { return }

        let totalSeconds = CMTimeGetSeconds(duration)
        let targetTime = CMTime(seconds: Double(progressSlider.value) * totalSeconds, preferredTimescale: 1)
        currentTimeLabel.text = formatTime(CMTimeGetSeconds(targetTime))
    }

    @objc private func sliderTouchUp() {
        guard let player = player,
              let duration = player.currentItem?.duration else { return }

        let totalSeconds = CMTimeGetSeconds(duration)
        let targetTime = CMTime(seconds: Double(progressSlider.value) * totalSeconds, preferredTimescale: 1)
        player.seek(to: targetTime)
    }

    @objc private func toggleControls() {
        UIView.animate(withDuration: 0.3) {
            self.controlsView.alpha = self.controlsView.alpha > 0 ? 0 : 1
        }
    }

    @objc private func close() {
        dismiss(animated: true)
    }

    @objc private func playerDidFinishPlaying() {
        isPlaying = false
        updatePlayPauseButton()
    }

    // MARK: - Helpers

    private func updatePlayPauseButton() {
        let imageName = isPlaying ? "pause.fill" : "play.fill"
        playPauseButton.setImage(UIImage(systemName: imageName, withConfiguration: UIImage.SymbolConfiguration(pointSize: 44)), for: .normal)
    }

    private func updateProgress(time: CMTime) {
        guard let duration = player?.currentItem?.duration else { return }

        let currentSeconds = CMTimeGetSeconds(time)
        let totalSeconds = CMTimeGetSeconds(duration)

        if totalSeconds > 0 {
            progressSlider.value = Float(currentSeconds / totalSeconds)
            currentTimeLabel.text = formatTime(currentSeconds)
            durationLabel.text = formatTime(totalSeconds)
        }
    }

    private func formatTime(_ seconds: Double) -> String {
        guard seconds.isFinite else { return "00:00" }
        let mins = Int(seconds) / 60
        let secs = Int(seconds) % 60
        return String(format: "%02d:%02d", mins, secs)
    }

    private func showError(_ message: String) {
        loadingIndicator.stopAnimating()
        let alert = UIAlertController(title: "Error", message: message, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: "OK", style: .default) { [weak self] _ in
            self?.dismiss(animated: true)
        })
        present(alert, animated: true)
    }
}
