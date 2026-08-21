package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.repository.SettingsRepository
import com.smartflow.ui.theme.ThemePreference
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class SettingsViewModel(
    private val settingsRepository: SettingsRepository
) : ViewModel() {

    val themePreference: StateFlow<ThemePreference> = settingsRepository.themePreferenceFlow
        .stateIn(
            scope = viewModelScope,
            started = SharingStarted.WhileSubscribed(5000),
            initialValue = ThemePreference.SYSTEM_DEFAULT
        )

    fun setThemePreference(preference: ThemePreference) {
        viewModelScope.launch {
            settingsRepository.saveThemePreference(preference)
        }
    }
}
