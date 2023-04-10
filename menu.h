
#ifndef MENU_H
#define MENU_H
struct MenuItem{
	char title[20]; 
};

struct MenuState{
	MenuItem items[4];
	B32 initilized;
	OpenglContext gl_context;
	U8 selected;
	B32 click;
};
#endif