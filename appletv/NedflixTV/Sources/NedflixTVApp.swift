/*
 * Nedflix for Apple TV - Main App
 * Full-featured media streaming app for tvOS
 */

import SwiftUI
import AVKit

@main
struct NedflixTVApp: App {
    @StateObject private var appState = AppState()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(appState)
        }
    }
}

// MARK: - App State
class AppState: ObservableObject {
    @Published var serverURL: String = UserDefaults.standard.string(forKey: "serverURL") ?? ""
    @Published var token: String? = UserDefaults.standard.string(forKey: "authToken")
    @Published var isAuthenticated: Bool = false
    @Published var mediaLibrary: [MediaItem] = []
    @Published var categories: [MediaCategory] = []
    @Published var isLoading: Bool = false
    @Published var error: String?

    init() {
        isAuthenticated = token != nil && !serverURL.isEmpty
    }

    func saveSettings() {
        UserDefaults.standard.set(serverURL, forKey: "serverURL")
        if let token = token {
            UserDefaults.standard.set(token, forKey: "authToken")
        }
    }
}

// MARK: - Models
struct MediaItem: Identifiable, Codable {
    let id: String
    let name: String
    let path: String
    let type: String
    let duration: Double?
    let thumbnail: String?
    let description: String?
    let year: Int?
    let rating: Double?
}

struct MediaCategory: Identifiable, Codable {
    let id: String
    let name: String
    let items: [MediaItem]
}

// MARK: - Content View
struct ContentView: View {
    @EnvironmentObject var appState: AppState

    var body: some View {
        NavigationView {
            if appState.isAuthenticated {
                MediaBrowserView()
            } else {
                SetupView()
            }
        }
    }
}

// MARK: - Setup View
struct SetupView: View {
    @EnvironmentObject var appState: AppState
    @State private var serverURL: String = ""
    @State private var username: String = ""
    @State private var password: String = ""
    @State private var isConnecting: Bool = false
    @State private var errorMessage: String?

    var body: some View {
        VStack(spacing: 40) {
            // Logo
            Image(systemName: "play.tv.fill")
                .font(.system(size: 120))
                .foregroundColor(.red)

            Text("Nedflix")
                .font(.system(size: 72, weight: .bold))

            Text("Connect to your media server")
                .font(.headline)
                .foregroundColor(.secondary)

            // Server URL
            VStack(alignment: .leading, spacing: 8) {
                Text("Server URL")
                    .font(.caption)
                    .foregroundColor(.secondary)
                TextField("http://192.168.1.100:3000", text: $serverURL)
                    .textFieldStyle(.plain)
                    .padding()
                    .background(Color.gray.opacity(0.2))
                    .cornerRadius(8)
            }
            .frame(width: 600)

            // Username
            VStack(alignment: .leading, spacing: 8) {
                Text("Username (optional)")
                    .font(.caption)
                    .foregroundColor(.secondary)
                TextField("username", text: $username)
                    .textFieldStyle(.plain)
                    .padding()
                    .background(Color.gray.opacity(0.2))
                    .cornerRadius(8)
            }
            .frame(width: 600)

            // Password
            VStack(alignment: .leading, spacing: 8) {
                Text("Password (optional)")
                    .font(.caption)
                    .foregroundColor(.secondary)
                SecureField("password", text: $password)
                    .textFieldStyle(.plain)
                    .padding()
                    .background(Color.gray.opacity(0.2))
                    .cornerRadius(8)
            }
            .frame(width: 600)

            // Connect button
            Button(action: connect) {
                if isConnecting {
                    ProgressView()
                        .progressViewStyle(CircularProgressViewStyle())
                } else {
                    Text("Connect")
                        .font(.title2)
                        .frame(width: 200)
                }
            }
            .disabled(serverURL.isEmpty || isConnecting)
            .buttonStyle(.borderedProminent)
            .tint(.red)

            if let error = errorMessage {
                Text(error)
                    .foregroundColor(.red)
                    .font(.caption)
            }
        }
        .padding(60)
        .onAppear {
            serverURL = appState.serverURL
        }
    }

    private func connect() {
        isConnecting = true
        errorMessage = nil

        APIClient.shared.configure(serverURL: serverURL)

        Task {
            do {
                if !username.isEmpty {
                    let token = try await APIClient.shared.authenticate(username: username, password: password)
                    await MainActor.run {
                        appState.token = token
                    }
                }

                // Test connection
                _ = try await APIClient.shared.fetchLibrary()

                await MainActor.run {
                    appState.serverURL = serverURL
                    appState.isAuthenticated = true
                    appState.saveSettings()
                    isConnecting = false
                }
            } catch {
                await MainActor.run {
                    errorMessage = "Connection failed: \(error.localizedDescription)"
                    isConnecting = false
                }
            }
        }
    }
}

// MARK: - Media Browser View
struct MediaBrowserView: View {
    @EnvironmentObject var appState: AppState
    @State private var categories: [MediaCategory] = []
    @State private var isLoading: Bool = true
    @State private var selectedItem: MediaItem?

    var body: some View {
        ScrollView {
            LazyVStack(alignment: .leading, spacing: 40) {
                // Header
                HStack {
                    Image(systemName: "play.tv.fill")
                        .font(.system(size: 48))
                        .foregroundColor(.red)
                    Text("Nedflix")
                        .font(.largeTitle)
                        .bold()
                    Spacer()
                    Button(action: { refreshLibrary() }) {
                        Image(systemName: "arrow.clockwise")
                    }
                    NavigationLink(destination: SettingsView()) {
                        Image(systemName: "gearshape")
                    }
                }
                .padding(.horizontal, 80)

                if isLoading {
                    ProgressView("Loading library...")
                        .frame(maxWidth: .infinity)
                        .padding(100)
                } else if categories.isEmpty {
                    VStack(spacing: 20) {
                        Image(systemName: "film")
                            .font(.system(size: 80))
                            .foregroundColor(.secondary)
                        Text("No media found")
                            .font(.title2)
                            .foregroundColor(.secondary)
                    }
                    .frame(maxWidth: .infinity)
                    .padding(100)
                } else {
                    ForEach(categories) { category in
                        MediaRowView(category: category, onSelect: { item in
                            selectedItem = item
                        })
                    }
                }
            }
            .padding(.vertical, 40)
        }
        .fullScreenCover(item: $selectedItem) { item in
            PlayerView(item: item)
        }
        .onAppear {
            refreshLibrary()
        }
    }

    private func refreshLibrary() {
        isLoading = true
        Task {
            do {
                let library = try await APIClient.shared.fetchLibrary()
                await MainActor.run {
                    categories = library
                    isLoading = false
                }
            } catch {
                await MainActor.run {
                    appState.error = error.localizedDescription
                    isLoading = false
                }
            }
        }
    }
}

// MARK: - Media Row View
struct MediaRowView: View {
    let category: MediaCategory
    let onSelect: (MediaItem) -> Void

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text(category.name)
                .font(.title2)
                .fontWeight(.semibold)
                .padding(.horizontal, 80)

            ScrollView(.horizontal, showsIndicators: false) {
                LazyHStack(spacing: 30) {
                    ForEach(category.items) { item in
                        MediaCardView(item: item)
                            .onTapGesture {
                                onSelect(item)
                            }
                    }
                }
                .padding(.horizontal, 80)
            }
        }
    }
}

// MARK: - Media Card View
struct MediaCardView: View {
    let item: MediaItem
    @State private var isFocused: Bool = false

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            ZStack {
                // Thumbnail
                AsyncImage(url: URL(string: item.thumbnail ?? "")) { image in
                    image
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                } placeholder: {
                    Rectangle()
                        .fill(Color.gray.opacity(0.3))
                        .overlay(
                            Image(systemName: item.type == "video" ? "film" : "music.note")
                                .font(.system(size: 40))
                                .foregroundColor(.gray)
                        )
                }
                .frame(width: 300, height: 170)
                .clipped()
                .cornerRadius(8)

                // Play overlay
                if isFocused {
                    Color.black.opacity(0.3)
                        .cornerRadius(8)
                    Image(systemName: "play.fill")
                        .font(.system(size: 40))
                        .foregroundColor(.white)
                }
            }
            .scaleEffect(isFocused ? 1.05 : 1.0)
            .animation(.easeInOut(duration: 0.2), value: isFocused)

            Text(item.name)
                .font(.caption)
                .lineLimit(2)
                .frame(width: 300, alignment: .leading)

            if let duration = item.duration {
                Text(formatDuration(duration))
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }
        }
        .focusable(true) { focused in
            isFocused = focused
        }
    }

    private func formatDuration(_ seconds: Double) -> String {
        let mins = Int(seconds) / 60
        let secs = Int(seconds) % 60
        return String(format: "%d:%02d", mins, secs)
    }
}

// MARK: - Player View
struct PlayerView: View {
    let item: MediaItem
    @Environment(\.dismiss) var dismiss
    @State private var player: AVPlayer?
    @State private var isPlaying: Bool = true
    @State private var currentTime: Double = 0
    @State private var duration: Double = 0
    @State private var showControls: Bool = true

    var body: some View {
        ZStack {
            // Video player
            if let player = player {
                VideoPlayer(player: player)
                    .edgesIgnoringSafeArea(.all)
            } else {
                Color.black
                    .edgesIgnoringSafeArea(.all)
                ProgressView("Loading...")
            }

            // Controls overlay
            if showControls {
                PlayerControlsOverlay(
                    title: item.name,
                    isPlaying: isPlaying,
                    currentTime: currentTime,
                    duration: duration,
                    onPlayPause: togglePlayPause,
                    onSeek: seek,
                    onClose: { dismiss() }
                )
            }
        }
        .onAppear {
            setupPlayer()
        }
        .onDisappear {
            player?.pause()
            player = nil
        }
        .onReceive(Timer.publish(every: 0.5, on: .main, in: .common).autoconnect()) { _ in
            updateProgress()
        }
        .gesture(
            TapGesture().onEnded { _ in
                withAnimation {
                    showControls.toggle()
                }
            }
        )
    }

    private func setupPlayer() {
        guard let url = APIClient.shared.getStreamURL(for: item) else {
            return
        }

        let playerItem = AVPlayerItem(url: url)
        player = AVPlayer(playerItem: playerItem)
        player?.play()

        // Get duration
        Task {
            if let asset = player?.currentItem?.asset {
                let dur = try? await asset.load(.duration)
                if let dur = dur {
                    await MainActor.run {
                        duration = CMTimeGetSeconds(dur)
                    }
                }
            }
        }
    }

    private func togglePlayPause() {
        if isPlaying {
            player?.pause()
        } else {
            player?.play()
        }
        isPlaying.toggle()
    }

    private func seek(to time: Double) {
        let cmTime = CMTime(seconds: time, preferredTimescale: 1)
        player?.seek(to: cmTime)
    }

    private func updateProgress() {
        guard let player = player else { return }
        currentTime = CMTimeGetSeconds(player.currentTime())
    }
}

// MARK: - Player Controls Overlay
struct PlayerControlsOverlay: View {
    let title: String
    let isPlaying: Bool
    let currentTime: Double
    let duration: Double
    let onPlayPause: () -> Void
    let onSeek: (Double) -> Void
    let onClose: () -> Void

    var body: some View {
        VStack {
            // Top bar
            HStack {
                Button(action: onClose) {
                    Image(systemName: "xmark")
                        .font(.title2)
                }
                Spacer()
                Text(title)
                    .font(.title3)
                    .fontWeight(.semibold)
                Spacer()
            }
            .padding(40)
            .background(
                LinearGradient(
                    gradient: Gradient(colors: [Color.black.opacity(0.8), Color.clear]),
                    startPoint: .top,
                    endPoint: .bottom
                )
            )

            Spacer()

            // Center controls
            HStack(spacing: 60) {
                Button(action: { onSeek(max(0, currentTime - 10)) }) {
                    Image(systemName: "gobackward.10")
                        .font(.system(size: 44))
                }

                Button(action: onPlayPause) {
                    Image(systemName: isPlaying ? "pause.fill" : "play.fill")
                        .font(.system(size: 60))
                }

                Button(action: { onSeek(min(duration, currentTime + 10)) }) {
                    Image(systemName: "goforward.10")
                        .font(.system(size: 44))
                }
            }

            Spacer()

            // Bottom progress bar
            VStack(spacing: 16) {
                // Progress slider
                GeometryReader { geometry in
                    ZStack(alignment: .leading) {
                        // Track
                        Rectangle()
                            .fill(Color.gray.opacity(0.5))
                            .frame(height: 6)
                            .cornerRadius(3)

                        // Progress
                        Rectangle()
                            .fill(Color.red)
                            .frame(width: duration > 0 ? CGFloat(currentTime / duration) * geometry.size.width : 0, height: 6)
                            .cornerRadius(3)
                    }
                }
                .frame(height: 6)
                .focusable(true)

                // Time labels
                HStack {
                    Text(formatTime(currentTime))
                        .font(.caption)
                        .monospacedDigit()
                    Spacer()
                    Text(formatTime(duration))
                        .font(.caption)
                        .monospacedDigit()
                }
            }
            .padding(40)
            .background(
                LinearGradient(
                    gradient: Gradient(colors: [Color.clear, Color.black.opacity(0.8)]),
                    startPoint: .top,
                    endPoint: .bottom
                )
            )
        }
    }

    private func formatTime(_ seconds: Double) -> String {
        guard seconds.isFinite else { return "00:00" }
        let mins = Int(seconds) / 60
        let secs = Int(seconds) % 60
        return String(format: "%02d:%02d", mins, secs)
    }
}

// MARK: - Settings View
struct SettingsView: View {
    @EnvironmentObject var appState: AppState
    @Environment(\.dismiss) var dismiss

    var body: some View {
        Form {
            Section("Server") {
                LabeledContent("URL", value: appState.serverURL)
                LabeledContent("Status", value: appState.isAuthenticated ? "Connected" : "Disconnected")
            }

            Section {
                Button("Disconnect") {
                    appState.token = nil
                    appState.isAuthenticated = false
                    UserDefaults.standard.removeObject(forKey: "authToken")
                    dismiss()
                }
                .foregroundColor(.red)
            }

            Section("About") {
                LabeledContent("Version", value: "1.0.0")
                LabeledContent("Build", value: "1")
            }
        }
        .navigationTitle("Settings")
    }
}
