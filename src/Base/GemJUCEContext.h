/*-----------------------------------------------------------------
 
 GEM - Graphics Environment for Multimedia
 
 definitions a JUCE opengl context
 
 Copyright (c) 2024 Timothy Schoen
 
 For information on usage and redistribution, and for a DISCLAIMER OF ALL
 WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
 
 -----------------------------------------------------------------*/

#include <map>

class GemJUCEWindow;
namespace juce
{
class OpenGLContext;
}

class WindowInfo
{
public:
    
    // Constructor
    WindowInfo() :
    fs(0),
    have_constContext(0)
    {
    }
    
    int fs;                 // FullScreen
    int have_constContext;  // 1 if we have a constant context
    
    // Only have this helper function on plugdata's side
#if PLUGDATA
    GemJUCEWindow* getWindow()
    {
        void* instance = libpd_this_instance();
        if(window.find(instance) == window.end()) return nullptr;
        
        return window.at(instance);
    }
    
    juce::OpenGLContext* getContext()
    {
        void* instance = libpd_this_instance();
        if(context.find(instance) == context.end()) return nullptr;
        
        return context.at(instance);
    }
#endif
    
    std::map<void*, juce::OpenGLContext*> context;
    std::map<void*, GemJUCEWindow*> window;
};

/*-----------------------------------------------------------------
 -------------------------------------------------------------------
 CLASS
 WindowHints
 
 Hints for window creation
 
 DESCRIPTION
 
 -----------------------------------------------------------------*/
class WindowHints
{
public:
    
    //////////
    // Should the window be realized
    int actuallyDisplay;
    
    //////////
    // Single or double buffered
    int buffer;
    
    //////////
    // The width/x of the window
    int width;
    //////////
    // The height/y of the window
    int height;
    
    //////////
    // the real width/height of the window (set by createGemWindow())
    int real_w, real_h;
    
    //////////
    // The offset/x of the window (likely tobe overridden by the window-manager)
    int x_offset;
    //////////
    // The offset/y of the window (likely tobe overridden by the window-manager)
    int y_offset;
    
    //////////
    // Should we do fullscreen ?
    int fullscreen;
    
    //////////
    // Is there a second screen ?
    int secondscreen;
    
    //////////
    // Should there be a window border?
    int border;
    
    //////////
    // mode for full-screen antialiasing
    int fsaa;
    
    ///// if we can use a different display , this has its meaning under X
    const char* display;
    
    //////////////
    // display some title....
    const char* title;
};
