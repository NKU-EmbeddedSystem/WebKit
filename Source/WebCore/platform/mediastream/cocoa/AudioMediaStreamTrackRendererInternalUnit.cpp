/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AudioMediaStreamTrackRendererInternalUnit.h"

#if ENABLE(MEDIA_STREAM)

#include "AudioSampleDataSource.h"
#include "AudioSession.h"
#include "CAAudioStreamDescription.h"
#include "Logging.h"

#include <pal/spi/cocoa/AudioToolboxSPI.h>
#include <wtf/FastMalloc.h>
#include <wtf/Lock.h>

#if PLATFORM(COCOA)
#include "CoreAudioCaptureDevice.h"
#include "CoreAudioCaptureDeviceManager.h"
#endif

#include <pal/cf/CoreMediaSoftLink.h>

namespace WebCore {

class LocalAudioMediaStreamTrackRendererInternalUnit final : public AudioMediaStreamTrackRendererInternalUnit {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit LocalAudioMediaStreamTrackRendererInternalUnit(RenderCallback&&);

private:
    void createAudioUnitIfNeeded();

    // AudioMediaStreamTrackRendererInternalUnit API.
    void start() final;
    void stop() final;
    void retrieveFormatDescription(CompletionHandler<void(const CAAudioStreamDescription*)>&&) final;
    void setAudioOutputDevice(const String&) final;

    static OSStatus renderingCallback(void*, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32 inBusNumber, UInt32 sampleCount, AudioBufferList*);

    RenderCallback m_renderCallback;
    std::unique_ptr<CAAudioStreamDescription> m_outputDescription;
    AudioComponentInstance m_remoteIOUnit { nullptr };
    bool m_isStarted { false };
#if PLATFORM(MAC)
    uint32_t m_deviceID { 0 };
#endif
};

UniqueRef<AudioMediaStreamTrackRendererInternalUnit> AudioMediaStreamTrackRendererInternalUnit::createLocalInternalUnit(RenderCallback&& renderCallback)
{
    return makeUniqueRef<LocalAudioMediaStreamTrackRendererInternalUnit>(WTFMove(renderCallback));
}

LocalAudioMediaStreamTrackRendererInternalUnit::LocalAudioMediaStreamTrackRendererInternalUnit(RenderCallback&& renderCallback)
    : m_renderCallback(WTFMove(renderCallback))
{
}

void LocalAudioMediaStreamTrackRendererInternalUnit::retrieveFormatDescription(CompletionHandler<void(const CAAudioStreamDescription*)>&& callback)
{
    createAudioUnitIfNeeded();
    callback(m_outputDescription.get());
}

void LocalAudioMediaStreamTrackRendererInternalUnit::setAudioOutputDevice(const String& deviceID)
{
#if PLATFORM(MAC)
    auto device = CoreAudioCaptureDeviceManager::singleton().coreAudioDeviceWithUID(deviceID);

    if (!device && !deviceID.isEmpty()) {
        RELEASE_LOG(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::setAudioOutputDeviceId - did not find device");
        return;
    }

    auto audioUnitDeviceID = device ? device->deviceID() : 0;
    if (m_deviceID == audioUnitDeviceID)
        return;

    bool shouldRestart = m_isStarted;
    if (m_isStarted)
        stop();

    m_deviceID = audioUnitDeviceID;

    if (shouldRestart)
        start();
#else
    UNUSED_PARAM(deviceID);
#endif
}

void LocalAudioMediaStreamTrackRendererInternalUnit::start()
{
    if (m_isStarted)
        return;

    createAudioUnitIfNeeded();
    if (!m_remoteIOUnit)
        return;

    if (auto error = AudioOutputUnitStart(m_remoteIOUnit)) {
        RELEASE_LOG_ERROR(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::start AudioOutputUnitStart failed, error = %d", error);
        AudioComponentInstanceDispose(m_remoteIOUnit);
        m_remoteIOUnit = nullptr;
        return;
    }
    m_isStarted = true;
    RELEASE_LOG(WebRTC, "AudioMediaStreamTrackRendererInternalUnit is started");
}

void LocalAudioMediaStreamTrackRendererInternalUnit::stop()
{
    if (!m_remoteIOUnit)
        return;

    if (m_isStarted) {
        AudioOutputUnitStop(m_remoteIOUnit);
        m_isStarted = false;
    }

    AudioComponentInstanceDispose(m_remoteIOUnit);
    m_remoteIOUnit = nullptr;
}

void LocalAudioMediaStreamTrackRendererInternalUnit::createAudioUnitIfNeeded()
{
    ASSERT(!m_remoteIOUnit || m_outputDescription);
    if (m_remoteIOUnit)
        return;

    CAAudioStreamDescription outputDescription;
    AudioComponentInstance remoteIOUnit { nullptr };

    AudioComponentDescription ioUnitDescription { kAudioUnitType_Output, 0, kAudioUnitManufacturer_Apple, 0, 0 };
#if PLATFORM(IOS_FAMILY)
    ioUnitDescription.componentSubType = kAudioUnitSubType_RemoteIO;
#else
    ioUnitDescription.componentSubType = kAudioUnitSubType_DefaultOutput;
#endif

    AudioComponent ioComponent = AudioComponentFindNext(nullptr, &ioUnitDescription);
    ASSERT(ioComponent);
    if (!ioComponent) {
        RELEASE_LOG_ERROR(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::createAudioUnit unable to find remote IO unit component");
        return;
    }

    auto error = AudioComponentInstanceNew(ioComponent, &remoteIOUnit);
    if (error) {
        RELEASE_LOG_ERROR(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::createAudioUnit unable to open vpio unit, error = %d", error);
        return;
    }

#if PLATFORM(IOS_FAMILY)
    UInt32 param = 1;
    error = AudioUnitSetProperty(remoteIOUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &param, sizeof(param));
    if (error) {
        RELEASE_LOG_ERROR(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::createAudioUnit unable to enable vpio unit output, error = %d", error);
        return;
    }
#endif

#if PLATFORM(MAC)
    if (m_deviceID) {
        error = AudioUnitSetProperty(remoteIOUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &m_deviceID, sizeof(m_deviceID));
        if (error) {
            RELEASE_LOG_ERROR(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::createAudioUnit unable to set unit device ID %d, error %d (%.4s)", (int)m_deviceID, (int)error, (char*)&error);
            return;
        }
    }
#endif

    AURenderCallbackStruct callback = { LocalAudioMediaStreamTrackRendererInternalUnit::renderingCallback, this };
    error = AudioUnitSetProperty(remoteIOUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, 0, &callback, sizeof(callback));
    if (error) {
        RELEASE_LOG_ERROR(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::createAudioUnit unable to set vpio unit speaker proc, error = %d", error);
        return;
    }

    UInt32 size = sizeof(outputDescription.streamDescription());
    error  = AudioUnitGetProperty(remoteIOUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &outputDescription.streamDescription(), &size);
    if (error) {
        RELEASE_LOG_ERROR(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::createAudioUnit unable to get input stream format, error = %d", error);
        return;
    }

    outputDescription.streamDescription().mSampleRate = AudioSession::sharedSession().sampleRate();

    error = AudioUnitSetProperty(remoteIOUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &outputDescription.streamDescription(), sizeof(outputDescription.streamDescription()));
    if (error) {
        RELEASE_LOG_ERROR(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::createAudioUnit unable to set input stream format, error = %d", error);
        return;
    }

    error = AudioUnitInitialize(remoteIOUnit);
    if (error) {
        RELEASE_LOG_ERROR(WebRTC, "AudioMediaStreamTrackRendererInternalUnit::createAudioUnit AudioUnitInitialize() failed, error = %d", error);
        return;
    }

    m_outputDescription = makeUnique<CAAudioStreamDescription>(outputDescription);
    m_remoteIOUnit = remoteIOUnit;
}

OSStatus LocalAudioMediaStreamTrackRendererInternalUnit::renderingCallback(void* processor, AudioUnitRenderActionFlags* actionFlags, const AudioTimeStamp* timeStamp, UInt32, UInt32 sampleCount, AudioBufferList* ioData)
{
    return static_cast<LocalAudioMediaStreamTrackRendererInternalUnit*>(processor)->m_renderCallback(sampleCount, *ioData, timeStamp->mSampleTime, timeStamp->mHostTime, *actionFlags);
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
