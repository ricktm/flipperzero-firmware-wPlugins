#include <gui/gui_i.h>
#include <gui/view.h>
#include <gui/elements.h>
#include <gui/canvas.h>
#include <furi.h>
#include <input/input.h>
#include <dolphin/dolphin.h>

#include "../desktop_i.h"
#include "desktop_view_main.h"
#include "desktop/desktop_settings/desktop_settings_app.h"

uint32_t codeSequence = 0;

struct DesktopMainView {
    View* view;
    DesktopMainViewCallback callback;
    void* context;
    TimerHandle_t poweroff_timer;
    bool is_gamemode;
};

#define DESKTOP_MAIN_VIEW_POWEROFF_TIMEOUT 5000

static void desktop_view_main_dumbmode_changed(bool isThisGameMode) {
    DesktopSettingsApp* app = malloc(sizeof(DesktopSettingsApp));
    LOAD_DESKTOP_SETTINGS(&app->settings);
    app->settings.is_dumbmode = isThisGameMode;
    SAVE_DESKTOP_SETTINGS(&app->settings);
}

static void desktop_main_poweroff_timer_callback(TimerHandle_t timer) {
    DesktopMainView* main_view = pvTimerGetTimerID(timer);
    main_view->callback(DesktopMainEventOpenPowerOff, main_view->context);
}

void desktop_main_set_callback(
    DesktopMainView* main_view,
    DesktopMainViewCallback callback,
    void* context) {
    furi_assert(main_view);
    furi_assert(callback);
    main_view->callback = callback;
    main_view->context = context;
}

View* desktop_main_get_view(DesktopMainView* main_view) {
    furi_assert(main_view);
    return main_view->view;
}

bool desktop_main_input(InputEvent* event, void* context) {
    furi_assert(event);
    furi_assert(context);
    DesktopMainView* main_view = context;
	
	// change to only check for game mode setting on keypress
	if (event->type == InputTypeShort || event->type == InputTypeLong) {
		main_view->is_gamemode = false;
		DesktopSettings* desktop_settings = malloc(sizeof(DesktopSettings));
		LOAD_DESKTOP_SETTINGS(desktop_settings);
		if(desktop_settings->is_dumbmode) main_view->is_gamemode = true;
		free(desktop_settings);
	}
	
    if(!main_view->is_gamemode) {
        if(event->type == InputTypeShort) {
            if(event->key == InputKeyOk) {
                main_view->callback(DesktopMainEventOpenMenu, main_view->context);
            } else if(event->key == InputKeyUp) {
                main_view->callback(DesktopMainEventOpenLockMenu, main_view->context);
            } else if(event->key == InputKeyDown) {
                main_view->callback(DesktopMainEventOpenArchive, main_view->context);
            } else if(event->key == InputKeyLeft) {
                main_view->callback(DesktopMainEventOpenFavoritePrimary, main_view->context);
            } else if(event->key == InputKeyRight) {
                main_view->callback(DesktopMainEventOpenPassport, main_view->context);
            }
        } else if(event->type == InputTypeLong) {
            if(event->key == InputKeyOk) {
                main_view->callback(DesktopAnimationEventNewIdleAnimation, main_view->context);
            } else if(event->key == InputKeyUp) {
                main_view->callback(DesktopMainEventOpenFavoriteGame, main_view->context);
            } else if(event->key == InputKeyDown) {
                main_view->callback(DesktopMainEventOpenGames, main_view->context);
            } else if(event->key == InputKeyLeft) {
                main_view->callback(DesktopMainEventOpenFavoriteSecondary, main_view->context);
            } else if(event->key == InputKeyRight) {
                // THIS DOESNT WORK
            }
        }
    } else {
        if(event->type == InputTypeShort) {
            if(event->key == InputKeyOk) {
                if(codeSequence == 5 || codeSequence == 7) {
                    codeSequence++;
                } else {
                    codeSequence = 0;
                    main_view->callback(DesktopMainEventOpenGames, main_view->context);
                }
            } else if(event->key == InputKeyUp) {
                if(codeSequence == 0 || codeSequence == 1) {
                    codeSequence++;
                } else {
                    codeSequence = 0;
                    main_view->callback(DesktopMainEventOpenFavoriteGame, main_view->context);
                }
            } else if(event->key == InputKeyDown) {
                // PREFER TO OPEN GAMES MENU
                if(codeSequence == 2 || codeSequence == 3) {
                    codeSequence++;
                } else {
                    codeSequence = 0;
                }
            } else if(event->key == InputKeyLeft) {
                if(codeSequence == 4 || codeSequence == 6) {
                    codeSequence++;
                } else {
                    codeSequence = 0;
                    main_view->callback(DesktopMainEventOpenClock, main_view->context);
                }
            } else if(event->key == InputKeyRight) {
                // GOES TO PASSPORT NO MATTER WHAT
            }
            if(codeSequence == 8) {
                // UNLOCK!
                codeSequence = 0;
                desktop_view_main_dumbmode_changed(0);
                main_view->callback(DesktopMainEventOpenMenu, main_view->context);
            }
        } else if(event->type == InputTypeLong) {
            if(event->key == InputKeyOk) {
                main_view->callback(DesktopAnimationEventNewIdleAnimation, main_view->context);
            } else if(event->key == InputKeyUp) {
                main_view->callback(DesktopMainEventOpenFavoriteGame, main_view->context);
            } else if(event->key == InputKeyDown) {
                main_view->callback(DesktopMainEventOpenGames, main_view->context);
            } else if(event->key == InputKeyLeft) {
                main_view->callback(DesktopMainEventOpenClock, main_view->context);
            } else if(event->key == InputKeyRight) {
                // THIS DOESNT WORK, PASSPORT WILL ONLY OPEN ON REGULAR RIGHT, NOTHING CAN GET ASSIGNED HERE
            }
        }
    }

    if(event->key == InputKeyBack) {
        if(event->type == InputTypePress) {
            xTimerChangePeriod(
                main_view->poweroff_timer,
                pdMS_TO_TICKS(DESKTOP_MAIN_VIEW_POWEROFF_TIMEOUT),
                portMAX_DELAY);
        } else if(event->type == InputTypeRelease) {
            xTimerStop(main_view->poweroff_timer, portMAX_DELAY);
        }
    }

    return true;
}

DesktopMainView* desktop_main_alloc() {
    DesktopMainView* main_view = malloc(sizeof(DesktopMainView));
	
    main_view->is_gamemode = false;
    DesktopSettings* desktop_settings = malloc(sizeof(DesktopSettings));
    LOAD_DESKTOP_SETTINGS(desktop_settings);
    if(desktop_settings->is_dumbmode) main_view->is_gamemode = true;
    free(desktop_settings);
	
    main_view->view = view_alloc();
    view_allocate_model(main_view->view, ViewModelTypeLockFree, 1);
    view_set_context(main_view->view, main_view);
    view_set_input_callback(main_view->view, desktop_main_input);

    main_view->poweroff_timer = xTimerCreate(
        NULL,
        pdMS_TO_TICKS(DESKTOP_MAIN_VIEW_POWEROFF_TIMEOUT),
        pdFALSE,
        main_view,
        desktop_main_poweroff_timer_callback);

    return main_view;
}

void desktop_main_free(DesktopMainView* main_view) {
    furi_assert(main_view);
    view_free(main_view->view);
    furi_timer_free(main_view->poweroff_timer);
    free(main_view);
}