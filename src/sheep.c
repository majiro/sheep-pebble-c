/*


 */

#include <pebble.h>
#include "sheep.h"

#define FPS 10

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
static BitmapLayer *sheep_image_white_layer;
static BitmapLayer *sheep_image_black_layer;

static TextLayer *text_layer; // Used as a background to help demonstrate transparency.

static void progress_timer_callback(void *data);

static int counter = 0;
static char counter_buffer[256];
static char *nofsheep = " sheep";

static int DEFAULT_WIDTH = 144;
static int DEFAULT_HEIGHT = 144;

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

int max_sheep_in_flock = sizeof(sheep_flock) / sizeof(sheep_flock[0][0]);

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

  // Use GCompOpOr to display the white portions of the sheep image
  sheep_image_white_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_alignment(sheep_image_white_layer, GAlignTopLeft);
  bitmap_layer_set_bitmap(sheep_image_white_layer, sheep00_image_white);
  bitmap_layer_set_compositing_mode(sheep_image_white_layer, GCompOpOr);
  layer_add_child(window_layer, bitmap_layer_get_layer(sheep_image_white_layer));

  // Use GCompOpClear to display the black portions of the sheep image
  sheep_image_black_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_alignment(sheep_image_black_layer, GAlignTopLeft);
  bitmap_layer_set_bitmap(sheep_image_black_layer, sheep00_image_black);
  bitmap_layer_set_compositing_mode(sheep_image_black_layer, GCompOpClear);
  layer_add_child(window_layer, bitmap_layer_get_layer(sheep_image_black_layer));

  text_layer = text_layer_create(GRect(0,0, 144, 15));
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static int calc_jump_x(int y){
  fw = fence_image.width();
  fh = fence_image.height();
  return -1 * (y - DEFAULT_HEIGHT) * fw / fh + (DEFAULT_WIDTH - fw) / 2;
}


static void send_out_sheep(int asheep){
  sheep_flock[asheep][IS_RUNNING] = True;
  sheep_flock[asheep][X] = DEFAULT_WIDTH + 17;
  sheep_flock[asheep][Y] = DEFAULT_HEIGHT - ground_height + ( rand() % (ground_height - 13);
  sheep_flock[asheep][x_on_jump] = calc_jump_x(sheep_flock[asheep][Y]);
}

static void update() {
  // Send out a sheep
  if (gate_is_widely_open) {
    for (int i=0; i<max_sheep_in_flock; i++){
      if(sheep_flock[i][IS_RUNNING] == 0){
        send_out_sheep(i);
        break;
      }
    }
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
  bitmap_layer_destroy(sheep_image_white_layer);
  bitmap_layer_destroy(sheep_image_black_layer);

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
