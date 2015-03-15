/*


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

static GBitmap *sheep00_image;

static GBitmap *sheep00_image_white;
static GBitmap *sheep00_image_black;
static GBitmap *sheep01_image_white;
static GBitmap *sheep01_image_black;
static BitmapLayer *sheep_image_white_layer;
static BitmapLayer *sheep_image_black_layer;

static Layer *canvas_white_layer;
static Layer *canvas_black_layer;

static TextLayer *text_layer; // Used as a background to help demonstrate transparency.

static void progress_timer_callback(void *data);

static int counter = 0;
static char counter_buffer[256];
static char *nofsheep = " sheep";

#define DEFAULT_WIDTH 144
#define DEFAULT_HEIGHT 144

#define X_MOVING_DIST 5
#define Y_MOVING_DIST 3

#define TOP_ON_JUMP 5

#define GROUND_HEIGHT_RATIO 0.9
static int ground_height = 108;

static int gate_is_widely_open = 0;

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
  GRect bounds = layer_get_bounds(this_layer);

  // Get the center of the screen (non full-screen)
//  GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));
/*
  // Draw the 'loop' of the 'P'
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 40);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 35);

  // Draw the 'stalk'
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(32, 40, 5, 100), 0, GCornerNone);
*/
  graphics_context_set_compositing_mode(ctx, GCompOpOr);

  graphics_draw_bitmap_in_rect(ctx, sheep00_image_white, GRect(32,40,17,13));

//  graphics_draw_bitmap_in_rect(ctx, sheep01_image_white, GRect(32+10,40+10,17,13));
  graphics_draw_bitmap_in_rect(ctx, sheep01_image_black, GRect(32+10,40+10,17,13));

  graphics_draw_bitmap_in_rect(ctx, sheep00_image, GRect(32+20,40+20,17,12));
}

static void canvas_black_update_proc(Layer *this_layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpClear);
  graphics_draw_bitmap_in_rect(ctx, sheep00_image_black, GRect(32,40,17,13));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
//  GRect bounds = layer_get_bounds(window_layer);

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

  sheep00_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SHEEP00A);

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
  sheep_flock[asheep][Y] = DEFAULT_HEIGHT - ground_height + ( rand() % (ground_height - 13));
  sheep_flock[asheep][X_ON_JUMP] = calc_jump_x(sheep_flock[asheep][Y]);
}

static void update() {
  // Send out a sheep
  if (gate_is_widely_open) {
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
  }

  counter++;
  mknofsheep(counter++, nofsheep, counter_buffer);
}


static void draw() {
  text_layer_set_text(text_layer, counter_buffer);
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
//  layer_destroy(text_layer);

  gbitmap_destroy(bg_image);
  gbitmap_destroy(fence_image_white);
  gbitmap_destroy(fence_image_black);
  gbitmap_destroy(sheep00_image_white);
  gbitmap_destroy(sheep00_image_black);
  gbitmap_destroy(sheep01_image_white);
  gbitmap_destroy(sheep01_image_black);

}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(window, false /* Animated */);

  progress_timer = app_timer_register(sleep_time /* milliseconds */, progress_timer_callback, NULL);
}

static void deinit(void) {
  window_destroy(window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
