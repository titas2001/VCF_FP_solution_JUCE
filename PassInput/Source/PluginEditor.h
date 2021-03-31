/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class PassInputAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private juce::Slider::Listener 
{
public:
    PassInputAudioProcessorEditor (PassInputAudioProcessor&);
    ~PassInputAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void sliderValueChanged (juce::Slider* slider) override; // [3]
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PassInputAudioProcessor& audioProcessor;
    
    juce::Slider controlK;
    juce::Slider controlVt;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PassInputAudioProcessorEditor)
};
