/*
    License goes here.
    
    You are free to do whatever you want with this code.
*/


#ifndef RES_PATH
    #define RES_PATH
#endif

#ifndef STAT_PATH
    #define STAT_PATH
#endif


// window size, not resizable in runtime
#define WIDTH 1600
#define HEIGHT 900


#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <SDL/SDL.h>
#include <cairo/cairo.h>


int W = WIDTH, H = HEIGHT;
// target size depends on window size
int D = 50 * (WIDTH/800.0);
//~ int D = 300;


SDL_Surface *screen, 
            *img, 
            *cursor, 
            *shot, 
            *shot_blood, 
            *rabbit, 
            *rabbit_values;

cairo_t *img_ctx;

SDL_Rect target;
SDL_Rect rect_cur;

double average = 0, count_10_value = 0;
int count = 0, count_10_first_shot = 0;
bool headshot, no_rabbit = false, write_stat = true;

typedef struct {
    Uint8 *data;
    int length;
} Wave;

Wave *wav;
int wav_pos;


void draw_rabbit(cairo_t *ctx, double size);

double rnd(double max) {
    return  max * rand() / RAND_MAX;
}

double get_time_ms() {
    /*
        'Uint32 SDL_GetTicks(void);' can be used for portability.
        It returns milliseconds since SDL initialization.
    */
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double t = ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
    return t;
}


void draw_text(char *text, double font_size, double dy, double r, double g, double b) {
    cairo_set_font_size(img_ctx, font_size);
    cairo_set_source_rgb(img_ctx, r, g, b);
    
    cairo_text_extents_t te;
    cairo_text_extents(img_ctx, text, &te);
    
    cairo_move_to(img_ctx, W/2 - te.width/2 - te.x_bearing, H/2 - te.height/2 - te.y_bearing + dy);
    cairo_show_text(img_ctx, text);
    
    SDL_Rect rect = {W/2 - te.width/2 - 1, H/2 - te.height/2 + dy - 1, te.width + 3, te.height + 3};
    SDL_BlitSurface(img, &rect, screen, &rect);
}

void draw_value(double r, double t) {
    // calculate points
    double Vr = 1 - 2 * r / D;
    double Vt = (t - 500.0) / 1000;
    double V = (1200 + 200*headshot) * Vr * pow(0.1, 2*Vt);
    count_10_value += V;
    
    char str[32];
    
    // draw points
    sprintf(str, "%i", (int)V);
    draw_text(str, W/5.33, 0, 1, 0.55, 0);
    
    // draw milliseconds
    if (headshot) {
        sprintf(str, "HEADSHOT!!! %i ms", (int)t);
    } else {
        sprintf(str, "%i ms", (int)t);
    }
    draw_text(str, W/40.0, W/11.0, 0, 0.55, 1);

    if (t < 400) {
        draw_text("Amazing reaction!", W/80.0, 1.25*W/11.0, 1, 1, 1);
    } else if (t > 1000) {
        draw_text("Slowpoke", W/80.0, 1.25*W/11.0, 1, 1, 1);
    }
    
    // milliseconds are also printed to stdout
    printf("%i, ", (int)t);
    fflush(stdout);
}

void draw_sum() {
    char str[32];
    
    // sum
    sprintf(str, "Sum of 10: %i", (int)count_10_value);
    draw_text(str, W/10.0, 0, 0, 0.55, 1);
    
    // accuracy
    sprintf(str, "First shot accuracy: %i/10", count_10_first_shot);
    draw_text(str, W/40.0, W/15.0, 1, 1, 1);

    
    // append sum of points to a file
    if (write_stat) {
        sprintf(str, "%f\n", count_10_value);
        FILE *f = fopen(STAT_PATH ".target_stat", "a");
        fputs(str, f);
        fclose(f);
    }

    count_10_value = 0;
    count_10_first_shot = 0;
}

// thin blue stripe on the bottom of the window
void draw_percentage() {
    int cnt = (count % 10) + 1;
    cairo_set_source_rgb(img_ctx, 0, 0.5, 1);
    cairo_rectangle(img_ctx, 0, H-1, W/10.0 * cnt, 1);
    cairo_fill(img_ctx);
}


void draw_cursor(cairo_t *cursor_ctx) {    // 19x19 px
    // put shape
    cairo_move_to(cursor_ctx, 0, 9.5);
    cairo_line_to(cursor_ctx, 7, 9.5);
    cairo_move_to(cursor_ctx, 12, 9.5);
    cairo_line_to(cursor_ctx, 19, 9.5);
    
    cairo_move_to(cursor_ctx, 9.5, 0);
    cairo_line_to(cursor_ctx, 9.5, 7);
    cairo_move_to(cursor_ctx, 9.5, 12);
    cairo_line_to(cursor_ctx, 9.5, 19);

    cairo_new_sub_path(cursor_ctx);
    cairo_arc(cursor_ctx, 9.5, 9.5, 7, 0, 2*M_PI);
    
    // draw black bg lines
    cairo_set_source_rgb(cursor_ctx, 0, 0, 0);
    cairo_set_line_width(cursor_ctx, 2);
    cairo_stroke_preserve(cursor_ctx);
    
    // draw white fg lines
    cairo_set_source_rgb(cursor_ctx, 1, 1, 1);
    cairo_set_line_width(cursor_ctx, 1);
    cairo_stroke(cursor_ctx);
    
    // dot
    cairo_set_source_rgb(cursor_ctx, 1, 1, 1);
    cairo_rectangle(cursor_ctx, 9, 9, 1, 1);
    cairo_fill(cursor_ctx);
}


void draw_target() {
    int cx = D/2 + rnd(W-D);
    int cy = D/2 + rnd(H-D);
    
    target = (SDL_Rect){cx-D/2, cy-D/2, D, D};

    if (no_rabbit) {
        // draw round target with concentric stripes
        // bg
        cairo_set_source_rgb(img_ctx, 1, 0.3, 0);
        cairo_arc(img_ctx, cx, cy, D/2, 0, 2*M_PI);
        cairo_fill(img_ctx);
        
        // stripes
        cairo_set_source_rgb(img_ctx, 1, 0.8, 0);
        cairo_set_line_width(img_ctx, D/7.0);
        cairo_arc(img_ctx, cx, cy, D/2.0/7.0*4, 0, 2*M_PI);
        cairo_stroke(img_ctx);
        cairo_arc(img_ctx, cx, cy, D/2.0/7.0, 0, 2*M_PI);
        cairo_fill(img_ctx);
    } else {
        // blit rabbit
        SDL_BlitSurface(rabbit, NULL, img, &target);
    }
}


double check_round_target(int cx, int cy) { 
    double r = hypot(cx - (target.x + D/2.0) ,
                      cy - (target.y + D/2.0) );
    
    if (r <= D/2.0) {
        return r;
    } else {
        return -1;
    }
}

double check_rabbit(int cx, int cy) {
    int dx = (cx - target.x) * 520.0 / D;    // 'rabbit_values.png' is ~520 px in both sizes,
    int dy = (cy - target.y) * 520.0 / D;    // also draw_rabbit() in 'rabbit.c' uses 520 for scaling the rabbit
    
    headshot = false;
    
    if (dx < 0 || dy < 0 || dx >= rabbit_values->w || dy >= rabbit_values->h) {
        return -1;
    }
    
    unsigned char *p = (unsigned char *)((int*)rabbit_values->pixels + dy*rabbit_values->w + dx);
    int bv = p[2];
    int hs = p[1];  // headshot if pixel has only red component
    
    if (bv == 0) {
        return -1;
    }
    
    double r = (255-bv) / 255.0 * D/2;
    
    if (hs == 0) {
        headshot = true;
    }
    
    return r;
}

double check_target(int cx, int cy) {
    if (no_rabbit) {
        return check_round_target(cx, cy);
    } else {
        return check_rabbit(cx, cy);
    }
}

void audio_callback(void *userdata, Uint8 *buffer, int buffer_len) {
    int wav_length = wav->length;
    Uint8 *wav_data = wav->data;
    
    if (buffer_len > wav_length - wav_pos) {
        buffer_len = wav_length - wav_pos;
    }
    
    memcpy(buffer, wav_data + wav_pos, buffer_len);
    wav_pos += buffer_len;
    
    if (wav_pos == wav_length) {
        SDL_PauseAudio(1);
    }
}

void play(Wave *w) {
    wav = w;
    wav_pos = 0;
    SDL_PauseAudio(0);
}


unsigned int timer_callback(unsigned int interval) {
    SDL_Event e;
    e.type = SDL_USEREVENT;
    SDL_PushEvent(&e);
}


SDL_Surface * surface_from_image(char *png, int ox, int oy) {
    // Am I doing it wrong? How do you create SDL surface from PNG using cairo?
    
    cairo_surface_t *png_cs = cairo_image_surface_create_from_png(png);
    int w = cairo_image_surface_get_width(png_cs) + ox + 1;
    int h = cairo_image_surface_get_height(png_cs) + oy + 1;
    
    SDL_Surface *surf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    cairo_surface_t *cs = cairo_image_surface_create_for_data(surf->pixels, CAIRO_FORMAT_ARGB32, w, h, surf->pitch);
    
    cairo_t *ctx = cairo_create(cs);
    cairo_set_source_surface(ctx, png_cs, ox, oy);
    cairo_paint(ctx);
    
    cairo_destroy(ctx);
    cairo_surface_destroy(cs);
    cairo_surface_destroy(png_cs);
    
    return surf;
}


int main (int argc, char * argv[]) {
    
    no_rabbit = argc > 1;
    
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
    SDL_ShowCursor(0);

    screen = SDL_SetVideoMode(W, H, 32, SDL_ASYNCBLIT | SDL_HWSURFACE);
    
    // 'img' is the main backbuffer surface
    img = SDL_CreateRGBSurface(SDL_SWSURFACE, W, H, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000);
    cairo_surface_t *img_s;
    img_s = cairo_image_surface_create_for_data(img->pixels, CAIRO_FORMAT_ARGB32, W, H, img->pitch);
    img_ctx = cairo_create(img_s);
    
    cursor = SDL_CreateRGBSurface(SDL_SWSURFACE, 19, 19, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    {
        cairo_surface_t *cursor_s;
        cursor_s = cairo_image_surface_create_for_data(cursor->pixels, CAIRO_FORMAT_ARGB32, 19, 19, cursor->pitch);
        cairo_t *cursor_ctx = cairo_create(cursor_s);
       draw_cursor(cursor_ctx);
        cairo_destroy(cursor_ctx);
        cairo_surface_destroy(cursor_s);
    }

    // shot hole image, 'shot_blood' is the same image but only red channel is used
    shot = surface_from_image(RES_PATH "res/shot.png", 2, 2);
    if (no_rabbit) {
        shot_blood = shot;
    } else {
        shot_blood = SDL_CreateRGBSurfaceFrom(shot->pixels, shot->w, shot->h, 32, shot->pitch, 0x00ff0000, 0, 0, 0xff000000);
    }
    
    rabbit = SDL_CreateRGBSurface(SDL_SWSURFACE, D, D, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    {
        cairo_surface_t *rabbit_s;
        rabbit_s = cairo_image_surface_create_for_data(rabbit->pixels, CAIRO_FORMAT_ARGB32, D, D, rabbit->pitch);
        cairo_t *rabbit_ctx = cairo_create(rabbit_s);
       draw_rabbit(rabbit_ctx, D);
        cairo_destroy(rabbit_ctx);
        cairo_surface_destroy(rabbit_s);
    }

    // image describing rabbit vulnerability values, if a pixel has only red channel it is treated as rabbit head pixel
    rabbit_values = surface_from_image(RES_PATH "res/rabbit_values.png", 0, 0);

    // REMEMBER? I PROMISED.
    cairo_select_font_face(img_ctx, "Troika", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(img_ctx, W/5.33);
    draw_text("Target", W/5.33, 0, 0, 0.55, 1);
    
    // waves
    Wave sniper, blood, rabbit, reload;
    SDL_AudioSpec spec;
    SDL_LoadWAV(RES_PATH "res/sniper.wav", &spec, &sniper.data, &sniper.length);
    if (no_rabbit) {
        SDL_LoadWAV(RES_PATH "res/hit.wav", &spec, &blood.data, &blood.length);
        SDL_LoadWAV(RES_PATH "res/pop.wav", &spec, &rabbit.data, &rabbit.length);
    } else {
        SDL_LoadWAV(RES_PATH "res/blood.wav", &spec, &blood.data, &blood.length);
        SDL_LoadWAV(RES_PATH "res/rabbit.wav", &spec, &rabbit.data, &rabbit.length);
    }
    SDL_LoadWAV(RES_PATH "res/reload.wav", &spec, &reload.data, &reload.length);
    spec.callback = audio_callback;
    spec.samples = 512;
    SDL_OpenAudio(&spec, &spec);
    
    
    srand(time(NULL));    

    bool wait = true, 
          wipe_cursor = false,
          count_10 = true, 
          first_shot = true,
          strange_var_name = false;
    
    double total_time;

    SDL_Event event;
    {
        int x, y;
        SDL_GetMouseState(&x, &y);
        rect_cur = (SDL_Rect){x-9, y-9, 19, 19};
    }

    
    while (true) {
        
        SDL_WaitEvent(&event);
        do {
            switch (event.type) {
                
                case SDL_QUIT:
                    goto exit;

                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            goto exit;
                        case SDLK_w:
                            strange_var_name = !strange_var_name;
                            write_stat = false;
                            break;
                    }
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (wipe_cursor) {
                        SDL_BlitSurface(img, &rect_cur, screen, &rect_cur);
                        wipe_cursor = false;
                    }
                    rect_cur.x = event.motion.x - 9;
                    rect_cur.y = event.motion.y - 9;
                    break;
                
                case SDL_MOUSEBUTTONDOWN:
                {
                    if (strange_var_name) {
                        SDL_BlitSurface(img, &rect_cur, screen, &rect_cur);
                        rect_cur.x = target.x + D/2 + D/4 - 9;
                        rect_cur.y = target.y + D/2 - D/12 - 9;
                        event.button.x = rect_cur.x + 9;
                        event.button.y = rect_cur.y + 9;
                    }
                    
                    double r = check_target(event.button.x, event.button.y);
                
                    if (wait && count_10 && count_10_value == 0) {
                        play(&reload);
                        count_10 = false;
                        SDL_SetTimer(1000, timer_callback);
                    } else {
                        if (r >= 0) {                                               // bingo!
                            play(&blood);
                            if (first_shot) {
                                count_10_first_shot += 1;
                            }
                            SDL_BlitSurface(shot_blood, NULL, img, &rect_cur);
                        } else {
                            play(&sniper);
                            SDL_BlitSurface(shot, NULL, img, &rect_cur);
                        }
                        first_shot = false;
                        SDL_BlitSurface(img, &rect_cur, screen, &rect_cur);
                    }
                
                    if (!wait) {
                        wait = (r >= 0);
                        if (wait) {                                                 // bingo!
                            total_time = get_time_ms() - total_time;
                            average += total_time;
                            count += 1;
                            if (count % 10 == 0)  {
                                count_10 = true;
                                SDL_SetTimer(2000, timer_callback);             // push SDL_USEREVENT in 2 seconds
                            } else {
                                SDL_SetTimer(500 + rnd(6000), timer_callback);  // push SDL_USEREVENT
                            }
                            draw_value(r, total_time);
                        }
                    }
                    break;
                }
                
                case SDL_USEREVENT:
                    // wipe
                    cairo_set_source_rgb(img_ctx, 0, 0, 0);
                    cairo_paint(img_ctx);
                    if (!count_10) draw_percentage();
                    SDL_BlitSurface(img, NULL, screen, NULL);
                
                    // draw
                    if (count_10) {
                        draw_sum();
                    } else {
                        play(&rabbit);
                        
                        draw_target();
                        SDL_BlitSurface(img, &target, screen, &target);
                        
                        wait = false;
                        total_time = get_time_ms();
                        first_shot = true;
                    }
                    break;
            }
        } while (SDL_PollEvent(&event));
        
        SDL_BlitSurface(cursor, NULL, screen, &rect_cur);
        wipe_cursor = true;
        
        SDL_Flip(screen);
        
    }

    
exit:
    
    printf("\nAverage: %f milliseconds\n", average/count);
    
    SDL_FreeWAV(reload.data);
    SDL_FreeWAV(rabbit.data);
    SDL_FreeWAV(blood.data);
    SDL_FreeWAV(sniper.data);
    
    cairo_destroy(img_ctx);
    cairo_surface_destroy(img_s);
    
    if (!no_rabbit) {
        SDL_FreeSurface(shot_blood);
    }
    SDL_FreeSurface(shot);
    SDL_FreeSurface(cursor);
    SDL_FreeSurface(img);
    SDL_FreeSurface(screen);

    SDL_Quit();

    return 0;
}
