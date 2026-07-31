package com.smartflow.presentation.components

import androidx.compose.foundation.Image
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.RowScope
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import com.smartflow.R

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SmartFlowTopAppBar(
    title: String? = null,
    showBackButton: Boolean = false,
    onBackClick: () -> Unit = {},
    actions: @Composable RowScope.() -> Unit = {}
) {
    val themePref = com.smartflow.ui.theme.LocalThemePreference.current
    val isSystemDark = isSystemInDarkTheme()
    val wordmarkRes = when (themePref) {
        com.smartflow.ui.theme.ThemePreference.MONOCHROME_LIGHT -> com.smartflow.R.drawable.wordmark_black
        com.smartflow.ui.theme.ThemePreference.MONOCHROME_DARK -> com.smartflow.R.drawable.wordmark_white
        com.smartflow.ui.theme.ThemePreference.LIGHT -> com.smartflow.R.drawable.wordmark_light
        com.smartflow.ui.theme.ThemePreference.DARK -> com.smartflow.R.drawable.wordmark_dark
        com.smartflow.ui.theme.ThemePreference.SYSTEM_DEFAULT -> if (isSystemDark) com.smartflow.R.drawable.wordmark_dark else com.smartflow.R.drawable.wordmark_light
    }

    if (title != null) {
        CenterAlignedTopAppBar(
            title = {
                Text(
                    text = title,
                    style = MaterialTheme.typography.titleLarge
                )
            },
            navigationIcon = {
                if (showBackButton) {
                    IconButton(onClick = onBackClick) {
                        Icon(imageVector = Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                }
            },
            actions = actions,
            colors = TopAppBarDefaults.centerAlignedTopAppBarColors(
                containerColor = MaterialTheme.colorScheme.background,
                titleContentColor = MaterialTheme.colorScheme.onBackground,
                actionIconContentColor = MaterialTheme.colorScheme.onBackground,
                navigationIconContentColor = MaterialTheme.colorScheme.onBackground
            )
        )
    } else {
        TopAppBar(
            title = {
                Image(
                    painter = painterResource(id = wordmarkRes),
                    contentDescription = "SmartFlow Wordmark",
                    modifier = Modifier.height(24.dp).padding(start = 8.dp)
                )
            },
            navigationIcon = {
                if (showBackButton) {
                    IconButton(onClick = onBackClick) {
                        Icon(imageVector = Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                }
            },
            actions = actions,
            colors = TopAppBarDefaults.topAppBarColors(
                containerColor = MaterialTheme.colorScheme.background,
                titleContentColor = MaterialTheme.colorScheme.onBackground,
                actionIconContentColor = MaterialTheme.colorScheme.onBackground,
                navigationIconContentColor = MaterialTheme.colorScheme.onBackground
            )
        )
    }
}
