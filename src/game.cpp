#include "glutil.cpp"

static Arena scratch_arena;
static Arena persist_arena;

enum AssetTag {
    Tiles,
    SlopesAndStairs,
    Flooring,
    TileOverlay,
    Objects,
    UIElements,
    Cubes,
    Trees,
    Count
};

struct TilemapAsset {
    Texture texture;
    int tile_width;
    int tile_height;
};

struct TilemapAssetID {
    int row;
    int column;
    AssetTag tag;
};

#define TILE_WATER_1 { 1, 2, Tiles }

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

struct Camera {
    v2 position;
    f32 zoom;
};

struct GameState {
    b32 initialized;

    TilemapAsset assets[AssetTag::Count];

    Camera camera;

    Tilemap tilemap;
    Font font;

    int view_offset_x;
    int view_offset_y;

    Texture test_texture;
    Texture atlas;
};

void PrintInputEvent(InputEvent *event);
void InitializeGameState(GameState *state);
void ProcessInputEvents(GameState *state);

#define ZOOM_INCREMENT 0.05f
#define TILEMAP_ROTATION (2 / 3.0f) // 4 : 3 ?
#define PAN_SPEED 15

int GetTilemapAssetRows(TilemapAsset *asset) {
    int result = asset->texture.height / asset->tile_height;
    return result;
}

int GetTilemapAssetColumns(TilemapAsset *asset) {
    int result = asset->texture.width / asset->tile_width;
    return result;
}

Rect GetAssetTextureRect(TilemapAssetID id, GameState *state) {
    TilemapAsset *asset = &state->assets[id.tag];

    int rows = GetTilemapAssetRows(asset);
    int cols = GetTilemapAssetColumns(asset);

    Rect result = {
        id.column * (f32)asset->tile_width,
        id.row * (f32)asset->tile_height,
        (f32)asset->tile_width,
        (f32)asset->tile_height
    };

    return result;
}

Texture GetAssetTexture(TilemapAssetID id, GameState *state) {
    Texture result = state->assets[id.tag].texture;
    return result;
}

void LoadTilemapAssets(GameState *state) {
    TilemapAsset *tiles = &state->assets[Tiles];
    LoadTexture("assets/isometric-asset-pack/256x192 Tiles.png", &tiles->texture);
    tiles->tile_width = 256;
    tiles->tile_height = 192;

    TilemapAsset *slopes_and_stairs = &state->assets[SlopesAndStairs];
    LoadTexture("assets/isometric-asset-pack/256x192 SlopesAndStairs.png", &slopes_and_stairs->texture);
    slopes_and_stairs->tile_width = 256;
    slopes_and_stairs->tile_height = 192;

    TilemapAsset *ui_elements = &state->assets[UIElements];
    LoadTexture("assets/isometric-asset-pack/256x256 UI Elements.png", &ui_elements->texture);
    ui_elements->tile_width = 256;
    ui_elements->tile_height = 256;

    TilemapAsset *cubes = &state->assets[Cubes];
    LoadTexture("assets/isometric-asset-pack/256x256 Cubes.png", &cubes->texture);
    cubes->tile_width = 256;
    cubes->tile_height = 256;

    TilemapAsset *objects = &state->assets[Objects];
    LoadTexture("assets/isometric-asset-pack/256x256 Objects.png", &objects->texture);
    objects->tile_width = 256;
    objects->tile_height = 256;

    TilemapAsset *flooring = &state->assets[Flooring];
    LoadTexture("assets/isometric-asset-pack/256x152 Floorings.png", &flooring->texture);
    flooring->tile_width = 256;
    flooring->tile_height = 152;

    TilemapAsset *trees = &state->assets[Trees];
    LoadTexture("assets/isometric-asset-pack/256x512 Trees.png", &trees->texture);
    trees->tile_width = 256;
    trees->tile_height = 512;

    TilemapAsset *overlays = &state->assets[TileOverlay];
    LoadTexture("assets/isometric-asset-pack/256x128 Tile Overlays.png", &overlays->texture);
    overlays->tile_width = 256;
    overlays->tile_height = 128;
}

// TODO(lmk): transform all tilemap coordinates at once

mat2 CreateTilemapTransform(Tilemap *tilemap) {
    mat2 result;

    f32 half_tile_width = tilemap->tile_width * 0.5f;
    f32 half_tile_height = tilemap->tile_height * 0.5f;
    f32 r = TILEMAP_ROTATION;

    result.c0 = { 1.0f * half_tile_width, r * half_tile_height };
    result.c1 = { -1.0f * half_tile_width, r * half_tile_height };

    return result;
}

v2 TileToScreen(int tile_x, int tile_y, Tilemap *tilemap, Camera camera) {
    mat2 transform = CreateTilemapTransform(tilemap);

    v2 result = MMul(transform, {(f32)tile_x, (f32)tile_y});

    result.x -= tilemap->tile_width * 0.5f; // move origin back to top-left
    //result.y -= tilemap->tile_height * 0.5f;

    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    result.x += fb_width*0.5f; // center the tilemap horizontally

    int tile_map_center_y = tilemap->tile_height * tilemap->rows * TILEMAP_ROTATION * 0.5f;
    int fb_center_y = fb_height*0.5f;
    int center_y_offset = fb_center_y - tile_map_center_y;
    result.y += center_y_offset; // center the tilemap vertically

    result += camera.position;

    return result;
}

v2 ScreenToTile(int x, int y, Tilemap *tilemap, Camera camera) {
    mat2 transform = CreateTilemapTransform(tilemap);
    mat2 inv = Inverse(transform);

    v2 screen_coords = { (f32)x, (f32)y };

    // I'm a little confused why we don't need to undo this operation...
    //screen_coords.x += tilemap->tile_width * 0.5f; // move origin back to top-left

    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    int tile_map_center_y = tilemap->tile_height * tilemap->rows * TILEMAP_ROTATION * 0.5f;
    int fb_center_y = fb_height*0.5f;
    int center_y_offset = fb_center_y - tile_map_center_y;

    // move the tilemap back to top left
    screen_coords.x -= fb_width*0.5f;
    screen_coords.y -= center_y_offset;

    // offset from the tilemaps current position
    screen_coords -= camera.position;

    v2 result = MMul(inv, screen_coords);

    return result;
}

v2 Scale(v2 p, f32 width, f32 height, f32 s) {
    v2 ps = p * s;
    return ps;
}

void UpdateAndRender(GameState *state) {
    if (!state->initialized) {
        InitializeGameState(state);

        fprintf(stdout, "%d x %d tiles", state->tilemap.columns, state->tilemap.rows);
    }

    ProcessInputEvents(state);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw texture preview

    int fb_width, fb_height;
    GetWindowFramebufferSize(&fb_width, &fb_height);

    v4 line_color = { 0.0f, 0.0f, 1.0f, 1.0f };
    v4 fill_color = { .50f, .50f, 1.0f, 1.0f };

    v2 cursor_tile = ScreenToTile(platform.cursor_x, platform.cursor_y, &state->tilemap, state->camera);

    Texture tile_texture = GetAssetTexture(TILE_WATER_1, state);
    Rect water_uv_rect = GetAssetTextureRect(TILE_WATER_1, state);

    for (int r = 0; r < state->tilemap.rows; ++r) {
        for (int c = 0; c < state->tilemap.columns; ++c) {
            v2 screen_coords = TileToScreen(c, r, &state->tilemap, state->camera);

            if (r == (int)cursor_tile.y && c == (int)cursor_tile.x) {
                screen_coords.y -= 20;
            }

            Rect dest_rect = {
                screen_coords.x,
                screen_coords.y,
                (f32)state->tilemap.tile_width,
                (f32)state->tilemap.tile_height
            };

            Rect uv_rect = {
                r * 256.0f,
                c * 192.0f,
                256,
                192
            };

            DrawTextureRect(tile_texture, dest_rect, water_uv_rect);
            DrawPixel(screen_coords.x, screen_coords.y, fill_color, 5);
        }
    }

    DrawPixel(fb_width*0.5, fb_height*0.5, fill_color, 5);
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

    scratch_arena = CreateArena(Kilobytes(16));
    persist_arena = CreateArena(Kilobytes(16));

    state->tilemap.tile_width = 256;
    state->tilemap.tile_height = 192;
    state->tilemap.columns = 10;
    state->tilemap.rows = 10;

    state->camera.zoom = 0.25;

    LoadTexture("assets/isometric-asset-pack/256x192 Tiles.png", &state->atlas);
    LoadTilemapAssets(state);
}

void ProcessInputEvents(GameState *state) {
    InputEvent event;

    while (GetNextInputEvent(&event)) {
        if (event.type == InputEventType::CursorPositionEvent) {
            // TODO: Mouse is hovering window
            if (IsButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
                // move world
                v2 delta = { (f32)event.dx, (f32)event.dy };
                v2 dir = Normalize(delta);
                v2 offset = dir * PAN_SPEED;

                state->camera.position.x += event.dx;
                state->camera.position.y += event.dy;
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

        if (event.type == InputEventType::MouseScrollEvent) {
            state->camera.zoom = Max(state->camera.zoom + event.y * ZOOM_INCREMENT, 0.1f);

            Tilemap *tilemap = &state->tilemap;

            f32 old_tile_width = tilemap->tile_width;
            f32 old_tile_height = tilemap->tile_height;

            tilemap->tile_width = 256 * state->camera.zoom;
            tilemap->tile_height = 192 * state->camera.zoom;

            // re-center camera after zoom
            f32 dx = (old_tile_width - tilemap->tile_width) * 0.5f;
            f32 dy = (old_tile_height - tilemap->tile_height) * 0.5f;
            state->camera.position.x += dx;
            state->camera.position.y += dy;
        }
    }
}

void *ArenaAlloc(Arena *arena, u64 count) {
    if (arena->count + count > arena->size) {
        u64 new_size = arena->size * 2;
        arena->base_address = realloc(arena->base_address, new_size);
        fprintf(stderr, "INFO: Arena resized: (%lld -> %lld)\n", arena->size, new_size);
        arena->size = new_size;
    }

    void *result = (u8 *)arena->base_address + arena->count;
    arena->count += count;

    memset(result, 0, count);

    return result;
}

Arena CreateArena(u64 size) {
    Arena result = {};
    result.base_address = malloc(size);
    result.size = size;

    return result;
}
