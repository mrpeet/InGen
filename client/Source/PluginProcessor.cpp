#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ingen {

InGenSamplerAudioProcessor::InGenSamplerAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Start background health poller thread
    pollerThread.startThread();
}

InGenSamplerAudioProcessor::~InGenSamplerAudioProcessor()
{
    // Gracefully stop the polling thread first, waiting indefinitely until it's dead
    pollerThread.signalThreadShouldExit();
    pollerThread.stopThread (-1);

    // Shutdown the Python server process
    stopServer();
}

const juce::String InGenSamplerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool InGenSamplerAudioProcessor::acceptsMidi() const
{
    return true;
}

bool InGenSamplerAudioProcessor::producesMidi() const
{
    return false;
}

bool InGenSamplerAudioProcessor::isMidiEffect() const
{
    return false;
}

double InGenSamplerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int InGenSamplerAudioProcessor::getNumPrograms()
{
    return 1;
}

int InGenSamplerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void InGenSamplerAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String InGenSamplerAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void InGenSamplerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void InGenSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    samplerEngine.prepareToPlay (sampleRate, samplesPerBlock);
}

void InGenSamplerAudioProcessor::releaseResources()
{
}

bool InGenSamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void InGenSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output channels that have no input data to prevent feedback noise
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Render Layer A & Layer B synthesiser engines in parallel
    samplerEngine.processBlock (buffer, midiMessages);
}

bool InGenSamplerAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* InGenSamplerAudioProcessor::createEditor()
{
    return new InGenSamplerAudioProcessorEditor (*this);
}

void InGenSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void InGenSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

void InGenSamplerAudioProcessor::launchServerAsync()
{
    if (serverStatus == ServerStatus::online || serverStatus == ServerStatus::launching)
        return;

    serverStatus = ServerStatus::launching;
    
    juce::Thread::launch ([this]() {
        juce::String pythonPath = "C:\\Users\\peetb\\Desktop\\InGen\\server\\venv\\Scripts\\python.exe";
        juce::String scriptPath = "C:\\Users\\peetb\\Desktop\\InGen\\server\\app\\main.py";
        
        juce::File pythonFile (pythonPath);
        juce::File scriptFile (scriptPath);
        
        if (pythonFile.existsAsFile() && scriptFile.existsAsFile())
        {
            // Launch the python process in a new, visible cmd window so the user can monitor live logs
            juce::String command = "cmd.exe /c start \"InGen AI Server\" cmd /c \"\"" + pythonPath + "\" \"" + scriptPath + "\"\"";
            
            bool started = serverProcess.start (command);
            if (started)
            {
                juce::Logger::writeToLog ("[InGen Server Launcher] Child process spawned successfully.");
            }
            else
            {
                juce::Logger::writeToLog ("[InGen Server Launcher] FAILED to spawn child process.");
                serverStatus = ServerStatus::offline;
            }
        }
        else
        {
            juce::Logger::writeToLog ("[InGen Server Launcher] Error: python.exe or main.py not found.");
            serverStatus = ServerStatus::offline;
        }
    });
}

void InGenSamplerAudioProcessor::stopServer()
{
    // 1. Send quick async POST to /shutdown to let the server shut itself down elegantly
    juce::URL url ("http://127.0.0.1:8071/shutdown");
    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                       .withStatusCode (&statusCode)
                       .withConnectionTimeoutMs (1000);
    
    auto urlWithData = url.withPOSTData ("");
    std::unique_ptr<juce::InputStream> stream (urlWithData.createInputStream (options));
    juce::ignoreUnused (stream);

    // 2. Force kill the child process if still active
    serverProcess.kill();
    serverStatus = ServerStatus::offline;
}

void InGenSamplerAudioProcessor::checkServerHealth()
{
    // Skip health checks while active generation is running to avoid socket flooding
    if (isGenerating.load())
        return;

    juce::URL url ("http://127.0.0.1:8071/health");
    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                       .withStatusCode (&statusCode)
                       .withConnectionTimeoutMs (800); // 800ms fast connection timeout
    
    std::unique_ptr<juce::InputStream> stream (url.createInputStream (options));
    
    if (stream != nullptr && statusCode == 200)
    {
        juce::String responseText = stream->readEntireStreamAsString();
        juce::var response = juce::JSON::parse (responseText);
        
        if (response.getProperty ("status", "error").toString() == "healthy")
        {
            serverStatus = ServerStatus::online;
            return;
        }
    }
    
    // If it is currently launching, let the launching status remain until it connects or timeout occurs
    if (serverStatus != ServerStatus::launching)
    {
        serverStatus = ServerStatus::offline;
    }
}

} // namespace ingen

// Entry point creator required by JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ingen::InGenSamplerAudioProcessor();
}
