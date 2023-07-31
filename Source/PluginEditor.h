/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Custom/Params.h"
#include "Custom/LookAndFeel.h"
#include "Custom/SvgComps.h"
#include "Custom/ResponseComp.h"
#include "Custom/colors.h"
using namespace juce;


class SqueezeFilterAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SqueezeFilterAudioProcessorEditor (SqueezeFilterAudioProcessor&);
    ~SqueezeFilterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    
//    bool keyPressed(const juce::KeyPress& key) override
//    {
//        DBG(key.getKeyCode());
//        DBG(juce::KeyPress::numberPad1);
//        if (key.getKeyCode() == 49)
//                 {
//                     const float ratio = 16.0/ 9.0;
//                     setSize (500, juce::roundToInt (1500.0 / ratio));
//                     setResizeLimits (500,  juce::roundToInt (500.0 / ratio),
//                                          1500, juce::roundToInt (1500.0 / ratio));
//                     getConstrainer()->setFixedAspectRatio (ratio);
//                     repaint();
//                     return true;
//                 }
//        return false;
//    }

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    
    SqueezeFilterAudioProcessor& audioProcessor;
    
    CustomSlider lowCutFreqSlider,
    highCutFreqSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider,
    squeezeSlider,offsetSlider;
    
    //juce::DrawableButton analyzerEnabledButton{ "analyzerEnabledButton", juce::DrawableButton::ButtonStyle::ImageStretched };
    juce::DrawableButton zoomOneButton{ "zoomOneButton", juce::DrawableButton::ButtonStyle::ImageStretched };

    MyTwoValueSlider lpHpSlider;
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment 
    lowCutSlopeSliderAttachment,
    highCutSlopeSliderAttachment,
    squeezeSliderAttachment,
    offsetSliderAttachment;

    using ButtonAttachment = APVTS::ButtonAttachment;
    //ButtonAttachment analyzerEnabledButtonAttachment;

    ResponseCurveComponent responseCurveComponent;
    std::vector<juce::Component*> getComps();

    // Move LAF to its own component/file
    CustomTwoValSliderLaf twoValLaf;
    CustomSliderLaf sliderLaf;
    CustomCrossover crossOverLaf;
    CustomSlopSlider slopSliderLaf;
    
    int currentZoomState;

    svgOffsetComp offsetIkon;
    svgSlopeComp slopIcon;
    svgSlopeComp slopIcon2;
    svgSqueezeComp squeezeIcon;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqueezeFilterAudioProcessorEditor)
};
