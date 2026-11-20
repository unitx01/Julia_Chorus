#include "PluginEditor.h"

namespace
{
const juce::Colour kPanel   { 0xff1f3b4d }; // enclosure colour
const juce::Colour kAccent  { 0xffe8c170 }; // knob / text accent
const juce::Colour kText    { 0xfff2efe6 };
}

CaesarChorusEditor::CaesarChorusEditor (CaesarChorusProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    lookAndFeel.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    lookAndFeel.setColour (juce::Slider::thumbColourId, kText);
    lookAndFeel.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lookAndFeel.setColour (juce::ComboBox::backgroundColourId, kPanel.darker (0.4f));
    lookAndFeel.setColour (juce::ComboBox::outlineColourId, kAccent.withAlpha (0.6f));
    setLookAndFeel (&lookAndFeel);

    setupKnob (rate,  "rate",  "Rate");
    setupKnob (depth, "depth", "Depth");
    setupKnob (lag,   "lag",   "Lag");
    setupKnob (blend, "blend", "Blend");

    shapeBox.addItemList ({ "Triangle", "Sine" }, 1);
    addAndMakeVisible (shapeBox);
    shapeAttachment = std::make_unique<ComboAttachment> (processor.apvts, "shape", shapeBox);

    shapeLabel.setText ("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType (juce::Justification::centred);
    shapeLabel.setColour (juce::Label::textColourId, kText);
    addAndMakeVisible (shapeLabel);

    setSize (460, 330);
}

CaesarChorusEditor::~CaesarChorusEditor()
{
    setLookAndFeel (nullptr);
}

void CaesarChorusEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible (k.slider);
    k.attachment = std::make_unique<SliderAttachment> (processor.apvts, paramId, k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, kText);
    k.label.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    addAndMakeVisible (k.label);
}

void CaesarChorusEditor::paint (juce::Graphics& g)
{
    g.fillAll (kPanel.darker (0.6f));

    auto panel = getLocalBounds().toFloat().reduced (8.0f);
    g.setColour (kPanel);
    g.fillRoundedRectangle (panel, 12.0f);
    g.setColour (kAccent.withAlpha (0.5f));
    g.drawRoundedRectangle (panel, 12.0f, 1.5f);

    g.setColour (kAccent);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText ("CAESAR CHORUS", getLocalBounds().removeFromTop (52), juce::Justification::centred);
}

void CaesarChorusEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (40); // title

    // Top row: Rate, Depth, Lag (like the pedal's top row)
    auto top = area.removeFromTop (area.getHeight() / 2);
    const int colW = top.getWidth() / 3;

    auto place = [] (Knob& k, juce::Rectangle<int> r)
    {
        k.label.setBounds (r.removeFromTop (22));
        k.slider.setBounds (r);
    };

    place (rate,  top.removeFromLeft (colW));
    place (depth, top.removeFromLeft (colW));
    place (lag,   top);

    // Bottom row: Shape switch under Rate, Blend under Lag
    auto bottom = area;
    auto shapeArea = bottom.removeFromLeft (colW);
    bottom.removeFromLeft (colW);

    shapeLabel.setBounds (shapeArea.removeFromTop (22));
    shapeBox.setBounds (shapeArea.withSizeKeepingCentre (110, 28));

    place (blend, bottom);
}
