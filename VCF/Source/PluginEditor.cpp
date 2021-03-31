/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VCFAudioProcessorEditor::VCFAudioProcessorEditor (VCFAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), audioTree(vts)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (300, 250);
    
    // these define the parameters of our slider object
    controlK.setSliderStyle (juce::Slider::LinearBarVertical);
    controlK.addListener(this);
    controlK.setTextBoxStyle (juce::Slider::NoTextBox, false, 90, 0);
    addAndMakeVisible(controlK);
    sliderAttachK.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioTree, "controlK_ID", controlK));
    controlK.setPopupDisplayEnabled (true, false, this);
    labelK.setText(("Feedback Gain"), juce::dontSendNotification);
    labelK.setFont(juce::Font("Slope Opera", 16, 0));
    labelK.setColour(juce::Label::textColourId, juce::Colour(3, 3, 3));
    addAndMakeVisible(labelK);

    controlF0.setSliderStyle(juce::Slider::LinearVertical);
    controlF0.addListener(this);
    controlF0.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    addAndMakeVisible(controlF0);
    sliderAttachF0.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(audioTree, "controlF0_ID", controlF0));
    controlF0.setPopupDisplayEnabled(true, false, this);
    labelF0.setText(("Fundamental"), juce::dontSendNotification);
    labelF0.setFont(juce::Font("Slope Opera", 16, 0));
    labelF0.setColour(juce::Label::textColourId, juce::Colour(3, 3, 3));
    addAndMakeVisible(labelF0);

}

VCFAudioProcessorEditor::~VCFAudioProcessorEditor()
{
    sliderAttachK.reset();
    sliderAttachF0.reset();
}

//==============================================================================
void VCFAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::white);
 
    // set the current drawing colour to black
    g.setColour (juce::Colours::black);
 
    // set the font size and draw text to the screen
    g.setFont (15.0f);
    g.setFont(juce::Font("Slope Opera", 35.0f, 1));
    g.drawFittedText ("VCF", getLocalBounds(), juce::Justification::centred, 1);
}

void VCFAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    int sliderPaddingBottom = 60, sliderWidth = 40, labelHeight = 20;

    controlK.setBounds (40, 30, sliderWidth, getHeight() - sliderPaddingBottom);
    controlF0.setBounds(getWidth()-80, 30, 20, getHeight() - sliderPaddingBottom);
    
    labelK.setBounds(40+sliderWidth/2-80/2, getHeight()- labelHeight, 80, labelHeight);
    labelF0.setBounds((getWidth()-70-80/2), getHeight()- labelHeight, 70, labelHeight);

}
void VCFAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
}