package com.smartflow.data

import com.polidea.rxandroidble3.RxBleClient
import com.polidea.rxandroidble3.RxBleConnection
import io.reactivex.rxjava3.core.Observable
import io.reactivex.rxjava3.core.Single
import org.json.JSONObject
import java.util.UUID

class BleProvisioningClient(private val rxBleClient: RxBleClient) {
    
    private val SERVICE_UUID = UUID.fromString("4fafc201-1fb5-459e-8fcc-c5c9c331914b")
    private val CHAR_CMD_UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26a8")
    private val CHAR_RESP_UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26a9")

    fun scanForDevices(): Observable<String> {
        return rxBleClient.scanBleDevices()
            .filter { result -> result.bleDevice.name?.startsWith("SmartFlow-") == true }
            .map { result -> result.bleDevice.macAddress }
            .distinct()
    }

    // Shared connection logic
    private fun connect(macAddress: String): Observable<RxBleConnection> {
        return rxBleClient.getBleDevice(macAddress)
            .establishConnection(false)
            .flatMapSingle { conn -> conn.requestMtu(512).map { conn } }
    }

    fun scanWifiNetworks(macAddress: String): Observable<JSONObject> {
        return connect(macAddress).flatMap { connection ->
            writeWithResponses(connection) {
                val cmd = JSONObject().apply {
                    put("v", 1)
                    put("id", UUID.randomUUID().toString())
                    put("cmd", "scan_wifi")
                }
                writeCharacteristic(connection, CHAR_CMD_UUID, cmd.toString())
            }
        }
    }

    fun provisionDevice(macAddress: String, ssid: String, pass: String): Observable<JSONObject> {
        return connect(macAddress).flatMap { connection ->
            writeWithResponses(connection) {
                val tokenReqId = UUID.randomUUID().toString()
                val tokenCmd = JSONObject().apply {
                    put("v", 1)
                    put("id", tokenReqId)
                    put("cmd", "get_token")
                }
                val connectReqId = UUID.randomUUID().toString()
                val connectCmd = JSONObject().apply {
                    put("v", 1)
                    put("id", connectReqId)
                    put("cmd", "connect_wifi")
                    put("ssid", ssid)
                    put("password", pass)
                }
                writeCharacteristic(connection, CHAR_CMD_UUID, tokenCmd.toString())
                    .flatMap { writeCharacteristic(connection, CHAR_CMD_UUID, connectCmd.toString()) }
            }
        }
    }

    /** Retrieves the proof from a temporary transfer/release BLE session only. */
    fun retrieveOwnershipPairingProof(macAddress: String): Observable<JSONObject> {
        return connect(macAddress).flatMap { connection ->
            writeWithResponses(connection) {
                val cmd = JSONObject().apply {
                    put("v", 1)
                    put("id", UUID.randomUUID().toString())
                    put("cmd", "get_token")
                }
                writeCharacteristic(connection, CHAR_CMD_UUID, cmd.toString())
            }
        }
    }

    private fun writeWithResponses(
        connection: RxBleConnection,
        write: () -> Single<ByteArray>
    ): Observable<JSONObject> {
        return connection.setupNotification(CHAR_RESP_UUID).flatMap { notifications ->
            Observable.create { emitter ->
                val notificationDisposable = notifications.subscribe(
                    { bytes ->
                        try {
                            emitter.onNext(JSONObject(String(bytes, Charsets.UTF_8)))
                        } catch (error: Exception) {
                            emitter.onError(error)
                        }
                    },
                    { error -> emitter.onError(error) }
                )
                val writeDisposable = write().subscribe(
                    { },
                    { error -> emitter.onError(error) }
                )
                emitter.setCancellable {
                    notificationDisposable.dispose()
                    writeDisposable.dispose()
                }
            }
        }
    }

    fun sendFactoryReset(macAddress: String): Single<ByteArray> {
        val device = rxBleClient.getBleDevice(macAddress)
        return device.establishConnection(false)
            .firstOrError()
            .flatMap { connection ->
                val cmd = JSONObject().apply {
                    put("v", 1)
                    put("id", UUID.randomUUID().toString())
                    put("cmd", "reset")
                }
                writeCharacteristic(connection, CHAR_CMD_UUID, cmd.toString())
            }
    }

    private fun writeCharacteristic(
        connection: RxBleConnection,
        uuid: UUID,
        value: String
    ): Single<ByteArray> {
        return connection.writeCharacteristic(uuid, value.toByteArray(Charsets.UTF_8))
    }
}
