#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

class UdpSocket {
public:
    struct Datagram {
        std::string remoteIp;
        uint16_t remotePort = 0;
        std::vector<uint8_t> data;
    };

    using OnDatagramFn = std::function<void(const Datagram&)>;

    UdpSocket() = default;
    ~UdpSocket() { Close(); }

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    // Bind to localPort on all interfaces (0.0.0.0). Returns true on success.
    bool Bind(uint16_t localPort);

    // Send bytes to remote endpoint. Returns true on success.
    bool SendTo(const char* remoteIp, uint16_t remotePort, const uint8_t* data, size_t size);

    // Optional: allow broadcast (255.255.255.255 / subnet broadcast)
    bool EnableBroadcast(bool enable);

    // Start background receive loop. Safe to call once after Bind().
    void StartReceive(OnDatagramFn cb);

    // Stop thread and close socket. Safe to call multiple times.
    void Close();

    bool IsOpen() const { return m_sock != INVALID_SOCKET; }

private:
    void ReceiveLoop();

    SOCKET m_sock = INVALID_SOCKET;
    std::atomic<bool> m_running{ false };
    std::thread m_thread;
    OnDatagramFn m_onDatagram;

    bool m_wsaStarted = false;
};
