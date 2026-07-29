#include "serial_sink.h"

SerialSink::SerialSink(HardwareSerial* port, unsigned long baud)
    : serialPort(port), baudRate(baud) {}

void SerialSink::begin() {
    if (serialPort) {
        serialPort->begin(baudRate);
    }
}

size_t SerialSink::write(uint8_t c) {
    if (serialPort) {
        return serialPort->write(c);
    }
    return 0;
}
