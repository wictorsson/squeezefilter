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
//    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
//    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
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
//    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> mags;
    mags.resize(w);
    
    for(int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        
//        if(!monoChain.isBypassed<ChainPositions::Peak>())
//            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
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
: AudioProcessorEditor (&p), audioProcessor (p),lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCutFreq", lowCutFreqSlider),highCutFreqSliderAttachment(audioProcessor.apvts, "HighCutFreq", highCutFreqSlider),lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCutSlope", lowCutSlopeSlider),highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCutSlope", highCutSlopeSlider),squeezeSliderAttachment(audioProcessor.apvts, "SqueezeValue", squeezeSlider),offsetSliderAttachment(audioProcessor.apvts, "OffsetValue", offsetSlider), analyzerEnabledButtonAttachment(audioProcessor.apvts, "AnalyzerEnabled", analyzerEnabledButton) ,responseCurveComponent(audioProcessor)

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
    
    zoomOneButton.setButtonText("[ ]");

    zoomOneButton.onClick = [this] {
        currentZoomState = (currentZoomState + 1) % 3; // Toggle between 0, 1, 2
        
        const float ratio = 16.0 / 9.0;
        
        if (currentZoomState == 0) {
            setSize(500, juce::roundToInt(1500.0 / ratio));
            setResizeLimits(500, juce::roundToInt(500.0 / ratio), 1500, juce::roundToInt(1500.0 / ratio));
            getConstrainer()->setFixedAspectRatio(ratio);
        }
        else if (currentZoomState == 1) {
             const float ratio = 16.0/ 9.0;
                    setSize (800, juce::roundToInt (1500.0 / ratio));
                        setResizeLimits (500,  juce::roundToInt (500.0 / ratio),
                                                                  1500, juce::roundToInt (1500.0 / ratio));
                          getConstrainer()->setFixedAspectRatio (ratio);
                      repaint();
        }
        else if (currentZoomState == 2) {
            const float ratio = 16.0/ 9.0;
                       setSize (1500, juce::roundToInt (1500.0 / ratio));
                       setResizeLimits (500,  juce::roundToInt (500.0 / ratio),
                                                   1500, juce::roundToInt (1500.0 / ratio));
                   getConstrainer()->setFixedAspectRatio (ratio);
                       repaint();
        }
        
        repaint();
    };


    addAndMakeVisible(zoomOneButton);

    addAndMakeVisible(twoValueSlider);
    twoValueSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
    twoValueSlider.setRange(std::log10(20.0), std::log10(20000.0)); // Range in logarithmic scale
    
    twoValueSlider.addListener(this);
    twoValueSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    
    twoValueSlider.setMaxValue(std::log10(*audioProcessor.apvts.getRawParameterValue("HighCutFreq")));
    twoValueSlider.setMinValue(std::log10(*audioProcessor.apvts.getRawParameterValue("LowCutFreq")));
   
    addAndMakeVisible(squeezeLabel);
    squeezeLabel.setFont(juce::Font (12.0f, juce::Font::bold));
    squeezeLabel.setText("|---|", juce::dontSendNotification);
    squeezeLabel.attachToComponent(&squeezeSlider, false);
    squeezeLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(offsetLabel);
    offsetLabel.setFont(juce::Font (12.0f, juce::Font::bold));
    offsetLabel.setText("<--->", juce::dontSendNotification);
    offsetLabel.attachToComponent(&offsetSlider, false);
    offsetLabel.setJustificationType(juce::Justification::centred);
    
    
    addAndMakeVisible(slopeLabel);
    slopeLabel.setFont(juce::Font (12.0f, juce::Font::bold));
    slopeLabel.setText("/ --", juce::dontSendNotification);
    slopeLabel.attachToComponent(&lowCutSlopeSlider, true);
    slopeLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(slopeLabel2);
    slopeLabel2.setFont(juce::Font (12.0f, juce::Font::bold));
    slopeLabel2.setText("/ --", juce::dontSendNotification);
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
    
    squeezeSlider.setLookAndFeel(&sliderLaf);
    twoValueSlider.setLookAndFeel(&twoValLaf);
    offsetSlider.setLookAndFeel(&crossOverLaf);
    highCutSlopeSlider.setLookAndFeel(&slopSliderLaf);
    lowCutSlopeSlider.setLookAndFeel(&slopSliderLaf);
    
    setResizable (true, true);
    
    const float ratio = 16.0/ 9.0;
    setSize (p.getEditorWidth(), p.getEditorHeight());
    setResizeLimits (500,  juce::roundToInt (500.0 / ratio),
                         1500, juce::roundToInt (1500.0 / ratio));
    
    getConstrainer()->setFixedAspectRatio (ratio);
  
   // setWantsKeyboardFocus(true);

}

SqueezeFilterAudioProcessorEditor::~SqueezeFilterAudioProcessorEditor()
{
    twoValueSlider.setLookAndFeel(nullptr);
    squeezeSlider.setLookAndFeel(nullptr);
    offsetSlider.setLookAndFeel(nullptr);
    highCutSlopeSlider.setLookAndFeel(nullptr);
    lowCutSlopeSlider.setLookAndFeel(nullptr);
    
   // removeKeyListener(this);
       //    delete myKeyListener;
    
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
    
   
    
    auto bounds = getLocalBounds().reduced(0,0);
    auto buttonArea = bounds.removeFromTop(35).removeFromLeft(55).reduced(8, 8);

    zoomOneButton.setBounds(buttonArea);
    
    
    bounds = bounds.removeFromBottom(bounds.getHeight() * 0.92f);
    bounds = bounds.removeFromRight(bounds.getWidth() * 0.98f);

    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.8f);
    auto topSliderArea = responseArea.removeFromTop(responseArea.getWidth()* 0.08f);
    auto modifySliderArea = responseArea.removeFromRight(bounds.getWidth() * 0.15f);
    
    offsetSlider.setBounds(topSliderArea.removeFromLeft(bounds.getWidth() * 0.85f).reduced(responseArea.getWidth()*0.2, 10));
    squeezeSlider.setBounds(modifySliderArea);

    responseCurveComponent.setBounds(responseArea);
   
    auto sliderBounds = responseArea.reduced(responseArea.getWidth() * 0.018f, 0.0f);
    twoValueSlider.setBounds(sliderBounds);
    
    bounds = bounds.removeFromRight(bounds.getWidth() * 0.98f);
    auto analyzerArea = bounds.removeFromLeft(bounds.getWidth() * 0.1f).removeFromTop(bounds.getHeight() * 0.4f);
    analyzerEnabledButton.setBounds(analyzerArea);
    
    
    auto filterKnobsArea = bounds.removeFromRight(bounds.getWidth()* 0.9);
    
    auto lowCutArea = filterKnobsArea.removeFromLeft(filterKnobsArea.getWidth() * 0.33f);
    auto highCutArea = filterKnobsArea;

    lowCutSlopeSlider.setBounds(lowCutArea.reduced(45, 0));
    highCutSlopeSlider.setBounds(highCutArea.removeFromLeft(highCutArea.getWidth() * 0.5f).reduced(45, 0));
    
    audioProcessor.setEditorSize (getWidth(), getHeight());
    
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
    analyzerEnabledButton.setButtonText("~~~");
    return
    {
      
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

// ******* LAF *****************************************************************************************
using namespace juce;

//CustomSlopSlider

CustomSlopSlider::CustomSlopSlider(){}

void CustomSlopSlider::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos,
                                       float minSliderPos,
                                       float maxSliderPos,
                                       const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (slider.isBar())
    {
       //g.setColour (slider.findC olour (Slider::trackColourId));
        g.setColour (juce::Colour::fromFloatRGBA(0.34f, 0.64f, 0.56f, 1.0f).darker(0.4f));
        g.fillRect (slider.isHorizontal() ? juce::Rectangle<float> (static_cast<float> (x), (float) y + 0.5f, sliderPos - (float) x, (float) height - 1.0f)
                                          : juce::Rectangle<float> ((float) x + 0.5f, sliderPos, (float) width - 1.0f, (float) y + ((float) height - sliderPos)));
    }
    else
    {
        auto isTwoVal   = (style == juce::Slider::SliderStyle::TwoValueVertical   || style == juce::Slider::SliderStyle::TwoValueHorizontal);
        auto isThreeVal = (style == juce::Slider::SliderStyle::ThreeValueVertical || style == juce::Slider::SliderStyle::ThreeValueHorizontal);

        auto trackWidth = juce::jmin (6.0f, slider.isHorizontal() ? (float) height * 0.25f : (float) width * 0.25f);
        
        trackWidth = trackWidth * 2;

        juce::Point<float> startPoint (slider.isHorizontal() ? (float) x : (float) x + (float) width * 0.5f,
                                 slider.isHorizontal() ? (float) y + (float) height * 0.5f : (float) (height + y));

        juce::Point<float> endPoint (slider.isHorizontal() ? (float) (width + x) : startPoint.x,
                               slider.isHorizontal() ? startPoint.y : (float) y);

        juce::Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.strokePath (backgroundTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

        juce::Path valueTrack;
        juce::Point<float> minPoint, maxPoint, thumbPoint;

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
        thumbWidth = thumbWidth * 2;

        valueTrack.startNewSubPath (minPoint);
        valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
        g.setColour (juce::Colours::lightblue);
        g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });


        if (! isTwoVal)
        {
            g.setColour (juce::Colours::lightblue);
          //  g.setColour (slider.findColour (juce::Slider::backgroundColourId));
            g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
            g.setColour (slider.findColour (juce::Slider::backgroundColourId));
//            g.setColour (juce::Colours::orange);
            g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth) * 0.8,  0.8 * static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
        }

        
        if (isTwoVal || isThreeVal)
        {
            auto sr = juce::jmin (trackWidth, (slider.isHorizontal() ? (float) height : (float) width) * 0.4f);
            auto pointerColour = slider.findColour (juce::Slider::thumbColourId);

            if (slider.isHorizontal())
            {
                drawPointer (g, minSliderPos - sr,
                             juce::jmax (0.0f, (float) y + (float) height * 0.5f - trackWidth * 2.0f),
                             trackWidth * 2.0f, pointerColour, 2);

                drawPointer (g, maxSliderPos - trackWidth,
                             juce::jmin ((float) (y + height) - trackWidth * 2.0f, (float) y + (float) height * 0.5f),
                             trackWidth * 2.0f, pointerColour, 4);
            }
            else
            {
                drawPointer (g, juce::jmax (0.0f, (float) x + (float) width * 0.5f - trackWidth * 2.0f),
                             minSliderPos - trackWidth,
                             trackWidth * 2.0f, pointerColour, 1);

                drawPointer (g, juce::jmin ((float) (x + width) - trackWidth * 2.0f, (float) x + (float) width * 0.5f), maxSliderPos - sr,
                             trackWidth * 2.0f, pointerColour, 3);
            }
        }
    }
}

juce::Label* CustomSlopSlider::createSliderTextBox (juce::Slider& slider)
{
    auto* l = new juce::Label();

    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, slider.findColour (juce::Slider::textBoxTextColourId));
    l->setColour (juce::Label::textWhenEditingColourId, slider.findColour (juce::Slider::textBoxTextColourId));
    l->setColour (juce::Label::outlineWhenEditingColourId, juce::Colours::transparentWhite);
    l->setInterceptsMouseClicks (false, false);
    l->setFont (14.0f);

    return l;
}


CustomCrossover::CustomCrossover(){}

void CustomCrossover::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos,
                                       float minSliderPos,
                                       float maxSliderPos,
                                       const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (slider.isBar())
    {
       //g.setColour (slider.findC olour (Slider::trackColourId));
        g.setColour (juce::Colour::fromFloatRGBA(0.34f, 0.64f, 0.56f, 1.0f).darker(0.4f));
        g.fillRect (slider.isHorizontal() ? juce::Rectangle<float> (static_cast<float> (x), (float) y + 0.5f, sliderPos - (float) x, (float) height - 1.0f)
                                          : juce::Rectangle<float> ((float) x + 0.5f, sliderPos, (float) width - 1.0f, (float) y + ((float) height - sliderPos)));
    }
    else
    {
        auto isTwoVal   = (style == juce::Slider::SliderStyle::TwoValueVertical   || style == juce::Slider::SliderStyle::TwoValueHorizontal);
        auto isThreeVal = (style == juce::Slider::SliderStyle::ThreeValueVertical || style == juce::Slider::SliderStyle::ThreeValueHorizontal);

        auto trackWidth = juce::jmin (6.0f, slider.isHorizontal() ? (float) height * 0.25f : (float) width * 0.25f);
        
        trackWidth = trackWidth * 2;

        juce::Point<float> startPoint (slider.isHorizontal() ? (float) x : (float) x + (float) width * 0.5f,
                                 slider.isHorizontal() ? (float) y + (float) height * 0.5f : (float) (height + y));

        juce::Point<float> endPoint (slider.isHorizontal() ? (float) (width + x) : startPoint.x,
                               slider.isHorizontal() ? startPoint.y : (float) y);

        juce::Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.strokePath (backgroundTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

        juce::Path valueTrack;
        juce::Point<float> minPoint, maxPoint, thumbPoint;

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

//        auto thumbWidth = getSliderThumbRadius (slider) * 3 ;
//
//
//        if (! isTwoVal)
//        {
//            g.setColour (juce::Colour::fromFloatRGBA(0.2941f, 0.4784f, 0.2784f, 1.0f));
//            g.fillRoundedRectangle (juce::Rectangle<float> (static_cast<float> (thumbWidth) * 0.2, static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint), 3);
//        }
        
        auto thumbWidth = getSliderThumbRadius (slider);
        thumbWidth = thumbWidth * 2;

//        valueTrack.startNewSubPath (minPoint);
//        valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
//        g.setColour (juce::Colours::lightblue);
//        g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });
        
        // Changed filled path to go from 0 - value
        valueTrack.startNewSubPath (width * 0.5f + thumbWidth/2, startPoint.y);
        valueTrack.lineTo (maxPoint);

        g.setColour (juce::Colours::lightblue);

        g.strokePath (valueTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });


        if (! isTwoVal)
        {
            g.setColour (juce::Colours::lightblue);
          //  g.setColour (slider.findColour (juce::Slider::backgroundColourId));
            g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
            g.setColour (slider.findColour (juce::Slider::backgroundColourId));
//            g.setColour (juce::Colours::orange);
            g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth) * 0.8,  0.8 * static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
        }

        
        
        
        if (isTwoVal || isThreeVal)
        {
            auto sr = juce::jmin (trackWidth, (slider.isHorizontal() ? (float) height : (float) width) * 0.4f);
            auto pointerColour = slider.findColour (juce::Slider::thumbColourId);

            if (slider.isHorizontal())
            {
                drawPointer (g, minSliderPos - sr,
                             juce::jmax (0.0f, (float) y + (float) height * 0.5f - trackWidth * 2.0f),
                             trackWidth * 2.0f, pointerColour, 2);

                drawPointer (g, maxSliderPos - trackWidth,
                             juce::jmin ((float) (y + height) - trackWidth * 2.0f, (float) y + (float) height * 0.5f),
                             trackWidth * 2.0f, pointerColour, 4);
            }
            else
            {
                drawPointer (g, juce::jmax (0.0f, (float) x + (float) width * 0.5f - trackWidth * 2.0f),
                             minSliderPos - trackWidth,
                             trackWidth * 2.0f, pointerColour, 1);

                drawPointer (g, juce::jmin ((float) (x + width) - trackWidth * 2.0f, (float) x + (float) width * 0.5f), maxSliderPos - sr,
                             trackWidth * 2.0f, pointerColour, 3);
            }
        }
    }
}

juce::Label* CustomCrossover::createSliderTextBox (juce::Slider& slider)
{
    auto* l = new juce::Label();

    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, slider.findColour (juce::Slider::textBoxTextColourId));
    l->setColour (juce::Label::textWhenEditingColourId, slider.findColour (juce::Slider::textBoxTextColourId));
    l->setColour (juce::Label::outlineWhenEditingColourId, juce::Colours::transparentWhite);
    l->setInterceptsMouseClicks (false, false);
    l->setFont (14.0f);

    return l;
}


CustomSlider::CustomSlider(){}
void CustomSlider::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos,
                                       float minSliderPos,
                                       float maxSliderPos,
                                       const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (slider.isBar())
    {
        g.setColour (slider.findColour (Slider::trackColourId));
        g.fillRect (slider.isHorizontal() ? Rectangle<float> (static_cast<float> (x), (float) y + 0.5f, sliderPos - (float) x, (float) height - 1.0f)
                                          : Rectangle<float> ((float) x + 0.5f, sliderPos, (float) width - 1.0f, (float) y + ((float) height - sliderPos)));

        drawLinearSliderOutline (g, x, y, width, height, style, slider);
    }
    else
    {
        auto isTwoVal   = (style == Slider::SliderStyle::TwoValueVertical   || style == Slider::SliderStyle::TwoValueHorizontal);
        auto isThreeVal = (style == Slider::SliderStyle::ThreeValueVertical || style == Slider::SliderStyle::ThreeValueHorizontal);

        auto trackWidth = jmin (6.0f, slider.isHorizontal() ? (float) height * 0.25f : (float) width * 0.25f);
        trackWidth = trackWidth * 2;
        

        Point<float> startPoint (slider.isHorizontal() ? (float) x : (float) x + (float) width * 0.5f,
                                 slider.isHorizontal() ? (float) y + (float) height * 0.5f : (float) (height + y));

        Point<float> endPoint (slider.isHorizontal() ? (float) (width + x) : startPoint.x,
                               slider.isHorizontal() ? startPoint.y : (float) y);

        Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        g.setColour (slider.findColour (Slider::backgroundColourId));
        g.strokePath (backgroundTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });

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
        thumbWidth = thumbWidth * 2;

        valueTrack.startNewSubPath (minPoint);
        valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
        g.setColour (juce::Colours::lightblue);
        g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });

        if (! isTwoVal)
        {
            g.setColour (juce::Colours::lightblue);
           // g.setColour (slider.findColour (juce::Slider::backgroundColourId));
            g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
            
            g.setColour (slider.findColour (juce::Slider::backgroundColourId));
            g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth) * 0.8,  0.8 * static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
        }

        if (isTwoVal || isThreeVal)
        {
            auto sr = jmin (trackWidth, (slider.isHorizontal() ? (float) height : (float) width) * 0.4f);
            auto pointerColour = slider.findColour (Slider::thumbColourId);

            if (slider.isHorizontal())
            {
                drawPointer (g, minSliderPos - sr,
                             jmax (0.0f, (float) y + (float) height * 0.5f - trackWidth * 2.0f),
                             trackWidth * 2.0f, pointerColour, 2);

                drawPointer (g, maxSliderPos - trackWidth,
                             jmin ((float) (y + height) - trackWidth * 2.0f, (float) y + (float) height * 0.5f),
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

        if (slider.isBar())
            drawLinearSliderOutline (g, x, y, width, height, style, slider);
    }
}


juce::Label* CustomSlider::createSliderTextBox (juce::Slider& slider)
{
    auto* l = new juce::Label();

    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, slider.findColour (juce::Slider::textBoxTextColourId));
    l->setColour (juce::Label::textWhenEditingColourId, slider.findColour (juce::Slider::textBoxTextColourId));
    l->setColour (juce::Label::outlineWhenEditingColourId, juce::Colours::transparentWhite);
    l->setInterceptsMouseClicks (false, false);
    l->setFont (14.0f);

    return l;
}



void CustomTwoValSliderLaf::drawPointer (Graphics& g, const float x, const float y, const float diameter,
                                  const Colour& colour, const int direction, float height)
{
    Path p;
    float lengthFactor = height * 0.035;  // Adjust the length factor as desired
  //  diameter = diameter * height * 0.025;
    
    p.startNewSubPath (x + diameter * 0.5f, y);
    p.lineTo (x + diameter, y + diameter * 0.2f * lengthFactor);
    p.lineTo (x + diameter, y + diameter * lengthFactor);
    p.lineTo (x, y + diameter * lengthFactor);
    p.lineTo (x, y + diameter * 0.2f * lengthFactor);
    p.closeSubPath();

    p.applyTransform (AffineTransform::rotation ((float) direction * MathConstants<float>::halfPi,
                                                 x + diameter * 0.5f, y + diameter * 0.5f));

    // Create a gradient from orange to transparent
    ColourGradient gradient;
    
    if(direction == 4)
    {
        gradient = ColourGradient(Colours::orange, 0.0f, y, Colour(0x00FF7F00), 0.0f, y + diameter * lengthFactor, false);
        gradient.addColour(1.0f, Colour(0x00FF7F00));
    }
    else if(direction == 2)
    {
//        gradient = ColourGradient(Colour(0x00FF7F00), 0.0f, y, Colours::orange, 0.0f, y - diameter * lengthFactor, false);
//               gradient.addColour(1.0f, Colours::orange);
        
        gradient = ColourGradient(Colours::orange, 0.0f, y, Colour(0x00FF7F00), 0.0f, y - diameter * lengthFactor, false);
        gradient.addColour(1.0f, Colour(0x00FF7F00));
    }
    
    

    g.setGradientFill(gradient);
    
  //  g.setColour (colour);
    g.fillPath (p);
    

}


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
                                    trackWidth * 2.0f, pointerColour, 2, height);

                       drawPointer (g, maxSliderPos - trackWidth,
                                    jmin ((float) (y + height) - trackWidth * 2.0f, (float) y + (float) (height + 18) * 0.5f),
                                    trackWidth * 2.0f, pointerColour, 4,height);
                   }
                   else
                   {
                       drawPointer (g, jmax (0.0f, (float) x + (float) width * 0.5f - trackWidth * 2.0f),
                                    minSliderPos - trackWidth,
                                    trackWidth * 2.0f, pointerColour, 1,height);

                       drawPointer (g, jmin ((float) (x + width) - trackWidth * 2.0f, (float) x + (float) width * 0.5f), maxSliderPos - sr,
                                    trackWidth * 2.0f, pointerColour, 3, height);
                   }
               }
           }
       }
