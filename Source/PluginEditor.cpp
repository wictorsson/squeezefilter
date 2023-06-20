/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


ResponseCurveComponent::ResponseCurveComponent(SqueezeFilterAudioProcessor& p) : audioProcessor(p),
//leftChannelFifo(&audioProcessor.leftChannelFifo)
leftPathProducer(audioProcessor.leftChannelFifo), rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for(auto param : params)
    {
        param->addListener(this);
    }
    
   // addAndMakeVisible(analyzerEnabledButton);
    
    startTimerHz(60); 
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for(auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float>fftbounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    
    while(leftChannelFifo->getNumCompleteBuffersAvailable()>0)
    {
        if(leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            auto size = tempIncomingBuffer.getNumSamples();
            
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0), monoBuffer.getReadPointer(0, size), monoBuffer.getNumSamples()-size);
            
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples()-size),tempIncomingBuffer.getReadPointer(0,0), size);
            
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }
    
 
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / (double)fftSize;
    
    while(leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks()>0)
    {
        std::vector<float> fftData;
        if(leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftbounds,fftSize, binWidth, -48.f);
        }
    }
    
    while(pathProducer.getNumPathsAvailable())
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
    
    
}

void ResponseCurveComponent::timerCallback()
{
    
    if(shouldShowFFTAnalysis){
        auto fftBounds = getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();
        
        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }

//    if(parametersChanged.compareAndSetBool(false, true))
//    {
//        auto chainSettings = getChainSettings(audioProcessor.apvts);
//        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
//        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
//
//        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
//        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
//
//        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
//
//        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
//
//    }
    
    // REFACTOR - do only once then check if parameterchanged like above
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
  
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
    repaint();
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour::fromFloatRGBA (0.08f, 0.08f, 0.08f, 1.0f));

    g.drawImage(background, getLocalBounds().toFloat());
   // auto bounds = getLocalBounds();
    auto responseArea = getAnalysisArea();
    auto w = responseArea.getWidth();
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> mags;
    mags.resize(w);
    
    for(int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        
        if(!monoChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if(!lowcut.isBypassed<0>())
        {
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!lowcut.isBypassed<1>())
        {
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!lowcut.isBypassed<2>())
        {
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!lowcut.isBypassed<3>())
        {
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        if(!highcut.isBypassed<0>())
        {
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!highcut.isBypassed<1>())
        {
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!highcut.isBypassed<2>())
        {
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!highcut.isBypassed<3>())
        {
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        mags[i] = Decibels::gainToDecibels(mag);
    }
    
    Path responseCurve;
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    for( size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    
    if(shouldShowFFTAnalysis)
    {
        
        auto leftChannelFFTPath = leftPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(),responseArea.getY()));
        
        g.setColour(Colours::lightblue);
        g.strokePath(leftChannelFFTPath, PathStrokeType(3));
        
        auto rightChannelFFTPath = rightPathProducer.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(),responseArea.getY()));
        
        g.setColour(Colours::lightblue);
        g.strokePath(rightChannelFFTPath, PathStrokeType(3));
        
    }
    
    g.setColour(Colours::darkgrey);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
    
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
    
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(),getHeight(), true);
    
    Graphics g(background);
    
    Array<float> freqs
    {
        50,100,500,1000,2000,5000,10000
    };
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    Array<float> xs;
    
    for(auto f : freqs)
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }
    
    g.setColour(Colours::darkgrey);
    for(auto x : xs)
    {
        g.drawVerticalLine(x, top + 20, bottom);
    }
    
    Array<float> gain
    {
        -24, -12,0,12,24
    };
    

    for(auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        g.setColour(gDb == 0.f ? Colours::lightblue : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
    
    g.setColour(Colours::lightblue);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    for(int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];
        
        bool addK = false;
        String str;
        if(f>999.f)
        {
            addK = true;
            f /= 1000.f;
            
        }
        str << f;
        if(addK)
            str << "k";
     //   str << "Hz";
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(20);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
    
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);
    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}

//==============================================================================
SqueezeFilterAudioProcessorEditor::SqueezeFilterAudioProcessorEditor (SqueezeFilterAudioProcessor& p)
: AudioProcessorEditor (&p), audioProcessor (p), peakFreqSliderAttachment(audioProcessor.apvts, "PeakFreq", peakFreqSlider),peakGainSliderAttachment(audioProcessor.apvts, "PeakGain", peakGainSlider),peakQualitySliderAttachment(audioProcessor.apvts, "PeakQuality", peakQualitySlider),lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCutFreq", lowCutFreqSlider),highCutFreqSliderAttachment(audioProcessor.apvts, "HighCutFreq", highCutFreqSlider),lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCutSlope", lowCutSlopeSlider),highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCutSlope", highCutSlopeSlider),squeezeSliderAttachment(audioProcessor.apvts, "SqueezeValue", squeezeSlider),offsetSliderAttachment(audioProcessor.apvts, "OffsetValue", offsetSlider), analyzerEnabledButtonAttachment(audioProcessor.apvts, "AnalyzerEnabled", analyzerEnabledButton) ,responseCurveComponent(audioProcessor)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    for(auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    
    
    auto safePtr = juce::Component::SafePointer<SqueezeFilterAudioProcessorEditor>(this);
    analyzerEnabledButton.onClick = [safePtr]()
    {
        if(auto* comp = safePtr.getComponent())
        {
            auto enabled = comp->analyzerEnabledButton.getToggleState();
            comp->responseCurveComponent.toggleAnalyzerIsEnabled(enabled);
        }
    };
    
    addAndMakeVisible(twoValueSlider);
    twoValueSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
    
   
    
    twoValueSlider.setRange(std::log10(20.0), std::log10(20000.0)); // Range in logarithmic scale
    
    twoValueSlider.addListener(this);
    twoValueSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    
    twoValueSlider.setMaxValue(std::log10(*audioProcessor.apvts.getRawParameterValue("HighCutFreq")));
    twoValueSlider.setMinValue(std::log10(*audioProcessor.apvts.getRawParameterValue("LowCutFreq")));
   
    addAndMakeVisible(squeezeLabel);
    squeezeLabel.setFont(juce::Font (12.0f, juce::Font::bold));
    squeezeLabel.setText("Squeeze", juce::dontSendNotification);
    squeezeLabel.attachToComponent(&squeezeSlider, false);
    squeezeLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(offsetLabel);
    offsetLabel.setFont(juce::Font (12.0f, juce::Font::bold));
    offsetLabel.setText("Offset", juce::dontSendNotification);
    offsetLabel.attachToComponent(&offsetSlider, false);
    offsetLabel.setJustificationType(juce::Justification::centred);
    
    
    addAndMakeVisible(slopeLabel);
    slopeLabel.setFont(juce::Font (12.0f, juce::Font::bold));
    slopeLabel.setText("Slope", juce::dontSendNotification);
    slopeLabel.attachToComponent(&lowCutSlopeSlider, true);
    slopeLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(slopeLabel2);
    slopeLabel2.setFont(juce::Font (12.0f, juce::Font::bold));
    slopeLabel2.setText("Slope", juce::dontSendNotification);
    slopeLabel2.attachToComponent(&highCutSlopeSlider, true);
    slopeLabel2.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(freqLabel2);
    freqLabel2.setFont(juce::Font (12.0f, juce::Font::bold));
    freqLabel2.setText("Freq", juce::dontSendNotification);
    freqLabel2.attachToComponent(&highCutFreqSlider, true);
    freqLabel2.setJustificationType(juce::Justification::centred);
 
    
    addAndMakeVisible(freqLabel);
    freqLabel.setFont(juce::Font (12.0f, juce::Font::bold));
    freqLabel.setText("Freq", juce::dontSendNotification);
    freqLabel.attachToComponent(&lowCutFreqSlider, true);
    freqLabel.setJustificationType(juce::Justification::centred);
    
    twoValueSlider.setLookAndFeel(&twoValLaf);
    
    setSize (650, 330);
}

SqueezeFilterAudioProcessorEditor::~SqueezeFilterAudioProcessorEditor()
{
    twoValueSlider.setLookAndFeel(nullptr);
    
}

//==============================================================================
void SqueezeFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (Colour::fromFloatRGBA (0.08f, 0.08f, 0.08f, 1.0f));
    

}

void SqueezeFilterAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds().reduced(0.025f,0.025f);
    bounds = bounds.removeFromBottom(bounds.getHeight() * 0.92f);
    bounds = bounds.removeFromRight(bounds.getWidth() * 0.98f);
    
  
   
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.8f);
    auto topSliderArea = responseArea.removeFromTop(responseArea.getWidth()* 0.08f);
    
    auto modifySliderArea = responseArea.removeFromRight(bounds.getWidth() * 0.2f);
    
    
    offsetSlider.setBounds(topSliderArea.removeFromLeft(bounds.getWidth() * 0.8f).reduced(responseArea.getWidth()*0.2, 10));
    //offsetSlider.setBounds(modifySliderArea.removeFromRight(modifySliderArea.getWidth() * 0.5f));
    squeezeSlider.setBounds(modifySliderArea);

    responseCurveComponent.setBounds(responseArea);
    auto sliderBounds = responseArea.reduced(responseArea.getWidth() * 0.018f, 0.0f);
    twoValueSlider.setBounds(sliderBounds);
   
    bounds = bounds.removeFromRight(bounds.getWidth() * 0.92f);
    
    //analyzerEnabledButton.setBounds(bounds.removeFromTop(bounds.getHeight()*0.2f));
    auto filterKnobsArea = bounds.removeFromLeft(bounds.getWidth()* 0.8);
    auto lowCutArea = filterKnobsArea.removeFromLeft(filterKnobsArea.getWidth() * 0.33f);
    auto highCutArea = filterKnobsArea.removeFromRight(filterKnobsArea.getWidth() * 0.5f);
  
//    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight()* 0.5f));
//    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight()* 0.5f));
    analyzerEnabledButton.setBounds(filterKnobsArea);
    lowCutSlopeSlider.setBounds(lowCutArea.reduced(45, 0));
    highCutSlopeSlider.setBounds(highCutArea.reduced(45, 0));
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
    analyzerEnabledButton.setButtonText("Analyzer");
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &squeezeSlider,
        &offsetSlider,
        &analyzerEnabledButton,
        &responseCurveComponent
    };
}

using namespace juce;
void CustomTwoValSliderLaf::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos,
                                              float minSliderPos,
                                              float maxSliderPos,
                                              const Slider::SliderStyle style, Slider& slider)
       {
           if (slider.isBar())
           {
//               g.setColour (slider.findColour (Slider::trackColourId));
//               g.fillRect (slider.isHorizontal() ? Rectangle<float> (static_cast<float> (x), (float) y + 0.5f, sliderPos - (float) x, (float) height - 1.0f)
//                                                 : Rectangle<float> ((float) x + 0.5f, sliderPos, (float) width - 1.0f, (float) y + ((float) height - sliderPos)));
           }
           else
           {
               auto isTwoVal   = (style == Slider::SliderStyle::TwoValueVertical   || style == Slider::SliderStyle::TwoValueHorizontal);
               auto isThreeVal = (style == Slider::SliderStyle::ThreeValueVertical || style == Slider::SliderStyle::ThreeValueHorizontal);

               auto trackWidth = jmin (6.0f, slider.isHorizontal() ? (float) height * 0.25f : (float) width * 0.25f);

               Point<float> startPoint (slider.isHorizontal() ? (float) x : (float) x + (float) width * 0.5f,
                                        slider.isHorizontal() ? (float) y + (float) height * 0.5f : (float) (height + y));

               Point<float> endPoint (slider.isHorizontal() ? (float) (width + x) : startPoint.x,
                                      slider.isHorizontal() ? startPoint.y : (float) y);

               Path backgroundTrack;
               backgroundTrack.startNewSubPath (startPoint);
               backgroundTrack.lineTo (endPoint);
//               g.setColour (slider.findColour (Slider::backgroundColourId));
//               g.strokePath (backgroundTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });

               Path valueTrack;
               Point<float> minPoint, maxPoint, thumbPoint;

               if (isTwoVal || isThreeVal)
               {
                   minPoint = { slider.isHorizontal() ? minSliderPos : (float) width * 0.5f,
                                slider.isHorizontal() ? (float) height * 0.5f : minSliderPos };

                   if (isThreeVal)
                       thumbPoint = { slider.isHorizontal() ? sliderPos : (float) width * 0.5f,
                                      slider.isHorizontal() ? (float) height * 0.5f : sliderPos };

                   maxPoint = { slider.isHorizontal() ? maxSliderPos : (float) width * 0.5f,
                                slider.isHorizontal() ? (float) height * 0.5f : maxSliderPos };
               }
               else
               {
                   auto kx = slider.isHorizontal() ? sliderPos : ((float) x + (float) width * 0.5f);
                   auto ky = slider.isHorizontal() ? ((float) y + (float) height * 0.5f) : sliderPos;

                   minPoint = startPoint;
                   maxPoint = { kx, ky };
               }

               auto thumbWidth = getSliderThumbRadius (slider);

               valueTrack.startNewSubPath (minPoint);
               valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
//               g.setColour (slider.findColour (Slider::trackColourId));
//               g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });

               if (! isTwoVal)
               {
//                   g.setColour (slider.findColour (Slider::thumbColourId));
//                   g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
               }

               if (isTwoVal || isThreeVal)
               {
                   auto sr = jmin (trackWidth, (slider.isHorizontal() ? (float) height : (float) width) * 0.4f);
                   auto pointerColour = juce::Colours::orange;

                   if (slider.isHorizontal())
                   {
                       drawPointer (g, minSliderPos - sr,
                                    jmax (0.0f, (float) y + (float) (height + 3) * 0.5f - trackWidth * 2.0f),
                                    trackWidth * 2.0f, pointerColour, 2);

                       drawPointer (g, maxSliderPos - trackWidth,
                                    jmin ((float) (y + height) - trackWidth * 2.0f, (float) y + (float) (height + 18) * 0.5f),
                                    trackWidth * 2.0f, pointerColour, 4);
                   }
                   else
                   {
                       drawPointer (g, jmax (0.0f, (float) x + (float) width * 0.5f - trackWidth * 2.0f),
                                    minSliderPos - trackWidth,
                                    trackWidth * 2.0f, pointerColour, 1);

                       drawPointer (g, jmin ((float) (x + width) - trackWidth * 2.0f, (float) x + (float) width * 0.5f), maxSliderPos - sr,
                                    trackWidth * 2.0f, pointerColour, 3);
                   }
               }
           }
       }
