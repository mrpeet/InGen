#pragma once

#include "AudioGeneratorAPI.h"
#include <juce_core/juce_core.h>

namespace ingen {

/**
 * Concrete implementation of AudioGeneratorAPI that performs real-time, asynchronous
 * HTTP REST requests to the local Python AI generation server on Port 8000.
 */
class LocalPythonServerClient : public AudioGeneratorAPI
{
public:
    LocalPythonServerClient() = default;
    ~LocalPythonServerClient() override = default;

    void generateTonalSamples (const PromptConfig& config, TonalCallback callback) override
    {
        performRequest (config, callback, true);
    }

    void generateFoleySamples (const PromptConfig& config, FoleyCallback callback) override
    {
        performRequest (config, callback, false);
    }

    void shutdownServer()
    {
        juce::Thread::launch ([]() {
            juce::URL url ("http://127.0.0.1:8071/shutdown");
            int statusCode = 0;
            auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                               .withStatusCode (&statusCode)
                               .withConnectionTimeoutMs (2000); // 2-second quick timeout
            
            auto urlWithData = url.withPOSTData ("");
            std::unique_ptr<juce::InputStream> stream (urlWithData.createInputStream (options));
            juce::Logger::writeToLog ("[InGen Client] Shutdown command sent. Status: " + juce::String (statusCode));
        });
    }

private:
    void performRequest (const PromptConfig& config, 
                         std::function<void(const std::vector<AudioResult>&, const juce::String&)> callback, 
                         bool isTonal)
    {
        // Run on background thread using JUCE launch to avoid blocking the message manager
        juce::Thread::launch ([config, callback, isTonal]() {
            juce::String endpoint = isTonal ? "/generate/tonal" : "/generate/foley";
            juce::URL url ("http://127.0.0.1:8071" + endpoint);

            // Construct JSON Payload
            juce::DynamicObject::Ptr json = new juce::DynamicObject();
            json->setProperty ("prompt", config.prompt);
            json->setProperty ("note_count", config.noteCount);
            json->setProperty ("octaves", config.octaves);
            json->setProperty ("foley_count", config.foleyCount);
            json->setProperty ("temperature", config.temperature);
            json->setProperty ("model", isTonal ? config.tonalModel : config.foleyModel);

            juce::var jsonVar (json.get());
            juce::String jsonPayload = juce::JSON::toString (jsonVar);

            int statusCode = 0;
            auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                               .withStatusCode (&statusCode)
                               .withConnectionTimeoutMs (90000) // 90 seconds timeout for AI generation
                               .withExtraHeaders ("Content-Type: application/json");

            auto urlWithData = url.withPOSTData (jsonPayload);

            std::unique_ptr<juce::InputStream> stream (urlWithData.createInputStream (options));

            if (stream == nullptr || (statusCode != 200 && statusCode != 202))
            {
                juce::MessageManager::callAsync ([callback]() {
                    callback ({}, "Failed to connect to local AI server (Port 8071). Make sure the server is running!");
                });
                return;
            }

            juce::String responseText = stream->readEntireStreamAsString();
            juce::var response = juce::JSON::parse (responseText);

            if (response.isVoid() || response["status"].toString() != "accepted")
            {
                juce::MessageManager::callAsync ([callback, response]() {
                    callback ({}, "Server failed to accept generation request.");
                });
                return;
            }

            juce::String taskId = response["task_id"].toString();
            
            // Poll status of the background task
            juce::var taskResponse;
            bool success = false;
            int retries = 0;
            const int maxRetries = 1800; // 900 seconds (15 minutes) maximum polling time
            
            while (retries < maxRetries)
            {
                juce::Thread::sleep (500); // Wait 500ms
                retries++;

                juce::URL pollUrl ("http://127.0.0.1:8071/task/" + taskId);
                int pollStatusCode = 0;
                auto pollOptions = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                       .withStatusCode (&pollStatusCode)
                                       .withConnectionTimeoutMs (2000); // Quick 2-second timeout

                std::unique_ptr<juce::InputStream> pollStream (pollUrl.createInputStream (pollOptions));
                if (pollStream != nullptr && pollStatusCode == 200)
                {
                    juce::String pollResponseText = pollStream->readEntireStreamAsString();
                    taskResponse = juce::JSON::parse (pollResponseText);

                    juce::String taskStatus = taskResponse["status"].toString();
                    if (taskStatus == "success")
                    {
                        success = true;
                        break;
                    }
                    else if (taskStatus == "failed")
                    {
                        juce::MessageManager::callAsync ([callback, taskResponse]() {
                            callback ({}, "Generation failed: " + taskResponse["error"].toString());
                        });
                        return;
                    }
                }
            }

            if (!success)
            {
                juce::MessageManager::callAsync ([callback]() {
                    callback ({}, "Generation request timed out on the local AI server.");
                });
                return;
            }

            auto& taskResult = taskResponse["result"];
            auto* samplesArray = taskResult["samples"].getArray();
            if (samplesArray == nullptr || samplesArray->size() == 0)
            {
                juce::MessageManager::callAsync ([callback]() {
                    callback ({}, "Server returned no audio samples.");
                });
                return;
            }

            std::vector<AudioResult> results;
            juce::File tempDir = juce::File::getSpecialLocation (juce::File::SpecialLocationType::tempDirectory)
                                             .getChildFile ("InGenSampler_Cache");
            tempDir.createDirectory();

            for (int i = 0; i < samplesArray->size(); ++i)
            {
                auto& sampleVar = samplesArray->getReference (i);
                juce::String fileUrlStr = "http://127.0.0.1:8071" + sampleVar["url"].toString();
                juce::String filename = sampleVar["filename"].toString();
                int midiKey = static_cast<int> (sampleVar["midi_root_key"]);
                float cents = static_cast<float> (sampleVar["pitch_correction_cents"]);

                juce::URL fileUrl (fileUrlStr);
                juce::File targetFile = tempDir.getChildFile (filename);

                int fileStatusCode = 0;
                auto fileOptions = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                       .withStatusCode (&fileStatusCode);
                
                std::unique_ptr<juce::InputStream> fileStream (fileUrl.createInputStream (fileOptions));

                if (fileStream != nullptr && fileStatusCode == 200)
                {
                    targetFile.deleteFile();
                    juce::FileOutputStream fos (targetFile);
                    if (fos.openedOk())
                    {
                        fos.writeFromInputStream (*fileStream, -1);
                        fos.flush();
                    }

                    if (targetFile.existsAsFile())
                    {
                        results.push_back ({targetFile, config.prompt, midiKey, cents});
                    }
                }
            }

            if (results.empty())
            {
                juce::MessageManager::callAsync ([callback]() {
                    callback ({}, "Failed to download WAV files from server cache.");
                });
            }
            else
            {
                juce::MessageManager::callAsync ([callback, results]() {
                    callback (results, "");
                });
            }
        });
    }
};

} // namespace ingen
