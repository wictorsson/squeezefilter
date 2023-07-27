/*
  ==============================================================================

    Params.h

  ==============================================================================
*/

#pragma once
#include "LookAndFeel.h"

class MyTwoValueSlider : public juce::Slider
    {
    public:
        MyTwoValueSlider(juce::Slider::SliderStyle style_, juce::RangedAudioParameter* minpar, juce::RangedAudioParameter* maxpar) :
            minAttach(*minpar, [this](float x) { setMinValue(x, juce::dontSendNotification); }),
            maxAttach(*maxpar, [this](float x) { setMaxValue(x, juce::dontSendNotification); })
        {
            setTextBoxStyle(NoTextBox, true, 0, 0);
            setSliderStyle(style_);
            setRange(minpar->getNormalisableRange().start, maxpar->getNormalisableRange().end);

            maxAttach.sendInitialUpdate();
            minAttach.sendInitialUpdate();
            
            onDragStart = [this]()
            {
                thumbThatWasDragged = getThumbBeingDragged();
                minAttach.beginGesture();
                maxAttach.beginGesture();
                    
            };
            onValueChange = [this]()
            {
                
                minAttach.setValueAsPartOfGesture(getMinValue());
                maxAttach.setValueAsPartOfGesture(getMaxValue());

                    
            };
            onDragEnd = [this]()
            {
                minAttach.endGesture();
                maxAttach.endGesture();
            };
        }
        
        
        void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override
           {
               MouseWheelDetails wheelNew = wheel;
               wheelNew.deltaY /= 2.f;
               
               if(getMinValue()- wheelNew.deltaY*2000 <= getMaxValue()+ wheelNew.deltaY*2000){
                   minAttach.setValueAsCompleteGesture(getMinValue()- wheelNew.deltaY*2000);
                   maxAttach.setValueAsCompleteGesture(getMaxValue()+ wheelNew.deltaY*2000);
                   setMinValue(getMinValue() - wheelNew.deltaY*2000);
                   setMaxValue(getMaxValue() + wheelNew.deltaY*2000);

               }
               Slider::mouseWheelMove(e, wheelNew);
           }
        
    private:
        juce::ParameterAttachment minAttach;
        juce::ParameterAttachment maxAttach;
        int thumbThatWasDragged = 0;
        CustomTwoValSliderLaf twoValLaf;
};


struct CustomSlider : juce::Slider
{
    CustomSlider() : juce::Slider(juce::Slider::SliderStyle::LinearVertical, juce::Slider::TextEntryBoxPosition::TextBoxRight)
    {
        
    }
};

struct AnalyzerButton : juce::ToggleButton{};
