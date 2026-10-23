#pragma once

#include "game/CameraRig.hpp"
#include "game/Player.hpp"
#include "game/World.hpp"

#include <string>

// Machine à états du jeu : possède la fenêtre et tous les systèmes.
class Game {
public:
    enum class State { Menu, Playing, Paused, GameOver, Victory };

    struct Options {
        int smokeFrames = 0;         // > 0 : mode test automatique
        std::string smokeScreenshot; // capture enregistrée à la fin du test
    };

    explicit Game(const Options& options);
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void run();

private:
    void update(float dt);
    void draw() const;
    void drawMenu() const;
    void drawPauseOverlay() const;
    void startNewGame();

    Options options_;
    int frameCount_ = 0;
    State state_ = State::Menu;
    World world_;
    Player player_;
    CameraRig cameraRig_;
};
