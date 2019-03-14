#ifndef _OPENGL_VIDEO_H__
#define _OPENGL_VIDEO_H__

// Qt
#include <QRect>
#include <QObject>

// MythTV
#include "videooutbase.h"
#include "videoouttypes.h"
#include "mythrender_opengl.h"
#include "mythavutil.h"
#include "mythopenglinterop.h"

// Std
#include <vector>
#include <map>
using std::vector;
using std::map;

class OpenGLVideo : public QObject
{
    Q_OBJECT

  public:
    enum VideoShaderType
    {
        Default       = 0, // Plain blit
        Progressive   = 1, // Progressive video frame
        InterlacedTop = 2, // Deinterlace with top field first
        InterlacedBot = 3, // Deinterlace with bottom field first
        Interlaced    = 4, // Interlaced but do not deinterlace
        ShaderCount   = 5
    };

    static VideoFrameType ProfileToType(const QString &Profile);
    static QString        TypeToProfile(VideoFrameType Type);

    OpenGLVideo(MythRenderOpenGL *Render, VideoColourSpace *ColourSpace,
                QSize VideoDim, QSize VideoDispDim, QRect DisplayVisibleRect,
                QRect DisplayVideoRect, QRect videoRect,
                bool ViewportControl, QString Profile);
   ~OpenGLVideo();

    bool    IsValid(void) const;
    void    UpdateInputFrame(const VideoFrame *Frame);
    bool    AddDeinterlacer(QString &Deinterlacer);
    void    SetDeinterlacing(bool Deinterlacing);
    QString GetDeinterlacer(void) const;
    void    PrepareFrame(VideoFrame *Frame, bool TopFieldFirst, FrameScanType Scan,
                         StereoscopicMode Stereo, bool DrawBorder = false);
    void    SetMasterViewport(QSize Size);
    QSize   GetVideoSize(void) const;
    QString GetProfile() const;

  public slots:
    void    SetVideoRects(const QRect &DisplayVideoRect, const QRect &VideoRect);
    void    UpdateColourSpace(void);
    void    UpdateShaderParameters(void);

  private:
    bool    CreateVideoShader(VideoShaderType Type, QString Deinterlacer = QString());
    void    RotateTextures(void);
    void    TearDownDeinterlacer(void);

    bool           m_valid;
    VideoFrameType m_inputType;           ///< Usually YV12 for software, VDPAU etc for hardware
    VideoFrameType m_outputType;          ///< Set by profile for software or decoder for hardware
    MythRenderOpenGL *m_render;
    QSize          m_videoDispDim;        ///< Useful video frame size e.g. 1920x1080
    QSize          m_videoDim;            ///< Total video frame size e.g. 1920x1088
    QSize          m_masterViewportSize;  ///< Current viewport into which OpenGL is rendered
    QRect          m_displayVisibleRect;  ///< Total useful, visible rectangle
    QRect          m_displayVideoRect;    ///< Sub-rect of display_visible_rect for video
    QRect          m_videoRect;           ///< Sub-rect of video_disp_dim to display (after zoom adjustments etc)
    QString        m_hardwareDeinterlacer;
    bool           m_hardwareDeinterlacing; ///< OpenGL deinterlacing is enabled
    VideoColourSpace *m_videoColourSpace;
    bool           m_viewportControl;     ///< Video has control over view port
    QOpenGLShaderProgram* m_shaders[ShaderCount];
    int            m_shaderCost[ShaderCount];
    vector<MythVideoTexture*> m_referenceTextures; ///< Up to 3 reference textures for filters
    vector<MythVideoTexture*> m_inputTextures;     ///< Textures with raw video data
    QOpenGLFramebufferObject* m_frameBuffer;
    MythVideoTexture*         m_frameBufferTexture;
    QSize          m_inputTextureSize;    ///< Actual size of input texture(s)
    uint           m_referenceTexturesNeeded; ///< Number of reference textures still required
    QOpenGLFunctions::OpenGLFeatures m_features; ///< Default features available from Qt
    int            m_extraFeatures;       ///< OR'd list of extra, Myth specific features
    bool           m_resizing;
    bool           m_forceResize;         ///< Global setting to force a resize stage
    GLenum         m_textureTarget;       ///< Some interops require custom texture targets
};
#endif // _OPENGL_VIDEO_H__
