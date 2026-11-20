#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class CaesarChorusEditor : public juce::AudioProcessorEditor
{
public:
    explicit CaesarChorusEditor (CaesarChorusProcessor&);
    ~CaesarChorusEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name);

    CaesarChorusProcessor& processor;
    juce::LookAndFeel_V4 lookAndFeel;

    Knob rate, depth, lag, blend;
    juce::ComboBox shapeBox;
    juce::Label shapeLabel;
    std::unique_ptr<ComboAttachment> shapeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaesarChorusEditor)
};
