#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Isi menu "Setting..." per pad: Durasi, Feed In, Feed Out, Volume.
class SettingsPanel : public juce::Component
{
public:
    SettingsPanel (MultiPadProcessor& p, int padIndex)
    {
        title.setText (p.getPadName (padIndex) + "  -  SETTING", juce::dontSendNotification);
        title.setFont (juce::Font (18.0f, juce::Font::bold));
        title.setJustificationType (juce::Justification::centred);
        title.setColour (juce::Label::textColourId, juce::Colour (0xFFFFFFFF));
        addAndMakeVisible (title);

        setup (p, padIndex, 0, "Durasi",   "dur", 10.0);
        setup (p, padIndex, 1, "Feed In",  "in",   0.0);
        setup (p, padIndex, 2, "Feed Out", "out",  0.0);
        setup (p, padIndex, 3, "Volume",   "vol", 100.0);

        hint.setText ("Durasi 1-10 dtk  |  Feed In/Out 0-5 dtk (0 = Off)  |  Volume 1-100",
                      juce::dontSendNotification);
        hint.setFont (juce::Font (11.5f));
        hint.setJustificationType (juce::Justification::centred);
        hint.setColour (juce::Label::textColourId, juce::Colour (0xFFD8F2FF));
        addAndMakeVisible (hint);

        setSize (330, 250);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (14, 10);
        title.setBounds (r.removeFromTop (30));
        r.removeFromTop (6);
        for (int i = 0; i < 4; ++i)
        {
            auto row = r.removeFromTop (38);
            labels[i].setBounds (row.removeFromLeft (70));
            sliders[i].setBounds (row);
        }
        hint.setBounds (r.removeFromBottom (22));
    }

private:
    void setup (MultiPadProcessor& p, int padIndex, int i, const juce::String& text,
                const char* suffix, double defaultValue)
    {
        labels[i].setText (text, juce::dontSendNotification);
        labels[i].setFont (juce::Font (15.0f, juce::Font::bold));
        labels[i].setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (labels[i]);

        sliders[i].setSliderStyle (juce::Slider::LinearHorizontal);
        sliders[i].setTextBoxStyle (juce::Slider::TextBoxRight, false, 72, 24);
        addAndMakeVisible (sliders[i]);

        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            p.apvts, MultiPadProcessor::paramId (padIndex, suffix), sliders[i]);

        sliders[i].setDoubleClickReturnValue (true, defaultValue);
    }

    juce::Label title, hint;
    juce::Label labels[4];
    juce::Slider sliders[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[4];
};
