/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
VCFAudioProcessor::VCFAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    audioTree(*this, nullptr, juce::Identifier("PARAMETERS"),
        { std::make_unique<juce::AudioParameterFloat>("controlK_ID","ControlK",juce::NormalisableRange<float>(0.0, 4.0, 0.001),0.5),
          std::make_unique<juce::AudioParameterFloat>("controlF0_ID","ControlF0",juce::NormalisableRange<float>(50.0, 3000.0, 1.0),1000.0)
        }),
    lowPassFilter(juce::dsp::IIR::Coefficients< float >::makeLowPass((48000.0 * 4.0), 20000.0))
#endif
{
    oversampling.reset(new juce::dsp::Oversampling<float>(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));
    audioTree.addParameterListener("controlK_ID", this);
    audioTree.addParameterListener("controlF0_ID", this);

    controlledK = 0.5;
    controlledF0 = 500.0;

}

VCFAudioProcessor::~VCFAudioProcessor()
{
    oversampling.reset();
}

//==============================================================================
const juce::String VCFAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VCFAudioProcessor::acceptsMidi() const
{
    return false;
}

bool VCFAudioProcessor::producesMidi() const
{
    return false;
}

bool VCFAudioProcessor::isMidiEffect() const
{
    return false;
}

double VCFAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VCFAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VCFAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VCFAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VCFAudioProcessor::getProgramName (int index)
{
    return {};
}

void VCFAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void VCFAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    oversampling->reset();
    oversampling->initProcessing(static_cast<size_t> (samplesPerBlock));

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate * 4;
    spec.maximumBlockSize = samplesPerBlock * 3;
    spec.numChannels = getTotalNumOutputChannels();

    lowPassFilter.prepare(spec);
    lowPassFilter.reset();
    // Set the constants
    Fs = sampleRate;
    T = 1 / Fs;
    C = 0.01e-6;
    f0 = 500.0;
    Vt = 0.026;
    I0 = 2.0 * Fs * std::tan(2.0 * 3.14 * f0 / Fs / 2.0) * 8.0 * C * Vt;
    eta = 1.836;
    gamma = eta * Vt;
    err = 10e-4;
    K = 0.5; // gfbbk in MALTLAB
    // Set the initial values to zero
    voutTemp = 0.0;
    vc2Temp = vc3Temp = vc4Temp = 0.0;
    vc11Temp = vc21Temp = vc31Temp = vc41Temp = 0.0;
    vc1 = vc2 = vc3 = vc4 = 0.0;
    vc11 = vc21 = vc31 = vc41 = 0.0;
    vin1 = 0.0;
    vout = 0.0;
    //u1Temp = u2Temp = u3Temp = u4Temp = u5Temp = 0.0;
    s1 = s2 = s3 = s4 = 0.0;
    xc1 = xc2 = xc3 = xc4 = 0.0;
    controlledK = 0.5;
    controlledF0 = 500.0;
}

void VCFAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool VCFAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
     || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;
        
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()/*
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()*/)
        return false;
    
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void VCFAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto mainInputOutput = getBusBuffer(buffer, true, 0);
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    /***************************************************************************************/
    // 1. Fill a new array here with upsampled input
    //      a. zeros array of size buffer*N
    //      b. assign input value to every N'th sample in the zeros array
    // 2. Apply low pass
    // 3. Run the VCF
    // 4. Apply low pass again 
    // 5. For loop to downsample

    I0 = 2.0 * Fs * std::tan(2.0 * 3.14 * controlledF0 / Fs / 2.0) * 8.0 * C * Vt; // slider controls the Vt


    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());


    juce::dsp::AudioBlock<float> blockInput(buffer);
    juce::dsp::AudioBlock<float> blockOutput = oversampling->processSamplesUp(blockInput);

    updateFilter(0);
    lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(blockOutput));
    for (int channel = 0; channel < blockOutput.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < blockOutput.getNumSamples(); ++sample)
        {
            vin = blockOutput.getSample(channel, sample);
            vc4Temp = 0.0;
            vc4Past = 1.0;

            while (std::abs(vc4Temp - vc4Past) > std::abs(vc4Past) * err)
            {
                vc4Past = vc4Temp;

                vin1Temp = std::tanh((vin - voutTemp) / (2.0 * Vt));

                xc1Temp = (I0 / 2.0 / C) * (vin1Temp + vc11Temp);
                vc1Temp = T / 2.0 * xc1Temp + s1;
                vc11Temp = std::tanh((vc2Temp - vc1Temp) / (2.0 * gamma));

                xc2Temp = (I0 / 2.0 / C) * (vc21Temp - vc11Temp);
                vc2Temp = T / 2.0 * xc2Temp + s2;
                vc21Temp = std::tanh((vc3Temp - vc2Temp) / (2.0 * gamma));

                xc3Temp = (I0 / 2.0 / C) * (vc31Temp - vc21Temp);
                vc3Temp = T / 2.0 * xc3Temp + s3;
                vc31Temp = std::tanh((vc4Temp - vc3Temp) / (2.0 * gamma));

                xc4Temp = (I0 / 2.0 / C) * (-vc41Temp - vc31Temp);
                vc4Temp = T / 2.0 * xc4Temp + s4;
                vc41Temp = std::tanh(vc4Temp / (6.0 * gamma));

                voutTemp = vc4Temp / 2.0 + vc4Temp * controlledK; //noteOnVel = K in literature = gfdbk in MATLAB
            }
            //updates
            vin1 = vin1Temp;
            xc1 = xc1Temp;
            xc2 = xc2Temp;
            xc3 = xc3Temp;
            xc4 = xc4Temp;
            vc1 = vc1Temp;
            vc2 = vc2Temp;
            vc3 = vc3Temp;
            vc4 = vc4Temp;
            vc11 = vc11Temp;
            vc21 = vc21Temp;
            vc31 = vc31Temp;
            vc41 = vc41Temp;
            vout = voutTemp;

            s1 = T / 2 * xc1 + vc1;
            s2 = T / 2 * xc2 + vc2;
            s3 = T / 2 * xc3 + vc3;
            s4 = T / 2 * xc4 + vc4;
            blockOutput.setSample(channel, sample, vout);
        }
    }
    updateFilter(0);
    lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(blockOutput));

    oversampling->processSamplesDown(blockInput);

    //updateFilter(1);
    //lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(blockOutput));
}
//==============================================================================
bool VCFAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}
void VCFAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    //Parameters update  when sliders moved
    if (parameterID == "controlK_ID") {
        controlledK = newValue;
    }
    else if (parameterID == "controlF0_ID") {
        controlledF0 = newValue;
    }
}
void VCFAudioProcessor::updateFilter(bool realFreq)
{
    float frequency;
    if (realFreq) frequency = 48e3;
    else frequency = 48e3 * 4;

    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(frequency, frequency / 4);
}

juce::AudioProcessorEditor* VCFAudioProcessor::createEditor()
{
    return new VCFAudioProcessorEditor (*this, audioTree);
}

//==============================================================================
void VCFAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void VCFAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VCFAudioProcessor();
}
