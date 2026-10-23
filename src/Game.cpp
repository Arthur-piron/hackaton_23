#include "Game.hpp"

#include "raylib.h"

namespace {
constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr int kTargetFps = 144;
// Au-delà, on ralentit la simulation plutôt que de laisser traverser les murs.
constexpr float kMaxFrameTime = 0.05f;
constexpr const char* kWindowTitle = "LAST LIGHT";
}

Game::Game(const Options& options)
    : options_(options)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kScreenWidth, kScreenHeight, kWindowTitle);
    InitAudioDevice();
    SetTargetFPS(kTargetFps);
    SetExitKey(KEY_NULL); // Échap sert à la pause, pas à quitter.
    startNewGame();
    if (options_.smokeFrames > 0) state_ = State::Playing;
}

Game::~Game()
{
    CloseAudioDevice();
    CloseWindow();
}

void Game::run()
{
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime() < kMaxFrameTime ? GetFrameTime() : kMaxFrameTime;
        update(dt);
        draw();

        ++frameCount_;
        if (options_.smokeFrames > 0 && frameCount_ >= options_.smokeFrames) {
            if (!options_.smokeScreenshot.empty()) TakeScreenshot(options_.smokeScreenshot.c_str());
            break;
        }
    }
}

void Game::startNewGame()
{
    player_.reset({0.0f, 0.0f, 0.0f});
    cameraRig_.snapTo(player_.position());
}

void Game::update(float dt)
{
    switch (state_) {
    case State::Menu:
        if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startNewGame();
            state_ = State::Playing;
        }
        break;
    case State::Playing:
        if (IsKeyPressed(KEY_ESCAPE)) {
            state_ = State::Paused;
            break;
        }
        player_.update(dt, world_);
        cameraRig_.update(dt, player_.position());
        break;
    case State::Paused:
        if (IsKeyPressed(KEY_ESCAPE)) state_ = State::Playing;
        if (IsKeyPressed(KEY_Q) && IsKeyDown(KEY_LEFT_CONTROL)) state_ = State::Menu;
        break;
    case State::GameOver:
    case State::Victory:
        if (IsKeyPressed(KEY_ENTER)) state_ = State::Menu;
        break;
    }
}

void Game::draw() const
{
    BeginDrawing();
    ClearBackground({12, 14, 22, 255});

    if (state_ == State::Menu) {
        drawMenu();
    } else {
        BeginMode3D(cameraRig_.camera());
        world_.draw();
        player_.draw();
        EndMode3D();

        if (state_ == State::Paused) drawPauseOverlay();
    }

    DrawFPS(10, 10);
    EndDrawing();
}

void Game::drawMenu() const
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    const char* title = "LAST LIGHT";
    const char* hint = "Entree pour commencer";
    DrawText(title, (w - MeasureText(title, 80)) / 2, h / 3, 80, {230, 200, 120, 255});
    DrawText(hint, (w - MeasureText(hint, 24)) / 2, h / 3 + 110, 24, LIGHTGRAY);
}

void Game::drawPauseOverlay() const
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, {0, 0, 0, 150});
    const char* text = "PAUSE";
    const char* hint = "Echap : reprendre   Ctrl+Q : menu";
    DrawText(text, (w - MeasureText(text, 60)) / 2, h / 2 - 60, 60, RAYWHITE);
    DrawText(hint, (w - MeasureText(hint, 20)) / 2, h / 2 + 10, 20, LIGHTGRAY);
}
