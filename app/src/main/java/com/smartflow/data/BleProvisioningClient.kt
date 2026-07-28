package com.smartflow.data

import com.polidea.rxandroidble3.RxBleClient
import com.polidea.rxandroidble3.RxBleConnection
import io.reactivex.rxjava3.core.Observable
import io.reactivex.rxjava3.core.Single
import java.util.UUID

class BleProvisioningClient(private val rxBleClient: RxBleClient) {
    
    // UUIDs must match the firmware (ble_provisioning.cpp)
    private val SERVICE_UUID = UUID.fromString("4fafc201-1fb5-459e-8fcc-c5c9c331914b")
    private val CHAR_SSID_UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26a8")
    private val CHAR_PASS_UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26a9")
    private val CHAR_TOKEN_UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26aa")
    private val CHAR_COMMIT_UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26ab")
    private val CHAR_STATUS_UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26ac")
    private val CHAR_RESET_UUID  = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26ad")

    fun scanForDevices(): Observable<String> {
        return rxBleClient.scanBleDevices()
            .filter { result -> result.bleDevice.name?.startsWith("SmartFlow-") == true }
            .map { result -> result.bleDevice.macAddress }
            .distinct()
    }

    fun provisionDevice(macAddress: String, ssid: String, pass: String): Observable<String> {
        val device = rxBleClient.getBleDevice(macAddress)
        return device.establishConnection(false)
            .flatMap { connection ->
                connection.setupNotification(CHAR_STATUS_UUID)
                    .flatMap { notificationObservable ->
                        val tokenSingle = readCharacteristic(connection, CHAR_TOKEN_UUID)
                            .flatMap { tokenBytes ->
                                val token = String(tokenBytes, Charsets.UTF_8)
                                writeCharacteristic(connection, CHAR_SSID_UUID, ssid)
                                    .flatMap { writeCharacteristic(connection, CHAR_PASS_UUID, pass) }
                                    .flatMap { writeCharacteristic(connection, CHAR_COMMIT_UUID, "1") }
                                    .map { "TOKEN:$token" }
                            }
                        
                        Observable.merge(
                            tokenSingle.toObservable(),
                            notificationObservable.map { bytes -> String(bytes, Charsets.UTF_8) }
                        )
                    }
            }
    }

    fun sendFactoryReset(macAddress: String): Single<ByteArray> {
        val device = rxBleClient.getBleDevice(macAddress)
        return device.establishConnection(false)
            .firstOrError()
            .flatMap { connection ->
                writeCharacteristic(connection, CHAR_RESET_UUID, "RESET")
            }
    }

    private fun writeCharacteristic(
        connection: RxBleConnection,
        uuid: UUID,
        value: String
    ): Single<ByteArray> {
        return connection.writeCharacteristic(uuid, value.toByteArray(Charsets.UTF_8))
    }

    private fun readCharacteristic(
        connection: RxBleConnection,
        uuid: UUID
    ): Single<ByteArray> {
        return connection.readCharacteristic(uuid)
    }
}
