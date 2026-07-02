#ifndef HOMESCREENVIEW_HPP
#define HOMESCREENVIEW_HPP

#include <gui_generated/homescreen_screen/HomeScreenViewBase.hpp>
#include <gui/homescreen_screen/HomeScreenPresenter.hpp>

class HomeScreenView : public HomeScreenViewBase
{
public:
    HomeScreenView();
    virtual ~HomeScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // HOMESCREENVIEW_HPP
