/*
 // Copyright (c) 2026 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include "../Base/GemWindow.h"

#define WHEELUP    3
#define WHEELDOWN  4
#define WHEELLEFT  5
#define WHEELRIGHT 6

// Gemcanvas: special plugdata-exclusive object to allow rendering Gem content onto the patch canvas
struct GemCanvas
{
    static inline std::mutex s_mutex;
    static inline std::unordered_map<t_pdinstance*, GemCanvas*> s_active;

    static GemCanvas* getActive()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_active[libpd_this_instance()];
    }

    // FBO that Gem is going to render to, this will allow us to copy the pixels onto the Pd canvas
    GLuint m_fbo = 0;
    GLuint m_fboTexture = 0;
    GLuint m_fboDepthRB = 0;
    int m_fboWidth = 0;
    int m_fboHeight = 0;
    std::vector<uint8_t> m_readbackBuf; // Buffer that pixels get read into

    juce::CriticalSection pixelLock;
    std::vector<uint8_t> pixelFront;
    int pixelW = 0;
    int pixelH = 0;
    std::atomic<bool> frameDirty{false};

    std::atomic<int> requestedWidth{500};
    std::atomic<int> requestedHeight{500};
    std::atomic<int> framerate = 60;
    std::atomic<float> scale = 1.0f;

    std::function<void(juce::MouseEvent const&, bool)> mouseCallback;
    std::function<void(juce::MouseEvent const&)> motionCallback;
    std::function<void(juce::MouseEvent const&, juce::MouseWheelDetails const& details)> scrollCallback;
    std::function<void(juce::KeyPress const& k)> keyCallback;
    std::function<void(juce::KeyPress const& k)> keyUpCallback;

    void activate()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_active[libpd_this_instance()] = this;
    }

    void deactivate()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto* this_pd = libpd_this_instance();
        if (s_active[this_pd] == this)
            s_active[this_pd] = nullptr;
    }

    void createFBO(int w, int h)
    {
        destroyFBO();
        m_fboWidth  = w;
        m_fboHeight = h;

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        glGenTextures(1, &m_fboTexture);
        glBindTexture(GL_TEXTURE_2D, m_fboTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                     GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_fboTexture, 0);

        glGenRenderbuffers(1, &m_fboDepthRB);
        glBindRenderbuffer(GL_RENDERBUFFER, m_fboDepthRB);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, m_fboDepthRB);

        jassert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void destroyFBO()
    {
        if (m_fbo)        { glDeleteFramebuffers(1, &m_fbo);         m_fbo = 0; }
        if (m_fboTexture) { glDeleteTextures(1, &m_fboTexture);      m_fboTexture = 0; }
        if (m_fboDepthRB) { glDeleteRenderbuffers(1, &m_fboDepthRB); m_fboDepthRB = 0; }
        m_fboWidth = m_fboHeight = 0;
    }

    bool beginFrame(int& outW, int& outH)
    {
        int rw = requestedWidth.load() * scale;
        int rh = requestedHeight.load() * scale;
        if (rw < 1) rw = 500;
        if (rh < 1) rh = 500;

        // Create or resize
        if (!m_fbo || rw != m_fboWidth || rh != m_fboHeight) {
            createFBO(rw, rh);
        }

        if (!m_fbo) return false;

        outW = m_fboWidth;
        outH = m_fboHeight;

        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glViewport(0, 0, m_fboWidth, m_fboHeight);
        return true;
    }

    void endFrame()
    {
        if (!m_fbo) return;

        int const w = m_fboWidth;
        int const h = m_fboHeight;
        int const stride = w * 4;

        // Read entire FBO (GL gives bottom-row-first)
        std::vector<uint8_t> tmp(stride * h);
        glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, tmp.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Flip vertically into the readback buffer
        m_readbackBuf.resize(stride * h);
        for (int y = 0; y < h; ++y) {
            std::memcpy(m_readbackBuf.data() + y * stride,
                        tmp.data() + (h - 1 - y) * stride,
                        stride);
        }

        // Publish to front-buffer
        {
            juce::ScopedLock sl(pixelLock);
            std::swap(pixelFront, m_readbackBuf);
            pixelW = w;
            pixelH = h;
        }
        frameDirty = true;
    }

    // Read pixels (called from nanovg render function)
    bool pullPixels(std::vector<uint8_t>& dest, int& w, int& h)
    {
        juce::ScopedLock sl(pixelLock);
        if (pixelFront.empty()) return false;
        dest = pixelFront;
        w = pixelW;
        h = pixelH;
        frameDirty = false;
        return true;
    }
};

// Pd object (wrapper around GemCanvas)
typedef struct _gemcanvas {
    t_object x_obj;
    GemCanvas canvas;
} t_gemcanvas;

#ifndef GEM_NO_SETUP
static t_class* gemcanvas_class = nullptr;

static void* gemcanvas_new(void)
{
    auto* x = reinterpret_cast<t_gemcanvas*>(pd_new(gemcanvas_class));
    new (&x->canvas) GemCanvas();
    x->canvas.activate();
    return x;
}

static void gemcanvas_fps(t_gemcanvas* x, t_floatarg f)
{
    x->canvas.framerate = f;
}

static void gemcanvas_free(t_gemcanvas* x)
{
    x->canvas.deactivate();
    x->canvas.~GemCanvas();
}

static void gemcanvas_setup(void)
{
    gemcanvas_class = class_new(
        gensym("gemcanvas"),
        reinterpret_cast<t_newmethod>(gemcanvas_new),
        reinterpret_cast<t_method>(gemcanvas_free),
        sizeof(t_gemcanvas),
        CLASS_DEFAULT,
        A_NULL);

    class_addfloat(gemcanvas_class, gemcanvas_fps);
}
#endif


class GEM_EXPORT gemjucewindow : public GemWindow
{
    CPPEXTERN_HEADER(gemjucewindow, GemWindow);

    GemCanvas* m_activeCanvas = nullptr;
    bool m_windowHidden = false;

    class Window final : public juce::Component
                       , public juce::Timer
    {
        struct ResizeListener final : public juce::ComponentBoundsConstrainer {
            std::function<void()> onBegin, onEnd;
            ResizeListener() { setSizeLimits(50, 50, 30000, 30000); }
            void resizeStart() override { if (onBegin) onBegin(); }
            void resizeEnd()   override { if (onEnd)   onEnd();   }
        };
        ResizeListener m_resizeListener;
        juce::OpenGLContext m_glContext;
        juce::Thread::ThreadID m_activeThread = nullptr;
        t_pdinstance* m_pdInstance = nullptr;
        juce::Point<int> m_lastMousePos;
        juce::Array<juce::KeyPress> m_heldKeys;
        bool m_shiftDown = false, m_ctrlDown = false, m_altDown = false, m_cmdDown = false;
        float m_lastScale = 1.0f;
        std::atomic<int> m_lastW, m_lastH;;
        gemjucewindow* m_owner;

    public:
        Window(juce::Rectangle<int> bounds, bool border,
               t_pdinstance* pdInstance, gemjucewindow* owner)
            : m_pdInstance(pdInstance), m_owner(owner)
        {
            setOpaque(true);

            m_glContext.setSwapInterval(0);
            m_glContext.setMultisamplingEnabled(true);
            juce::OpenGLPixelFormat fmt(8, 8, 16, 8);
            fmt.multisamplingLevel = 2;
            m_glContext.setPixelFormat(fmt);
            m_glContext.attachTo(*this);

            int flags = 0;
            if (border) {
#if JUCE_WINDOWS
                flags = juce::ComponentPeer::windowHasTitleBar
                      | juce::ComponentPeer::windowHasDropShadow
                      | juce::ComponentPeer::windowIsResizable;
#else
                flags = juce::ComponentPeer::windowHasTitleBar
                      | juce::ComponentPeer::windowHasDropShadow
                      | juce::ComponentPeer::windowIsResizable
                      | juce::ComponentPeer::windowHasMinimiseButton
                      | juce::ComponentPeer::windowHasMaximiseButton;
#endif
            }
            addToDesktop(flags);

#if JUCE_WINDOWS
            bounds = bounds.translated(10, 30);
#endif
            // Call async because this will send a bounds message, which locks the audiot thread,
            // while we're already locking the message thread if we got here

            setBounds(bounds);

            m_lastW = bounds.getWidth();
            m_lastH = bounds.getHeight();

            setWantsKeyboardFocus(true);
            setVisible(true);

            if (auto* peer = getPeer()) {
                m_resizeListener.onBegin = [this] {
                    m_owner->m_resizing = true;
                };
                m_resizeListener.onEnd = [this] {
                    m_owner->m_resizing = false;
                };
                peer->setConstrainer(&m_resizeListener);
            }

            startTimerHz(30);
        }

        ~Window() override
        {
            stopTimer();
            m_glContext.detach();
        }

        void makeContextCurrent()
        {
            checkThread();
            m_glContext.makeActive();
        }

        void swapBuffers() {
            checkThread();
            m_glContext.swapBuffers();
        }

        juce::OpenGLContext& glContext() { return m_glContext; }

        void setTitle(const std::string& title)
        {
            juce::MessageManager::callAsync(
                [safe = juce::Component::SafePointer<Window>(this),
                 t = juce::String::fromUTF8(title.c_str())] {
                    if (auto* w = safe.getComponent())
                        if (auto* peer = w->getPeer())
                            peer->setTitle(t);
                });
        }

        void setFullscreen(bool on)
        {
            if (auto* peer = getPeer())
                peer->setFullScreen(on);
        }

        void checkScale()
        {
            if(m_glContext.isActive()) {
                auto oldScale = m_lastScale;
                m_lastScale =  m_glContext.getRenderingScale();
                if(oldScale != m_lastScale)
                {
                  int w = m_lastW * m_lastScale, h = m_lastH * m_lastScale;
                  m_owner->windowsizeCallback(w, h);
                  m_owner->framebuffersizeCallback(w, h);
                }
            }
        }

        void checkThread()
        {
            auto id = juce::Thread::getCurrentThreadId();
            if (id != m_activeThread) {
                m_glContext.initialiseOnThread();
                m_activeThread = id;
            }
        }

        void setThis() const { libpd_set_instance(m_pdInstance); }

        void resized() override
        {
            m_lastW = getWidth();
            m_lastH = getHeight();

            double s = m_lastScale;
            int w = getWidth() * s, h = getHeight() * s;

            if(m_owner->m_callingSync)
            {
              setThis();
              m_owner->windowsizeCallback(w, h);
              m_owner->framebuffersizeCallback(w, h);
            }
            else if(juce::MessageManager::getInstance()->isThisTheMessageThread())
            {
              sys_lock();
              setThis();
              m_owner->windowsizeCallback(w, h);
              m_owner->framebuffersizeCallback(w, h);
              sys_unlock();
            }
            else
            {
              juce::MessageManager::callAsync([this, w, h]{
                setThis();
                m_owner->windowsizeCallback(w, h);
                m_owner->framebuffersizeCallback(w, h);
              });
            }
        }

        void paint(juce::Graphics&) override {}

        void mouseDown(const juce::MouseEvent& e) override
        {
            setThis();
            int btn = e.mods.isRightButtonDown()  ? 2
                    : e.mods.isMiddleButtonDown() ? 1
                                                  : 0;
            setThis();
            sys_lock();
            m_owner->mousebuttonCallback(btn, 1, 0);
            sys_unlock();
        }

        void mouseUp(const juce::MouseEvent& e) override
        {
            setThis();
            int btn = e.mods.isRightButtonDown()  ? 2
                    : e.mods.isMiddleButtonDown() ? 1
                                                  : 0;
            setThis();
            sys_lock();
            m_owner->mousebuttonCallback(btn, 0, 0);
            sys_unlock();
        }

        void mouseMove(const juce::MouseEvent& e) override
        {
            if(m_lastMousePos == e.position.toInt()) return;
            m_lastMousePos = e.position.toInt();
            setThis();
            sys_lock();
            m_owner->mouseposCallback(static_cast<float>(e.x) * m_lastScale,
                                      static_cast<float>(e.y) * m_lastScale);
            sys_unlock();
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            if(m_lastMousePos == e.position.toInt()) return;
            m_lastMousePos = e.position.toInt();
            setThis();
            sys_lock();
            m_owner->mouseposCallback(static_cast<float>(e.x) * m_lastScale,
                                      static_cast<float>(e.y) * m_lastScale);
            sys_unlock();
        }

        void mouseWheelMove(const juce::MouseEvent&,
                            const juce::MouseWheelDetails& wheel) override
        {
            setThis();
            sys_lock();
            m_owner->scrollCallback(wheel.deltaX, wheel.deltaY);
            sys_unlock();
        }

        static void convertKeyCode(int& keynum, std::string& keysym)
        {
            if (keynum == juce::KeyPress::backspaceKey)
              keynum = 259, keysym = "BackSpace";
            else if (keynum == juce::KeyPress::tabKey)
                keynum = 258, keysym = "Tab";
            else if (keynum == juce::KeyPress::returnKey)
                keynum = 257, keysym = "Return";
            else if (keynum == juce::KeyPress::escapeKey)
                keynum = 256, keysym = "Escape";
            else if (keynum == juce::KeyPress::spaceKey)
                keynum = 32, keysym = "Space";
            else if (keynum == juce::KeyPress::deleteKey)
                keynum = 261, keysym = "Delete";
            else if (keynum == juce::KeyPress::upKey)
                keynum = 265, keysym = "Up";
            else if (keynum == juce::KeyPress::downKey)
                keynum = 264, keysym = "Down";
            else if (keynum == juce::KeyPress::leftKey)
                keynum = 262, keysym = "Left";
            else if (keynum == juce::KeyPress::rightKey)
                keynum = 263, keysym = "Right";
            else if (keynum == juce::KeyPress::homeKey)
                keynum = 268, keysym = "Home";
            else if (keynum == juce::KeyPress::endKey)
                keynum = 269, keysym = "End";
            else if (keynum == juce::KeyPress::pageUpKey)
                keynum = 266, keysym = "Prior";
            else if (keynum == juce::KeyPress::pageDownKey)
                keynum = 267, keysym = "Next";
            else if (keynum == juce::KeyPress::F1Key)
                keynum = 290, keysym = "F1";
            else if (keynum == juce::KeyPress::F2Key)
                keynum = 291, keysym = "F2";
            else if (keynum == juce::KeyPress::F3Key)
                keynum = 292, keysym = "F3";
            else if (keynum == juce::KeyPress::F4Key)
                keynum = 293, keysym = "F4";
            else if (keynum == juce::KeyPress::F5Key)
                keynum = 294, keysym = "F5";
            else if (keynum == juce::KeyPress::F6Key)
                keynum = 295, keysym = "F6";
            else if (keynum == juce::KeyPress::F7Key)
                keynum = 296, keysym = "F7";
            else if (keynum == juce::KeyPress::F8Key)
                keynum = 297, keysym = "F8";
            else if (keynum == juce::KeyPress::F9Key)
                keynum = 298, keysym = "F9";
            else if (keynum == juce::KeyPress::F10Key)
                keynum = 299, keysym = "F10";
            else if (keynum == juce::KeyPress::F11Key)
                keynum = 300, keysym = "F11";
            else if (keynum == juce::KeyPress::F12Key)
                keynum = 301, keysym = "F12";
            else if (keynum == juce::KeyPress::numberPad0)
                keynum = 48, keysym = "0";
            else if (keynum == juce::KeyPress::numberPad1)
                keynum = 49, keysym = "1";
            else if (keynum == juce::KeyPress::numberPad2)
                keynum = 50, keysym = "2";
            else if (keynum == juce::KeyPress::numberPad3)
                keynum = 51, keysym = "3";
            else if (keynum == juce::KeyPress::numberPad4)
                keynum = 52, keysym = "4";
            else if (keynum == juce::KeyPress::numberPad5)
                keynum = 53, keysym = "5";
            else if (keynum == juce::KeyPress::numberPad6)
                keynum = 54, keysym = "6";
            else if (keynum == juce::KeyPress::numberPad7)
                keynum = 55, keysym = "7";
            else if (keynum == juce::KeyPress::numberPad8)
                keynum = 56, keysym = "8";
            else if (keynum == juce::KeyPress::numberPad9)
                keynum = 57, keysym = "9";
        }

        bool keyPressed(const juce::KeyPress& key) override
        {
            std::string keyDescription = key.getTextDescription().fromLastOccurrenceOf(" ", false, false).toStdString();
            int keyNum = key.getKeyCode();
            convertKeyCode(keyNum, keyDescription);

            setThis();
            sys_lock();
            m_owner->keyCallback(keyNum, 1, keyDescription);
            sys_unlock();
            m_heldKeys.add(key);
            return true;
        }

        // Poll for key-up events (~30 Hz) since JUCE has no key-release event
        void timerCallback() override {
            auto mods = juce::ModifierKeys::getCurrentModifiers();

            auto checkModifier = [&](bool& currentState, bool newState, int glfwCode, std::string name) {
                if (newState != currentState) {
                    currentState = newState;
                    setThis();
                    sys_lock();
                    m_owner->keyCallback(glfwCode, currentState, name);
                    sys_unlock();
                }
            };

            auto hasFocus = hasKeyboardFocus(true);
            checkModifier(m_shiftDown, hasFocus && mods.isShiftDown(), 340, "Shift");
            checkModifier(m_ctrlDown,  hasFocus && mods.isCtrlDown(),  341, "Control");
            checkModifier(m_altDown,   hasFocus && mods.isAltDown(),   342, "Alt");
            checkModifier(m_cmdDown,   hasFocus && mods.isCommandDown(), 343, "Super");

            for (int i = m_heldKeys.size() - 1; i >= 0; --i) {
                auto key = m_heldKeys[i];
                if (!juce::KeyPress::isKeyCurrentlyDown(key.getKeyCode())) {
                    auto keyDescription = key.getTextDescription().fromLastOccurrenceOf(" ", false, false).toStdString();
                    auto keyNum = key.getKeyCode();
                    convertKeyCode(keyNum, keyDescription);

                    setThis();
                    sys_lock();
                    m_owner->keyCallback(key.getKeyCode(), 0, key.getTextDescription().toStdString());
                    sys_unlock();
                    m_heldKeys.remove(i);
                }
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Window)
    };

    void checkEmbedState()
    {
        GemCanvas* canvas = GemCanvas::getActive();

        if (canvas && !m_activeCanvas) {
            // A gemcanvas just appeared → hide the desktop window
            m_activeCanvas = canvas;

            canvas->scale = m_window->glContext().getRenderingScale();
            canvas->requestedWidth = m_window->getWidth();
            canvas->requestedHeight = m_window->getHeight();
            canvas->mouseCallback = [this](juce::MouseEvent const& e, bool down) {
                float scale = m_activeCanvas->scale;
                mouseposCallback(e.x * scale, e.y * scale);
                int btn = e.mods.isRightButtonDown() ? 2 : e.mods.isMiddleButtonDown() ? 1 : 0;
                mousebuttonCallback(btn, down, 0);
            };
            canvas->motionCallback = [this](juce::MouseEvent const& e) {
                float scale = m_activeCanvas->scale;
                mouseposCallback(e.x * scale, e.y * scale);
            };
            canvas->scrollCallback = [this](juce::MouseEvent const& e, juce::MouseWheelDetails const& wheel) {
                scrollCallback(wheel.deltaX, wheel.deltaY);
            };
            canvas->keyCallback = [this](juce::KeyPress const& key) {
                std::string keyDescription = key.getTextDescription().fromLastOccurrenceOf(" ", false, false).toStdString();
                int keyNum = key.getKeyCode();
                Window::convertKeyCode(keyNum, keyDescription);
                keyCallback(keyNum, 1, keyDescription);
            };
            canvas->keyUpCallback = [this](juce::KeyPress const& key) {
                std::string keyDescription = key.getTextDescription().fromLastOccurrenceOf(" ", false, false).toStdString();
                int keyNum = key.getKeyCode();
                Window::convertKeyCode(keyNum, keyDescription);
                keyCallback(keyNum, 0, keyDescription);
            };

            if (m_window && !m_windowHidden) {
                m_windowHidden = true;
                juce::MessageManager::callAsync(
                    [safe = juce::Component::SafePointer<Window>(m_window)] {
                        if (auto* win = safe.getComponent()) win->setVisible(false);
                    });
            }
        }
        else if (!canvas && m_activeCanvas) {
            // gemcanvas was deleted → clean up FBO, show desktop window
            makeCurrent();
            m_activeCanvas->destroyFBO();
            m_activeCanvas = nullptr;

            if (m_window && m_windowHidden) {
                m_windowHidden = false;
                juce::MessageManager::callAsync(
                    [safe = juce::Component::SafePointer<Window>(m_window)] {
                        if (auto* win = safe.getComponent()) {
                            win->setVisible(true);
                            win->resized();
                        }
                    });
            }
        }
    }

    // ── Desktop window state ─────────────────────────────────────────────

    Window* m_window = nullptr;
    std::atomic<bool> m_resizing = false;
    bool m_callingSync = false;
public:

    gemjucewindow(void)
    {
        m_width = m_height = 0;
    }

    virtual ~gemjucewindow(void)
    {
        destroyMess();
    }

    // ── GemWindow interface ──────────────────────────────────────────────

    bool makeCurrent(void) override
    {
        if (!m_window) return false;
        m_window->makeContextCurrent();
        return true;
    }

    void swapBuffers(void) override
    {
        if (m_window && !m_activeCanvas)
            m_window->swapBuffers();
    }

    void dispatch(void) override
    {
        // JUCE drives its own event loop; nothing to do here.
    }

    void render(void) override
    {
      if(!m_window || m_resizing) return; // don't render while we resize, since the resize events come in from the message thread
      makeCurrent();
      m_window->checkScale();

      checkEmbedState();

      // If a canvas is active, let it bind its FBO and set the viewport
      int fboW = 0, fboH = 0;
      bool embedded = m_activeCanvas && m_activeCanvas->beginFrame(fboW, fboH);

      if (embedded) {
          // Notify Gem of the FBO dimensions (only when they change)
          static int lastNotifiedW = 0, lastNotifiedH = 0;
          if (fboW != lastNotifiedW || fboH != lastNotifiedH) {
              windowsizeCallback(fboW, fboH);
              framebuffersizeCallback(fboW, fboH);
              lastNotifiedW = fboW;
              lastNotifiedH = fboH;
          }
      }

      if (!juce::MessageManager::getInstance()->isThisTheMessageThread())
      {
          GemWindow::render();
      }

      if (embedded) {
          m_activeCanvas->endFrame();
      }
    }

    void renderMess(void)
    {
        if (makeCurrent()) render();
    }

    void bufferMess(int buf) override
    {
        switch (buf) {
        case 1: case 2:
            m_buffer = buf;
            if (m_window) {
                static int warned = 0;
                if (!warned) post("changing buffer type will only affect newly created windows");
                warned = 1;
            }
            break;
        default:
            error("buffer can only be '1' (single) or '2' (double) buffered");
        }
    }

    void titleMess(const std::string& s) override
    {
        m_title = s;
        if (m_window) m_window->setTitle(s);
    }

    void dimensionsMess(unsigned int width, unsigned int height) override
    {
        if (width  < 1) { error("width must be greater than 0");  return; }
        if (height < 1) { error("height must be greater than 0"); return; }
        m_width  = width;
        m_height = height;
        if (m_window) {
            juce::MessageManager::callAsync(
                [safeWindow = juce::Component::SafePointer<Window>(m_window),
                 w = (int)width, h = (int)height] {
                    if (safeWindow) safeWindow->setSize(w, h);
                });
        }
    }

    void fullscreenMess(int on) override
    {
        m_fullscreen = on;

        juce::MessageManager::callAsync(
            [on, safeWindow = juce::Component::SafePointer<Window>(m_window)] {
              if (safeWindow) safeWindow->setFullscreen(on != 0);
            });
    }

    void offsetMess(int x, int y) override
    {
        m_xoffset = x;
        m_yoffset = y;
        if (m_window) {
            juce::MessageManager::callAsync(
                [safeWindow = juce::Component::SafePointer<Window>(m_window), x, y] {
                    if (safeWindow) safeWindow->setTopLeftPosition(x, y);
                });
        }
    }

    void callOnMessageThread(std::function<void()> callback)
    {
        auto onMessageThread = juce::MessageManager::getInstance()->isThisTheMessageThread();
        juce::ScopedValueSetter<bool> scopedSetter(m_callingSync, !onMessageThread);
        juce::MessageManager::callSync([callback](){
          callback();
        });
    }

    bool create(void) override
    {
        if (m_window) { error("window already made!"); return false; }

        callOnMessageThread([this](){
          m_window = new Window(
              juce::Rectangle<int>(
                  m_xoffset, m_yoffset,
                  m_width  ? (int)m_width  : 500,
                  m_height ? (int)m_height : 500),
              m_border != 0,
              libpd_this_instance(),
              this);
        });

        if (!m_window) { error("JUCE couldn't create window"); return false; }

        if (!makeCurrent()) {
            error("couldn't activate JUCE OpenGL context");
            destroyMess();
            return false;
        }

        if (!createGemWindow()) { destroyMess(); return false; }

        titleMess(m_title);
        cursorMess(m_cursor);

        int fw = m_window->getWidth(), fh = m_window->getHeight();
        dimension(fw, fh);
        framebuffersize(fw, fh);

        return true;
    }

    void createMess(const std::string&) override { create(); }

    void destroy(void) override
    {
        destroyGemWindow();
        m_window = nullptr;
        info("window", "closed");
    }

    void destroyMess(void) override
    {
        if (!m_window) return;

        if (m_activeCanvas) {
            makeCurrent();
            m_activeCanvas->destroyFBO();
            m_activeCanvas = nullptr;
        }
        m_windowHidden = false;

        Window* win = m_window;
        m_window = nullptr;
        callOnMessageThread([win] {
              win->glContext().detach();
              win->removeFromDesktop();
              delete win;
        });
        destroy();
    }

    void cursorMess(bool on) override
    {
        m_cursor = on;
        if (m_window)
            m_window->setMouseCursor(on ? juce::MouseCursor::NormalCursor
                                        : juce::MouseCursor::NoCursor);
    }

    void windowsizeCallback(int w, int h)     { dimension(w, h); }
    void framebuffersizeCallback(int w, int h) { framebuffersize(w, h); }

    int windowcloseCallback(void)
    {
        info("window", "destroy");
        return 0;
    }

    void windowrefreshCallback(void) { info("window", "exposed"); }

    void keyCallback(int key, int action, std::string const& name)
    {
        gemjucewindow::key(0, name, key, action);
    }

    void mousebuttonCallback(int button, int action, int /*mods*/)
    {
        switch (button) {
        case 1: button = 2; break;
        case 2: button = 1; break;
        default: break;
        }
        gemjucewindow::button(0, button, action);
    }

    void mouseposCallback(float x, float y) { motion(0, x, y); }

    void scrollCallback(float x, float y)
    {
        auto fire = [this](int dir, float amt) {
            for (int i = 0; i < static_cast<int>(amt < 0 ? -amt : amt); ++i) {
                button(0, dir, 1);
                button(0, dir, 0);
            }
        };
        fire(x > 0 ? WHEELUP : WHEELDOWN, x);
        fire(y > 0 ? WHEELRIGHT : WHEELLEFT, y);
    }
};

#ifndef GEM_NO_SETUP
void gemjucewindow::obj_setupCallback(t_class* classPtr)
{
}

CPPEXTERN_NEW(gemjucewindow);
#endif
