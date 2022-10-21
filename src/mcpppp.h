// generated by Fast Light User Interface Designer (fluid) version 1.0306

#ifndef mcpppp_h
#define mcpppp_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
extern void windowclosed(Fl_Double_Window*, void*);
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
extern void opensettings(Fl_Button*, void*);
extern void openhelp(Fl_Button*, void*);
#include <FL/Fl_Box.H>
#include <FL/Fl_Value_Slider.H>
extern void updateoutputlevel(Fl_Value_Slider*, void*);
#include <FL/Fl_Browser.H>
#include <FL/Fl_Check_Button.H>
extern void conversion(Fl_Check_Button*, void*);
#include <FL/Fl_Input.H>
extern void editpath(Fl_Input*, void*);
extern void browse(Fl_Button*, void*);
#include <FL/Fl_Scroll.H>
extern void reload(Fl_Button*, void*);
extern void selectall(Fl_Check_Button*, void*);
extern void run(Fl_Button*, void*);
extern void addrespath(Fl_Button*, void*);
extern void deleterespath(Fl_Button*, void*);
#include <FL/Fl_Window.H>
#include <FL/Fl_Help_View.H>

class UI {
public:
  UI();
  Fl_Double_Window *window;
  Fl_Box *box1;
  Fl_Box *box2;
  Fl_Value_Slider *outputlevelslider;
  Fl_Browser *output;
  Fl_Input *path_input;
  Fl_Scroll *scroll;
  Fl_Check_Button *allpacks;
  Fl_Button *run_button;
  Fl_Double_Window *edit_paths;
  Fl_Scroll *paths;
  Fl_Double_Window *settings;
  Fl_Window *help;
  Fl_Window *path_warning;
private:
  inline void cb_Close_i(Fl_Button*, void*);
  static void cb_Close(Fl_Button*, void*);
public:
  Fl_Check_Button *dontshowwarning;
  void show();
};
extern void settingchanged(Fl_Widget*, void*);
#endif
