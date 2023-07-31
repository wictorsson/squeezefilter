/*
  ==============================================================================

    ResponseComp.cpp

  ==============================================================================
*/

#include "ResponseComp.h"
#include "colors.h"

ResponseCurveComponent::ResponseCurveComponent(SqueezeFilterAudioProcessor& p) : audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo), rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    // Set freq response before timer starts
    auto chainSettings = getChainSettings(audioProcessor.apvts,audioProcessor.lastLowCutParam,audioProcessor.lastHighCutParam);
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
    
    for(auto param : params)
    {
        param->addListener(this);
    }
    startTimerHz(60);
    
   // audioProcessor.rmsLevelLeft.getCurrentValue();
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

void PathProducer::process(juce::Rectangle<float> fftbounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;

    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            auto size = tempIncomingBuffer.getNumSamples();

            // Create a separate copy of the tempIncomingBuffer to preserve the original audio data.
            juce::AudioBuffer<float> originalBuffer(tempIncomingBuffer);

            // Calculate the RMS value of the incoming buffer.
            float sumOfSquares = 0.0f;
            for (int channel = 0; channel < tempIncomingBuffer.getNumChannels(); ++channel)
            {
                const float* channelData = tempIncomingBuffer.getReadPointer(channel);
                for (int i = 0; i < size; ++i)
                {
                    float sample = channelData[i];
                    sumOfSquares += sample * sample;
                }
            }
            float rmsValue = std::sqrt(sumOfSquares / (tempIncomingBuffer.getNumChannels() * size));
            //float rmsValue = tempIncomingBuffer.getRMSLevel(0, 0, tempIncomingBuffer.getNumSamples());
            if (rmsValue > 0.05)
            {
                //APply gain to tempIncomingBuffer - - THE FADE

                // For example, you can copy the buffer to monoBuffer, apply gain, etc.
                juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0), monoBuffer.getReadPointer(0, size), monoBuffer.getNumSamples() - size);
                juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size), tempIncomingBuffer.getReadPointer(0, 0), size);
                
                leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
            }
            else
            {
                // If the RMS value is not greater than the threshold, do something else.
                // For example, you can apply a different gain to the buffer.
                monoBuffer.applyGain(0.99f);
                leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
            }
        }
    }

            



    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / (double)fftSize;

    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftbounds, fftSize, binWidth, -48.f);
        }
    }

    while (pathProducer.getNumPathsAvailable())
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
}


void ResponseCurveComponent::timerCallback()
{
    
    if(parametersChanged.compareAndSetBool(false, true) || doOnce)
    {
        doOnce = !doOnce;
        auto chainSettings = getChainSettings(audioProcessor.apvts,audioProcessor.lastLowCutParam,audioProcessor.lastHighCutParam);
        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
    }
    
    if(shouldShowFFTAnalysis){
        auto fftBounds = getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();
        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }
    
    repaint();
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
   // g.fillAll (Colour::fromFloatRGBA (0.10f, 0.11f, 0.13f, 1.0f));

    g.fillAll (Colour::fromFloatRGBA(0.42f, 0.42f, 0.15f, 0.0f));
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
   
   
    g.setColour(Colours::darkgrey);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
   // Colour myColourLime = Colour::fromFloatRGBA(0.5255f, 0.7098f, 0.2078f, 1.0f);
    g.setColour(myColourLime);
    g.strokePath(responseCurve, PathStrokeType(4.f));

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
