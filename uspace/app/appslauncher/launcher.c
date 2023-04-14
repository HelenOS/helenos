/*
 * Copyright (c) 2023 SimonJRiddix
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup launcher
 * @{
 */
/** @file Launcher
 */

#include "include/window.h"
#include "include/label.h"
#include "include/button.h"
#include "include/control.h"

#include <task.h>

#define newVector(name) vector name; vector_init(&name);
#define vector_get(v, pos) v.Get(&v, pos)
#define vector_count(v) v.Count(&v)

typedef struct sApp application;
struct sApp
{
    char* name;
    char* location;
    char** arguments;
};

typedef struct scategory category;
struct scategory
{
	char* name;
	//icon_t icon;
    vector applications;
};

// init application categories vector
vector categories;

// init window ui
Window window;

// extern application launcher code
extern int app_launchl(const char *, ...);

// on tile click
static void pb_clicked(ui_pbutton_t *pbutton, void * args)
{	
	if(app_launchl(((application *) args)->location, NULL) == EOK)
	{
		window.destroy(&window);
		task_kill(task_get_id());
	}
}

int main(int argc, char *argv[])
{
    vector_init(&categories);
    
    /// ACCESSORIES ///
    
    category category_accessory;
    vector_init(&category_accessory.applications);
    category_accessory.name = (char*) "Accessory";
    
    application notepad;
    notepad.name = (char*) "Notepad";
    notepad.location = (char*) "/app/notepad";
    category_accessory.applications.Add(&category_accessory.applications, &notepad);
    
    application calc;
    calc.name = (char*) "Calculator";
    calc.location = (char*) "/app/calculator";
    category_accessory.applications.Add(&category_accessory.applications, &calc);
    
    categories.Add(&categories, &category_accessory);
    
    /// TEST ///
    
    category category_test;
    vector_init(&category_test.applications);
    category_test.name = (char*) "Test";
    
    application ui_demo;
    ui_demo.name = (char*) "UI Demo";
    ui_demo.location = (char*) "/app/uidemo";
    category_test.applications.Add(&category_test.applications, &ui_demo);
    
    application gfx_demo;
    gfx_demo.name = (char*) "GFX Test";
    gfx_demo.location = (char*) "/app/gfxdemo";
    char* gfx_demo_args[1];
    gfx_demo_args[0] = (char*) "ui";
    gfx_demo.arguments = gfx_demo_args;
    category_test.applications.Add(&category_test.applications, &gfx_demo);
    
    categories.Add(&categories, &category_test);
        
    /// SYSTEM ///
    
    category category_system;
    category category_left_menu;
    vector_init(&category_system.applications);
    vector_init(&category_left_menu.applications);
    category_system.name = (char*) "System";
    category_left_menu.name = (char*) "System";
    
    application system_shutdown;
    system_shutdown.name = (char*) "Shutdown";
    system_shutdown.location = (char*) "/app/systemshutdown";
    category_left_menu.applications.Add(&category_left_menu.applications, &system_shutdown);
    
    application system_restart;
    system_restart.name = (char*) "Restart";
    system_restart.location = (char*) "/app/systemrestart";
    category_left_menu.applications.Add(&category_left_menu.applications, &system_restart);
    
    application system_setting;
    system_setting.name = (char*) "Setting";
    system_setting.location = (char*) "/app/setting";
    category_system.applications.Add(&category_system.applications, &system_setting);
    category_left_menu.applications.Add(&category_left_menu.applications, &system_setting);
    
    application sys_terminal;
    sys_terminal.name = (char*) "Terminal";
    sys_terminal.location = (char*) "/app/terminal";
    category_system.applications.Add(&category_system.applications, &sys_terminal);
    category_left_menu.applications.Add(&category_left_menu.applications, &sys_terminal);
    
    categories.Add(&categories, &category_system);
	
	// Create window
	init_window(&window, "window1", "Application Launcher");
	
	// Create small tiles on the left for system actions (shutdown, reboot, terminal...)
	for(int c = 0; c < vector_count(category_left_menu.applications); c++)
	{
		Button button_left;
		//char button_left_name[256];
		//sprintf(button_left_name, "btnl%d", c);
		init_button(&button_left, "button1", &window);
		button_left.rect.p0.x = 10;
		button_left.rect.p0.y = window.params.rect.p1.y - ( (c + 1) * (30 + 10));
		button_left.rect.p1.x = 50;
		button_left.rect.p1.y = button_left.rect.p0.y + 30;
		button_left.update_rect(&button_left);
		application* a = vector_get(category_left_menu.applications, c);
		button_left.set_text(&button_left, a->name);
		button_left.paint(&button_left);
		button_left.set_callback(&button_left, pb_clicked, a);
	}
	
	size_t separators = 0;
	
	int row = 0;
	int col = 0;
	
	// Populate categories and big application tiles
	for(int current_category = 0; current_category < vector_count(categories); current_category++)
	{
		category* cat = vector_get(categories, current_category);
		
		Label label_category;		
		label_category.rect.p0.x = 60;
		label_category.rect.p0.y = (30*row) + ( row * 10 ) + (row * 80) + 40;
		label_category.rect.p1.x = label_category.rect.p0.x + 170;
		label_category.rect.p1.y = label_category.rect.p0.y + 10;
		//char label_category_name[256];
		//sprintf(label_category_name, "lbl%d", current_category);
		init_label(&label_category, "label1", &window);	
		label_category.set_text(&label_category, cat->name);
		label_category.update_rect(&label_category);
		label_category.paint(&label_category);
		
		// Populate big application tiles
		for(int application_in_category = 0; application_in_category < vector_count(cat->applications); application_in_category++)
		{
			application* app;
			app = cat->applications.Get(&cat->applications, application_in_category);
			
			// if application name is empty, is not an application but a separator
			if(str_cmp("", app->name) == 0)
			{
				separators++;
			}
			else
			{
				int c_pos = 70 + ( col * 20 ) + ( col * 80 );
				if((c_pos + 80) > window.params.rect.p1.x)
				{
					col = 0;
					c_pos = 70 + ( col * 20 ) + ( col * 80 );
					row++;
				}
				
				Button button_application;
				//char button_application_name[256];
				//sprintf(button_application_name, "btn%d", (application_in_category+1)*c);
				init_button(&button_application, "button1", &window);
				button_application.rect.p0.x = c_pos;
				button_application.rect.p0.y = 20 + (30*row) + ( row * 10 ) + (row * 80) + 40;
				button_application.rect.p1.x = button_application.rect.p0.x + 80;
				button_application.rect.p1.y = button_application.rect.p0.y + 80;
				button_application.update_rect(&button_application);
				button_application.set_text(&button_application, app->name);
				button_application.set_callback(&button_application, pb_clicked, app);
				
				col++;
			}
		}
		
		row++;
		col = 0;
	}

	window.draw(&window);
	
	window.destroy(&window);
	
	return 0;
}

/** @}
 */
