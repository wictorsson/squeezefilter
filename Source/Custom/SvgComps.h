/*
  ==============================================================================

    SvgComps.h

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
using namespace juce;

class svgSlopeComp : public juce::Component
{
public:
    svgSlopeComp()
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(svgSlopeComp)
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

class svgOffsetComp : public juce::Component
{
public:
    svgOffsetComp()
    {
        svgImage = juce::Drawable::createFromImageData(BinaryData::offsetIkon_svg, BinaryData::offsetIkon_svgSize);
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(svgOffsetComp)
};

