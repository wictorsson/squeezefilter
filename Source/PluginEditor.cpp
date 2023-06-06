/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SqueezeFilterAudioProcessorEditor::SqueezeFilterAudioProcessorEditor (SqueezeFilterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    for(auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    setSize (600, 400);
}

SqueezeFilterAudioProcessorEditor::~SqueezeFilterAudioProcessorEditor()
{
}

//==============================================================================
void SqueezeFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void SqueezeFilterAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33f);
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33f);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5f);
    
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight()* 0.5f));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight()* 0.5f));
    highCutSlopeSlider.setBounds(highCutArea);
    
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33f));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5f));
    peakQualitySlider.setBounds(bounds);
    
}

std::vector<juce::Component*> SqueezeFilterAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider
    };
}
