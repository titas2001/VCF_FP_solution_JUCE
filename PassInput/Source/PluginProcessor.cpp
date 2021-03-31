/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
PassInputAudioProcessor::PassInputAudioProcessor()
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
        { std::make_unique<juce::AudioParameterFloat>("InputGain_ID","InputGain",juce::NormalisableRange<float>(0.0, 48.0,0.1),10.0),
          std::make_unique<juce::AudioParameterFloat>("OutputGain_ID","OutputGain",juce::NormalisableRange<float>(-48.0,10,0.1),0.0),
          std::make_unique<juce::AudioParameterFloat>("ToneControlle_ID","ToneControlle",juce::NormalisableRange<float>(20.0, 20000.0, 6.0),10000)
        }),
    lowPassFilter(juce::dsp::IIR::Coefficients< float >::makeLowPass((44100.0 * 4.0), 20000.0))

#endif
{
    oversampling.reset(new juce::dsp::Oversampling<float>(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));
    audioTree.addParameterListener("InputGain_ID", this);
    audioTree.addParameterListener("OutputGain_ID", this);
    audioTree.addParameterListener("ToneControlle_ID", this);
}

PassInputAudioProcessor::~PassInputAudioProcessor()
{
    oversampling.reset();
}

//==============================================================================
const juce::String PassInputAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PassInputAudioProcessor::acceptsMidi() const
{
    return false;
}

bool PassInputAudioProcessor::producesMidi() const
{
    return false;
}

bool PassInputAudioProcessor::isMidiEffect() const
{
    return false;
}

double PassInputAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PassInputAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PassInputAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PassInputAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PassInputAudioProcessor::getProgramName (int index)
{
    return {};
}

void PassInputAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PassInputAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
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
    K = 1.0; // gfbbk in MALTLAB
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
    upsamplingScale = 4;
}

void PassInputAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PassInputAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
     || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;
        
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()/*
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()*/)
        return false;
    
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}
void PassInputAudioProcessor::upSample(juce::AudioBuffer<float>& buffer, std::vector<float>& upVin, std::vector<float>& upVout, int i)
{
    auto mainInputOutput = getBusBuffer(buffer, true, 0);
    for (int j = 0; j < buffer.getNumSamples(); ++j)
    {
        const int offset = j * upsamplingScale;
        for (int step = 0; step < upsamplingScale; ++step)
        {
            upVout[offset + step] = *mainInputOutput.getWritePointer(i, j);
            upVin[offset + step] = *mainInputOutput.getReadPointer(i, j);
        }
    }
}

void PassInputAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto mainInputOutput = getBusBuffer(buffer, true, 0);
    
    //const unsigned int numChannels = mainInputOutput.getNumChannels();
    //const unsigned int numSamples = buffer.getNumSamples();
    //for (int j = 0; j < buffer.getNumSamples(); ++j)
    //{
    //    for (int i = upsamplingScale-1; i < mainInputOutput.getNumChannels()* upsamplingScale; i=i+4)
    //    {
    //        const int offset = j * upsamplingScale;
    //        for (int step = 0; step < upsamplingScale; ++step)
    //        {

    //            upVout[offset + step] = *mainInputOutput.getWritePointer(i, j);
    //            upVin[offset + step] = *mainInputOutput.getReadPointer(i, j);
    //    }
    //}
    /***************************************************************************************/
    // 1. Fill a new array here with upsampled input
    //      a. zeros array of size buffer*N
    //      b. assign input value to every N'th sample in the zeros array
    // 2. Apply low pass
    // 3. Run the VCF
    // 4. Apply low pass again 
    // 5. For loop to downsample

    I0 = 2.0 * Fs * std::tan(2.0 * 3.14 * f0 / Fs / 2.0) * 8.0 * C * Vt; // slider controls the Vt
    for (int j = 0; j < buffer.getNumSamples(); ++j)
    {

        for (int i = 0; i < mainInputOutput.getNumChannels(); ++i)
        {
            juce::dsp::AudioBlock<float> blockInput(buffer);
            juce::dsp::AudioBlock<float> blockOutput = oversampling->processSamplesUp(blockInput);

            updateFilter();
            lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(blockOutput));
            // upSample(buffer, upVin, upVout,i);
            // auto voutTemp = vout;
            //auto vin = /*upVin[i]*/ *mainInputOutput.getReadPointer(i, j);

            auto vin = *mainInputOutput.getReadPointer(i, j);
            for (int k = 0; k < 4; ++k)
            { 
                if (k != 0) vin = 0;
                else vin = *mainInputOutput.getReadPointer(i, j);

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

                    voutTemp = vc4Temp / 2.0 + vc4Temp * K; //noteOnVel = K in literature = gfdbk in MATLAB
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
            }

            *mainInputOutput.getWritePointer(i, j) = vout;

            
            
            //*mainInputOutput.getWritePointer(i, j) = noteOnVel * *mainInputOutput.getReadPointer(i, j);
        }
        
    }
}
//==============================================================================
bool PassInputAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}
void PassInputAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    //Parameters update  when sliders moved
    if (parameterID == "InputGain_ID") {
        //in db
        inputGainValue = pow(10, newValue / 20);
        //inputGainValue = newValue;
    }
    else if (parameterID == "OutputGain_ID") {
        //in db
        outputGainValue = pow(10, newValue / 20);

    }
    else if (parameterID == "ToneControlle_ID") {
        toneControlleValue = newValue;
    }



}
void PassInputAudioProcessor::updateFilter()
{
    float frequency = 44100 * 4;
    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(frequency, toneControlleValue);
}

juce::AudioProcessorEditor* PassInputAudioProcessor::createEditor()
{
    return new PassInputAudioProcessorEditor (*this);
}

//==============================================================================
void PassInputAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PassInputAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PassInputAudioProcessor();
}
