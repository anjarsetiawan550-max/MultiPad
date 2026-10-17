#include "GlobalKeys.h"

#if defined (_WIN32)

 #if defined (_MSC_VER)
  #pragma comment (lib, "user32.lib")
  #pragma comment (lib, "winmm.lib")
 #endif

 extern "C"
 {
    __declspec (dllimport) short        __stdcall GetAsyncKeyState (int vKey);
    __declspec (dllimport) unsigned int __stdcall timeBeginPeriod (unsigned int uPeriod);
    __declspec (dllimport) unsigned int __stdcall timeEndPeriod (unsigned int uPeriod);
 }

namespace LRGlobalKeys
{
    bool supported() { return true; }

    static bool down (int vk) { return (GetAsyncKeyState (vk) & 0x8000) != 0; }

    bool isPadKeyDown (int i)
    {
        if (i < 0 || i > 5) return false;
        return down (0x31 + i)      // '1'..'6'
            || down (0x61 + i);     // VK_NUMPAD1..6
    }

    bool modifiersHeld()
    {
        return down (0x11)   // VK_CONTROL
            || down (0x12)   // VK_MENU (Alt)
            || down (0x5B)   // VK_LWIN
            || down (0x5C);  // VK_RWIN
    }

    void beginHighResTimer() { timeBeginPeriod (1); }
    void endHighResTimer()   { timeEndPeriod (1); }
}

#elif defined (__APPLE__)

 #include <ApplicationServices/ApplicationServices.h>

namespace LRGlobalKeys
{
    bool supported() { return true; }

    static bool down (int code)
    {
        return CGEventSourceKeyState (kCGEventSourceStateCombinedSessionState, (CGKeyCode) code);
    }

    bool isPadKeyDown (int i)
    {
        static const int row[6]    = { 18, 19, 20, 21, 23, 22 };   // 1..6
        static const int keypad[6] = { 83, 84, 85, 86, 87, 88 };   // numpad 1..6
        if (i < 0 || i > 5) return false;
        return down (row[i]) || down (keypad[i]);
    }

    bool modifiersHeld()
    {
        return down (55) || down (54)    // Cmd
            || down (59) || down (62)    // Ctrl
            || down (58) || down (61);   // Option
    }

    void beginHighResTimer() {}
    void endHighResTimer() {}
}

#else

namespace LRGlobalKeys
{
    bool supported()           { return false; }
    bool isPadKeyDown (int)    { return false; }
    bool modifiersHeld()       { return false; }
    void beginHighResTimer()   {}
    void endHighResTimer()     {}
}

#endif
