#include "glutil.cpp"

/*
    https://www.youtube.com/watch?v=04oQ2jOUjkU&t=254s&ab_channel=SimonDev
    i,j
    rotate 45
    squash in half

    i moves horizontally 1 tile width and down a half tile width (1, 0.5)
    j moves horizontally -1 tile width and down a half tile width (-1, 0.5)

    to find the screen coordinates of a tile, transforms the tile coordinates tx(1, 0.5) + ty(-1, 0.5)

*/
struct Tilemap {
    int rows;
    int columns;
    int tile_width;
    int tile_height;
};

struct GameState {
    b32 initialized;

    Tilemap tilemap;

    int view_offset_x;
    int view_offset_y;

    Texture test_texture;
};

#define PAN_SPEED 15

void PrintInputEvent(InputEvent *event);
void InitializeGameState(GameState *state);
void ProcessInputEvents(GameState *state);

mat2 CreateTilemapTransform() {
    return{};
}

// TODO(lmk): matrix
v2 TileToScreen(int tile_x, int tile_y, Tilemap tilemap) {
    f32 half_tile_width = tilemap.tile_width * 0.5f;
    f32 half_tile_height = tilemap.tile_height * 0.5f;
    f32 r = 0.666666f;
    v2 x = { tile_x * 1.0f * half_tile_width, tile_x * r * half_tile_height };
    v2 y = { tile_y * -1.0f * half_tile_width, tile_y * r * half_tile_height };

    v2 result = VAdd(x, y);
    result.x -= half_tile_width; // move origin back to top-left

    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    result.x += fb_width*0.5f; // center the tilemap horizontally

    int tile_map_center_y = tilemap.tile_height * tilemap.rows * r * 0.5f;
    int fb_center_y = fb_height*0.5f;
    int center_y_offset = fb_center_y - tile_map_center_y;
    result.y += center_y_offset; // center the tilemap vertically

    return result;
}

void UpdateAndRender(GameState *state) {
    if (!state->initialized) {
        InitializeGameState(state);
    }

    ProcessInputEvents(state);

    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    v4 line_color = { 0.0f, 0.0f, 1.0f, 1.0f };
    v4 fill_color = { .50f, .50f, 1.0f, 1.0f };

    f32 scale = 0.5;

    f32 tile_map_width = state->tilemap.tile_width * state->tilemap.columns * scale;
    f32 tile_map_height = state->tilemap.tile_height * state->tilemap.rows * scale;

    DrawRectangleLines(0, tile_map_width, 0, tile_map_height, fill_color);

    int tile_map_center_x = tile_map_width * 0.5f;
    int screen_center_x = fb_width * 0.5f;
    int offset_x = screen_center_x - tile_map_center_x;

    int tile_map_center_y = tile_map_height * 0.5f;
    int screen_center_y = fb_height * 0.5f;
    int offset_y = screen_center_y - tile_map_center_y;

    DrawRectangleLines(0 + offset_x, tile_map_width + offset_x, 0 + offset_y, tile_map_height + offset_y, line_color);

    Texture temp = state->test_texture;
    temp.width = state->test_texture.width * scale;
    temp.height = state->test_texture.height * scale;

    for (int r = 0; r < state->tilemap.rows; ++r) {
        for (int c = 0; c < state->tilemap.columns; ++c) {
            v2 screen_coords = TileToScreen(c, r, state->tilemap);
            screen_coords.x += state->view_offset_x;
            screen_coords.y += state->view_offset_y;

            DrawTexture(state->test_texture, screen_coords.x, screen_coords.y);
        }
    }

    DrawPixel(fb_width*0.5, fb_height*0.5, fill_color, 5);

    //DrawTexture(temp, 0, 0);
}

void PrintInputEvent(InputEvent *event) {
    const char *device, *type;

    switch (event->device) {
        case InputDevice::Mouse: device = "Mouse"; break;
        case InputDevice::Keyboard: device = "Keyboard"; break;
        default: device = "Unknown device"; break;
    }

    switch (event->type) {
        case InputEventType::KeyEvent: type = "Key Event"; break;
        case InputEventType::CursorEnterEvent: type = "Cursor Enter Event"; break;
        case InputEventType::CursorPositionEvent: type = "Cursor Position Event"; break;
        case InputEventType::MouseButtonEvent: type = "Mouse Button Event"; break;
        case InputEventType::MouseScrollEvent: type = "Mouse Scroll Event"; break;
        default: type = "Unknown event type"; break;
    }

    fprintf(stdout, "Input Event: %s (%s)\n", device, type);

    if (event->type == InputEventType::CursorPositionEvent) {
        fprintf(stdout, "%.0f, %.0f, (%.0f, %.0f)\n", event->x, event->y, event->dx, event->dy);
    }
}

void InitializeGameState(GameState *state) {
    state->initialized = true;

    state->tilemap.tile_width = 256;
    state->tilemap.tile_height = 192;
    state->tilemap.columns = 10;
    state->tilemap.rows = 10;

    state->test_texture = LoadTexture("C:/work/projects/eco-strategy/assets/isometric-asset-pack/256x192Tile.png");
}

void ProcessInputEvents(GameState *state) {
    InputEvent event;

    while (GetNextInputEvent(&event)) {
        //PrintInputEvent(&event);

        if (event.type == InputEventType::CursorPositionEvent) {
            // TODO: Mouse is hovering window
            if (IsButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
                // move world
                v2 delta = { (f32)event.dx, (f32)event.dy };
                v2 dir = Normalize(delta);
                v2 offset = VMul(dir, PAN_SPEED);

                //state->view_offset_x += offset.x;
                //state->view_offset_y += offset.y;

                state->view_offset_x += event.dx;
                state->view_offset_y += event.dy;
            }
        }

        if (event.type == InputEventType::KeyEvent) {
            switch (event.key) {
                case GLFW_KEY_UP: state->view_offset_y -= PAN_SPEED; break;
                case GLFW_KEY_LEFT: state->view_offset_x -= PAN_SPEED; break;
                case GLFW_KEY_RIGHT: state->view_offset_x += PAN_SPEED; break;
                case GLFW_KEY_DOWN: state->view_offset_y += PAN_SPEED; break;
            }
        }
    }
}
