/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PassInputAudioProcessorEditor::PassInputAudioProcessorEditor (PassInputAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (300, 200);
    
    // these define the parameters of our slider object
    controlK.setSliderStyle (juce::Slider::LinearBarVertical);
    controlK.setRange (0.0, 4.0, 0.001);
    controlK.setTextBoxStyle (juce::Slider::NoTextBox, false, 90, 0);
    controlK.setPopupDisplayEnabled (true, false, this);
    controlK.setTextValueSuffix ("K");
    controlK.setValue(3.0);
    controlK.setSkewFactorFromMidPoint(3.0);

    controlVt.setSliderStyle(juce::Slider::LinearBarVertical);
    controlVt.setRange(100.0, 5000.0, 1.0);
    controlVt.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    controlVt.setPopupDisplayEnabled(true, false, this);
    controlVt.setTextValueSuffix("f0");
    controlVt.setValue(500.0);
    controlVt.setSkewFactorFromMidPoint(1000.0);


    audioProcessor.setK(controlK.getValue());
    audioProcessor.setVt(controlVt.getValue());
    // this function adds the slider to the editor
    addAndMakeVisible (&controlK);
    addAndMakeVisible(&controlVt);
    // add the listener to the slider
    controlK.addListener (this);
    controlVt.addListener(this);
}

PassInputAudioProcessorEditor::~PassInputAudioProcessorEditor()
{
}

//==============================================================================
void PassInputAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::white);
 
    // set the current drawing colour to black
    g.setColour (juce::Colours::black);
 
    // set the font size and draw text to the screen
    g.setFont (15.0f);
    g.drawFittedText ("K and f0", getLocalBounds(), juce::Justification::centred, 1);
}

void PassInputAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    controlK.setBounds (40, 30, 20, getHeight() - 60);
    controlVt.setBounds(80, 30, 40, getHeight() - 60);
}
void PassInputAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    audioProcessor.KControl = controlK.getValue();
    audioProcessor.VtControl = controlVt.getValue();
}