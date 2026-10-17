#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "LRLookAndFeel.h"

class MultiPadEditor;

// Koordinat desain (kanvas 1254 x 1254) dari asset kit.
namespace layout
{
    struct Rect { int x, y, w, h; };

    constexpr int canvas = 1254;

    constexpr Rect pads[6] = {
        { 152, 409, 306, 302 }, { 476, 409, 305, 302 }, { 799, 409, 307, 302 },
        { 152, 729, 306, 302 }, { 476, 729, 305, 302 }, { 799, 729, 307, 302 } };

    constexpr Rect labels[6] = {
        { 196, 540, 218, 79 }, { 520, 540, 218, 79 }, { 843, 540, 218, 79 },
        { 196, 860, 218, 79 }, { 520, 860, 218, 79 }, { 843, 860, 218, 79 } };
}

//==============================================================================
class PadComponent : public juce::Component,
                     public juce::SettableTooltipClient
{
public:
    PadComponent (MultiPadEditor& e, MultiPadProcessor& p, int padIndex);
    ~PadComponent() override;

    void setScale (float s) { scale = s; }
    void refresh();                 // nama / sampel berubah
    void tick();                    // dipanggil timer editor (30 Hz)

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void rebuildText();
    void showMenu();
    void launchChooser();
    void loadFile (const juce::File& f);
    void showRename();
    void showSettings();
    void setSuspended (bool shouldSuspend);

    MultiPadEditor& editor;
    MultiPadProcessor& proc;
    const int index;

    float scale = 1.0f;
    juce::Image original, scaled;
    juce::Path textPath;
    float textStroke = 4.0f;

    float flash = 0.0f;
    float progress = -1.0f;
    int lastTrigger = 0;
    bool loaded = false;
    bool suspended = false;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<juce::AlertWindow> renameWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadComponent)
};

//==============================================================================
class MultiPadEditor : public juce::AudioProcessorEditor,
                       private juce::ChangeListener,
                       private juce::Timer
{
public:
    explicit MultiPadEditor (MultiPadProcessor&);
    ~MultiPadEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;

    LRLookAndFeel& getLnf() { return lnf; }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    MultiPadProcessor& proc;
    LRLookAndFeel lnf;
    juce::TooltipWindow tooltipWindow { this, 600 };

    juce::Image bgOriginal, bgScaled;
    int originX = 0, originY = 0;
    std::unique_ptr<PadComponent> pads[mp::numPads];
    bool keyWasDown[mp::numPads] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiPadEditor)
};
