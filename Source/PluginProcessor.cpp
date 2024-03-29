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
{}

SqueezeFilterAudioProcessor::~SqueezeFilterAudioProcessor()
{}

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
    
    rmsLevelLeft.reset(sampleRate, 2.02);
    rmsLevelRight.reset(sampleRate, 2.02);
    
    rmsLevelLeft.setCurrentAndTargetValue(-48.f);
    rmsLevelRight.setCurrentAndTargetValue(-48.f);
    
    juce::dsp::ProcessSpec spec;
    
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    updateFilters();
    
    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);

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
    if(totalNumInputChannels > 1)
    {
        updateFilters();
        
        juce::dsp::AudioBlock<float> block(buffer);
        auto leftBlock = block.getSingleChannelBlock(0);
        auto rightBlock = block.getSingleChannelBlock(1);
        
        juce::dsp::ProcessContextReplacing<float>leftContext(leftBlock);
        juce::dsp::ProcessContextReplacing<float>rightContext(rightBlock);
        
        leftChain.process(leftContext);
        rightChain.process(rightContext);
        
        leftChannelFifo.update(buffer);
        rightChannelFifo.update(buffer);
    }
    
}

//==============================================================================
bool SqueezeFilterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SqueezeFilterAudioProcessor::createEditor()
{
    return new SqueezeFilterAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
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


// Filter coefficients
void updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}

juce::AudioProcessorValueTreeState::ParameterLayout SqueezeFilterAudioProcessor::createParameterLayout(){
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
        
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"lp",1}, "High Cut", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 1.f), 20000.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"hp",1}, "Low Cut", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 1.f), 20.0f));
    
    
    juce::StringArray stringArray;
    for(int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        //str << "db/Oct";
        stringArray.add(str);
    }
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"LowCutSlope", 1}, "LowCutSlope", stringArray, 1));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"HighCutSlope", 1}, "HighCutSlope", stringArray, 1));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"SqueezeValue", 1},
                                                           "SqueezeValue",
                                                           juce::NormalisableRange<float>(0.0001f, 1.0f),1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"OffsetValue", 1},
                                                              "OffsetValue",
                                                              juce::NormalisableRange<float>(-19980.f, 19980.f,0.01), 0.f));
//    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"AnalyzerEnabled",1}, "AnalyzerEnabled", false));
    return layout;
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
    auto chainSettings = getChainSettings(apvts,lastLowCutParam,lastHighCutParam);

    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SqueezeFilterAudioProcessor();
}
