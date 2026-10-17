#include "PluginEditor.h"
#include "SettingsPanel.h"
#include "GlobalKeys.h"
#include "BinaryData.h"

namespace
{
    const juce::Colour outlineBlue (0xFF0B3C8C);
    const juce::Colour bellYellow  (0xFFFFD43B);

    // Pilih font tebal-membulat yang tersedia di sistem; fallback ke sans-serif tebal.
    juce::Font makePadFont (float height)
    {
        static const juce::String face = []
        {
            const char* prefs[] = { "Fredoka One", "Fredoka", "Baloo 2", "Baloo Bhai 2", "Nunito Black",
                                    "Arial Rounded MT Bold", "Segoe UI Black", "Arial Black", "Verdana" };
            const auto all = juce::Font::findAllTypefaceNames();
            for (auto* n : prefs)
                if (all.contains (juce::String (n), true))
                    return juce::String (n);
            return juce::Font::getDefaultSansSerifFontName();
        }();

        const bool alreadyHeavy = face.containsIgnoreCase ("black") || face.containsIgnoreCase ("bold")
                                  || face.containsIgnoreCase ("fredoka one");
        return juce::Font (face, height, alreadyHeavy ? juce::Font::plain : juce::Font::bold);
    }
}

//==============================================================================
// PadComponent
PadComponent::PadComponent (MultiPadEditor& e, MultiPadProcessor& p, int padIndex)
    : editor (e), proc (p), index (padIndex)
{
    int size = 0;
    const auto* data = BinaryData::getNamedResource (("pad" + juce::String (padIndex + 1) + "_png").toRawUTF8(), size);
    if (data != nullptr)
        original = juce::ImageCache::getFromMemory (data, size);

    setOpaque (true);
    lastTrigger = proc.getTriggerCount (index);
}

PadComponent::~PadComponent()
{
    setSuspended (false);
    if (renameWindow != nullptr)
        renameWindow->exitModalState (0);
}

void PadComponent::setSuspended (bool shouldSuspend)
{
    if (shouldSuspend == suspended) return;
    suspended = shouldSuspend;
    if (suspended) proc.pushKeyboardSuspend();
    else           proc.popKeyboardSuspend();
}

void PadComponent::refresh()
{
    loaded = proc.hasSample (index);

    juce::String tip = proc.getPadName (index) + "   [tombol " + juce::String (index + 1) + "]\n";
    tip += loaded ? proc.getSampleInfo (index)
                  : juce::String ("Kosong - klik kanan > Upload Sampling");
    setTooltip (tip);

    rebuildText();
    repaint();
}

void PadComponent::resized()
{
    if (original.isValid() && getWidth() > 0 && getHeight() > 0)
        scaled = original.rescaled (getWidth(), getHeight(), juce::Graphics::highResamplingQuality);
    rebuildText();
}

void PadComponent::rebuildText()
{
    textPath.clear();
    if (getWidth() <= 0) return;

    const auto& lr = layout::labels[index];
    const auto& pr = layout::pads[index];
    const juce::Rectangle<float> label ((float) (lr.x - pr.x) * scale, (float) (lr.y - pr.y) * scale,
                                        (float) lr.w * scale, (float) lr.h * scale);

    const auto text = proc.getPadName (index);
    if (text.isEmpty()) return;

    const auto font = makePadFont (label.getHeight() * 0.98f);

    juce::GlyphArrangement ga;
    ga.addLineOfText (font, text, 0.0f, 0.0f);
    juce::Path p;
    ga.createPath (p);
    if (p.isEmpty()) return;

    // tinggi huruf kapital -> untuk pusatkan secara vertikal
    juce::GlyphArrangement gh;
    gh.addLineOfText (font, "H", 0.0f, 0.0f);
    juce::Path hp;
    gh.createPath (hp);
    const float capH = hp.isEmpty() ? font.getHeight() * 0.7f : hp.getBounds().getHeight();

    const auto b = p.getBounds();
    const float maxW = label.getWidth() * 0.94f;
    const float k = juce::jmin (1.0f, maxW / juce::jmax (1.0f, b.getWidth()));   // ukuran teks mengikuti lebar pad

    p.applyTransform (juce::AffineTransform::translation (-b.getCentreX(), capH * 0.5f)
                          .scaled (k)
                          .translated (label.getCentreX(), label.getCentreY()));
    textPath = p;
    textStroke = juce::jmax (2.0f, 10.0f * scale * k);
}

void PadComponent::paint (juce::Graphics& g)
{
    const float s = scale;
    const auto b = getLocalBounds().toFloat();

    if (scaled.isValid())
        g.drawImageAt (scaled, 0, 0);
    else
        g.fillAll (juce::Colour (0xFF29A8FF));

    const auto shape = b.reduced (9.0f * s);
    const float corner = 26.0f * s;

    // efek ditekan / dipukul
    if (flash > 0.001f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.34f * flash));
        g.fillRoundedRectangle (shape, corner);
        g.setColour (juce::Colours::white.withAlpha (0.85f * flash));
        g.drawRoundedRectangle (shape.reduced (2.0f * s), corner, juce::jmax (1.5f, 4.0f * s));
    }

    // nama pad: bayangan -> outline biru tua -> isi putih
    if (! textPath.isEmpty())
    {
        juce::Path shadow (textPath);
        shadow.applyTransform (juce::AffineTransform::translation (0.0f, 3.0f * s));
        g.setColour (juce::Colour (0x66072A6B));
        g.strokePath (shadow, juce::PathStrokeType (textStroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (outlineBlue);
        g.strokePath (textPath, juce::PathStrokeType (textStroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (juce::Colours::white);
        g.fillPath (textPath);
    }

    // LED status (kuning = ada sampel)
    const float ledR = 8.0f * s;
    const juce::Point<float> led (b.getRight() - 40.0f * s, 40.0f * s);
    const bool playing = progress >= 0.0f;
    if (loaded)
    {
        if (playing)
        {
            g.setColour (bellYellow.withAlpha (0.35f));
            g.fillEllipse (led.x - ledR * 2.0f, led.y - ledR * 2.0f, ledR * 4.0f, ledR * 4.0f);
        }
        g.setColour (outlineBlue);
        g.fillEllipse (led.x - ledR - 2.0f * s, led.y - ledR - 2.0f * s, ledR * 2.0f + 4.0f * s, ledR * 2.0f + 4.0f * s);
        g.setColour (playing ? juce::Colour (0xFFFFF59A) : bellYellow);
        g.fillEllipse (led.x - ledR, led.y - ledR, ledR * 2.0f, ledR * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillEllipse (led.x - ledR * 0.5f, led.y - ledR * 0.6f, ledR * 0.55f, ledR * 0.55f);
    }
    else
    {
        g.setColour (outlineBlue.withAlpha (0.55f));
        g.fillEllipse (led.x - ledR - 2.0f * s, led.y - ledR - 2.0f * s, ledR * 2.0f + 4.0f * s, ledR * 2.0f + 4.0f * s);
        g.setColour (juce::Colour (0xFF8FD0FF).withAlpha (0.8f));
        g.fillEllipse (led.x - ledR, led.y - ledR, ledR * 2.0f, ledR * 2.0f);
    }

    // bar progres di bawah lonceng
    if (playing)
    {
        const juce::Rectangle<float> track (b.getWidth() * 0.22f, 268.0f * s, b.getWidth() * 0.56f, 9.0f * s);
        g.setColour (outlineBlue.withAlpha (0.7f));
        g.fillRoundedRectangle (track.expanded (2.0f * s), track.getHeight());
        g.setColour (juce::Colour (0xFF0A2A66).withAlpha (0.6f));
        g.fillRoundedRectangle (track, track.getHeight() * 0.5f);
        auto fill = track;
        fill.setWidth (juce::jmax (track.getHeight(), track.getWidth() * juce::jlimit (0.0f, 1.0f, progress)));
        g.setColour (bellYellow);
        g.fillRoundedRectangle (fill, track.getHeight() * 0.5f);
    }
}

void PadComponent::tick()
{
    bool dirty = false;

    const int tc = proc.getTriggerCount (index);
    if (tc != lastTrigger)
    {
        lastTrigger = tc;
        flash = 1.0f;
        dirty = true;
    }
    else if (flash > 0.0f)
    {
        flash = juce::jmax (0.0f, flash - 0.09f);
        dirty = true;
    }

    const float p = proc.getPadProgress (index);
    if ((p < 0.0f) != (progress < 0.0f) || std::abs (p - progress) > 0.003f)
    {
        progress = p;
        dirty = true;
    }

    if (dirty) repaint();
}

void PadComponent::mouseDown (const juce::MouseEvent& e)
{
    editor.grabKeyboardFocus();

    if (e.mods.isPopupMenu())
    {
        showMenu();
        return;
    }

    if (! e.mods.isLeftButtonDown())
        return;

    if (proc.hasSample (index))
        proc.triggerPad (index);
    else
        launchChooser();          // pad kosong: langsung tawarkan upload sampling
}

//------------------------------------------------------------------------------
void PadComponent::showMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&editor.getLnf());
    m.addSectionHeader (proc.getPadName (index));
    m.addItem (1, "Upload Sampling...");
    m.addItem (2, "Setting...");
    m.addItem (3, "Rename...");
    m.addSeparator();
    m.addItem (4, "Hapus Sampling", proc.hasSample (index));

    juce::Component::SafePointer<PadComponent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [safe] (int result)
    {
        if (safe.getComponent() == nullptr || result == 0) return;

        switch (result)
        {
            case 1: safe->launchChooser();                         break;
            case 2: safe->showSettings();                          break;
            case 3: safe->showRename();                            break;
            case 4: safe->proc.clearSample (safe->index);          break;
            default: break;
        }
    });
}

void PadComponent::launchChooser()
{
    auto startDir = juce::File (proc.getLastDir());
    if (! startDir.isDirectory())
        startDir = juce::File::getSpecialLocation (juce::File::userMusicDirectory);

    chooser = std::make_unique<juce::FileChooser> ("Pilih file audio untuk " + proc.getPadName (index),
                                                   startDir, proc.getFileWildcard());

    setSuspended (true);   // angka yang diketik di dialog tidak boleh memicu pad

    juce::Component::SafePointer<PadComponent> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
    {
        if (safe.getComponent() == nullptr) return;
        safe->setSuspended (false);

        const auto f = fc.getResult();
        if (f != juce::File())
            safe->loadFile (f);
    });
}

void PadComponent::loadFile (const juce::File& f)
{
    const auto r = proc.loadSampleFromFile (index, f);
    if (r.failed())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "Gagal memuat sampling", r.getErrorMessage(), "OK");
    else
        proc.setLastDir (f.getParentDirectory().getFullPathName());
}

void PadComponent::showSettings()
{
    auto content = std::make_unique<SettingsPanel> (proc, index);
    const auto area = editor.getLocalArea (this, getLocalBounds());
    juce::CallOutBox::launchAsynchronously (std::move (content), area, &editor);
}

void PadComponent::showRename()
{
    auto* aw = new juce::AlertWindow ("Rename " + proc.getPadName (index),
                                      "Nama pad (maks. " + juce::String (mp::maxNameLength) + " karakter):",
                                      juce::MessageBoxIconType::NoIcon, this);
    aw->addTextEditor ("name", proc.getPadName (index), juce::String());
    if (auto* te = aw->getTextEditor ("name"))
        te->setInputRestrictions (mp::maxNameLength);
    aw->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Batal", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    renameWindow = aw;
    setSuspended (true);

    juce::Component::SafePointer<PadComponent> safe (this);
    juce::Component::SafePointer<juce::AlertWindow> safeAw (aw);

    aw->enterModalState (true,
                         juce::ModalCallbackFunction::create ([safe, safeAw] (int result)
                         {
                             if (safe.getComponent() == nullptr) return;
                             safe->setSuspended (false);

                             if (result == 1 && safeAw.getComponent() != nullptr)
                                 safe->proc.setPadName (safe->index, safeAw->getTextEditorContents ("name"));
                         }),
                         true);
}

//==============================================================================
// Editor
MultiPadEditor::MultiPadEditor (MultiPadProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    bgOriginal = juce::ImageCache::getFromMemory (BinaryData::bg_png, BinaryData::bg_pngSize);

    for (int i = 0; i < mp::numPads; ++i)
    {
        pads[i] = std::make_unique<PadComponent> (*this, proc, i);
        addAndMakeVisible (*pads[i]);
    }

    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setResizeLimits (350, 350, layout::canvas, layout::canvas);
    getConstrainer()->setFixedAspectRatio (1.0);

    const int sz = juce::jlimit (350, layout::canvas, proc.getEditorSize());
    setSize (sz, sz);

    proc.addChangeListener (this);
    for (auto& pad : pads) pad->refresh();
    startTimerHz (30);
}

MultiPadEditor::~MultiPadEditor()
{
    stopTimer();
    proc.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void MultiPadEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF1E6FD0));
    if (bgScaled.isValid())
        g.drawImageAt (bgScaled, originX, originY);
}

void MultiPadEditor::resized()
{
    const int side = juce::jmin (getWidth(), getHeight());
    originX = (getWidth()  - side) / 2;
    originY = (getHeight() - side) / 2;
    const float s = (float) side / (float) layout::canvas;

    if (bgOriginal.isValid() && side > 0)
        bgScaled = bgOriginal.rescaled (side, side, juce::Graphics::highResamplingQuality);

    for (int i = 0; i < mp::numPads; ++i)
    {
        const auto& r = layout::pads[i];
        pads[i]->setScale (s);
        pads[i]->setBounds (juce::Rectangle<float> ((float) r.x * s, (float) r.y * s,
                                                    (float) r.w * s, (float) r.h * s)
                                .translated ((float) originX, (float) originY)
                                .toNearestInt());
    }

    proc.setEditorSize (side);
}

void MultiPadEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    for (auto& pad : pads) pad->refresh();
}

void MultiPadEditor::timerCallback()
{
    for (auto& pad : pads) pad->tick();
}

//------------------------------------------------------------------------------
// Klik kanan di luar pad: opsi keyboard global + bantuan
void MultiPadEditor::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    if (! e.mods.isPopupMenu()) return;

    juce::PopupMenu m;
    m.setLookAndFeel (&lnf);
    m.addSectionHeader ("MULTI PAD LR PRO");
    m.addItem (1, "Tombol 1-6 tetap bunyi walau window tidak aktif",
               LRGlobalKeys::supported(), proc.getGlobalKeysEnabled());
    m.addItem (2, "Bantuan...");

    juce::Component::SafePointer<MultiPadEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [safe] (int result)
    {
        if (safe.getComponent() == nullptr) return;

        if (result == 1)
            safe->proc.setGlobalKeysEnabled (! safe->proc.getGlobalKeysEnabled());
        else if (result == 2)
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::InfoIcon, "MULTI PAD LR PRO",
                "Klik kiri pad / tombol 1-6 (juga numpad) = mainkan sampling.\n"
                "MIDI note C1-F1 (36-41) = PAD 1-6.\n\n"
                "Klik kanan pad:\n"
                " - Upload Sampling: pilih file audio dari penyimpanan\n"
                " - Setting: durasi 1-10 dtk, Feed In, Feed Out, Volume\n"
                " - Rename: ganti nama pad\n\n"
                "Klik kanan di luar pad: aktif/nonaktif tombol keyboard global.",
                "OK");
    });
}

bool MultiPadEditor::keyPressed (const juce::KeyPress&)
{
    return false;
}

// Dipakai hanya bila keyboard global tidak aktif / tidak didukung OS:
// menangani tombol 1-6 saat window plugin sedang fokus.
bool MultiPadEditor::keyStateChanged (bool)
{
    if (proc.isKeyboardPollingActive())
        return false;

    const auto mods = juce::ModifierKeys::currentModifiers;
    const bool modsHeld = mods.isCtrlDown() || mods.isAltDown() || mods.isCommandDown();

    bool handled = false;
    for (int i = 0; i < mp::numPads; ++i)
    {
        const bool d = ! modsHeld
                       && (juce::KeyPress::isKeyCurrentlyDown ('1' + i)
                           || juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::numberPad1 + i));

        if (d && ! keyWasDown[i])
        {
            proc.triggerPad (i);
            handled = true;
        }
        keyWasDown[i] = d;
    }
    return handled;
}
