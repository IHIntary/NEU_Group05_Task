#ifndef LOGINVIEW_HPP
#define LOGINVIEW_HPP

#include <gui_generated/login_screen/LOGINViewBase.hpp>
#include <gui/login_screen/LOGINPresenter.hpp>

class LOGINView : public LOGINViewBase
{
public:
    LOGINView();
    virtual ~LOGINView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // LOGINVIEW_HPP
