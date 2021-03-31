/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/

class VCFAudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    VCFAudioProcessor();
    ~VCFAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    //==============================================================================
    //void setK(double val) { controlledK = val; };
    //void setVt(double val) { controlledF0 = val; };
    void parameterChanged(const juce::String& parameterID, float newValue);
    void updateFilter(bool realFreq);
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioProcessorValueTreeState audioTree;

private:
    double K, controlledK, Vt, controlledF0, f0;
    double I0, C, Fs, gamma, eta, err, T;
    double vin1, vc1, vc2, vc3, vc4, vc11, vc21, vc31, vc41, /*u1, u2, u3, u4, u5,*/ s1, s2, s3, s4, xc1, xc2, xc3, xc4;
    double vc4Past;
    double vin1Temp, vc1Temp, vc11Temp, vc2Temp, vc21Temp, vc3Temp, vc31Temp, vc4Temp, vc41Temp,
        /*u1Temp, u2Temp, u3Temp, u4Temp, u5Temp, s1Temp, s2Temp, s3Temp, s4Temp,*/ xc1Temp, xc2Temp, xc3Temp, xc4Temp;
    
    float vin;
    float vout, voutTemp;
    float* voutP = &vout;
    int upsamplingScale;
    //juce::AudioProcessorValueTreeState audioTree;
    juce::dsp::ProcessorDuplicator< juce::dsp::IIR::Filter <float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;

    std::vector<float> upVout, upVin;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VCFAudioProcessor)
};
