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
    
    //const auto fftBounds = getAnalysisArea().toFloat();
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

    if(parametersChanged.compareAndSetBool(false, true))
    {
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        
        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
       // repaint();
    }
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
    
   // responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    for( size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    
    if(shouldShowFFTAnalysis)
    {
        
        auto leftChannelFFTPath = leftPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(),responseArea.getY()));
        
        g.setColour(Colours::white);
        g.strokePath(leftChannelFFTPath, PathStrokeType(1));
        
        auto rightChannelFFTPath = rightPathProducer.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(),responseArea.getY()));
        
        g.setColour(Colours::white);
        g.strokePath(rightChannelFFTPath, PathStrokeType(1));
        
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
        g.drawVerticalLine(x, top, bottom);
    }
    
    Array<float> gain
    {
        -24, -12,0,12,24
    };
    

    for(auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        g.setColour(gDb == 0.f ? Colours::orange : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
   // g.drawRect(getRenderArea());
    
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
        r.setY(1);
        
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
    
    
    setSize (650, 350);
}

SqueezeFilterAudioProcessorEditor::~SqueezeFilterAudioProcessorEditor()
{}

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
    
    auto bounds = getLocalBounds();
    auto marginTop = bounds.removeFromTop(bounds.getHeight() * 0.08f);
    auto marginLeft = bounds.removeFromLeft(bounds.getWidth() * 0.01f);
    auto marginRight = bounds.removeFromRight(bounds.getWidth() * 0.01f);
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.5f);
  
    auto modifySliderArea = responseArea.removeFromRight(bounds.getWidth() * 0.25f);
    
    offsetSlider.setBounds(modifySliderArea.removeFromRight(modifySliderArea.getWidth()*0.5f));
    squeezeSlider.setBounds(modifySliderArea);
    
    
    responseCurveComponent.setBounds(responseArea);
    auto marginRightMid = bounds.removeFromLeft(bounds.getWidth() * 0.08f);
    auto filterKnobsArea = bounds.removeFromLeft(bounds.getWidth() * 0.66f);
    
    auto lowCutArea = filterKnobsArea.removeFromLeft(filterKnobsArea.getWidth() * 0.33f);
    auto highCutArea = filterKnobsArea.removeFromRight(filterKnobsArea.getWidth() * 0.5f);
    
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight()* 0.5f));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight()* 0.5f));
    highCutSlopeSlider.setBounds(highCutArea);
    analyzerEnabledButton.setBounds(bounds);
    
}



std::vector<juce::Component*> SqueezeFilterAudioProcessorEditor::getComps()
{
    lowCutSlopeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    highCutSlopeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    lowCutFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    highCutFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
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
