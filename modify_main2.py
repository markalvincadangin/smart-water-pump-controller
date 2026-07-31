import sys

with open('app/src/main/java/com/smartflow/MainActivity.kt', 'r', encoding='utf-8') as f:
    content = f.read()

imports_to_add = '''import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Notifications
import androidx.compose.material.icons.filled.Settings
import androidx.navigation.compose.currentBackStackEntryAsState
'''
content = content.replace('import androidx.compose.runtime.setValue\n', 'import androidx.compose.runtime.setValue\n' + imports_to_add)

old_nav_host = '    NavHost(navController = navController, startDestination = startDestination) {'
new_nav_host = '''    val snackbarHostState = remember { SnackbarHostState() }
    val navBackStackEntry by navController.currentBackStackEntryAsState()
    val currentRoute = navBackStackEntry?.destination?.route

    val bottomNavRoutes = listOf("device_list", "notifications", "settings")
    val showBottomNav = currentRoute in bottomNavRoutes

    Scaffold(
        snackbarHost = { SnackbarHost(snackbarHostState) },
        bottomBar = {
            if (showBottomNav) {
                NavigationBar(
                    containerColor = MaterialTheme.colorScheme.surface
                ) {
                    NavigationBarItem(
                        icon = { Icon(Icons.Default.Home, contentDescription = "Devices") },
                        label = { Text("Devices") },
                        selected = currentRoute == "device_list",
                        onClick = {
                            if (currentRoute != "device_list") {
                                navController.navigate("device_list") {
                                    popUpTo("device_list") { saveState = true }
                                    launchSingleTop = true
                                    restoreState = true
                                }
                            }
                        }
                    )
                    NavigationBarItem(
                        icon = { Icon(Icons.Default.Notifications, contentDescription = "Alerts") },
                        label = { Text("Alerts") },
                        selected = currentRoute == "notifications",
                        onClick = {
                            if (currentRoute != "notifications") {
                                navController.navigate("notifications") {
                                    popUpTo("device_list") { saveState = true }
                                    launchSingleTop = true
                                    restoreState = true
                                }
                            }
                        }
                    )
                    NavigationBarItem(
                        icon = { Icon(Icons.Default.Settings, contentDescription = "Settings") },
                        label = { Text("Settings") },
                        selected = currentRoute == "settings",
                        onClick = {
                            if (currentRoute != "settings") {
                                navController.navigate("settings") {
                                    popUpTo("device_list") { saveState = true }
                                    launchSingleTop = true
                                    restoreState = true
                                }
                            }
                        }
                    )
                }
            }
        }
    ) { innerPadding ->
        NavHost(
            navController = navController, 
            startDestination = startDestination,
            modifier = Modifier.padding(innerPadding)
        ) {'''
content = content.replace(old_nav_host, new_nav_host)

old_closing = '''            }
        }
    }
}
'''
new_closing = '''            }
        }
    }
    }
}
'''
if content.endswith(old_closing):
    content = content[:-len(old_closing)] + new_closing
else:
    # Use a safer replacement for the end of the file
    content = content + "\n    }\n}\n"

with open('app/src/main/java/com/smartflow/MainActivity.kt', 'w', encoding='utf-8') as f:
    f.write(content)
