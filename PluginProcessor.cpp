#include "PluginProcessor.h"
#include "PluginEditor.h"

CaesarChorusProcessor::CaesarChorusProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    rateParam  = apvts.getRawParameterValue ("rate");
    depthParam = apvts.getRawParameterValue ("depth");
    lagParam   = apvts.getRawParameterValue ("lag");
    blendParam = apvts.getRawParameterValue ("blend");
    shapeParam = apvts.getRawParameterValue ("shape");
}

juce::AudioProcessorValueTreeState::ParameterLayout CaesarChorusProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    // Rate: skewed so the slow end gets more of the knob's travel
    NormalisableRange<float> rateRange (0.05f, 10.0f, 0.0f);
    rateRange.setSkewForCentre (1.0f);

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "rate", 1 }, "Rate", rateRange, 0.8f,
        AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "depth", 1 }, "Depth", NormalisableRange<float> (0.0f, 1.0f), 0.5f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "lag", 1 }, "Lag", NormalisableRange<float> (0.0f, 1.0f), 0.3f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "blend", 1 }, "Blend", NormalisableRange<float> (0.0f, 1.0f), 0.5f));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "shape", 1 }, "Shape", StringArray { "Triangle", "Sine" }, 0));

    return layout;
}

void CaesarChorusProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getTotalNumOutputChannels());
}

void CaesarChorusProcessor::reset()
{
    engine.reset();
}

bool CaesarChorusProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void CaesarChorusProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    caesar::ChorusEngine::Params p;
    p.rateHz = rateParam->load();
    p.depth  = depthParam->load();
    p.lag    = lagParam->load();
    p.blend  = blendParam->load();
    p.sine   = shapeParam->load() > 0.5f;

    engine.process (buffer.getArrayOfWritePointers(), buffer.getNumChannels(),
                    buffer.getNumSamples(), p);
}

juce::AudioProcessorEditor* CaesarChorusProcessor::createEditor()
{
    return new CaesarChorusEditor (*this);
}

void CaesarChorusProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CaesarChorusProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CaesarChorusProcessor();
}
