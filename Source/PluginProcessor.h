/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Custom/Filter.h"
#include "Custom/Fifo.h"

//==============================================================================
/**
*/
class SqueezeFilterAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SqueezeFilterAudioProcessor();
    ~SqueezeFilterAudioProcessor() override;
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif
    
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    //==============================================================================
    const juce::String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
   // juce::AudioProcessorValueTreeState::ParameterLayout initializeGUI();
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};
    
    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo {Channel::Left};
    SingleChannelSampleFifo<BlockType> rightChannelFifo {Channel::Right};
    
    // Save and set GUI resize
    int getEditorWidth()
    {
        auto size = apvts.state.getOrCreateChildWithName ("lastSize", nullptr);
        return size.getProperty ("width", 650);
    }
    int getEditorHeight()
    {
        const float ratio = 16.0/ 9.0;
        auto size = apvts.state.getOrCreateChildWithName ("lastSize", nullptr);
        return size.getProperty ("height", 650.0 / ratio);
    }

    void setEditorSize (int width, int height)
    {
        auto size = apvts.state.getOrCreateChildWithName ("lastSize", nullptr);
        size.setProperty ("width", width, nullptr);
        size.setProperty ("height", height, nullptr);
    }
    
    double lastLowCutParam;
    double lastHighCutParam;
    
private:
   
    //STEREO
    juce::LinearSmoothedValue<float> rmsLevelLeft, rmsLevelRight;
    MonoChain leftChain, rightChain;
    
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);

    void updateFilters();
  
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqueezeFilterAudioProcessor)
};
