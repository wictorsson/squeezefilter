/*
  ==============================================================================

    LookAndFeel.h

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

using namespace juce;

class CustomSlopSlider : public juce::LookAndFeel_V4
{
public:
    CustomSlopSlider();
   void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider& slider) override;
};


class CustomCrossover : public juce::LookAndFeel_V4
{
public:
    CustomCrossover();
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider& slider) override;
};

class CustomSliderLaf : public juce::LookAndFeel_V4
{
public:
    CustomSliderLaf();
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider& slider) override;
private:
    std::string type;
    
};

class CustomTwoValSliderLaf : public juce::LookAndFeel_V4

{
public:
   
    void drawPointer (Graphics&, float x, float y, float diameter,
                      const Colour&, int direction,float height) ;
    
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;
    
};
