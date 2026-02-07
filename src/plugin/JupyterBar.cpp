#include "JupyterBar.h"
#include <algorithm>
#include <vector>

using namespace cnoid;

namespace cnoid {

class ButtonClass
{
public:
    ToolButton *self;
    std::string name;
    bool state;
    long counter;

public:
    ButtonClass (ToolButton *_self) : self(_self), state(false), counter(0) { }
};

class JupyterBar::Impl
{
public:
    Impl(JupyterBar* _self);

    JupyterBar *self;
    std::vector<ButtonClass *> buttons;

    void addButton(const char *icon, const char *tooltip);
#if 0
    void addButton(QIcon &icon, const char *tooltip);
#endif
    void addToggleButton(const char *icon, const char *tooltip);

    ButtonClass *findButton(const std::string &name);
};

}

JupyterBar* JupyterBar::instance()
{
    static JupyterBar* instance = new JupyterBar;
    return instance;
}

JupyterBar::JupyterBar()
    : ToolBar("JupyterBar")
{
    impl = new Impl(this);
}

JupyterBar::~JupyterBar()
{
    delete impl;
}

JupyterBar::Impl::Impl(JupyterBar* _self)
{
    self = _self;

    addButton("JBtn", "Sample Jupyter Button");
    //addToggleButton("J Toggle", "Jupyter Toggle Button");
}

void JupyterBar::Impl::addButton(const char *icon, const char *tooltip)
{
    ButtonClass *bt = new ButtonClass(self->addButton(icon));
    bt->name = icon;
    bt->self->setToolTip(tooltip);
    bt->self->sigClicked().connect( [this, bt] () {
        bt->counter++;
        bt->state = !(bt->state);
    } );
    //
    buttons.push_back(bt);
}
void JupyterBar::Impl::addToggleButton(const char *icon, const char *tooltip)
{

    ButtonClass *bt = new ButtonClass(self->addToggleButton(icon));
    bt->name = icon;
    bt->self->setToolTip(tooltip);
    bt->self->sigToggled().connect( [this, bt] (bool _in) {
        bt->counter++;
        bt->state = _in;
    } );
    //
    buttons.push_back(bt);
}
#if 0
void JupyterBar::Impl::addButton(QIcon &icon, const char *tooltip)
{
    ButtonClass *bt = new ButtonClass(self->addButton(icon));
    bt->self->setToolTip(tooltip);
    bt->self->sigClicked().connect( [this, bt] () {
        bt->counter++;
        bt->state = !(bt->state);
    } );
    //
    buttons.push_back(bt);
}
#endif

ButtonClass *JupyterBar::Impl::findButton(const std::string &name)
{
    auto it = std::find_if(buttons.begin(), buttons.end(),
                           [&name] (const auto *bt) {
                               return bt && bt->name == name;
                           } );
    return (it != buttons.end()) ? *it : nullptr;
}

bool JupyterBar::addUserButton(const std::string &name)
{
    ButtonClass *bt = impl->findButton(name);
    if (!!bt) {
        return false;
    }
    impl->addButton(name.c_str(), name.c_str());
    return true;
}
bool JupyterBar::addUserToggleButton(const std::string &name)
{
    ButtonClass *bt = impl->findButton(name);
    if (!!bt) {
        return false;
    }
    impl->addToggleButton(name.c_str(), name.c_str());
    return true;
}
long JupyterBar::getUserButton(const std::string &name)
{
    ButtonClass *bt = impl->findButton(name);
    if (!bt) {
        throw std::runtime_error("Button not found: " + name);
    }
    return bt->counter;
}
bool JupyterBar::getUserButtonState(const std::string &name)
{
    ButtonClass *bt = impl->findButton(name);
    if (!bt) {
        throw std::runtime_error("Button not found: " + name);
    }
    return bt->state;
}
