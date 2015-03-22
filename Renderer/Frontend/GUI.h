#if 0
#ifndef _GUI_H
#define _GUI_H
#include "../../essentials.h"

/* 
    Ground rules: This GUI strictly works in a virtual space
    with dimensions ([0, A], [0, 1.0]) where A is the aspect
    ratio of the display, which may vary. Floating point is
    the order of the day, not unsigned integers of 1600, 900,
    720, etc.

    (0, 0) is the lower left corner.
    (A, 1) is the upper right corner.

    Trying to minimize, hopefully have absolutely no OpenGL
    calls in my GUI module. It should be reliant upon the
    frontend and just maybe, maybe the backend.

    For this framework I have decided to go with instanced
    rendering with per-instance uniforms for the modelview
    matrices.
*/

namespace GUI {

struct WidgetManager;
struct Widget;
struct AABB;

typedef std::shared_ptr<Widget> WidgetPtr;
typedef std::list<WidgetPtr> Container;
typedef AABB AxisAlignedBoundingbox;
typedef glm::uvec2 Vec2;

struct AABB {
    Vec2            minor_extent;
    Vec2            major_extent;
};

struct Widget {
    friend          WidgetManager;
    explicit        Widget(void);
    virtual         ~Widget(void);

    virtual void    show(void);
    virtual void    hide(void);

    virtual void    add_child_widget(WidgetPtr w);

protected:
    virtual void    draw(void) const;

    AABB            m_bounds;
    Container       m_children;
    bool            m_visible;
    uint16_t        m_unique_id;
};

struct WidgetManager {
    explicit        WidgetManager(void);
    virtual         ~WidgetManager(void);

    void            set_root(WidgetPtr w);
    void            set_focus(WidgetPtr w);

    virtual void    draw(void) const;

protected:
    WidgetPtr       m_root;
    WidgetPtr       m_focus;
};

}  // ~namespace GUI

#endif  // ~_GUI_H

#endif