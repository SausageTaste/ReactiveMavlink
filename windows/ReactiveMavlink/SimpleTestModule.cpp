#include "pch.h"
#include "NativeModules.h"

using namespace winrt::Microsoft::ReactNative;

REACT_MODULE(SimpleTestModule, L"SimpleTest");

struct SimpleTestModule {
    int m_value{ 42 };

    REACT_SYNC_METHOD(getValue);
    int getValue() noexcept {
        return m_value;
    }
};
