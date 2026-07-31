import sys

with open('app/src/main/java/com/smartflow/MainActivity.kt', 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
'''        setContent {
            com.smartflow.ui.theme.SmartFlowTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    AppNavigation(bleProvisioningClient, deviceRepository, cloudStore)
                }
            }
        }''', 
'''        setContent {
            val themePreference by settingsRepository.themePreferenceFlow.collectAsState(
                initial = com.smartflow.ui.theme.ThemePreference.SYSTEM_DEFAULT
            )
            com.smartflow.ui.theme.SmartFlowTheme(themePreference = themePreference) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    AppNavigation(bleProvisioningClient, deviceRepository, cloudStore, settingsRepository)
                }
            }
        }''')

content = content.replace(
'''fun AppNavigation(
    bleProvisioningClient: BleProvisioningClient,
    deviceRepository: DeviceRepository,
    cloudStore: FirebaseCloudStore
) {''',
'''fun AppNavigation(
    bleProvisioningClient: BleProvisioningClient,
    deviceRepository: DeviceRepository,
    cloudStore: FirebaseCloudStore,
    settingsRepository: com.smartflow.data.repository.SettingsRepository
) {''')

content = content.replace(
'''        composable("settings") {
            val viewModel = remember { SettingsViewModel() }
            SettingsScreen(
                onBack = { navController.popBackStack() },
                onManageAccount = { navController.navigate("account") }
            )
        }''',
'''        composable("settings") {
            val viewModel = remember { SettingsViewModel(settingsRepository) }
            SettingsScreen(
                viewModel = viewModel,
                onBack = { navController.popBackStack() },
                onManageAccount = { navController.navigate("account") }
            )
        }''')

# Clean up extra closing braces before setContent
content = content.replace('''
    }
    }
}

@Composable
fun AppNavigation''', '''
}

@Composable
fun AppNavigation''')

with open('app/src/main/java/com/smartflow/MainActivity.kt', 'w', encoding='utf-8') as f:
    f.write(content)
