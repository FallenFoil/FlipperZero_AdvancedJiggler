#include "jiggler.h"
#include <input/input.h>
#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_usb.h>
#include <furi_hal_usb_hid.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <dolphin/dolphin.h>

#include "advanced_mouse_jiggler_icons.h"

#define MOUSE_MOVE_SHORT 5
#define MOUSE_MOVE_LONG 20

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriTimer* timer;
    int running;
    uint8_t counter;
    int exit;
} AppData;

typedef enum {
    EventTypeInput,
} EventType;

typedef struct {
    InputEvent input;
} UsbMouseEvent;

static void mouse_jiggler_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    AppData* app_data = context;

    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(canvas, 17, 3, AlignLeft, AlignTop, "Advanced Mouse Jiggler");
    canvas_set_font(canvas, FontSecondary);

    // Ok
    canvas_draw_icon(canvas, 32, 25, &I_box);

    if(app_data->running == 1) {
        elements_slightly_rounded_box(canvas, 35, 27, 60, 13);
        canvas_set_color(canvas, ColorWhite);
    }

    canvas_draw_icon(canvas, 43, 29, &I_btn);

    if(app_data->running == 1) {
        elements_multiline_text_aligned(canvas, 60, 37, AlignLeft, AlignBottom, "Stop");
    } else {
        elements_multiline_text_aligned(canvas, 60, 37, AlignLeft, AlignBottom, "Start");
    }
    canvas_set_color(canvas, ColorBlack);

    // Back
    canvas_draw_icon(canvas, 0, 54, &I_go_back);
    elements_multiline_text_aligned(canvas, 13, 62, AlignLeft, AlignBottom, "Exit");
}

static void mouse_jiggler_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    // AppData* app_data = context;

    // if(event->type == InputTypeShort) {
    //     switch(event->key) {
    //         case InputKeyOk:
    //             app_data->running = !app_data->running;
    //             break;
    //         case InputKeyBack:
    //             app_data->running = false;
    //             app_data->exit = true;
    //             furi_timer_stop(app_data->timer);
    //             break;
    //         default:
    //             break;
    //     }
    // }

    FuriMessageQueue* event_queue = context;

    UsbMouseEvent mouseEvent;
    mouseEvent.input = *event;
    furi_message_queue_put(event_queue, &mouseEvent, FuriWaitForever);
}

// static void mouse_jiggler_timer_callback(void* context) {
//     furi_assert(context);
//     AppData* app_data = context;
//     if(app_data->running == 1) {
//         app_data->counter++;
//         furi_hal_hid_mouse_move((app_data->counter % 2 == 0) ? MOUSE_MOVE_SHORT : -MOUSE_MOVE_SHORT, 0);
//     }
// }

void mouse_jiggler_free(AppData* app_data) {
    furi_assert(app_data);

    // furi_timer_stop(app_data->timer);
    // furi_timer_free(app_data->timer);

    free(app_data);
}

AppData* mouse_jiggler_alloc(FuriMessageQueue* event_queue) {
    AppData* app_data = malloc(sizeof(AppData));

    app_data->gui = furi_record_open("gui");

    app_data->view_port = view_port_alloc();
    view_port_draw_callback_set(app_data->view_port, mouse_jiggler_draw_callback, app_data);
    view_port_input_callback_set(app_data->view_port, mouse_jiggler_input_callback, event_queue);
    gui_add_view_port(app_data->gui, app_data->view_port, GuiLayerFullscreen);

    // app_data->timer = furi_timer_alloc(mouse_jiggler_timer_callback, FuriTimerTypePeriodic, app_data);
    // furi_timer_start(app_data->timer, 500);

    app_data->running = 0;
    app_data->counter = 0;
    app_data->exit = 0;

    return app_data;
}

int32_t advanced_mouse_jiggler(void* p){
    UNUSED(p);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(UsbMouseEvent));
    furi_check(event_queue);

    // FuriHalUsbInterface* usb_mode_prev = furi_hal_usb_get_config();
    // furi_hal_usb_set_config(&usb_hid);

    AppData* app_data = mouse_jiggler_alloc(event_queue);

    UsbMouseEvent event;
    while(1) {
        FuriStatus event_status = furi_message_queue_get(event_queue, &event, 100);

        furi_hal_hid_mouse_move(MOUSE_MOVE_SHORT, 0);
        furi_delay_ms(500);
        furi_hal_hid_mouse_move(-MOUSE_MOVE_SHORT, 0);
        furi_delay_ms(500);

        if(event_status == FuriStatusOk) { 
            if(event.input.key == InputKeyBack) {
                break;
            }
        }

        view_port_update(app_data->view_port);
    }

    // furi_hal_usb_set_config(usb_mode_prev);

    gui_remove_view_port(app_data->gui, app_data->view_port);
    view_port_free(app_data->view_port);
    furi_record_close("gui");
    free(app_data);
    
    return 0;
}