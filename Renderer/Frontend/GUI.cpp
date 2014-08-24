#include "../Backend/GL.h"
#include "GUI.h"

namespace GUI {

// a lot of these definitions will be blank
// this is intentional

static uint16_t unique_id_counter = 0;

Widget::Widget(void) : m_visible(false) {
    m_bounds.minor_extent = glm::vec2(0.0f, 0.0f);
    m_bounds.major_extent = glm::vec2(1.0f, 1.0f);
    m_unique_id = unique_id_counter++;
}

Widget::~Widget(void) { 
    //for (auto child : m_children)
        //child.reset();
}

void Widget::show(void) { 
    m_visible = true;
    for (auto child : m_children)
        child->show();
}
void Widget::hide(void) { 
    m_visible = false;
    for (auto child : m_children)
        child->hide();
}
void Widget::draw(void) const {
    if (false == m_visible)
        return;

    // Draw yourself first, then the children
    // ....
    for (auto child : m_children)
        child->draw();
}

void Widget::add_child_widget(WidgetPtr w) {
    m_children.push_back(w);
}

WidgetManager::WidgetManager(void) :
    m_root(nullptr),
    m_focus(nullptr) {
}
WidgetManager::~WidgetManager(void) {
    //m_root.reset();
}

void WidgetManager::set_root(WidgetPtr root) {
    assert(nullptr != root);
    m_root = root;
}

void WidgetManager::set_focus(WidgetPtr focus) {
    assert(nullptr != focus);
    m_focus = focus;
}

void WidgetManager::draw(void) const {
    if (nullptr == m_root)
        return;
    m_root->draw();
}

}  // ~namespace GUI