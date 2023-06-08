/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SqueezeFilterAudioProcessor::SqueezeFilterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SqueezeFilterAudioProcessor::~SqueezeFilterAudioProcessor()
{
}

//==============================================================================
const juce::String SqueezeFilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SqueezeFilterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SqueezeFilterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SqueezeFilterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SqueezeFilterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SqueezeFilterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SqueezeFilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SqueezeFilterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SqueezeFilterAudioProcessor::getProgramName (int index)
{
    return {};
}

void SqueezeFilterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SqueezeFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    juce::dsp::ProcessSpec spec;
    
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    updateFilters();

}


void SqueezeFilterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SqueezeFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SqueezeFilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();
   
    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    juce::dsp::ProcessContextReplacing<float>leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float>rightContext(rightBlock);
    
    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool SqueezeFilterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SqueezeFilterAudioProcessor::createEditor()
{
    return new SqueezeFilterAudioProcessorEditor (*this);
   // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SqueezeFilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SqueezeFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if(tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts){
    ChainSettings settings;
    auto offset = apvts.getRawParameterValue("OffsetValue")->load();

    if(offset > 0)
    {
        settings.lowCutFreq = apvts.getRawParameterValue("LowCutFreq")->load() * apvts.getRawParameterValue("SqueezeValue")->load() + offset *  (1 - apvts.getRawParameterValue("LowCutFreq")->load() / 19980);

        settings.highCutFreq = 20000 - (20000 - apvts.getRawParameterValue("HighCutFreq")->load()) * apvts.getRawParameterValue("SqueezeValue")->load() + offset * (1 - apvts.getRawParameterValue("HighCutFreq")->load() / 19980);
        
    }
    else if(offset < 0)
    {
        settings.lowCutFreq = apvts.getRawParameterValue("LowCutFreq")->load() * apvts.getRawParameterValue("SqueezeValue")->load() + offset * apvts.getRawParameterValue("SqueezeValue")->load() *   (apvts.getRawParameterValue("LowCutFreq")->load()-20) / 19980;
        
      
        settings.highCutFreq = 20020 - (20000 - apvts.getRawParameterValue("HighCutFreq")->load()) * apvts.getRawParameterValue("SqueezeValue")->load() + offset *  apvts.getRawParameterValue("HighCutFreq")->load() / 19980;
        
    }
    else
    {
        settings.lowCutFreq = apvts.getRawParameterValue("LowCutFreq")->load() * apvts.getRawParameterValue("SqueezeValue")->load();
        
        settings.highCutFreq = 20000 - (20000 - apvts.getRawParameterValue("HighCutFreq")->load()) * apvts.getRawParameterValue("SqueezeValue")->load();
       
    }
    
   
    settings.peakFreq = apvts.getRawParameterValue("PeakFreq")->load();
    settings.peakGainDecibels = apvts.getRawParameterValue("PeakGain")->load();
    settings.peakQuality = apvts.getRawParameterValue("PeakQuality")->load();
    
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCutSlope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCutSlope")->load());
    return settings;
}

void/*SqueezeFilterAudioProcessor::*/updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}

juce::AudioProcessorValueTreeState::ParameterLayout SqueezeFilterAudioProcessor::createParameterLayout(){
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCutFreq",
                                                           "LowCutFreq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCutFreq",
                                                           "HighCutFreq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("PeakFreq",
                                                           "PeakFreq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 750.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("PeakGain",
                                                           "PeakGain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 0.5f), 0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("PeakQuality",
                                                           "PeakQuality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 0.5f), 0.0f));
    
    juce::StringArray stringArray;
    for(int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        str << "db/Oct";
        stringArray.add(str);
    }
    
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCutSlope", "LowCutSlope", stringArray, 0));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCutSlope", "HighCutSlope", stringArray, 0));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("SqueezeValue",
                                                           "SqueezeValue",
                                                           juce::NormalisableRange<float>(0.001f, 1.0f),0.01f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("OffsetValue",
                                                           "OffsetValue",
                                                           juce::NormalisableRange<float>(-19980.f, 19980.f,0.01), 0.f));
    
    return layout;
}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainDecibels));
}

void SqueezeFilterAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings)
{
//    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainDecibels));
    
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
}


void SqueezeFilterAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    
}

void SqueezeFilterAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void SqueezeFilterAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);
    
    updatePeakFilter(chainSettings);
    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SqueezeFilterAudioProcessor();
}
