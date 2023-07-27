/*
  ==============================================================================

    Filter.cpp
 
  ==============================================================================
*/
#include <JuceHeader.h>
#include "Filter.h"

using namespace juce;

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts, double& lastLowCut, double& lastHighCut){
  
    ChainSettings settings;
    float offset = apvts.getRawParameterValue("OffsetValue")->load();
    double lowCutFreq = apvts.getRawParameterValue("hp")->load();
    double highCutFreq = apvts.getRawParameterValue("lp")->load();

    double minValue = 20.0;
    double maxValue = 20000.0;
       // Map the value to the range 0-1
    double normalizedLowCutValue = (lowCutFreq - minValue) / (maxValue - minValue);
       // Apply exponential mapping
    double convertedLowCutValue = minValue * std::pow(maxValue / minValue, normalizedLowCutValue);
    lowCutFreq = convertedLowCutValue;
    double normalizedHighCutValue = (highCutFreq - minValue) / (maxValue - minValue);
       // Apply exponential mapping
    double convertedHighCutValue = minValue * std::pow(maxValue / minValue, normalizedHighCutValue);
    highCutFreq = convertedHighCutValue;
    
    auto squeezeValue = apvts.getRawParameterValue("SqueezeValue")->load();

    //Clamp double Slider
    if(lowCutFreq > highCutFreq)
    {
        highCutFreq = lastHighCut;
        lowCutFreq = lastLowCut;
    }
        lastHighCut = highCutFreq;
        lastLowCut = lowCutFreq;
        
        if(offset >= 0)
        {
            float maxMin = lowCutFreq;
            offset =  offset * std::pow(offset / 20000, 2);
            offset = jmap(offset, 0.0f, 20000.0f, 0.0f, 20000.0f - maxMin);
            settings.lowCutFreq = lowCutFreq * squeezeValue + offset;
            settings.highCutFreq = 20000 - (20000 - highCutFreq) * squeezeValue  + offset;
        }
        
        else
        {
            float maxMin = highCutFreq;
            offset = jmap(offset, -20000.0f, 0.0f, -maxMin, 0.0f);
            settings.lowCutFreq = lowCutFreq * squeezeValue + offset;
            settings.highCutFreq = 20000 - (20000 - highCutFreq) * squeezeValue + offset;
            
        }
    
    // Clamp the values to the valid range
    settings.lowCutFreq = std::clamp(settings.lowCutFreq, 20.f, 20000.0f);
    settings.highCutFreq = std::clamp(settings.highCutFreq, 20.0f, 20000.f);
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCutSlope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCutSlope")->load());
        
    return settings;
}
