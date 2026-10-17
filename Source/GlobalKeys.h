#pragma once

// Pembacaan status tombol keyboard secara global (walau window plugin tidak fokus).
// File ini sengaja TIDAK memakai JUCE supaya header OS tidak bentrok.
namespace LRGlobalKeys
{
    bool supported();                 // true di Windows & macOS
    bool isPadKeyDown (int padIndex); // padIndex 0..5 -> tombol 1..6 (baris angka + numpad)
    bool modifiersHeld();             // Ctrl / Alt / Win / Cmd sedang ditahan
    void beginHighResTimer();         // timer 1 ms (Windows) agar polling responsif
    void endHighResTimer();
}
