#define SDL_MAIN_USE_CALLBACKS 1
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// SDL stuff
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *ring;
#define WINDOW_WIDTH 800
#define DEGREE_TO_RADIANS 57.3f

// timing
#define FRAME_RATE_IN_MS 40
#define LEG_MOVE_RATE_IN_FRAMES 5
static Uint64 lastFrame = 0;
static int frameCount = 0;
// lobsters
#define BTW_PUNCHES_TIME_IN_FRAMES 8
#define WINDUP_TIME_IN_FRAMES 6
#define PUNCH_TIME_IN_FRAMES 6

// game constants
#define MOVE_STEP 5.0f
#define ANGLE_STEP 2.0f

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

static const char* RED_TEXTURE_PNG_NAMES[NUM_TEXTURES] =
{
    "red_neutral_in",
    "red_neutral_out",
    "red_windup_in",
    "red_windup_out",
    "red_punch_in",
    "red_punch_out",
};

static const char* BLUE_TEXTURE_PNG_NAMES[NUM_TEXTURES] =
{
    "blue_neutral_in",
    "blue_neutral_out",
    "blue_windup_in",
    "blue_windup_out",
    "blue_punch_in",
    "blue_punch_out",
};

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

typedef struct HITBOX_CIRCLE
{
    int radius;
    int y_offset;
} HitboxCircle;

#define NUM_HURTBOX_CIRCLES 4
#define C1_RADIUS 25
#define C1_Y_OFFSET -45
#define C2_RADIUS 40
#define C2_Y_OFFSET 0
#define C3_RADIUS 25
#define C3_Y_OFFSET 45
#define C4_RADIUS 25
#define C4_Y_OFFSET 85
#define PUNCHBOX_RADIUS 30
#define PUNCHBOX_Y_OFFSET -100

#define RED_START_X 125
#define RED_START_Y 500
#define RED_START_ANGLE 45

#define BLUE_START_X 550
#define BLUE_START_Y 50
#define BLUE_START_ANGLE 225 

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
    bool hitWithPunch;
    bool legsOut;
    SDL_Texture *textures[NUM_TEXTURES];
    bool buttonState[NUM_BUTTONS];
    HitboxCircle hurtbox[NUM_HURTBOX_CIRCLES];
    HitboxCircle punchbox;
} Lobster;

enum LOBSTER_POSITIONS
{
    RED,
    BLUE,
    NUM_LOBS
};

// red and blue!
Lobster lobs[NUM_LOBS];

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

    // Setting up lobs
    const char **textureNames[NUM_LOBS] = {RED_TEXTURE_PNG_NAMES, BLUE_TEXTURE_PNG_NAMES};
    for(int j = 0; j < NUM_LOBS; j++)
    {
        // loading textures
        for(int i = 0; i < NUM_TEXTURES; i++)
        {
            char *img_path = NULL;
            SDL_Surface *surface = NULL;
            SDL_asprintf(&img_path, "%sresources/%s.png", SDL_GetBasePath(), textureNames[j][i]);
            surface = SDL_LoadPNG(img_path);
            SDL_free(img_path);
            if(!surface)
            {
                SDL_Log("Could not load image: %s", SDL_GetError());
                return SDL_APP_FAILURE;
            }
        
            lobs[j].textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
            if(!lobs[j].textures[i])
            {
                SDL_Log("Could not create texture: %s", SDL_GetError());
                return SDL_APP_FAILURE;
            }
        }
    
        // setting up other variables
        lobs[j].width = lobs[j].textures[0]->w;
        lobs[j].height = lobs[j].textures[0]->h;
        // lobs[j].x = (float)((WINDOW_WIDTH - lobs[j].width) / 2.0f);
        // lobs[j].y = (float)((WINDOW_WIDTH - lobs[j].height) / 2.0f);
        // lobs[j].angle = 0;
        lobs[j].center.x = lobs[j].width / 2.0f;
        lobs[j].center.y = lobs[j].height / 2.0f;
        lobs[j].punchState = NEUTRAL;
        lobs[j].timeInPunchState = 0;
        lobs[j].lastPunchLeft = true;
        lobs[j].hitWithPunch = false;
        lobs[j].legsOut = true;
        for(int i = 0; i < NUM_BUTTONS; i++)
        {
            lobs[j].buttonState[i] = false;
        }

        lobs[j].hurtbox[0].radius = C1_RADIUS;
        lobs[j].hurtbox[0].y_offset = C1_Y_OFFSET;
        
        lobs[j].hurtbox[1].radius = C2_RADIUS;
        lobs[j].hurtbox[1].y_offset = C2_Y_OFFSET;
        
        lobs[j].hurtbox[2].radius = C3_RADIUS;
        lobs[j].hurtbox[2].y_offset = C3_Y_OFFSET;
        
        lobs[j].hurtbox[3].radius = C4_RADIUS;
        lobs[j].hurtbox[3].y_offset = C4_Y_OFFSET;

        lobs[j].punchbox.radius = PUNCHBOX_RADIUS;
        lobs[j].punchbox.y_offset = PUNCHBOX_Y_OFFSET;
    }
    lobs[RED].x = RED_START_X;
    lobs[RED].y = RED_START_Y;
    lobs[RED].angle = RED_START_ANGLE;

    lobs[BLUE].x = BLUE_START_X;
    lobs[BLUE].y = BLUE_START_Y;
    lobs[BLUE].angle = BLUE_START_ANGLE;

    // loading ring
    char *img_path = NULL;
    SDL_Surface *surface = NULL;
    SDL_asprintf(&img_path, "%sresources/%s.png", SDL_GetBasePath(), "ring");
    surface = SDL_LoadPNG(img_path);
    SDL_free(img_path);
    if(!surface)
    {
        SDL_Log("Could not load image: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    ring = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if(!ring)
    {
        SDL_Log("Could not create texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
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
            lobs[RED].buttonState[UP] = !lobs[RED].buttonState[UP];
            break;
        case SDL_SCANCODE_A:
            lobs[RED].buttonState[LEFT] = !lobs[RED].buttonState[LEFT];
            break;
        case SDL_SCANCODE_S:
            lobs[RED].buttonState[DOWN] = !lobs[RED].buttonState[DOWN];
            break;
        case SDL_SCANCODE_D:
            lobs[RED].buttonState[RIGHT] = !lobs[RED].buttonState[RIGHT];
            break;
        case SDL_SCANCODE_Q:
            lobs[RED].buttonState[COUNTER_CLOCKWISE] = !lobs[RED].buttonState[COUNTER_CLOCKWISE];
            break;
        case SDL_SCANCODE_E:
            lobs[RED].buttonState[CLOCKWISE] = !lobs[RED].buttonState[CLOCKWISE];
            break;
        case SDL_SCANCODE_C:
            lobs[RED].buttonState[START_PUNCH] = !lobs[RED].buttonState[START_PUNCH];
            break;
            
        case SDL_SCANCODE_O:
            lobs[BLUE].buttonState[UP] = !lobs[BLUE].buttonState[UP];
            break;
        case SDL_SCANCODE_K:
            lobs[BLUE].buttonState[LEFT] = !lobs[BLUE].buttonState[LEFT];
            break;
        case SDL_SCANCODE_L:
            lobs[BLUE].buttonState[DOWN] = !lobs[BLUE].buttonState[DOWN];
            break;
        case SDL_SCANCODE_SEMICOLON:
            lobs[BLUE].buttonState[RIGHT] = !lobs[BLUE].buttonState[RIGHT];
            break;
        case SDL_SCANCODE_I:
            lobs[BLUE].buttonState[COUNTER_CLOCKWISE] = !lobs[BLUE].buttonState[COUNTER_CLOCKWISE];
            break;
        case SDL_SCANCODE_P:
            lobs[BLUE].buttonState[CLOCKWISE] = !lobs[BLUE].buttonState[CLOCKWISE];
            break;
        case SDL_SCANCODE_SLASH:
            lobs[BLUE].buttonState[START_PUNCH] = !lobs[BLUE].buttonState[START_PUNCH];
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

bool checkPunchCollision(Lobster *punching)
{
    Lobster *target = &lobs[BLUE];
    if(punching == &lobs[BLUE])
    {
        target = &lobs[RED];
    }

    bool hit = false;

    SDL_FPoint punchPoint;
    // SDL_Log("angle: %f, cos: %f, sin: %f", punching->angle, SDL_cosf((punching->angle - 90) / DEGREE_TO_RADIANS), SDL_sinf((punching->angle - 90) / DEGREE_TO_RADIANS));
    punchPoint.x = punching->x + punching->width / 2.0f + SDL_cosf((punching->angle + 90) / DEGREE_TO_RADIANS) * punching->punchbox.y_offset;
    punchPoint.y = punching->y + punching->height / 2.0f + SDL_sinf((punching->angle + 90) / DEGREE_TO_RADIANS) * punching->punchbox.y_offset;

    for(int i = 0; i < NUM_HURTBOX_CIRCLES && !hit; i++)
    {
        SDL_FPoint hurtboxPoint;
        hurtboxPoint.x = target->x + target->width / 2.0f + SDL_cosf((target->angle + 90) / DEGREE_TO_RADIANS) * target->hurtbox[i].y_offset;
        hurtboxPoint.y = target->y + target->height / 2.0f + SDL_sinf((target->angle + 90) / DEGREE_TO_RADIANS) * target->hurtbox[i].y_offset;
    
        // SDL_Log("punch coords: (%f,%f), hurtbox coords: (%f,%f)", punchPoint.x, punchPoint.y, hurtboxPoint.x, hurtboxPoint.y);

        float distance = SDL_sqrtf(SDL_powf(hurtboxPoint.x - punchPoint.x, 2) + SDL_powf(hurtboxPoint.y - punchPoint.y, 2));
        // SDL_Log("distance: %f, difference: %d", distance, punching->punchbox.radius + target->hurtbox[i].radius);
        if(distance < punching->punchbox.radius + target->hurtbox[i].radius)
        {

            static int counter = 0;
            hit = true;
            counter++;
        }
    }
    return hit;
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
            if(!lob->hitWithPunch)
            {
                lobs->hitWithPunch = checkPunchCollision(lob);
                if(lobs->hitWithPunch)
                {

                }
            }
            if(lob->timeInPunchState > PUNCH_TIME_IN_FRAMES)
            {
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

        for(int i = 0; i < NUM_LOBS; i++)
        {
            handleLobsterMovement(&lobs[i]);
            handlePunchStateMachine(&lobs[i]);
        }

        lastFrame += FRAME_RATE_IN_MS;
    }
        
    SDL_SetRenderDrawColor(renderer, 224, 193, 164, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    SDL_RenderTexture(renderer, ring, NULL, NULL);

    // drawing lobsters
    for(int i = 0; i < NUM_LOBS; i++)
    {
        SDL_FRect destination;
        destination.x = lobs[i].x;
        destination.y = lobs[i].y;
        destination.w = lobs[i].width;
        destination.h = lobs[i].height;
    
        // SDL_RenderTexture(renderer, texture, NULL, &destination);
    
        int textureIndex = 2 * lobs[i].punchState;
        if(lobs[i].legsOut)
        {
            textureIndex++;
        }
        SDL_Texture *toRender = lobs[i].textures[textureIndex];
    
        SDL_FlipMode flip = SDL_FLIP_NONE;
        if(lobs[i].punchState != NEUTRAL && !lobs[i].lastPunchLeft)
        {
            flip = SDL_FLIP_HORIZONTAL;
        }
        SDL_RenderTextureRotated(renderer, toRender, NULL, &destination, lobs[i].angle, &lobs[i].center, flip);
    }

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    for(int i = 0; i < NUM_TEXTURES; i++)
    {
        if(lobs[RED].textures[i] != NULL)
        {
            SDL_DestroyTexture(lobs[RED].textures[i]);
        }
    }
}