////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.at
//
// Implementation file
//
//    Copyright (c) 1997-1999 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "Event.h"
#include "Base/GemWinCreate.h"

#include <mutex>
#include <stdlib.h>
#include <m_pd.h>

/////////////////////////////////////////////////////////
// The callbacks
//
/////////////////////////////////////////////////////////

static void eventClock(void *x);

typedef enum {
  NONE,
  MOTION,
  BUTTON,
  WHEEL,
  KEYBOARD,
  RESIZE
} gem_event_t;

typedef struct _event_queue_item {
  gem_event_t type;
  struct _event_queue_item*next;
  const char*string;
  int x;
  int y;
  int state;
  int axis;
  int value;
  int which;
} gem_event_queue_item_t;

struct EventQueue
{
    struct CallbackList {
      CallbackList() : data(NULL), func(NULL), next(NULL) {}
      void *data;
      void *func;
      CallbackList *next;
    };
    
    CallbackList *motionList = NULL;
    CallbackList *buttonList = NULL;
    CallbackList *wheelList = NULL;
    CallbackList *keyboardList = NULL;
    CallbackList *resizeList = NULL;
    
    EventQueue()
    {
        first = NULL;
        last = NULL;
        clock = clock_new(NULL, reinterpret_cast<t_method>(eventClock));
        clock_delay(clock, 15);
    }
    
    gem_event_queue_item_t*first;
    gem_event_queue_item_t*last;
    std::mutex queueLock;
    t_clock *clock;
    
    static inline EventQueue* get()
    {
        return &instances[pd_this];
    }
    
    static inline std::map<void*, EventQueue> instances = std::map<void*, EventQueue>();
};


/////////////////////////////////////////////////////////
// Motion callbacks
//
/////////////////////////////////////////////////////////
GEM_EXTERN void setMotionCallback(MOTION_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& motionList = eventQueue->motionList;

  auto* newCallback = new EventQueue::CallbackList;
  newCallback->data = data;
  newCallback->func = (void *)callback;

  if (!motionList) {
      motionList = newCallback;
  } else {
    auto* theList = motionList;
    while(theList->next) {
      theList = theList->next;
    }
    theList->next = newCallback;
  }
}

GEM_EXTERN void removeMotionCallback(MOTION_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& motionList = eventQueue->motionList;

  auto* theList = motionList;
  if (!theList) {
    return;
  } else if (theList->func == (void *)callback &&
             theList->data == data) {
      motionList = theList->next;
    delete theList;
  } else {
    while(theList->next) {
      if (theList->next->func == (void *)callback &&
          theList->next->data == data) {
        auto* holder = theList->next;
        theList->next = holder->next;
        delete holder;
        return;
      }
      theList = theList->next;
    }
  }
}
/////////////////////////////////////////////////////////
// Button callbacks
//
/////////////////////////////////////////////////////////
GEM_EXTERN void setButtonCallback(BUTTON_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& buttonList = eventQueue->buttonList;
    
  auto* newCallback = new EventQueue::CallbackList;

  newCallback->data = data;
  newCallback->func = (void *)callback;

  if (!buttonList) {
      buttonList = newCallback;
  } else {
    auto* theList = buttonList;
    while(theList->next) {
      theList = theList->next;
    }
    theList->next = newCallback;
  }
}
GEM_EXTERN void removeButtonCallback(BUTTON_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& buttonList = eventQueue->buttonList;
  auto* theList = buttonList;
  if (!theList) {
    return;
  } else if (theList->func == (void *)callback &&
             theList->data == data) {
      buttonList = theList->next;
    delete theList;
  } else {
    while(theList->next) {
      if (theList->next->func == (void *)callback &&
          theList->next->data == data) {
        auto* holder = theList->next;
        theList->next = holder->next;
        delete holder;
        return;
      }
      theList = theList->next;
    }
  }
}
/////////////////////////////////////////////////////////
// Wheel callbacks
//
/////////////////////////////////////////////////////////
GEM_EXTERN void setWheelCallback(WHEEL_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& wheelList = eventQueue->wheelList;
  auto* newCallback = new EventQueue::CallbackList;
  newCallback->data = data;
  newCallback->func = (void *)callback;

  if (!wheelList) {
      wheelList = newCallback;
  } else {
    auto* theList = wheelList;
    while(theList->next) {
      theList = theList->next;
    }
    theList->next = newCallback;
  }
}
GEM_EXTERN void removeWheelCallback(WHEEL_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& wheelList = eventQueue->wheelList;
  auto *theList = wheelList;
  if (!theList) {
    return;
  } else if (theList->func == (void *)callback &&
             theList->data == data) {
      wheelList = theList->next;
    delete theList;
  } else {
    while(theList->next) {
      if (theList->next->func == (void *)callback &&
          theList->next->data == data) {
        auto *holder = theList->next;
        theList->next = holder->next;
        delete holder;
        return;
      }
      theList = theList->next;
    }
  }
}
/////////////////////////////////////////////////////////
// Keyboard callbacks
//
/////////////////////////////////////////////////////////
GEM_EXTERN void setKeyboardCallback(KEYBOARD_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& keyboardList = eventQueue->keyboardList;
    
  auto* newCallback = new EventQueue::CallbackList;
  newCallback->data = data;
  newCallback->func = (void *)callback;

  if (!keyboardList) {
      keyboardList = newCallback;
  } else {
    auto* theList = keyboardList;
    while(theList->next) {
      theList = theList->next;
    }
    theList->next = newCallback;
  }
}
GEM_EXTERN void removeKeyboardCallback(KEYBOARD_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& keyboardList = eventQueue->keyboardList;
    
  auto* theList = keyboardList;
  if (!theList) {
    return;
  } else if (theList->func == (void *)callback &&
             theList->data == data) {
      keyboardList = theList->next;
    delete theList;
  } else {
    while(theList->next) {
      if (theList->next->func == (void *)callback &&
          theList->next->data == data) {
        auto* holder = theList->next;
        theList->next = holder->next;
        delete holder;
        return;
      }
      theList = theList->next;
    }
  }
}
/////////////////////////////////////////////////////////
// Resize callbacks
//
/////////////////////////////////////////////////////////
GEM_EXTERN void setResizeCallback(RESIZE_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& resizeList = eventQueue->resizeList;
    
  auto* newCallback = new EventQueue::CallbackList;
  newCallback->data = data;
  newCallback->func = (void *)callback;

  if (!resizeList) {
      resizeList = newCallback;
  } else {
    auto* theList = resizeList;
    while(theList->next) {
      theList = theList->next;
    }
    theList->next = newCallback;
  }
}
GEM_EXTERN void removeResizeCallback(RESIZE_CB callback, void *data)
{
  auto* eventQueue = EventQueue::get();
  auto*& resizeList = eventQueue->resizeList;
    
  auto* theList = resizeList;
  if (!theList) {
    return;
  } else if (theList->func == (void *)callback &&
             theList->data == data) {
      resizeList = theList->next;
    delete theList;
  } else {
    while(theList->next) {
      if (theList->next->func == (void *)callback &&
          theList->next->data == data) {
        auto* holder = theList->next;
        theList->next = holder->next;
        delete holder;
        return;
      }
      theList = theList->next;
    }
  }
}

static gem_event_queue_item_t* createEvent(gem_event_t type, const char*string,
    int x, int y, int state, int axis, int value, int which)
{
  gem_event_queue_item_t*ret=new gem_event_queue_item_t;
  ret->type=type;
  ret->next=NULL;
  ret->string=string;
  ret->x=x;
  ret->y=y;
  ret->state=state;
  ret->axis=axis;
  ret->value=value;
  ret->which=which;

  return ret;
}
static void deleteEvent( gem_event_queue_item_t* event)
{
  auto* event_queue = EventQueue::get();

  if(event == event_queue->first) {
    event_queue->first=event->next;
  }

  if(event == event_queue->last) {
    event_queue->last=NULL;
  }

  event->type=NONE;
  event->next=NULL;
  event->string=0;
  event->x=0;
  event->y=0;
  event->state=0;
  event->axis=0;
  event->value=0;
  event->which=0;

  delete event;
}

static void addEvent(gem_event_t type, const char*string, int x, int y,
                     int state, int axis, int value, int which)
{
  auto* event_queue = EventQueue::get();

  event_queue->queueLock.lock();
  gem_event_queue_item_t*item=createEvent(type, string, x, y, state, axis,
                                          value, which);
  if(NULL==event_queue->first) {
    event_queue->first=item;
  }
  if(event_queue->last) {
    event_queue->last->next=item;
  }
  event_queue->last=item;
  event_queue->queueLock.unlock();
}

static void dequeueEvents(void)
{
  if(!gemWinSetCurrent()) return;

  EventQueue::CallbackList* theList=NULL;
  auto* eventQueue = EventQueue::get();
    
  if (NULL==eventQueue) {
    pd_error(0, "dequeue NULL queue");
    return;
  }
  gem_event_queue_item_t*events = eventQueue->first;
  if(NULL==events) {
    return;
  }
  while(events) {

    switch(events->type) {
    case( MOTION):
      theList = eventQueue->motionList;
      while(theList) {
        MOTION_CB callback = (MOTION_CB)theList->func;
        (*callback)(events->x, events->y, theList->data);
        theList = theList->next;
      }
      break;
    case( BUTTON):
      theList = eventQueue->buttonList;
      while(theList) {
        BUTTON_CB callback = (BUTTON_CB)theList->func;
        (*callback)(events->which, events->state, events->x, events->y,
                    theList->data);
        theList = theList->next;
      }
      break;
    case( WHEEL):
      theList = eventQueue->wheelList;
      while(theList) {
        WHEEL_CB callback = (WHEEL_CB)theList->func;
        (*callback)(events->axis, events->value, theList->data);
        theList = theList->next;
      }
      break;
    case( KEYBOARD):
      theList = eventQueue->keyboardList;
      while(theList) {
        KEYBOARD_CB callback = (KEYBOARD_CB)theList->func;
        (*callback)(events->string, events->value, events->state, theList->data);
        theList = theList->next;
      }
      break;
    case( RESIZE):
      theList = eventQueue->resizeList;
      while(theList) {
        RESIZE_CB callback = (RESIZE_CB)theList->func;
        (*callback)(events->x, events->y, theList->data);
        theList = theList->next;
      }
      break;
    default:
      break;
    }

    gem_event_queue_item_t*old = events;
    events=events->next;

    deleteEvent(old);
  }
}

static void eventClock(void *x)
{
    auto* eventQueue = EventQueue::get();
    eventQueue->queueLock.lock();
    dequeueEvents();
    eventQueue->queueLock.unlock();
    
    clock_delay(eventQueue->clock, 15);
}

/////////////////////////////////////////////////////////
// Trigger events
//
/////////////////////////////////////////////////////////
GEM_EXTERN void triggerMotionEvent(int x, int y)
{
  addEvent(MOTION, NULL, x, y, 0, 0, 0, 0);
}
GEM_EXTERN void triggerButtonEvent(int which, int state, int x, int y)
{
  addEvent(BUTTON, NULL, x, y, state, 0, 0, which);
}
GEM_EXTERN void triggerWheelEvent(int axis, int value)
{
  addEvent(WHEEL, NULL, 0, 0, 0, axis, value, 0);
}
GEM_EXTERN void triggerKeyboardEvent(const char *string, int value, int state)
{
  addEvent(KEYBOARD, gensym(string)->s_name, 0, 0, state, 0, value, 0);
}
GEM_EXTERN void triggerResizeEvent(int xSize, int ySize)
{
  addEvent(RESIZE, NULL, xSize, ySize, 0, 0, 0, 0);
}
