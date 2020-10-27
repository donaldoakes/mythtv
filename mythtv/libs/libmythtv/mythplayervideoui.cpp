// MythTV
#include "mythlogging.h"
#include "tv_play.h"
#include "mythvideooutgpu.h"
#include "mythplayervideoui.h"

#define LOC QString("PlayerVideo: ")

MythPlayerVideoUI::MythPlayerVideoUI(MythMainWindow* MainWindow, TV* Tv, PlayerContext* Context, PlayerFlags Flags)
  : MythPlayerAudioUI(MainWindow, Tv, Context, Flags)
{
    connect(this, &MythPlayerVideoUI::CheckCallbacks, this, &MythPlayerVideoUI::ProcessCallbacks);
}

void MythPlayerVideoUI::WindowResized(const QSize& /*Size*/)
{
    ReinitOSD();
}

bool MythPlayerVideoUI::InitVideo()
{
    if (!(m_playerCtx && m_decoder))
        return false;

    auto * video = MythVideoOutputGPU::Create(m_mainWindow,
                    m_decoder->GetCodecDecoderName(), m_decoder->GetVideoCodecID(),
                    m_videoDim, m_videoDispDim, m_videoAspect,
                    static_cast<float>(m_videoFrameRate),
                    static_cast<uint>(m_playerFlags), m_codecName, m_maxReferenceFrames);

    if (!video)
    {
        LOG(VB_GENERAL, LOG_ERR, LOC + "Couldn't create VideoOutput instance. Exiting..");
        SetErrored(tr("Failed to initialize video output"));
        return false;
    }

    // Simple lambda to update the OSD for picture attribute updates
    auto updateOsdPicAttr = [&](PictureAttribute Attribute, int Value)
    {
        QString text = toString(Attribute) + " " + toTypeString(kAdjustingPicture_Playback);
        UpdateOSDStatus(toTitleString(kAdjustingPicture_Playback), text, QString::number(Value),
                        kOSDFunctionalType_PictureAdjust, "%", Value * 10, kOSDTimeout_Med);
        ChangeOSDPositionUpdates(false);
    };

    // Toggle detect letter box
    auto toggleDetectLetterbox = [&]()
    {
        if (m_videoOutput)
        {
            m_detectLetterBox.SetDetectLetterbox(!m_detectLetterBox.GetDetectLetterbox(),
                                                  m_videoOutput->GetAdjustFill());
        }
    };

    m_videoOutput = video;

    // Inbound connections
    connect(video, &MythVideoOutputGPU::PictureAttributeChanged, updateOsdPicAttr);
    connect(video, &MythVideoBounds::UpdateOSDMessage, this,  QOverload<const QString&>::of(&MythPlayerVideoUI::UpdateOSDMessage));
    connect(video, &MythVideoBounds::VideoBoundsStateChanged, m_tv, &TV::VideoBoundsStateChanged);
    connect(m_tv,  &TV::ChangeOSDPositionUpdates, this,  &MythPlayerVideoUI::ChangeOSDPositionUpdates);
    connect(m_tv,  &TV::WindowResized,            this,  &MythPlayerVideoUI::WindowResized);
    connect(m_tv,  &TV::ChangeAdjustFill,         this,  &MythPlayerVideoUI::ToggleAdjustFill);
    connect(m_tv,  &TV::ChangeAspectOverride,     this,  &MythPlayerVideoUI::ToggleAspectOverride);
    connect(m_tv,  &TV::ToggleDetectLetterBox, toggleDetectLetterbox);

    // Passthrough signals
    connect(m_tv,  &TV::ChangePictureAttribute,   video, &MythVideoOutputGPU::ChangePictureAttribute);
    connect(m_tv,  &TV::ChangeStereoOverride,     video, &MythVideoOutputGPU::SetStereoOverride);
    connect(m_tv,  &TV::WindowResized,            video, &MythVideoOutputGPU::WindowResized);
    connect(m_tv,  &TV::EmbedPlayback,            video, &MythVideoOutputGPU::EmbedPlayback);

    // Update initial state. MythVideoOutput will have potentially adjusted state
    // at startup that we need to know about.
    m_videoOutput->RefreshVideoBoundsState();

    return true;
}

/*! \brief Convenience function to request and wait for a callback into the main thread.
 *
 * This is used by hardware decoders to ensure certain resources are created
 * and destroyed in the UI (render) thread.
*/
void MythPlayerVideoUI::HandleDecoderCallback(const QString& Debug, DecoderCallback::Callback Function,
                                              void* Opaque1, void* Opaque2)
{
    if (!Function)
        return;

    m_decoderCallbackLock.lock();
    QAtomicInt ready{0};
    QWaitCondition wait;
    LOG(VB_GENERAL, LOG_INFO, LOC + QString("Queuing callback for %1").arg(Debug));
    m_decoderCallbacks.append(DecoderCallback(Debug, Function, &ready, &wait, Opaque1, Opaque2));
    int count = 0;
    while (!ready && !wait.wait(&m_decoderCallbackLock, 100) && (count += 100))
        LOG(VB_GENERAL, LOG_WARNING, QString("Waited %1ms for %2").arg(count).arg(Debug));
    m_decoderCallbackLock.unlock();
}

void MythPlayerVideoUI::ProcessCallbacks()
{
    m_decoderCallbackLock.lock();
    for (auto *it = m_decoderCallbacks.begin(); it != m_decoderCallbacks.end(); ++it)
    {
        if (it->m_function)
        {
            LOG(VB_GENERAL, LOG_INFO, LOC + QString("Executing %1").arg(it->m_debug));
            it->m_function(it->m_opaque1, it->m_opaque2, it->m_opaque3);
        }
        if (it->m_ready)
            it->m_ready->ref();
    }
    m_decoderCallbacks.clear();
    m_decoderCallbackLock.unlock();
}

void MythPlayerVideoUI::ToggleAdjustFill(AdjustFillMode Mode)
{
    if (m_videoOutput)
    {
        m_detectLetterBox.SetDetectLetterbox(false, m_videoOutput->GetAdjustFill());
        m_videoOutput->ToggleAdjustFill(Mode);
        ReinitOSD();
        QString text = toString(m_videoOutput->GetAdjustFill());
        UpdateOSDMessage(text);
    }
}

void MythPlayerVideoUI::ToggleAspectOverride(AspectOverrideMode AspectMode)
{
    if (m_videoOutput)
    {
        m_videoOutput->ToggleAspectOverride(AspectMode);
        ReinitOSD();
        QString text = toString(m_videoOutput->GetAspectOverride());
        UpdateOSDMessage(text);
    }
}
