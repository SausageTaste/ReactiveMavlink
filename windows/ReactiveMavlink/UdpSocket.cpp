#include "pch.h"
#include "UdpSocket.h"
#include <stdexcept>

static bool StartWSA(bool& startedFlag) {
    if (startedFlag) return true;
    WSADATA wsa{};
    const int r = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (r != 0) return false;
    startedFlag = true;
    return true;
}

bool UdpSocket::Bind(uint16_t localPort) {
    Close();

    if (!StartWSA(m_wsaStarted))
        return false;

    m_sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == INVALID_SOCKET) {
        Close();
        return false;
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(localPort);

    if (::bind(m_sock, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
        Close();
        return false;
    }

    return true;
}

bool UdpSocket::EnableBroadcast(bool enable) {
    if (m_sock == INVALID_SOCKET) return false;
    BOOL opt = enable ? TRUE : FALSE;
    return ::setsockopt(m_sock, SOL_SOCKET, SO_BROADCAST,
        reinterpret_cast<const char*>(&opt), sizeof(opt)) == 0;
}

void UdpSocket::StartReceive(OnDatagramFn cb) {
    if (m_sock == INVALID_SOCKET) {
        throw std::runtime_error("UdpSocket::StartReceive called before Bind()");
    }
    if (m_running.load()) return;

    m_onDatagram = std::move(cb);
    m_running = true;
    m_thread = std::thread([this] { ReceiveLoop(); });
}

bool UdpSocket::SendTo(const char* remoteIp, uint16_t remotePort, const uint8_t* data, size_t size) {
    if (m_sock == INVALID_SOCKET) return false;
    if (!remoteIp || (!data && size != 0)) return false;

    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(remotePort);

    // IPv4 only for simplicity. If you need IPv6, we’ll extend to sockaddr_storage.
    if (::inet_pton(AF_INET, remoteIp, &to.sin_addr) != 1) {
        return false;
    }

    const int sent = ::sendto(
        m_sock,
        reinterpret_cast<const char*>(data),
        static_cast<int>(size),
        0,
        reinterpret_cast<const sockaddr*>(&to),
        sizeof(to));

    return sent != SOCKET_ERROR;
}

void UdpSocket::ReceiveLoop() {
    // Keep the buffer modest; UDP datagrams larger than MTU will fragment anyway.
    std::vector<uint8_t> buf(2048);

    while (m_running.load()) {
        sockaddr_in from{};
        int fromLen = sizeof(from);

        const int n = ::recvfrom(
            m_sock,
            reinterpret_cast<char*>(buf.data()),
            static_cast<int>(buf.size()),
            0,
            reinterpret_cast<sockaddr*>(&from),
            &fromLen);

        if (n == SOCKET_ERROR) {
            const int err = WSAGetLastError();
            // When socket is closed from another thread, recvfrom commonly errors out.
            if (!m_running.load())
                break;

            // For a “simple” class: just continue on transient errors.
            // You can add an OnError callback later.
            if (err == WSAEINTR || err == WSAENETDOWN || err == WSAECONNRESET) {
                continue;
            }
            continue;
        }

        if (n <= 0) continue;

        char ip[INET_ADDRSTRLEN]{};
        ::inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));
        const uint16_t port = ntohs(from.sin_port);

        Datagram d;
        d.remoteIp = ip;
        d.remotePort = port;
        d.data.assign(buf.begin(), buf.begin() + n);

        if (m_onDatagram) {
            m_onDatagram(d);
        }
    }
}

void UdpSocket::Close() {
    m_running = false;

    if (m_sock != INVALID_SOCKET) {
        ::closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }

    if (m_wsaStarted) {
        WSACleanup();
        m_wsaStarted = false;
    }

    m_onDatagram = nullptr;
}
