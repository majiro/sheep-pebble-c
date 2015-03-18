/*
  Sheep for Pebble-C

  License: WTFPL
 */

#include <pebble.h>
#include "sheep.h"

#define FPS 10

#define TRUE 1
#define FALSE 0

static Window *window;

static AppTimer *progress_timer;

static unsigned int sleep_time = FPS*10;

static GBitmap *bg_image;
static BitmapLayer *bg_image_layer;

static GBitmap *fence_image_white;
static GBitmap *fence_image_black;
static BitmapLayer *fence_image_white_layer;
static BitmapLayer *fence_image_black_layer;

static GBitmap *sheep00_image_white;
static GBitmap *sheep00_image_black;
static GBitmap *sheep01_image_white;
static GBitmap *sheep01_image_black;

static Layer *canvas_white_layer;
static Layer *canvas_black_layer;

static TextLayer *text_layer;

static void progress_timer_callback(void *data);

static int sheep_count = 0;
static char sheep_count_buffer[256];
static char *nofsheep = " sheep";

static int some_sheep_is_running=FALSE;

#define DEFAULT_WIDTH 144
#define DEFAULT_HEIGHT 144

#define X_MOVING_DIST 5
#define Y_MOVING_DIST 3

#define TOP_ON_JUMP 4

#define GROUND_HEIGHT_RATIO 0.9
static int ground_height = 70;

static int gate_is_widely_open = FALSE;

#define MAX_SHEEP_NUMBER 100

enum Sheep_Attr {
  IS_RUNNING,
  X,
  Y,
  PROGRESS_ON_JUMP,
  X_ON_JUMP,
  STRETCH_LEG
};

int sheep_flock[MAX_SHEEP_NUMBER][6];

/* simple base 10 only itoa */
char *
mknofsheep (int value, char *unit, char *result)
{
  char const digit[] = "0123456789";
  char *p = result;
  if (value < 0) {
    *p++ = '-';
    value *= -1;
  }

  /* move number of required chars and null terminate */
  int shift = value;
  do {
    ++p;
    shift /= 10;
  } while (shift);
  *p = '\0';

  /* populate result in reverse order */
  do {
    *--p = digit [value % 10];
    value /= 10;
  } while (value);

  strcat(result, unit);  

  return result;
}

static void canvas_white_update_proc(Layer *this_layer, GContext *ctx) {
  
  graphics_context_set_compositing_mode(ctx, GCompOpOr);

  // Draw the sheep
  for (int asheep=0;asheep<MAX_SHEEP_NUMBER;asheep++){
    if(sheep_flock[asheep][IS_RUNNING]==TRUE){
      if(sheep_flock[asheep][PROGRESS_ON_JUMP]>0){
        graphics_draw_bitmap_in_rect(ctx, sheep01_image_white, GRect(sheep_flock[asheep][X],sheep_flock[asheep][Y],17,12));
      } else {
	if(sheep_flock[asheep][STRETCH_LEG]==TRUE){
          graphics_draw_bitmap_in_rect(ctx, sheep01_image_white, GRect(sheep_flock[asheep][X], sheep_flock[asheep][Y],17,12));
	} else {
          graphics_draw_bitmap_in_rect(ctx, sheep00_image_white, GRect(sheep_flock[asheep][X], sheep_flock[asheep][Y],17,12));
	}
      }
    }
  }
}

static void canvas_black_update_proc(Layer *this_layer, GContext *ctx) {

  graphics_context_set_compositing_mode(ctx, GCompOpClear);

  // Draw the sheep
  for (int asheep=0;asheep<MAX_SHEEP_NUMBER;asheep++){
    if(sheep_flock[asheep][IS_RUNNING]==TRUE){
      if(sheep_flock[asheep][PROGRESS_ON_JUMP]>0){
        graphics_draw_bitmap_in_rect(ctx, sheep01_image_black,
                                     GRect(sheep_flock[asheep][X],sheep_flock[asheep][Y],17,12));

      } else {
        if(sheep_flock[asheep][STRETCH_LEG]==TRUE){
          graphics_draw_bitmap_in_rect(ctx, sheep01_image_black,
                                       GRect(sheep_flock[asheep][X],
                                             sheep_flock[asheep][Y],17,12));

        } else {
          graphics_draw_bitmap_in_rect(ctx, sheep00_image_black,
                                       GRect(sheep_flock[asheep][X],
                                             sheep_flock[asheep][Y],17,12));
        }
	sheep_flock[asheep][STRETCH_LEG]=(sheep_flock[asheep][STRETCH_LEG]+1)%2;

      }
    }
  }


}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  // We do this to account for the offset due to the status bar
  // at the top of the app window.
  GRect image_frame = layer_get_frame(window_layer);
  image_frame.origin.x = 0;
  image_frame.origin.y = 0;

  bg_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG);

  fence_image_white = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FENCE_WHITE);
  fence_image_black = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FENCE_BLACK);

  sheep00_image_white = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SHEEP00_WHITE);
  sheep00_image_black = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SHEEP00_BLACK);

  sheep01_image_white = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SHEEP01_WHITE);
  sheep01_image_black = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SHEEP01_BLACK);

  // Use GCompOpOr to display the white portions of the fence image
  bg_image_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_alignment(bg_image_layer, GAlignBottom);
  bitmap_layer_set_bitmap(bg_image_layer, bg_image);
  bitmap_layer_set_compositing_mode(bg_image_layer, GCompOpAssign);
  layer_add_child(window_layer, bitmap_layer_get_layer(bg_image_layer));

  // Use GCompOpOr to display the white portions of the fence image
  fence_image_white_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_alignment(fence_image_white_layer, GAlignBottom);
  bitmap_layer_set_bitmap(fence_image_white_layer, fence_image_white);
  bitmap_layer_set_compositing_mode(fence_image_white_layer, GCompOpOr);
  layer_add_child(window_layer, bitmap_layer_get_layer(fence_image_white_layer));

  // Use GCompOpClear to display the black portions of the fence image
  fence_image_black_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_alignment(fence_image_black_layer, GAlignBottom);
  bitmap_layer_set_bitmap(fence_image_black_layer, fence_image_black);
  bitmap_layer_set_compositing_mode(fence_image_black_layer, GCompOpClear);
  layer_add_child(window_layer, bitmap_layer_get_layer(fence_image_black_layer));

  // Create Layer
  canvas_white_layer = layer_create(GRect(0, 0, image_frame.size.w, image_frame.size.h));
  canvas_black_layer = layer_create(GRect(0, 0, image_frame.size.w, image_frame.size.h));

  layer_add_child(window_layer, canvas_white_layer);
  layer_add_child(window_layer, canvas_black_layer);

  // Set the update_proc
  layer_set_update_proc(canvas_white_layer, canvas_white_update_proc);
  layer_set_update_proc(canvas_black_layer, canvas_black_update_proc);


  text_layer = text_layer_create(GRect(0,0, 144, 15));
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static int calc_jump_x(int y){
  int fw = fence_image_white->bounds.size.w;
  int fh = fence_image_white->bounds.size.h;
  return -1 * (y - DEFAULT_HEIGHT) * fw / fh + (DEFAULT_WIDTH - fw) / 2;
}

static void send_out_sheep(int asheep){
  sheep_flock[asheep][IS_RUNNING] = TRUE;
  sheep_flock[asheep][X] = DEFAULT_WIDTH + 17;
  sheep_flock[asheep][Y] = DEFAULT_HEIGHT-5 - (rand()%(ground_height-5));
  sheep_flock[asheep][X_ON_JUMP] = calc_jump_x(sheep_flock[asheep][Y]);
}

static void clear_sheep(int asheep){
  sheep_flock[asheep][IS_RUNNING] = FALSE;
  sheep_flock[asheep][X] = 0;
  sheep_flock[asheep][Y] = 0;
  sheep_flock[asheep][PROGRESS_ON_JUMP] = 0;
  sheep_flock[asheep][X_ON_JUMP] = 0;
  sheep_flock[asheep][STRETCH_LEG] = FALSE;
}

static void update() {

  // Send out a sheep
  if (gate_is_widely_open==TRUE) {
    for (int i=0; i<MAX_SHEEP_NUMBER; i++){
      if(sheep_flock[i][IS_RUNNING] == 0){
        send_out_sheep(i);
        break;
      }
    }
  }

  // Update status for each sheep
  for(int asheep=0;asheep<MAX_SHEEP_NUMBER;asheep++){
    if (sheep_flock[asheep][IS_RUNNING]==FALSE){
        continue;
    }

    // Run
    sheep_flock[asheep][X] -= X_MOVING_DIST;

    // start to jump when sheep reach to jump point
    if (sheep_flock[asheep][PROGRESS_ON_JUMP]==0 && sheep_flock[asheep][X_ON_JUMP]<=sheep_flock[asheep][X] && sheep_flock[asheep][X] < sheep_flock[asheep][X_ON_JUMP] + X_MOVING_DIST){
        sheep_flock[asheep][PROGRESS_ON_JUMP] += 1;
    }

    // behavior during jump
    if (sheep_flock[asheep][PROGRESS_ON_JUMP] > 0 ){
      if (sheep_flock[asheep][PROGRESS_ON_JUMP] <= TOP_ON_JUMP) {
        sheep_flock[asheep][Y] -= Y_MOVING_DIST;
        sheep_flock[asheep][PROGRESS_ON_JUMP] += 1;
      } else if (sheep_flock[asheep][PROGRESS_ON_JUMP] <= TOP_ON_JUMP * 2){
        sheep_flock[asheep][Y] += Y_MOVING_DIST;
        sheep_flock[asheep][PROGRESS_ON_JUMP] += 1;
      } else {
        sheep_flock[asheep][PROGRESS_ON_JUMP] = 0;
      }
    }

    // count up
    if (sheep_flock[asheep][PROGRESS_ON_JUMP] == TOP_ON_JUMP){
      sheep_count += 1;
    }

    // go away and send out a sheep if there is no sheep on run
    if (sheep_flock[asheep][X] < -1 * 17){
      some_sheep_is_running = FALSE;
      clear_sheep(asheep);
      for (int asheep=0;asheep<MAX_SHEEP_NUMBER;asheep++) {
        if (sheep_flock[asheep][IS_RUNNING]==TRUE) {
          some_sheep_is_running = TRUE;
          break;
        }
      }
      if (some_sheep_is_running == FALSE) {
        send_out_sheep(asheep);
      }
    }
  }

  mknofsheep(sheep_count, nofsheep, sheep_count_buffer);
}


static void draw() {
  text_layer_set_text(text_layer, sheep_count_buffer);
}

static void progress_timer_callback(void *data) {
  progress_timer = app_timer_register(sleep_time /* milliseconds */, progress_timer_callback, NULL);
  update();
  draw();
}

static void window_unload(Window *window) {
  bitmap_layer_destroy(bg_image_layer);
  bitmap_layer_destroy(fence_image_white_layer);
  bitmap_layer_destroy(fence_image_black_layer);

  layer_destroy(canvas_white_layer);
  layer_destroy(canvas_black_layer);
  text_layer_destroy(text_layer);

  gbitmap_destroy(bg_image);
  gbitmap_destroy(fence_image_white);
  gbitmap_destroy(fence_image_black);
  gbitmap_destroy(sheep00_image_white);
  gbitmap_destroy(sheep00_image_black);
  gbitmap_destroy(sheep01_image_white);
  gbitmap_destroy(sheep01_image_black);

}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  gate_is_widely_open=TRUE;
}
void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  gate_is_widely_open=FALSE;
}

void config_provider(Window *window) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
}
  
static void init(void) {
  for (int asheep=0; asheep<MAX_SHEEP_NUMBER; asheep++){
    clear_sheep(asheep);
  }

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(window, false /* Animated */);

  window_set_click_config_provider(window, (ClickConfigProvider) config_provider);

  progress_timer = app_timer_register(sleep_time /* milliseconds */, progress_timer_callback, NULL);

  send_out_sheep(0);
}

static void deinit(void) {
  window_destroy(window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

