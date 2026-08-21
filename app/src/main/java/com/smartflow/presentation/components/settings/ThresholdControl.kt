package com.smartflow.presentation.components.settings

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import com.smartflow.ui.theme.LocalSpacing

@Composable
fun ThresholdControl(
    title: String,
    value: Float,
    onValueChange: (Float) -> Unit,
    valueRange: ClosedFloatingPointRange<Float>,
    steps: Int = 0,
    unit: String,
    modifier: Modifier = Modifier,
    description: String? = null,
    onErrorChange: (Boolean) -> Unit = {}
) {
    val spacing = LocalSpacing.current
    var textValue by remember(value) { mutableStateOf(if (value % 1f == 0f) value.toInt().toString() else value.toString()) }
    
    val parsedValue = textValue.toFloatOrNull()
    val isError = parsedValue == null || parsedValue !in valueRange
    
    LaunchedEffect(isError) {
        onErrorChange(isError)
    }

    Column(
        modifier = modifier
            .fillMaxWidth()
            .padding(vertical = spacing.small),
        verticalArrangement = Arrangement.spacedBy(spacing.extraSmall)
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = title,
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onSurface
            )
            
            OutlinedTextField(
                value = textValue,
                onValueChange = { newValue ->
                    // Only allow digits and optionally a decimal point
                    if (newValue.isEmpty() || newValue.matches(Regex("^\\d*\\.?\\d*$"))) {
                        textValue = newValue
                        val newFloat = newValue.toFloatOrNull()
                        if (newFloat != null && newFloat in valueRange) {
                            onValueChange(newFloat)
                        }
                    }
                },
                suffix = { Text(unit) },
                isError = isError,
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                singleLine = true,
                modifier = Modifier.width(120.dp)
            )
        }
        
        if (isError) {
            Text(
                text = "Must be between ${if (valueRange.start % 1f == 0f) valueRange.start.toInt() else valueRange.start} and ${if (valueRange.endInclusive % 1f == 0f) valueRange.endInclusive.toInt() else valueRange.endInclusive}",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.error,
                modifier = Modifier.align(Alignment.End)
            )
        }

        Slider(
            value = value,
            onValueChange = {
                onValueChange(it)
                textValue = if (it % 1f == 0f) it.toInt().toString() else it.toString()
            },
            valueRange = valueRange,
            steps = steps,
            modifier = Modifier.fillMaxWidth()
        )

        if (description != null) {
            Text(
                text = description,
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}
