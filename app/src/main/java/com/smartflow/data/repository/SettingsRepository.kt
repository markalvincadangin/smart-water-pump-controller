package com.smartflow.data.repository

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import com.smartflow.ui.theme.ThemePreference
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "settings")

class SettingsRepository(private val context: Context) {

    private val THEME_KEY = stringPreferencesKey("theme_preference")

    val themePreferenceFlow: Flow<ThemePreference> = context.dataStore.data
        .map { preferences ->
            val themeString = preferences[THEME_KEY] ?: ThemePreference.SYSTEM_DEFAULT.name
            try {
                ThemePreference.valueOf(themeString)
            } catch (e: IllegalArgumentException) {
                ThemePreference.SYSTEM_DEFAULT
            }
        }

    suspend fun saveThemePreference(themePreference: ThemePreference) {
        context.dataStore.edit { preferences ->
            preferences[THEME_KEY] = themePreference.name
        }
    }
}
