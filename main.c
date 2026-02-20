#define SDL_MAIN_USE_CALLBACKS 1
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
#define WINDOW_WIDTH 1000

// test texture
static SDL_Texture *texture = NULL;
static int texture_width = 0;
static int texture_height = 0;
static float texture_x = 0;
static float texture_y = 0;
static float texture_angle = 0;

// timing
static Uint64 last_step = 0;
#define STEP_RATE_MS 40

// game constants
#define MOVE_STEP 5.0f
#define ANGLE_STEP 2.0f

enum BUTTON_STATE_MAPPING
{
    W,
    A,
    S,
    D,
    Q,
    E,
    NUM_BUTTONS,
};

bool buttonState[] = {false, false, false, false, false, false};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Could not initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if(!SDL_CreateWindowAndRenderer("Lobster Boxing", WINDOW_WIDTH, WINDOW_WIDTH, SDL_WINDOW_MAXIMIZED, &window, &renderer))
    {
        SDL_Log("Could not initialize window and/or renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_WIDTH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // loading resources
    char *img_path = NULL;
    SDL_Surface *surface = NULL;
    SDL_asprintf(&img_path, "%sresources/img.png", SDL_GetBasePath());
    surface = SDL_LoadPNG(img_path);
    if(!surface)
    {
        SDL_Log("Could not load image: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_free(img_path);

    texture_width = surface->w;
    texture_height = surface->h;
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(!texture)
    {
        SDL_Log("Could not create texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_DestroySurface(surface);
    
    texture_x = ((float)(WINDOW_WIDTH - texture_width)) / 2.0f;
    texture_y = ((float)(WINDOW_WIDTH - texture_height)) / 2.0f;

    last_step = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

void update_button_state(SDL_Scancode key_code)
{
    switch(key_code)
    {
        case SDL_SCANCODE_W:
            buttonState[W] = !buttonState[W];
            break;
        case SDL_SCANCODE_A:
            buttonState[A] = !buttonState[A];
            break;
        case SDL_SCANCODE_S:
            buttonState[S] = !buttonState[S];
            break;
        case SDL_SCANCODE_D:
            buttonState[D] = !buttonState[D];
            break;
        case SDL_SCANCODE_Q:
            buttonState[Q] = !buttonState[Q];
            break;
        case SDL_SCANCODE_E:
            buttonState[E] = !buttonState[E];
            break;
        default:
            break;
    }
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch(event->type)
    {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
            
        case SDL_EVENT_KEY_DOWN:
            if(event->key.scancode == SDL_SCANCODE_ESCAPE)
            {
                return SDL_APP_SUCCESS;
            }
            else if(!event->key.repeat)
            {
                update_button_state(event->key.scancode);
            }
            break;

        case SDL_EVENT_KEY_UP:
            update_button_state(event->key.scancode);
            break;

        default:
            break;        
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    const Uint64 now = SDL_GetTicks();
    
    if(now - last_step >= STEP_RATE_MS)
    {
        // static float rotate 
        if(buttonState[W])
            texture_y -= MOVE_STEP;
            
        if(buttonState[A])
            texture_x -= MOVE_STEP;
            
        if(buttonState[S])
            texture_y += MOVE_STEP;
            
        if(buttonState[D])
            texture_x += MOVE_STEP;

        if(buttonState[E])
            texture_angle += ANGLE_STEP;

        if(buttonState[Q])
            texture_angle -= ANGLE_STEP;

        last_step += STEP_RATE_MS;
    }
        
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_FRect destination;
    destination.x = texture_x;
    destination.y = texture_y;
    destination.w = (float)texture_width;
    destination.h = (float)texture_height;

    SDL_FPoint center;
    center.x = texture_width / 2.0f;
    center.y = texture_height / 2.0f;

    // SDL_RenderTexture(renderer, texture, NULL, &destination);
    SDL_RenderTextureRotated(renderer, texture, NULL, &destination, texture_angle, &center, SDL_FLIP_NONE);

    SDL_RenderPresent(renderer);


    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_DestroyTexture(texture);
}