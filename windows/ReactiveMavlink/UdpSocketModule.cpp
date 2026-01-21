#include "pch.h"
#include "NativeModules.h"

#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Security.Cryptography.Core.h>
#include <winrt/Windows.Storage.Streams.h>

#include <memory>
#include <mutex>
#include <string>

#include "UdpSocket.h" // from earlier (UdpSocket class)

using namespace winrt;
using namespace winrt::Microsoft::ReactNative;

static std::string ToUtf8(winrt::hstring const& hs) {
	std::wstring ws{ hs };
	// naive UTF-16->UTF-8 for ASCII host strings; OK for IPs like "127.0.0.1"
	std::string s(ws.begin(), ws.end());
	return s;
}

static winrt::hstring ToHstring(std::string const& s) {
	return winrt::hstring(std::wstring(s.begin(), s.end()));
}

static winrt::hstring Base64Encode(std::vector<uint8_t> const& bytes) {
	using namespace winrt::Windows::Security::Cryptography;
	using namespace winrt::Windows::Storage::Streams;

	auto buf = CryptographicBuffer::CreateFromByteArray(bytes);
	return CryptographicBuffer::EncodeToBase64String(buf);
}

static std::vector<uint8_t> Base64Decode(winrt::hstring const& b64) {
	using namespace winrt::Windows::Security::Cryptography;
	using namespace winrt::Windows::Storage::Streams;

	IBuffer buf = CryptographicBuffer::DecodeFromBase64String(b64);

	winrt::com_array<uint8_t> arr;
	CryptographicBuffer::CopyToByteArray(buf, arr);

	return std::vector<uint8_t>(arr.begin(), arr.end());
}

// Visible to JS as NativeModules.UdpSocket
REACT_MODULE(UdpSocketModule, L"UdpSocket");

struct UdpSocketModule {
	// Called once when module is created
	REACT_INIT(Initialize);
	void Initialize(ReactContext const& ctx) noexcept {
		m_ctx = ctx;
	}

	std::atomic<int32_t> m_listenerCount{ 0 };
	std::atomic<bool> m_jsReady{ false };

	REACT_EVENT(onMessage);
	std::function<void(JSValue)> onMessage;

	REACT_METHOD(addListener);
	void addListener(std::string /*eventName*/) noexcept {
		m_listenerCount.fetch_add(1, std::memory_order_relaxed);
	}

	REACT_METHOD(removeListeners);
	void removeListeners(int32_t count) noexcept {
		m_listenerCount.fetch_sub(count, std::memory_order_relaxed);
	}

	REACT_METHOD(setJsReady);
	void setJsReady(bool ready) noexcept {
		m_jsReady.store(ready, std::memory_order_release);
		OutputDebugStringA(ready ? "JS READY\n" : "JS NOT READY\n");
	}

	// bind(port): Promise<void>
	REACT_METHOD(bind);
	void bind(int32_t port, ReactPromise<void> promise) noexcept {
		try {
			std::scoped_lock lock(m_mutex);

			m_udp = std::make_unique<UdpSocket>();
			if (!m_udp->Bind(static_cast<uint16_t>(port))) {
				m_udp.reset();
				promise.Reject("UdpSocket.Bind failed");
				return;
			}

			m_udp->EnableBroadcast(true);

			// Start background receive thread
			m_udp->StartReceive([this](UdpSocket::Datagram const& d) {
				// Send event to JS
				// IMPORTANT: Emit via JS dispatcher to avoid threading issues
				auto payloadHost = d.remoteIp;
				auto payloadPort = d.remotePort;
				auto payloadB64 = Base64Encode(d.data);

				char buf[256];
				snprintf(
					buf,
					sizeof(buf),
					"UDP RX from %s:%u, %zu bytes\n",
					d.remoteIp.c_str(),
					d.remotePort,
					d.data.size()
				);
				OutputDebugStringA(buf);
				});

			promise.Resolve();
		}
		catch (...) {
			promise.Reject("Exception during bind()");
		}
	}

	// close(): void
	REACT_METHOD(close);
	void close() noexcept {
		std::scoped_lock lock(m_mutex);
		if (m_udp) {
			m_udp->Close();
			m_udp.reset();
		}
	}

	// send(host, port, base64): Promise<void>
	REACT_METHOD(sendBase64);
	void sendBase64(std::string host, int32_t port, std::string dataBase64, ReactPromise<void> promise) noexcept {
		try {
			std::unique_lock lock(m_mutex);
			if (!m_udp || !m_udp->IsOpen()) {
				promise.Reject("Socket is not bound");
				return;
			}

			// Decode base64 -> bytes (need hstring)
			auto bytes = Base64Decode(winrt::to_hstring(dataBase64));
			bool ok = m_udp->SendTo(host.c_str(), static_cast<uint16_t>(port), bytes.data(), bytes.size());
			if (!ok) {
				promise.Reject("sendto() failed");
				return;
			}
			promise.Resolve();
		}
		catch (...) {
			promise.Reject("Exception during sendBase64()");
		}
	}

private:
	ReactContext m_ctx{ nullptr };
	std::mutex m_mutex;
	std::unique_ptr<UdpSocket> m_udp;
};
