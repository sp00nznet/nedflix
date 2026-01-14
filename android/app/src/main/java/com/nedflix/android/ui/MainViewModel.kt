/*
 * Nedflix Android - Main ViewModel
 */

package com.nedflix.android.ui

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.nedflix.android.data.*
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

data class MainUiState(
    val isLoading: Boolean = false,
    val error: String? = null,
    val mediaItems: List<MediaItem> = emptyList(),
    val currentPath: String = "/",
    val currentLibrary: Library = Library.MUSIC,
    val playingItem: MediaItem? = null,
    val showSettings: Boolean = false,
    val settings: AppSettings = AppSettings()
)

class MainViewModel : ViewModel() {

    private val apiClient = ApiClient()
    private val settingsManager = SettingsManager.instance

    private val _uiState = MutableStateFlow(MainUiState())
    val uiState: StateFlow<MainUiState> = _uiState.asStateFlow()

    init {
        loadSettings()
        loadLibrary(Library.MUSIC)
    }

    private fun loadSettings() {
        _uiState.update { it.copy(settings = settingsManager.settings) }
    }

    fun loadLibrary(library: Library, path: String = "/") {
        viewModelScope.launch {
            _uiState.update {
                it.copy(
                    isLoading = true,
                    error = null,
                    currentLibrary = library,
                    currentPath = path
                )
            }

            try {
                val items = apiClient.browse(library, path)
                _uiState.update {
                    it.copy(
                        isLoading = false,
                        mediaItems = items
                    )
                }
            } catch (e: Exception) {
                _uiState.update {
                    it.copy(
                        isLoading = false,
                        error = e.message ?: "Failed to load content"
                    )
                }
            }
        }
    }

    fun onMediaItemClick(item: MediaItem) {
        if (item.isDirectory) {
            loadLibrary(_uiState.value.currentLibrary, item.path)
        } else {
            playMedia(item)
        }
    }

    private fun playMedia(item: MediaItem) {
        _uiState.update { it.copy(playingItem = item) }
    }

    fun stopPlayback() {
        _uiState.update { it.copy(playingItem = null) }
    }

    fun navigateToSettings() {
        _uiState.update { it.copy(showSettings = true) }
    }

    fun closeSettings() {
        _uiState.update { it.copy(showSettings = false) }
    }

    fun updateSettings(settings: AppSettings) {
        settingsManager.settings = settings
        _uiState.update { it.copy(settings = settings) }
    }

    fun goBack(): Boolean {
        val currentPath = _uiState.value.currentPath
        if (currentPath != "/" && currentPath.contains("/")) {
            val parentPath = currentPath.substringBeforeLast("/").ifEmpty { "/" }
            loadLibrary(_uiState.value.currentLibrary, parentPath)
            return true
        }
        return false
    }
}
