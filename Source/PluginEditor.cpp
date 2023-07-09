/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


ResponseCurveComponent::ResponseCurveComponent(SqueezeFilterAudioProcessor& p) : audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo), rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for(auto param : params)
    {
        param->addListener(this);
    }
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
    auto chainSettings = getChainSettings(audioProcessor.apvts,audioProcessor.lastLowCutParam,audioProcessor.lastHighCutParam);
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
    g.fillAll (Colour::fromFloatRGBA (0.10f, 0.11f, 0.13f, 1.0f));

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
    
    g.setColour(Colours::orange);
    g.strokePath(responseCurve, PathStrokeType(4.f));

//    donePainting = true;
//    DBG("REPAINTED");
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
        g.drawVerticalLine(x, top + 20 * getLocalBounds().getWidth()* 0.0025, bottom);
    }
    
    Array<float> gain
    {
        -24, -12,0,12,24
    };
    
    for(auto gDb : gain)
    {
        auto y = jmap(gDb, -20.f, 20.f, float(bottom), float(top));
      //  auto y = jmap(gDb, -1.f, 1.f, float(bottom), float(top));
        
        g.setColour(gDb == 0.f ? Colours::lightblue : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
    
    g.setColour(Colours::lightblue);
    const int fontHeight = 10 * getLocalBounds().getWidth()* 0.0025;
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
        r.setY(12);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
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
    twoValueSlider.setLookAndFeel(nullptr);
    twoValueSlider2.setLookAndFeel(nullptr);
   // removeKeyListener(this);
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
        //&lowCutFreqSlider,
       // &highCutFreqSlider,
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
       // g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.setColour (Colour::fromFloatRGBA (0.1f, 0.12f, 0.15f, 1.0f));
        
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
       // g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.setColour (Colour::fromFloatRGBA (0.1f, 0.12f, 0.15f, 1.0f));
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
//        g.setColour (slider.findColour (Slider::trackColourId));
//        g.fillRect (slider.isHorizontal() ? Rectangle<float> (static_cast<float> (x), (float) y + 0.5f, sliderPos - (float) x, (float) height - 1.0f)
//                                          : Rectangle<float> ((float) x + 0.5f, sliderPos, (float) width - 1.0f, (float) y + ((float) height - sliderPos)));
//
//        drawLinearSliderOutline (g, x, y, width, height, style, slider);
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
        //g.setColour (slider.findColour (Slider::backgroundColourId));
        g.setColour (Colour::fromFloatRGBA (0.1f, 0.12f, 0.15f, 1.0f));
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

//        if (slider.isBar())
//            drawLinearSliderOutline (g, x, y, width, height, style, slider);
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
//        gradient = ColourGradient(Colours::orange, 0.0f, y, Colour(0x00FF7F00), 0.0f, y + diameter * lengthFactor * 0.8, false);
//        gradient.addColour(1.0f, Colour(0x00FF7F00));
        
        gradient = ColourGradient::vertical(Colours::orange, y,
                                            Colours::orange.withAlpha(0.0f), y + diameter * lengthFactor );
    }
    else if(direction == 2)
    {
//        gradient = ColourGradient(Colours::orange, 0.0f, y, Colour(0x00FF7F00), 0.0f, y - diameter * lengthFactor * 0.5, false);
     
        
        gradient = ColourGradient::vertical(Colours::orange, y,
                                            Colours::orange.withAlpha(0.0f), y - diameter * lengthFactor * 0.8);
        //gradient.addColour(1.0f, Colours::orange);
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

            //   auto thumbWidth = getSliderThumbRadius (slider);

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
                                    jmax (0.0f, (float) y + (float) (height - 18) * 0.5f - trackWidth * 2.0f),
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
