#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Tema biru Doraemon untuk menu klik kanan, panel Setting, dan tooltip.
class LRLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LRLookAndFeel()
    {
        using juce::Colour;

        setColour (juce::PopupMenu::backgroundColourId,            Colour (0xFF0E4DB0));
        setColour (juce::PopupMenu::textColourId,                  juce::Colours::white);
        setColour (juce::PopupMenu::headerTextColourId,            Colour (0xFFFFD43B));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Colour (0xFF35B6FF));
        setColour (juce::PopupMenu::highlightedTextColourId,       Colour (0xFF06306F));

        setColour (juce::Slider::textBoxTextColourId,       juce::Colours::white);
        setColour (juce::Slider::textBoxBackgroundColourId, Colour (0xFF0A2A66).withAlpha (0.55f));
        setColour (juce::Slider::textBoxOutlineColourId,    Colour (0xFF0B3C8C));
        setColour (juce::Slider::textBoxHighlightColourId,  Colour (0xFFFFD43B));
        setColour (juce::Label::textColourId,               juce::Colours::white);

        setColour (juce::TooltipWindow::backgroundColourId, Colour (0xFF0B3C8C));
        setColour (juce::TooltipWindow::textColourId,       juce::Colours::white);
        setColour (juce::TooltipWindow::outlineColourId,    Colour (0xFFFFD43B));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearHorizontal)
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                                    minSliderPos, maxSliderPos, style, slider);
            return;
        }

        const float cy = (float) y + (float) height * 0.5f;
        const float th = 10.0f;
        const juce::Rectangle<float> track ((float) x + 6.0f, cy - th * 0.5f, (float) width - 12.0f, th);

        g.setColour (juce::Colour (0xFF0B3C8C));
        g.fillRoundedRectangle (track.expanded (2.0f), th * 0.5f + 2.0f);
        g.setColour (juce::Colour (0xFF0A2A66).withAlpha (0.75f));
        g.fillRoundedRectangle (track, th * 0.5f);

        const auto fill = track.withRight (juce::jmax (track.getX() + 1.0f, sliderPos));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF8BE7FF), fill.getX(), 0.0f,
                                                 juce::Colour (0xFF29A8FF), fill.getRight(), 0.0f, false));
        g.fillRoundedRectangle (fill, th * 0.5f);

        // thumb berbentuk "lonceng" kuning
        const float r = 11.0f;
        g.setColour (juce::Colour (0xFF0B3C8C));
        g.fillEllipse (sliderPos - r - 2.0f, cy - r - 2.0f, 2.0f * r + 4.0f, 2.0f * r + 4.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xFFFFF08A), sliderPos - r * 0.5f, cy - r * 0.7f,
                                                 juce::Colour (0xFFFFB800), sliderPos + r * 0.6f, cy + r, true));
        g.fillEllipse (sliderPos - r, cy - r, 2.0f * r, 2.0f * r);
        g.setColour (juce::Colour (0xFF8A5A00));
        g.drawLine (sliderPos - r * 0.75f, cy + r * 0.2f, sliderPos + r * 0.75f, cy + r * 0.2f, 1.6f);
        g.fillEllipse (sliderPos - 2.0f, cy + r * 0.2f + 2.0f, 4.0f, 4.0f);
    }

    void drawCallOutBoxBackground (juce::CallOutBox& box, juce::Graphics& g,
                                   const juce::Path& path, juce::Image& cachedImage) override
    {
        if (cachedImage.isNull())
        {
            cachedImage = juce::Image (juce::Image::ARGB, box.getWidth(), box.getHeight(), true);
            juce::Graphics g2 (cachedImage);
            juce::DropShadow (juce::Colour (0xAA06214F), 10, { 0, 3 }).drawForPath (g2, path);
        }

        g.setColour (juce::Colours::black);
        g.drawImageAt (cachedImage, 0, 0);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF5CC8FF), 0.0f, 0.0f,
                                                 juce::Colour (0xFF1E7BE0), 0.0f, (float) box.getHeight(), false));
        g.fillPath (path);
        g.setColour (juce::Colour (0xFF0B3C8C));
        g.strokePath (path, juce::PathStrokeType (3.0f));
    }
};
