package com.smartflow.data

data class WifiNetwork(
    val ssid: String,
    val bssid: String,
    val rssi: Int,
    val auth: String
)
