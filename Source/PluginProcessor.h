#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include <memory>

namespace mp
{
    constexpr int    numPads          = 6;
    constexpr double maxSampleSeconds = 10.0;   // sampel hanya perlu dibaca 10 detik pertama
    constexpr int    midiBaseNote     = 36;     // C1..F1 -> PAD 1..6
    constexpr int    maxNameLength    = 12;

    struct Sample
    {
        juce::AudioBuffer<float> data;          // 1 atau 2 channel
        double sampleRate = 44100.0;
        int numFrames() const noexcept { return data.getNumSamples(); }
    };

    struct Voice
    {
        std::shared_ptr<const Sample> sample;
        bool    active = false;
        double  pos  = 0.0;     // posisi di sampel sumber (frame)
        double  step = 1.0;     // frame sumber per frame output
        juce::int64 t   = 0;    // frame output yang sudah lewat
        juce::int64 len = 0;    // total frame output
        int     fadeIn  = 0;
        int     fadeOut = 0;
        float   gain    = 1.0f;
        int     releaseLeft  = 0;   // > 0 : voice sedang dipotong halus (retrigger)
        int     releaseTotal = 0;
    };

    struct PadState
    {
        juce::SpinLock lock;                          // melindungi 'sample'
        std::shared_ptr<const Sample> sample;

        std::atomic<bool>  hasSample { false };
        std::atomic<bool>  pendingTrigger { false };
        std::atomic<int>   triggerCount { 0 };
        std::atomic<float> progress { -1.0f };        // <0 = tidak sedang bunyi

        std::atomic<float>* durParam = nullptr;
        std::atomic<float>* inParam  = nullptr;
        std::atomic<float>* outParam = nullptr;
        std::atomic<float>* volParam = nullptr;

        Voice voices[2];                              // hanya dipakai audio thread
    };
}

class MultiPadProcessor : public juce::AudioProcessor,
                          public juce::ChangeBroadcaster
{
public:
    MultiPadProcessor();
    ~MultiPadProcessor() override;

    // --- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MULTI PAD LR PRO"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Pad API (aman dipanggil dari thread mana pun)
    void triggerPad (int index);
    bool hasSample (int index) const;
    float getPadProgress (int index) const;
    int   getTriggerCount (int index) const;

    // --- Pad API (message thread)
    juce::String getPadName (int index) const;
    void         setPadName (int index, const juce::String& newName);
    juce::String getSampleInfo (int index) const;       // "nama_file.wav  (3.2 s)"
    juce::Result loadSampleFromFile (int index, const juce::File& file);
    void         clearSample (int index);
    juce::String getLastDir() const;
    juce::String getFileWildcard() const;               // filter dialog 'semua format audio'
    void         setLastDir (const juce::String& dir);

    // --- Keyboard global
    void setGlobalKeysEnabled (bool shouldBeEnabled);
    bool getGlobalKeysEnabled() const { return globalKeysEnabled.load(); }
    bool isKeyboardPollingActive() const;               // dipakai editor: kalau true, editor tidak ikut menangani tombol
    bool shouldPollKeys() const;                        // dipakai poller thread
    void pushKeyboardSuspend()  { suspendCount.fetch_add (1); }
    void popKeyboardSuspend()   { if (suspendCount.load() > 0) suspendCount.fetch_sub (1); }

    // --- UI
    int  getEditorSize() const { return editorSize.load(); }
    void setEditorSize (int s) { editorSize.store (s); }

    static juce::String paramId (int pad, const char* suffix);

    juce::AudioProcessorValueTreeState apvts;

private:
    class KeyPoller;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static std::shared_ptr<mp::Sample> readSample (juce::AudioFormatReader& reader);
    static juce::MemoryBlock encodeSample (const mp::Sample& s);
    std::shared_ptr<mp::Sample> decodeFromMemory (const juce::MemoryBlock& mb);

    void installSample (int index, std::shared_ptr<mp::Sample> s, const juce::String& fileName,
                        const juce::String& path, const juce::MemoryBlock& encoded);

    void startVoice (int index);
    void renderAll (juce::AudioBuffer<float>& buffer, int start, int num);
    static void renderVoice (mp::Voice& v, float* outL, float* outR, int n);

    struct PadMeta
    {
        juce::String name;
        juce::String fileName;
        juce::String path;
        double seconds = 0.0;
        juce::MemoryBlock encoded;
    };

    std::array<mp::PadState, mp::numPads> pads;

    mutable juce::CriticalSection metaLock;
    std::array<PadMeta, mp::numPads> meta;
    juce::String lastDir;

    juce::AudioFormatManager formatManager;
    double hostSampleRate = 44100.0;

    std::atomic<bool> globalKeysEnabled { true };
    std::atomic<int>  suspendCount { 0 };
    std::atomic<int>  editorSize { 700 };

    std::unique_ptr<KeyPoller> keyPoller;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiPadProcessor)
};
