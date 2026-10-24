# LAST LIGHT : build Linux reproductible.
#
#   Récupérer l'exécutable Linux (dossier dist/) :
#     docker build --target export --output dist .
#   Jouer depuis le conteneur (Linux, serveur X11 de l'hôte) :
#     docker build -t lastlight .
#     xhost +local:docker
#     docker run --rm -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --device /dev/dri --device /dev/snd lastlight

# --- Compilation -------------------------------------------------------------------
FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ca-certificates \
        libasound2-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev \
        libgl1-mesa-dev libglu1-mesa-dev libwayland-dev libxkbcommon-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
# Sources d'abord, assets ensuite : modifier un asset ne relance pas la compilation de raylib.
COPY CMakeLists.txt ./
COPY src/ src/
COPY assets/ assets/
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel \
    && mkdir -p /out \
    && cp build/lastlight /out/ \
    && cp -r build/assets /out/assets

COPY README.md CREDITS.md /out/

# --- Export : seulement les fichiers du jeu (docker build --output) ------------------
FROM scratch AS export
COPY --from=build /out/ /

# --- Exécution ---------------------------------------------------------------------
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        libasound2t64 libx11-6 libxrandr2 libxi6 libxcursor1 libxinerama1 libgl1 libglx-mesa0 libgl1-mesa-dri \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home player

COPY --from=build --chown=player /out/ /home/player/lastlight/
USER player
WORKDIR /home/player/lastlight
ENTRYPOINT ["./lastlight"]
