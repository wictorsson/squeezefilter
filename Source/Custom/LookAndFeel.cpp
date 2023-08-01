/*
  ==============================================================================

    LookAndFeel.cpp

  ==============================================================================
*/

#include "LookAndFeel.h"

#include "colors.h"
//using namespace juce;

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
        g.setColour (myColourLime);
        g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });


        if (! isTwoVal)
        {
            g.setColour (myColourLime);
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
        
        trackWidth = trackWidth * 2 ;

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

        
        auto thumbWidth = getSliderThumbRadius (slider);
 
        thumbWidth = thumbWidth * 2;
        
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
        


//        valueTrack.startNewSubPath (minPoint);
//        valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
//        g.setColour (juce::Colours::lightblue);
//        g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });
        
        // Changed filled path to go from 0 - value
        valueTrack.startNewSubPath (width * 0.5f + thumbWidth/2, startPoint.y);
        valueTrack.lineTo (maxPoint);

        g.setColour (myColourLime);

        g.strokePath (valueTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });


        if (! isTwoVal)
        {
            g.setColour (myColourLime);
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


CustomSliderLaf::CustomSliderLaf(){}
void CustomSliderLaf::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
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
        auto thumbWidth = getSliderThumbRadius (slider);
      
        
        trackWidth = trackWidth * 2;
        thumbWidth = thumbWidth * 2;
    
        
//        if(width < 106)
//        {
//
//
//
//        }
//        else
//        {
//            trackWidth = trackWidth * 2;
//            thumbWidth = thumbWidth * 2;
//        }
        
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

            minPoint = { startPoint }; // Apply offset to the minimum value
            maxPoint = { kx, ky };
        }

        

        valueTrack.startNewSubPath (minPoint);
        valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
        g.setColour (myColourLime);
        g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });

        if (! isTwoVal)
        {
            g.setColour (myColourLime);
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



juce::Label* CustomSliderLaf::createSliderTextBox (juce::Slider& slider)
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
      float lineWidth = diameter; // Make the diameter of the circle equivalent to the line width for a vertical line.

      p.startNewSubPath(x + lineWidth * 0.5f, y);
      p.lineTo(x + lineWidth * 0.5f, y + height);

  //    p.applyTransform(AffineTransform::rotation((float) direction * MathConstants<float>::halfPi,
            //                                     x + lineWidth * 0.5f, y + height * 0.5f));


    
    ColourGradient gradient(myColourLime.withAlpha(0.0f), x + lineWidth * 0.5f, y,
                            myColourLime, x + lineWidth * 0.5f, y + height * 0.4f, false);


       // Set the gradient as the stroke for the path
       g.setGradientFill(gradient);
       g.strokePath(p, PathStrokeType(lineWidth));
    
    if(direction == 2)
    {
        Path p;
          float lineWidth = diameter * 200; // Make the diameter of the circle equivalent to the line width for a vertical line.

          p.startNewSubPath(x - lineWidth * 0.5f, y);
          p.lineTo(x - lineWidth * 0.5f, y + height);

//          p.applyTransform(AffineTransform::rotation((float) direction * MathConstants<float>::halfPi,
//                                                     x + lineWidth * 0.5f, y + height * 0.5f));


        
        ColourGradient gradient(myColourLime.withAlpha(0.0f), x + lineWidth * 0.5f, y,
                                myColourLime.withAlpha(0.10f), x + lineWidth * 0.5f, y + height * 0.4f, false);


           // Set the gradient as the stroke for the path
           g.setGradientFill(gradient);
           g.strokePath(p, PathStrokeType(lineWidth));
    }
    if(direction == 4)
    {
        Path p;
          float lineWidth = diameter * 200; // Make the diameter of the circle equivalent to the line width for a vertical line.

          p.startNewSubPath(x + lineWidth * 0.5f, y);
          p.lineTo(x + lineWidth * 0.5f, y + height);

//          p.applyTransform(AffineTransform::rotation((float) direction * MathConstants<float>::halfPi,
//                                                     x + lineWidth * 0.5f, y + height * 0.5f));


        
        ColourGradient gradient(myColourLime.withAlpha(0.0f), x + lineWidth * 0.5f, y,
                                myColourLime.withAlpha(0.10f), x + lineWidth * 0.5f, y + height * 0.4f, false);


           // Set the gradient as the stroke for the path
           g.setGradientFill(gradient);
           g.strokePath(p, PathStrokeType(lineWidth));
    }
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

//               if (slider.isHorizontal())
//               {
//                   drawPointer (g, minSliderPos - sr,
//                                jmax (0.0f, (float) y + (float) (height/2) * 0.5f - trackWidth * 2.0f),
//                                trackWidth * height * 0.01f, pointerColour, 2, height/2);
//
//                   drawPointer (g, maxSliderPos - trackWidth,
//                                jmin ((float) (y + height * 2.f) - trackWidth * 2.0f, (float) y + (float) (height) * 0.7f),
//                                trackWidth * height * 0.01 , pointerColour, 4,height/2);
//               }
//               else
//               {
//                   drawPointer (g, jmax (0.0f, (float) x + (float) width * 0.5f - trackWidth * 2.0f),
//                                minSliderPos - trackWidth,
//                                trackWidth * 2.0f, pointerColour, 1,height);
//
//                   drawPointer (g, jmin ((float) (x + width) - trackWidth * 2.0f, (float) x + (float) width * 0.5f), maxSliderPos - sr,
//                                trackWidth * 2.0f, pointerColour, 3, height);
//               }
               
               if (isTwoVal || isThreeVal)
                    {
                    auto sr = jmin (trackWidth, (slider.isHorizontal() ? (float) height : (float) width) * 0.4f);
                       auto pointerColour = myColourLime;

//                       if (slider.isHorizontal())
//                       {
//                           drawPointer (g, minSliderPos - sr,
//                                        jmax (0.0f, (float) y + (float) (height - 18) * 0.5f - trackWidth * 2.0f),
//                                        trackWidth * 2.0f, pointerColour, 2, height);
//
//                           drawPointer (g, maxSliderPos - trackWidth,
//                                        jmin ((float) (y + height) - trackWidth * 2.0f, (float) y + (float) (height + 18) * 0.5f),
//                                        trackWidth * 2.0f, pointerColour, 4,height);
//                       }
                        
                        if (slider.isHorizontal())
                        {
                            drawPointer(g, minSliderPos - sr, y , height * 0.025, pointerColour, 2, height);
                            
                            drawPointer(g, maxSliderPos - trackWidth, y , height * 0.025, pointerColour, 4, height);
//                              drawPointer(g, maxSliderPos - trackWidth, y + (height + 18) * 0.5f, height, pointerColour, 0, height);

//                            drawPointer (g, maxSliderPos - trackWidth,
//                                         jmin ((float) (y + height) - trackWidth * 2.0f, (float) y + (float) (height + 18) * 0.5f),
//                                         trackWidth * 2.0f, pointerColour, 4,height);
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
