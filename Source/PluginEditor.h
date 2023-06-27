/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

using namespace juce;


class svgComp : public juce::Component
{
public:
    svgComp()
    {
         slopeImage = juce::Drawable::createFromImageData(BinaryData::slopeicon_svg, BinaryData::slopeicon_svgSize);
    }

    void paint(juce::Graphics& g) override
    {
        // Draw the image
        if (slopeImage != nullptr)
            slopeImage->drawWithin(g, getLocalBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
    
private:
    std::unique_ptr<juce::Drawable> slopeImage;
    int image;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(svgComp)
};

class svgSqueezeComp : public juce::Component
{
public:
    svgSqueezeComp()
    {
        svgImage = juce::Drawable::createFromImageData(BinaryData::squeezeicon_svg, BinaryData::squeezeicon_svgSize);
    }

    void paint(juce::Graphics& g) override
    {
        // Draw the image
        if (svgImage != nullptr)
            svgImage->drawWithin(g, getLocalBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
    
private:
    std::unique_ptr<juce::Drawable> svgImage;
    int image;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(svgSqueezeComp)
};


//class MyToggleButton : public juce::Button
//{
//public:
//    MyToggleButton()
//        : juce::Button("MyToggleButton"), currentState(0)
//    {
//        setClickingTogglesState(true);
//        setButtonText("State 1");
//    }
//
//    void buttonClicked()
//    {
//        currentState = (currentState + 1) % 3; // Toggle between 0, 1, 2
//
//        switch (currentState)
//        {
//            case 0:
//                setButtonText("State 1");
//                break;
//            case 1:
//                setButtonText("State 2");
//                break;
//            case 2:
//                setButtonText("State 3");
//                break;
//            default:
//                break;
//        }
//
//        // Notify listeners or perform any other necessary actions based on the current state
//    }
//
//private:
//    int currentState;
//};




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

class CustomSlider : public juce::LookAndFeel_V4
{
public:
    CustomSlider();
   
    
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


enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13,
    order16384 = 14,
};

template<typename BlockType>
struct FFTDataGenerator
{
    /**
     produces the FFT data from an audio buffer.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();
        
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());
        
        // first apply a windowing function to our data
        window->multiplyWithWindowingTable (fftData.data(), fftSize);       // [1]
        
        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform (fftData.data());  // [2]
        
        int numBins = (int)fftSize / 2;
        
        //normalize the fft values.
        for( int i = 0; i < numBins; ++i )
        {
            auto v = fftData[i];
//            fftData[i] /= (float) numBins;
            if( !std::isinf(v) && !std::isnan(v) )
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }
        
        //convert them to decibels
        for( int i = 0; i < numBins; ++i )
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }
        
        fftDataFifo.push(fftData);
    }
    
    void changeOrder(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>
        
        order = newOrder;
        auto fftSize = getFFTSize();
        
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
        
        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    
    Fifo<BlockType> fftDataFifo;
};


template<typename PathType>
struct AnalyzerPathGenerator
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize,
                      float binWidth,
                      float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                              negativeInfinity, 0.f,
                              float(bottom+10),   top);
        };

        auto y = map(renderData[0]);

//        jassert( !std::isnan(y) && !std::isinf(y) );
        if( std::isnan(y) || std::isinf(y) )
            y = bottom;
        
        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for( int binNum = 1; binNum < numBins; binNum += pathResolution )
        {
            y = map(renderData[binNum]);

//            jassert( !std::isnan(y) && !std::isinf(y) );

            if( !std::isnan(y) && !std::isinf(y) )
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::LinearVertical, juce::Slider::TextEntryBoxPosition::TextBoxRight)
    {
        
    }
};

struct CustomDoubleSlider : juce::Slider, juce::Slider::Listener
{
    CustomDoubleSlider() : juce::Slider(juce::Slider::SliderStyle::LinearVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    {
        
    }
    
};


struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<SqueezeFilterAudioProcessor::BlockType>& scff) : leftChannelFifo(&scff)
    {
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order16384);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
        
    }
    void process(juce::Rectangle<float>fftbounds, double sampleRate);
    juce::Path getPath(){return leftChannelFFTPath;};
    
private:
    
    SingleChannelSampleFifo<SqueezeFilterAudioProcessor::BlockType>* leftChannelFifo;
    
    juce::AudioBuffer<float> monoBuffer;
    
    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
    
    AnalyzerPathGenerator<juce::Path> pathProducer;
    
    juce::Path leftChannelFFTPath;
    
    
};

struct AnalyzerButton : juce::ToggleButton{};


struct ResponseCurveComponent: juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer
{
    ResponseCurveComponent (SqueezeFilterAudioProcessor&);
    ~ResponseCurveComponent();
    
    void parameterValueChanged (int parameterIndex, float newValue) override;

   /** Indicates that a parameter change gesture has started.

       E.g. if the user is dragging a slider, this would be called with gestureIsStarting
       being true when they first press the mouse button, and it will be called again with
       gestureIsStarting being false when they release it.

       IMPORTANT NOTE: This will be called synchronously, and many audio processors will
       call it during their audio callback. This means that not only has your handler code
       got to be completely thread-safe, but it's also got to be VERY fast, and avoid
       blocking. If you need to handle this event on your message thread, use this callback
       to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
       message thread.
   */
   void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {};
   
    void timerCallback() override;
    
    void paint(juce::Graphics& g) override;
    
    void resized() override;
    
    void toggleAnalyzerIsEnabled(bool enabled)
    {
        shouldShowFFTAnalysis = enabled;
    };
    
private:
    SqueezeFilterAudioProcessor& audioProcessor;
    juce::Atomic<bool>parametersChanged {false};
    MonoChain monoChain;

    juce::Image background;
    
    juce::Rectangle<int> getRenderArea();
    
    juce::Rectangle<int> getAnalysisArea();
    
    PathProducer leftPathProducer, rightPathProducer;
    
    bool shouldShowFFTAnalysis = true;
    
  //  CustomRotarySlider twoValueSlider;
    
   //AnalyzerButton analyzerEnabledButton;
};
//==============================================================================
/**
*/
class SqueezeFilterAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Slider::Listener
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
//        if (key.getKeyCode() == 50)
//                 {
//                     const float ratio = 16.0/ 9.0;
//                     setSize (700, juce::roundToInt (1500.0 / ratio));
//                     setResizeLimits (500,  juce::roundToInt (500.0 / ratio),
//                                          1500, juce::roundToInt (1500.0 / ratio));
//                     getConstrainer()->setFixedAspectRatio (ratio);
//                     repaint();
//                     return true;
//                 }
//        if (key.getKeyCode() == 51)
//                 {
//                     const float ratio = 16.0/ 9.0;
//                     setSize (1500, juce::roundToInt (1500.0 / ratio));
//                     setResizeLimits (500,  juce::roundToInt (500.0 / ratio),
//                                          1500, juce::roundToInt (1500.0 / ratio));
//                     getConstrainer()->setFixedAspectRatio (ratio);
//                     repaint();
//                     return true;
//                 }
//
//
//
//        return false;
//    }

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SqueezeFilterAudioProcessor& audioProcessor;

    void sliderValueChanged (juce::Slider * slider) override
    {
        if(slider == &twoValueSlider)
        {
          
            lowCutFreqSlider.setValue(std::pow(10.0, twoValueSlider.getMinValue()));
            highCutFreqSlider.setValue(std::pow(10.0, twoValueSlider.getMaxValue()));
            
        }
    }
    
    CustomRotarySlider lowCutFreqSlider,
    highCutFreqSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider,
    squeezeSlider,
    
    offsetSlider;
    
    juce::Slider twoValueSlider;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment lowCutFreqSliderAttachment,
    highCutFreqSliderAttachment,
    lowCutSlopeSliderAttachment,
    highCutSlopeSliderAttachment,
    squeezeSliderAttachment,
    offsetSliderAttachment;
    
    juce::TextButton analyzerEnabledButton;
   
    using ButtonAttachment = APVTS::ButtonAttachment;
    ButtonAttachment analyzerEnabledButtonAttachment;
    
    ResponseCurveComponent responseCurveComponent;
    std::vector<juce::Component*> getComps();

    juce::Label slopeLabel, slopeLabel2,  squeezeLabel, offsetLabel, freqLabel, freqLabel2;
    CustomTwoValSliderLaf twoValLaf;
    CustomSlider sliderLaf;
    CustomCrossover crossOverLaf;
    CustomSlopSlider slopSliderLaf;
    
    juce::TextButton zoomOneButton;
    int currentZoomState;
 
  
    juce::Image squeezeImage = juce::ImageCache::getFromMemory(BinaryData::squeezeImage_png, BinaryData::squeezeImage_pngSize);
    juce::ImageComponent squeezeImageComp;
    
 
    
    juce::ImageComponent slopeImageComp;
    juce::ImageComponent slopeImageComp2;


    svgComp slopIcon;
    svgComp slopIcon2;
    
    svgSqueezeComp squeezeIcon;
//    juce::TextButton zoomThreeButton;
   
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqueezeFilterAudioProcessorEditor)
};
