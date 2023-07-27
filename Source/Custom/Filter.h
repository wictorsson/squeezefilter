/*
  ==============================================================================

    Filter.h

  ==============================================================================
*/
#include <JuceHeader.h>

#pragma once

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings
{
    float lowCutFreq {0}, highCutFreq{0};
    Slope lowCutSlope{Slope::Slope_12}, highCutSlope{Slope::Slope_12};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts, double& lastLowCut, double& lastHighCut );


using Filter = juce::dsp::IIR::Filter<float>;
//CREATE 4 filters for the different slopes
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
//LowCut, HighCut
using MonoChain = juce::dsp::ProcessorChain<CutFilter, CutFilter>;


enum ChainPositions
{
    LowCut,
    HighCut,
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replacements);


template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType &leftLowCut, const CoefficientType& cutCoefficients, const Slope &lowCutSlope)
{
    leftLowCut.template setBypassed<0>(true);
    leftLowCut.template setBypassed<1>(true);
    leftLowCut.template setBypassed<2>(true);
    leftLowCut.template setBypassed<3>(true);
    
    switch(lowCutSlope)
    {
        case Slope_48:
        {
            update<3>(leftLowCut, cutCoefficients);
           
            break;
        }
        case Slope_36:
        {
            update<2>(leftLowCut, cutCoefficients);
            break;
        }
        case Slope_24:
        {
            update<1>(leftLowCut, cutCoefficients);
            break;
        }
        case Slope_12:
        {
            update<0>(leftLowCut, cutCoefficients);
            break;
        }
    }
}

inline auto makeLowCutFilter(const ChainSettings chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2 * (chainSettings.lowCutSlope) + 1);
}

inline auto makeHighCutFilter(const ChainSettings chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, sampleRate, 2 * (chainSettings.highCutSlope) + 1);
}
