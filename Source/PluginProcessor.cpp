#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GlobalKeys.h"

//==============================================================================
// Thread yang memeriksa tombol 1..6 secara global (di luar fokus window plugin).
class MultiPadProcessor::KeyPoller : public juce::Thread
{
public:
    explicit KeyPoller (MultiPadProcessor& p) : juce::Thread ("MultiPad key poller"), proc (p) {}

    void run() override
    {
        LRGlobalKeys::beginHighResTimer();
        bool wasDown[mp::numPads] = {};

        while (! threadShouldExit())
        {
            const bool active = proc.shouldPollKeys();
            const bool mods   = active && LRGlobalKeys::modifiersHeld();

            for (int i = 0; i < mp::numPads; ++i)
            {
                const bool d = active && ! mods && LRGlobalKeys::isPadKeyDown (i);
                if (d && ! wasDown[i])
                    proc.triggerPad (i);
                wasDown[i] = d;
            }
            wait (3);
        }

        LRGlobalKeys::endHighResTimer();
    }

private:
    MultiPadProcessor& proc;
};

//==============================================================================
juce::String MultiPadProcessor::paramId (int pad, const char* suffix)
{
    return "p" + juce::String (pad + 1) + "_" + suffix;
}

juce::AudioProcessorValueTreeState::ParameterLayout MultiPadProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto secText = [] (float v, int) { return String (v, 1) + " s"; };
    auto offText = [] (float v, int) { return v < 0.05f ? String ("Off") : String (v, 1) + " s"; };
    auto fromText = [] (const String& t) { return t.getFloatValue(); };

    for (int i = 0; i < mp::numPads; ++i)
    {
        const String n = "PAD " + String (i + 1) + " ";

        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { paramId (i, "dur"), 1 }, n + "Durasi",
            NormalisableRange<float> (1.0f, 10.0f, 0.1f), 10.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction (secText)
                                           .withValueFromStringFunction (fromText)));

        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { paramId (i, "in"), 1 }, n + "Feed In",
            NormalisableRange<float> (0.0f, 5.0f, 0.1f), 0.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction (offText)
                                           .withValueFromStringFunction (fromText)));

        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { paramId (i, "out"), 1 }, n + "Feed Out",
            NormalisableRange<float> (0.0f, 5.0f, 0.1f), 0.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction (offText)
                                           .withValueFromStringFunction (fromText)));

        layout.add (std::make_unique<AudioParameterInt> (
            ParameterID { paramId (i, "vol"), 1 }, n + "Volume", 1, 100, 100));
    }
    return layout;
}

//==============================================================================
MultiPadProcessor::MultiPadProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MultiPadState", createLayout())
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < mp::numPads; ++i)
    {
        auto& p = pads[(size_t) i];
        p.durParam = apvts.getRawParameterValue (paramId (i, "dur"));
        p.inParam  = apvts.getRawParameterValue (paramId (i, "in"));
        p.outParam = apvts.getRawParameterValue (paramId (i, "out"));
        p.volParam = apvts.getRawParameterValue (paramId (i, "vol"));
        meta[(size_t) i].name = "PAD " + juce::String (i + 1);
    }

    keyPoller = std::make_unique<KeyPoller> (*this);
    keyPoller->startThread();
}

MultiPadProcessor::~MultiPadProcessor()
{
    if (keyPoller != nullptr)
        keyPoller->stopThread (2000);
}

juce::AudioProcessorEditor* MultiPadProcessor::createEditor()
{
    return new MultiPadEditor (*this);
}

bool MultiPadProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MultiPadProcessor::prepareToPlay (double sampleRate, int)
{
    hostSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
}

//==============================================================================
// Keyboard global
bool MultiPadProcessor::isKeyboardPollingActive() const
{
    return globalKeysEnabled.load() && LRGlobalKeys::supported();
}

bool MultiPadProcessor::shouldPollKeys() const
{
    return isKeyboardPollingActive() && suspendCount.load() == 0;
}

void MultiPadProcessor::setGlobalKeysEnabled (bool shouldBeEnabled)
{
    globalKeysEnabled.store (shouldBeEnabled);
}

//==============================================================================
// Pad API
void MultiPadProcessor::triggerPad (int index)
{
    if (index < 0 || index >= mp::numPads) return;
    auto& p = pads[(size_t) index];
    p.triggerCount.fetch_add (1);
    p.pendingTrigger.store (true);
}

bool MultiPadProcessor::hasSample (int index) const
{
    return index >= 0 && index < mp::numPads && pads[(size_t) index].hasSample.load();
}

float MultiPadProcessor::getPadProgress (int index) const
{
    return (index >= 0 && index < mp::numPads) ? pads[(size_t) index].progress.load() : -1.0f;
}

int MultiPadProcessor::getTriggerCount (int index) const
{
    return (index >= 0 && index < mp::numPads) ? pads[(size_t) index].triggerCount.load() : 0;
}

juce::String MultiPadProcessor::getPadName (int index) const
{
    const juce::ScopedLock sl (metaLock);
    return meta[(size_t) index].name;
}

void MultiPadProcessor::setPadName (int index, const juce::String& newName)
{
    auto n = newName.trim().substring (0, mp::maxNameLength);
    if (n.isEmpty())
        n = "PAD " + juce::String (index + 1);
    {
        const juce::ScopedLock sl (metaLock);
        meta[(size_t) index].name = n;
    }
    sendChangeMessage();
}

juce::String MultiPadProcessor::getSampleInfo (int index) const
{
    const juce::ScopedLock sl (metaLock);
    const auto& m = meta[(size_t) index];
    if (m.fileName.isEmpty()) return {};
    return m.fileName + "  (" + juce::String (m.seconds, 1) + " s)";
}

juce::String MultiPadProcessor::getLastDir() const
{
    const juce::ScopedLock sl (metaLock);
    return lastDir;
}

juce::String MultiPadProcessor::getFileWildcard() const
{
    return formatManager.getWildcardForAllFormats()
         + ";*.wav;*.wave;*.aif;*.aiff;*.aifc;*.flac;*.ogg;*.oga;*.mp3;*.m4a;*.aac;*.wma;*.mp4;*.caf;*.opus";
}

void MultiPadProcessor::setLastDir (const juce::String& dir)
{
    const juce::ScopedLock sl (metaLock);
    lastDir = dir;
}

//==============================================================================
// Sampel
std::shared_ptr<mp::Sample> MultiPadProcessor::readSample (juce::AudioFormatReader& reader)
{
    if (reader.lengthInSamples <= 0 || reader.sampleRate <= 0.0)
        return nullptr;

    const int numCh = (int) juce::jmin (2u, reader.numChannels);
    if (numCh < 1) return nullptr;

    const auto maxFrames = (juce::int64) (reader.sampleRate * mp::maxSampleSeconds);
    const int frames = (int) juce::jmin (reader.lengthInSamples, maxFrames);
    if (frames <= 0) return nullptr;

    auto s = std::make_shared<mp::Sample>();
    s->sampleRate = reader.sampleRate;
    s->data.setSize (numCh, frames);
    s->data.clear();

    if (! reader.read (&s->data, 0, frames, 0, true, numCh > 1))
        return nullptr;

    return s;
}

juce::MemoryBlock MultiPadProcessor::encodeSample (const mp::Sample& s)
{
    juce::MemoryBlock mb;
    juce::FlacAudioFormat flac;
    {
        auto* mos = new juce::MemoryOutputStream (mb, false);   // dimiliki oleh writer
        std::unique_ptr<juce::AudioFormatWriter> w (flac.createWriterFor (
            mos, s.sampleRate, (unsigned int) s.data.getNumChannels(), 24, juce::StringPairArray(), 0));

        if (w != nullptr)
            w->writeFromAudioSampleBuffer (s.data, 0, s.data.getNumSamples());
        else
            mb.setSize (0);
    }   // writer ditutup di sini -> data FLAC lengkap
    return mb;
}

std::shared_ptr<mp::Sample> MultiPadProcessor::decodeFromMemory (const juce::MemoryBlock& mb)
{
    if (mb.getSize() == 0) return nullptr;
    std::unique_ptr<juce::AudioFormatReader> r (
        formatManager.createReaderFor (std::make_unique<juce::MemoryInputStream> (mb, false)));
    if (r == nullptr) return nullptr;
    return readSample (*r);
}

void MultiPadProcessor::installSample (int index, std::shared_ptr<mp::Sample> s, const juce::String& fileName,
                                       const juce::String& path, const juce::MemoryBlock& encoded)
{
    auto& p = pads[(size_t) index];
    std::shared_ptr<const mp::Sample> old;
    {
        const juce::SpinLock::ScopedLockType l (p.lock);
        old = std::move (p.sample);
        p.sample = s;
    }
    p.hasSample.store (s != nullptr);

    {
        const juce::ScopedLock sl (metaLock);
        auto& m = meta[(size_t) index];
        m.fileName = s != nullptr ? fileName : juce::String();
        m.path     = s != nullptr ? path : juce::String();
        m.seconds  = s != nullptr ? (double) s->numFrames() / s->sampleRate : 0.0;
        m.encoded  = s != nullptr ? encoded : juce::MemoryBlock();
    }

    old.reset();            // dibebaskan di message thread
    sendChangeMessage();
}

juce::Result MultiPadProcessor::loadSampleFromFile (int index, const juce::File& file)
{
    if (index < 0 || index >= mp::numPads)
        return juce::Result::fail ("Pad tidak valid.");
    if (! file.existsAsFile())
        return juce::Result::fail ("File tidak ditemukan.");

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
        return juce::Result::fail ("Format audio ini tidak bisa dibaca:\n" + file.getFileName());

    auto s = readSample (*reader);
    if (s == nullptr)
        return juce::Result::fail ("File audio kosong atau rusak:\n" + file.getFileName());

    auto encoded = encodeSample (*s);
    installSample (index, s, file.getFileName(), file.getFullPathName(), encoded);
    return juce::Result::ok();
}

void MultiPadProcessor::clearSample (int index)
{
    if (index < 0 || index >= mp::numPads) return;
    installSample (index, nullptr, {}, {}, {});
}

//==============================================================================
// Audio
void MultiPadProcessor::startVoice (int index)
{
    auto& p = pads[(size_t) index];

    std::shared_ptr<const mp::Sample> s;
    {
        const juce::SpinLock::ScopedTryLockType l (p.lock);
        if (! l.isLocked()) return;
        s = p.sample;
    }
    if (s == nullptr || s->numFrames() <= 0) return;

    // potong halus voice lama (retrigger tanpa klik)
    const int relSamples = juce::jmax (16, (int) (0.005 * hostSampleRate));
    for (auto& v : p.voices)
        if (v.active && v.releaseTotal == 0)
        {
            v.releaseTotal = relSamples;
            v.releaseLeft  = relSamples;
        }

    mp::Voice* slot = nullptr;
    for (auto& v : p.voices)
        if (! v.active) { slot = &v; break; }
    if (slot == nullptr)
        slot = (p.voices[1].releaseLeft < p.voices[0].releaseLeft) ? &p.voices[1] : &p.voices[0];

    const double durSec   = (double) p.durParam->load();
    const double inSec    = (double) p.inParam->load();
    const double outSec   = (double) p.outParam->load();
    const float  vol      = p.volParam->load() / 100.0f;

    const double srcRate  = s->sampleRate;
    const juce::int64 srcFrames = juce::jmin ((juce::int64) s->numFrames(), (juce::int64) (durSec * srcRate));
    const double step     = srcRate / hostSampleRate;
    const juce::int64 len = (juce::int64) ((double) srcFrames / step);
    if (len < 8) return;

    int fIn  = (int) (inSec  * hostSampleRate);
    int fOut = (int) (outSec * hostSampleRate);
    if ((juce::int64) fIn + fOut > len)
    {
        const double k = (double) len / (double) ((juce::int64) fIn + fOut);
        fIn  = (int) (fIn  * k);
        fOut = (int) (fOut * k);
    }
    const int minOut = juce::jmin ((int) (0.002 * hostSampleRate), (int) (len / 2));
    fOut = juce::jmax (fOut, minOut);          // fade-out minimum 2 ms agar tidak klik di ujung

    slot->sample = s;
    slot->active = true;
    slot->pos = 0.0;
    slot->step = step;
    slot->t = 0;
    slot->len = len;
    slot->fadeIn = fIn;
    slot->fadeOut = fOut;
    slot->gain = vol;
    slot->releaseLeft = 0;
    slot->releaseTotal = 0;
}

void MultiPadProcessor::renderVoice (mp::Voice& v, float* outL, float* outR, int n)
{
    const auto& buf = v.sample->data;
    const int srcN  = buf.getNumSamples();
    const float* l  = buf.getReadPointer (0);
    const float* r  = buf.getReadPointer (buf.getNumChannels() > 1 ? 1 : 0);

    for (int i = 0; i < n; ++i)
    {
        if (v.t >= v.len) { v.active = false; break; }

        const int i0 = (int) v.pos;
        if (i0 >= srcN)   { v.active = false; break; }
        const int i1 = juce::jmin (i0 + 1, srcN - 1);
        const float fr = (float) (v.pos - (double) i0);

        const float sl = l[i0] + (l[i1] - l[i0]) * fr;
        const float sr = r[i0] + (r[i1] - r[i0]) * fr;

        float g = v.gain;
        if (v.fadeIn > 0 && v.t < v.fadeIn)
            g *= (float) v.t / (float) v.fadeIn;

        const juce::int64 remaining = v.len - v.t;
        if (v.fadeOut > 0 && remaining < v.fadeOut)
            g *= (float) remaining / (float) v.fadeOut;

        if (v.releaseTotal > 0)
        {
            g *= (float) v.releaseLeft / (float) v.releaseTotal;
            if (--v.releaseLeft <= 0) v.active = false;
        }

        outL[i] += sl * g;
        outR[i] += sr * g;

        v.pos += v.step;
        ++v.t;
        if (! v.active) break;
    }

    if (! v.active)
        v.sample.reset();
}

void MultiPadProcessor::renderAll (juce::AudioBuffer<float>& buffer, int start, int num)
{
    if (num <= 0) return;
    float* outL = buffer.getWritePointer (0) + start;
    float* outR = buffer.getWritePointer (buffer.getNumChannels() > 1 ? 1 : 0) + start;

    for (auto& p : pads)
        for (auto& v : p.voices)
            if (v.active && v.sample != nullptr)
                renderVoice (v, outL, outR, num);
}

void MultiPadProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 1 || numSamples <= 0)
        return;

    // trigger dari mouse / keyboard
    for (int i = 0; i < mp::numPads; ++i)
        if (pads[(size_t) i].pendingTrigger.exchange (false))
            startVoice (i);

    // trigger dari MIDI (akurat sampai ke sampel)
    int pos = 0;
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (! msg.isNoteOn()) continue;

        const int idx = msg.getNoteNumber() - mp::midiBaseNote;
        if (idx < 0 || idx >= mp::numPads) continue;

        const int at = juce::jlimit (pos, numSamples, metadata.samplePosition);
        renderAll (buffer, pos, at - pos);
        pos = at;

        pads[(size_t) idx].triggerCount.fetch_add (1);
        startVoice (idx);
    }
    renderAll (buffer, pos, numSamples - pos);

    // progres playback untuk tampilan
    for (auto& p : pads)
    {
        float prog = -1.0f;
        for (auto& v : p.voices)
            if (v.active && v.releaseTotal == 0 && v.len > 0)
                prog = (float) ((double) v.t / (double) v.len);
        p.progress.store (prog);
    }
}

//==============================================================================
// State
void MultiPadProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    for (int i = state.getNumChildren(); --i >= 0;)
        if (state.getChild (i).hasType ("PAD"))
            state.removeChild (i, nullptr);

    state.setProperty ("globalKeys", globalKeysEnabled.load(), nullptr);
    state.setProperty ("uiSize", editorSize.load(), nullptr);

    {
        const juce::ScopedLock sl (metaLock);
        state.setProperty ("lastDir", lastDir, nullptr);

        for (int i = 0; i < mp::numPads; ++i)
        {
            const auto& m = meta[(size_t) i];
            juce::ValueTree pad ("PAD");
            pad.setProperty ("id", i, nullptr);
            pad.setProperty ("name", m.name, nullptr);
            pad.setProperty ("file", m.fileName, nullptr);
            pad.setProperty ("path", m.path, nullptr);
            if (m.encoded.getSize() > 0)
                pad.setProperty ("audio", juce::var (m.encoded), nullptr);
            state.appendChild (pad, nullptr);
        }
    }

    juce::MemoryOutputStream mos (destData, false);
    state.writeToStream (mos);
}

void MultiPadProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (! tree.isValid() || ! tree.hasType (apvts.state.getType()))
        return;

    globalKeysEnabled.store ((bool) tree.getProperty ("globalKeys", true));
    editorSize.store ((int) tree.getProperty ("uiSize", 700));
    setLastDir (tree.getProperty ("lastDir", "").toString());

    // ambil data pad lalu buang dari tree sebelum dipasang ke apvts
    juce::Array<juce::ValueTree> padTrees;
    for (int i = tree.getNumChildren(); --i >= 0;)
        if (tree.getChild (i).hasType ("PAD"))
        {
            padTrees.add (tree.getChild (i));
            tree.removeChild (i, nullptr);
        }

    apvts.replaceState (tree);

    for (auto& pt : padTrees)
    {
        const int idx = (int) pt.getProperty ("id", -1);
        if (idx < 0 || idx >= mp::numPads) continue;

        {
            const juce::ScopedLock sl (metaLock);
            meta[(size_t) idx].name = pt.getProperty ("name", "PAD " + juce::String (idx + 1)).toString();
        }

        const auto fileName = pt.getProperty ("file", "").toString();
        const auto path     = pt.getProperty ("path", "").toString();

        std::shared_ptr<mp::Sample> s;
        juce::MemoryBlock encoded;

        const juce::var audioVar = pt.getProperty ("audio");
        if (auto* mb = audioVar.getBinaryData())
        {
            encoded = *mb;
            s = decodeFromMemory (encoded);
        }

        if (s == nullptr && path.isNotEmpty())
        {
            juce::File f (path);
            if (f.existsAsFile())
            {
                std::unique_ptr<juce::AudioFormatReader> r (formatManager.createReaderFor (f));
                if (r != nullptr)
                {
                    s = readSample (*r);
                    if (s != nullptr) encoded = encodeSample (*s);
                }
            }
        }

        if (s != nullptr)
            installSample (idx, s, fileName, path, encoded);
        else
            installSample (idx, nullptr, {}, {}, {});
    }

    sendChangeMessage();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultiPadProcessor();
}
