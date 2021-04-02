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
class VCFAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private juce::Slider::Listener 
{
public:
    VCFAudioProcessorEditor (VCFAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~VCFAudioProcessorEditor() override;

    //==============================================================================
    void sliderValueChanged(juce::Slider* slider) override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VCFAudioProcessor& audioProcessor;
        juce::AudioProcessorValueTreeState& audioTree;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    juce::Slider controlK;
    juce::Slider controlF0;
    juce::Slider controlVt;
    juce::Label labelK;
    juce::Label labelF0;
    juce::Label labelVt;

    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachK;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachF0;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachVt;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VCFAudioProcessorEditor)
};
