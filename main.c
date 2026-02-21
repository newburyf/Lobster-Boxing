#define SDL_MAIN_USE_CALLBACKS 1
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// SDL stuff
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
#define WINDOW_WIDTH 800

// timing
#define FRAME_RATE_IN_MS 40
#define LEG_MOVE_RATE_IN_FRAMES 5
static Uint64 lastFrame = 0;
static int frameCount = 0;

// game constants
#define MOVE_STEP 5.0f
#define ANGLE_STEP 2.0f

// lobsters
#define BTW_PUNCHES_TIME_IN_FRAMES 8
#define WINDUP_TIME_IN_FRAMES 6
#define PUNCH_TIME_IN_FRAMES 6

typedef enum PUNCH_STATE
{
    NEUTRAL,
    WINDUP,
    PUNCH,
} PunchState;

enum TEXTURE_INDICES
{
    NEUTRAL_IN,
    NEUTRAL_OUT,
    WINDUP_IN,
    WINDUP_OUT,
    PUNCH_IN,
    PUNCH_OUT,
    NUM_TEXTURES,
};

static char* texture_png_names[NUM_TEXTURES] =
{
    "neutral_in",
    "neutral_out",
    "windup_in",
    "windup_out",
    "punch_in",
    "punch_out",
};

// enum BUTTON_STATE_MAPPING
// {
//     W,
//     A,
//     S,
//     D,
//     Q,
//     E,
//     C,
//     NUM_BUTTONS,
// };

enum BUTTON_STATE_MAPPING
{
    UP,
    LEFT,
    DOWN,
    RIGHT,
    COUNTER_CLOCKWISE,
    CLOCKWISE,
    START_PUNCH,
    NUM_BUTTONS,
};


typedef struct LOBSTER
{
    float x;
    float y;
    float width;
    float height;
    float angle;
    SDL_FPoint center;
    PunchState punchState;
    int timeInPunchState;
    bool lastPunchLeft;
    bool legsOut;
    SDL_Texture *textures[NUM_TEXTURES];
    bool buttonState[NUM_BUTTONS];
} Lobster;

// red!
Lobster red;

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

    // Setting up red!

    // loading textures
    for(int i = 0; i < NUM_TEXTURES; i++)
    {
        char *img_path = NULL;
        SDL_Surface *surface = NULL;
        SDL_asprintf(&img_path, "%sresources/%s.png", SDL_GetBasePath(), texture_png_names[i]);
        surface = SDL_LoadPNG(img_path);
        SDL_free(img_path);
        if(!surface)
        {
            SDL_Log("Could not load image: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }
    
        red.textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if(!red.textures[i])
        {
            SDL_Log("Could not create texture: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }
    }

    // setting up other variables
    red.width = red.textures[0]->w;
    red.height = red.textures[0]->h;
    red.x = (float)((WINDOW_WIDTH - red.width) / 2.0f);
    red.y = (float)((WINDOW_WIDTH - red.height) / 2.0f);
    red.angle = 0;
    red.center.x = red.width / 2.0f;
    red.center.y = red.height / 2.0f;
    red.punchState = NEUTRAL;
    red.timeInPunchState = 0;
    red.lastPunchLeft = true;
    red.legsOut = true;
    for(int i = 0; i < NUM_BUTTONS; i++)
    {
        red.buttonState[i] = false;
    }

    // starting timer
    lastFrame = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

void updateButtonState(SDL_Scancode key_code)
{
    switch(key_code)
    {
        case SDL_SCANCODE_W:
            red.buttonState[UP] = !red.buttonState[UP];
            break;
        case SDL_SCANCODE_A:
            red.buttonState[LEFT] = !red.buttonState[LEFT];
            break;
        case SDL_SCANCODE_S:
            red.buttonState[DOWN] = !red.buttonState[DOWN];
            break;
        case SDL_SCANCODE_D:
            red.buttonState[RIGHT] = !red.buttonState[RIGHT];
            break;
        case SDL_SCANCODE_Q:
            red.buttonState[COUNTER_CLOCKWISE] = !red.buttonState[COUNTER_CLOCKWISE];
            break;
        case SDL_SCANCODE_E:
            red.buttonState[CLOCKWISE] = !red.buttonState[CLOCKWISE];
            break;
        case SDL_SCANCODE_C:
            red.buttonState[START_PUNCH] = !red.buttonState[START_PUNCH];
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
                updateButtonState(event->key.scancode);
            }
            break;

        case SDL_EVENT_KEY_UP:
            updateButtonState(event->key.scancode);
            break;

        default:
            break;        
    }

    return SDL_APP_CONTINUE;
}

void handleLobsterMovement(Lobster *lob)
{
    bool movement = false;
    if(lob->buttonState[UP] && lob->y - MOVE_STEP > 0)
    {
        lob->y -= MOVE_STEP;
        movement = true;
    }

    if(lob->buttonState[DOWN] && lob->y + lob->height + MOVE_STEP < WINDOW_WIDTH)
    {
        lob->y += MOVE_STEP;
        movement = true;
    }

    if(lob->buttonState[LEFT] && lob->x - MOVE_STEP > 0)
    {
        lob->x -= MOVE_STEP;
        movement = true;
    }
        
    if(lob->buttonState[RIGHT] && lob->x + lob->width < WINDOW_WIDTH)
    {
        lob->x += MOVE_STEP;
        movement = true;
    }

    if(lob->buttonState[CLOCKWISE])
    {
        lob->angle += ANGLE_STEP;
        movement = true;
    }

    if(lob->buttonState[COUNTER_CLOCKWISE])
    {
        lob->angle -= ANGLE_STEP;
        movement = true;
    }

    if(movement && frameCount % LEG_MOVE_RATE_IN_FRAMES == 0)
    {
        lob->legsOut = !lob->legsOut;
    }
}

void handlePunchStateMachine(Lobster *lob)
{
    lob->timeInPunchState++;
    switch(lob->punchState)
    {
        case NEUTRAL:
            if(lob->timeInPunchState > BTW_PUNCHES_TIME_IN_FRAMES && lob->buttonState[START_PUNCH])
            {
                lob->timeInPunchState = 0;
                lob->punchState = WINDUP;
            }
            break;

        case WINDUP:
            if(lob->timeInPunchState > WINDUP_TIME_IN_FRAMES)
            {
                lob->timeInPunchState = 0;
                lob->punchState = PUNCH;
            }
            break;

        case PUNCH:
            if(lob->timeInPunchState > PUNCH_TIME_IN_FRAMES)
            {
                // check collisions here?
                lob->timeInPunchState = 0;
                lob->punchState = NEUTRAL;
                lob->lastPunchLeft = !lob->lastPunchLeft;
            }
    }

}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    const Uint64 now = SDL_GetTicks();
    
    // updating if on new frame
    if(now - lastFrame >= FRAME_RATE_IN_MS)
    {
        frameCount++;

        handleLobsterMovement(&red);
        handlePunchStateMachine(&red);

        lastFrame += FRAME_RATE_IN_MS;
    }
        
    SDL_SetRenderDrawColor(renderer, 224, 193, 164, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // drawing red
    SDL_FRect destination;
    destination.x = red.x;
    destination.y = red.y;
    destination.w = red.width;
    destination.h = red.height;

    // SDL_RenderTexture(renderer, texture, NULL, &destination);

    int textureIndex = 2 * red.punchState;
    if(red.legsOut)
    {
        textureIndex++;
    }
    SDL_Texture *toRender = red.textures[textureIndex];

    SDL_FlipMode flip = SDL_FLIP_NONE;
    if(red.punchState != NEUTRAL && !red.lastPunchLeft)
    {
        flip = SDL_FLIP_HORIZONTAL;
    }
    SDL_RenderTextureRotated(renderer, toRender, NULL, &destination, red.angle, &red.center, flip);

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    for(int i = 0; i < NUM_TEXTURES; i++)
    {
        if(red.textures[i] != NULL)
        {
            SDL_DestroyTexture(red.textures[i]);
        }
    }
}