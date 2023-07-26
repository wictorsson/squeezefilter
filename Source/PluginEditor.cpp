/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SqueezeFilterAudioProcessorEditor::SqueezeFilterAudioProcessorEditor (SqueezeFilterAudioProcessor& p) : AudioProcessorEditor (&p), audioProcessor
    (p) ,twoValueSlider2(juce::Slider::SliderStyle::TwoValueHorizontal, p.apvts.getParameter("hp"), p.apvts.getParameter("lp")),lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCutSlope", lowCutSlopeSlider),highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCutSlope", highCutSlopeSlider),squeezeSliderAttachment(audioProcessor.apvts, "SqueezeValue", squeezeSlider),offsetSliderAttachment(audioProcessor.apvts, "OffsetValue", offsetSlider), analyzerEnabledButtonAttachment(audioProcessor.apvts, "AnalyzerEnabled", analyzerEnabledButton) ,responseCurveComponent(audioProcessor)

{
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setResizable (true, true);

    const float ratio = 16.0/ 9.0;
    setSize (p.getEditorWidth(), p.getEditorHeight());
    setResizeLimits (550,  juce::roundToInt (550.0 / ratio),
                         1500, juce::roundToInt (1500.0 / ratio));
    getConstrainer()->setFixedAspectRatio (ratio);
    
    for(auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
//    auto safePtr = juce::Component::SafePointer<SqueezeFilterAudioProcessorEditor>(this);
//    analyzerEnabledButton.onClick = [safePtr]()
//    {
//        if(auto* comp = safePtr.getComponent())
//        {
//
//            auto enabled = comp->analyzerEnabledButton.getToggleState();
//            comp->responseCurveComponent.toggleAnalyzerIsEnabled(enabled);
//        }
//    };

   
    analyzerEnabledButton.setToggleable(true);
    bool isAnalyzerEnabled = *audioProcessor.apvts.getRawParameterValue("AnalyzerEnabled");
    responseCurveComponent.toggleAnalyzerIsEnabled(isAnalyzerEnabled);

    auto analyzerEnabledImage = juce::Drawable::createFromImageData(BinaryData::buttonactiveikon_svg, BinaryData::buttonactiveikon_svgSize);
    
    auto buttonactiveikonHover = juce::Drawable::createFromImageData(BinaryData::buttonactiveikonHover1_svg, BinaryData::buttonactiveikonHover1_svgSize);
    
    auto analyzerDisabledImage = juce::Drawable::createFromImageData (BinaryData::buttonemptyikon_svg, BinaryData::buttonemptyikon_svgSize);
    
    auto buttonemptyiconHover = juce::Drawable::createFromImageData(BinaryData::buttonemptyiconHover_svg, BinaryData::buttonemptyiconHover_svgSize);

    analyzerEnabledButton.setImages(analyzerDisabledImage.get(),buttonemptyiconHover.get(),analyzerDisabledImage.get(), analyzerDisabledImage.get(), analyzerEnabledImage.get(), buttonactiveikonHover.get(), analyzerEnabledImage.get(), analyzerEnabledImage.get());

    analyzerEnabledButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);
    analyzerEnabledButton.onClick = [this]()
    {
        bool newAnalyzerEnabledState = !analyzerEnabledButton.getToggleState();
           analyzerEnabledButton.setToggleState(newAnalyzerEnabledState, juce::NotificationType::dontSendNotification);
           *audioProcessor.apvts.getRawParameterValue("AnalyzerEnabled") = newAnalyzerEnabledState;
           responseCurveComponent.toggleAnalyzerIsEnabled(newAnalyzerEnabledState);

    };
    analyzerEnabledButton.setToggleState(isAnalyzerEnabled, juce::NotificationType::dontSendNotification);

    auto scaleImageButton2 = juce::Drawable::createFromImageData(BinaryData::scaleicon_svg, BinaryData::scaleicon_svgSize);
    auto scaleImageButtonHover = juce::Drawable::createFromImageData(BinaryData::screenscaleikonHover_svg, BinaryData::screenscaleikonHover_svgSize);

    
    zoomOneButton.setImages(scaleImageButtonHover.get(),scaleImageButton2.get(),scaleImageButton2.get(),scaleImageButtonHover.get(),scaleImageButton2.get(),scaleImageButtonHover.get(),scaleImageButton2.get(),scaleImageButtonHover.get());
    
    
    zoomOneButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);
    zoomOneButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    addAndMakeVisible(zoomOneButton);
    zoomOneButton.onClick = [this] {
        currentZoomState = (currentZoomState + 1) % 3; // Toggle between 0, 1, 2
        
        const float ratio = 16.0 / 9.0;
        
        if (currentZoomState == 0) {
            setSize(550, juce::roundToInt(1500.0 / ratio));
            setResizeLimits(550, juce::roundToInt(550.0 / ratio), 1500, juce::roundToInt(1500.0 / ratio));
            getConstrainer()->setFixedAspectRatio(ratio);
        }
        else if (currentZoomState == 1) {
             const float ratio = 16.0/ 9.0;
                    setSize (800, juce::roundToInt (1500.0 / ratio));
                        setResizeLimits (550,  juce::roundToInt (550.0 / ratio),
                                                                  1500, juce::roundToInt (1500.0 / ratio));
                          getConstrainer()->setFixedAspectRatio (ratio);
                      repaint();
        }
        else if (currentZoomState == 2) {
            const float ratio = 16.0/ 9.0;
                       setSize (1500, juce::roundToInt (1500.0 / ratio));
                       setResizeLimits (550,  juce::roundToInt (550.0 / ratio),
                                                   1500, juce::roundToInt (1500.0 / ratio));
                   getConstrainer()->setFixedAspectRatio (ratio);
                       repaint();
        }
        
        repaint();
    };
    
    addAndMakeVisible(slopIcon);
    addAndMakeVisible(slopIcon2);
    addAndMakeVisible(squeezeIcon);
    addAndMakeVisible(offsetIkon);
    
    addAndMakeVisible(twoValueSlider2);
    twoValueSlider2.setLookAndFeel(&twoValLaf);
    squeezeSlider.setLookAndFeel(&sliderLaf);
    offsetSlider.setLookAndFeel(&crossOverLaf);
    highCutSlopeSlider.setLookAndFeel(&slopSliderLaf);
    lowCutSlopeSlider.setLookAndFeel(&slopSliderLaf);
    
   
   // setWantsKeyboardFocus(true);
}

SqueezeFilterAudioProcessorEditor::~SqueezeFilterAudioProcessorEditor()
{
    squeezeSlider.setLookAndFeel(nullptr);
    offsetSlider.setLookAndFeel(nullptr);
    highCutSlopeSlider.setLookAndFeel(nullptr);
    lowCutSlopeSlider.setLookAndFeel(nullptr);

    twoValueSlider2.setLookAndFeel(nullptr);
  
       //    delete myKeyListener;
}

//==============================================================================
void SqueezeFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
   
        using namespace juce;
        float centerX = getWidth() / 2.0f;
        float centerY = getHeight() / 2.0f;
        float radius = jmin(centerX * 8, centerY * 2.5f);
        Colour colour2 = Colour::fromFloatRGBA(0.12f, 0.12f, 0.15f, 1.0f);
        Colour colour1 = Colour::fromFloatRGBA(0.18f, 0.22f, 0.25f, 1.0f);
        ColourGradient gradient(colour1, centerX, centerY, colour2, centerX, centerY + radius, true);
        g.setGradientFill(gradient);
        g.fillAll();
  
}

void SqueezeFilterAudioProcessorEditor::resized()
{
    audioProcessor.setEditorSize (getWidth(), getHeight());
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds().reduced(10,10);
    auto menuButtons = bounds.removeFromLeft(35).removeFromTop(60);
    auto buttonArea = menuButtons.removeFromTop(35);
    //buttonArea
    zoomOneButton.setBounds(buttonArea);
    bounds = bounds.removeFromBottom(bounds.getHeight() * 0.95f);
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.8f);
    auto topSliderArea = responseArea.removeFromTop(responseArea.getHeight()* 0.3f);
    auto labelarea = topSliderArea.removeFromRight(bounds.getWidth() * 0.15f);
    squeezeIcon.setBounds(labelarea.removeFromBottom(topSliderArea.getHeight()*0.5).removeFromTop(labelarea.getHeight()*0.8));
    auto modifySliderArea = responseArea.removeFromRight(bounds.getWidth() * 0.15f);
    squeezeSlider.setBounds(modifySliderArea);
    auto offsetRec = topSliderArea.removeFromLeft(bounds.getWidth() * 0.85f).reduced(responseArea.getWidth()*0.2, 0);
    offsetSlider.setBounds(offsetRec.removeFromBottom(offsetRec.getHeight()*0.65f).translated(0, - bounds.getHeight()*0.1));
    offsetIkon.setBounds(offsetRec.removeFromBottom(offsetRec.getHeight()*0.8));
    responseCurveComponent.setBounds(responseArea);
    int buttonLeft = responseCurveComponent.getX();
    int buttonTop = responseCurveComponent.getBottom(); // Add a spacing of 10 pixels
    analyzerEnabledButton.setBounds(buttonLeft, buttonTop + 5, 30, 30);
    auto sliderBounds = responseArea.reduced(responseArea.getWidth() * 0.001f, 0.0f);
    twoValueSlider2.setBounds(sliderBounds);
    auto filterKnobsArea = bounds.removeFromLeft(responseArea.getWidth());
    auto lowCutArea = filterKnobsArea.removeFromLeft(filterKnobsArea.getWidth() * 0.33f);
    lowCutArea = lowCutArea.removeFromRight(lowCutArea.getWidth()*0.8).translated(responseArea.getWidth()*0.1, 0);
    lowCutSlopeSlider.setBounds(lowCutArea.removeFromRight(lowCutArea.getWidth()* 0.6));
    slopIcon.setBounds(lowCutArea.removeFromRight(lowCutArea.getWidth()* 0.8).translated(5,  - lowCutArea.getHeight()*0.1));
    auto highCutArea = filterKnobsArea.removeFromLeft(filterKnobsArea.getWidth() * 0.5f);
    highCutSlopeSlider.setBounds(lowCutSlopeSlider.getBounds().translated(responseArea.getWidth()*0.4, 0));
    slopIcon2.setBounds(slopIcon.getBounds().translated(responseArea.getWidth()*0.4, 0));
   
}

std::vector<juce::Component*> SqueezeFilterAudioProcessorEditor::getComps()
{
    lowCutSlopeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    highCutSlopeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    lowCutSlopeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    highCutSlopeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lowCutFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    highCutFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    
    offsetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    squeezeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    offsetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    return
    {
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &squeezeSlider,
        &offsetSlider,
        &analyzerEnabledButton,
        &responseCurveComponent
    };
}


