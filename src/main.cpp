#include "Game.hpp"

#include <cstdlib>
#include <cstring>

// Usage : lastlight [--smoke <images> <capture.png>]
// --smoke lance directement une partie, simule <images> images, enregistre une
// capture puis quitte : sert à vérifier le rendu sans personne devant l'écran.
int main(int argc, char** argv)
{
    Game::Options options;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--smoke") == 0 && i + 2 < argc) {
            options.smokeFrames = std::atoi(argv[i + 1]);
            options.smokeScreenshot = argv[i + 2];
            i += 2;
        }
    }

    Game game(options);
    game.run();
    return 0;
}
