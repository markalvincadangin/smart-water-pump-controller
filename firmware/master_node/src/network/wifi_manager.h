#pragma once

class WifiManager {
public:
    static void init();
    static void connect();
    static void loop();
    static bool isConnected();
    static int getRssi();
};
