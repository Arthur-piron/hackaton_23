# LAST LIGHT — design du jeu

Survie zombie en 3D vue de dessus, C++17 / raylib 5.5. **Doit tourner sous Linux ET Windows**, livré en exécutable sur les deux.

## Ambition

L'objectif n'est pas « un jeu qui marche », c'est **un jeu qui en met plein la vue** : rendu nocturne avec vraies lumières dynamiques, particules, sang, secousses d'écran, son spatialisé, animations, menus soignés, boss, et un build propre sous Linux, Windows et Docker avec CI. On ne s'arrête pas au minimum : une fois le minimum jouable, chaque tour ajoute du contenu ou du polish.
Ordre de priorité absolu : **(1) ça compile partout, (2) c'est jouable, (3) c'est beau, (4) il y a du contenu**. On ne casse jamais `main`.

## Pitch

Panne générale, la nuit, dans un vieux cimetière. Tu es le gardien (`character-keeper`), dernier vivant. **Les morts craignent la lumière** : ta lampe torche les ralentit et les brûle, mais sa batterie se vide. Tiens 10 vagues jusqu'à l'aube en ramassant piles, munitions et ferraille, et en rallumant les lampadaires et braseros qui créent des zones sûres.

## Contrôles

| Entrée | Action |
|---|---|
| ZQSD / WASD (les deux) | Se déplacer |
| Maj | Sprint (endurance) |
| Souris | Viser (rayon caméra → sol) |
| Clic gauche | Tirer |
| Clic droit | Lampe torche (vide la batterie) |
| 1-4 / molette | Changer d'arme |
| E | Allumer un lampadaire / ramasser / acheter |
| R | Recharger |
| Échap | Pause |
| Manette (bonus) | Stick gauche/droit, gâchettes |

## Boucle de jeu

1. Une vague sort du brouillard par les bords du cimetière.
2. Survivre : balles, lumière, déplacement, barricades.
3. Les morts lâchent piles, munitions, ferraille.
4. Entre deux vagues (20 s) : rallumer un lampadaire (ferraille), poser une barricade, ou choisir 1 amélioration parmi 3.
5. Vague 10 : boss. Puis l'aube se lève (le ciel s'éclaircit, les zombies brûlent) : victoire.

## Ennemis (modèles Kenney Graveyard Kit)

| Type | Modèle | Comportement |
|---|---|---|
| Marcheur | `character-zombie` | Lent, en groupe, anim `walk` |
| Coureur | `character-zombie` teinté rouge, échelle 0.9 | Rapide, fragile, anim `sprint` |
| Squelette | `character-skeleton` | Insensible à la lumière, fragile aux balles |
| Vampire | `character-vampire` | Fuit la lumière puis contourne, rapide |
| Spectre | `character-ghost` (semi-transparent) | Traverse les barricades, éteint les lumières proches |
| Boss « Le Rongé » | `character-zombie` échelle 3, teinté | Charge, frappe au sol (onde de choc), invoque des marcheurs |

## Armes (Kenney Blaster Kit)

Pistolet (infini, lent) → fusil à pompe (dispersion) → mitraillette → lance-fusées (zone). Chaque arme : modèle `blaster-*` tenu dans la main droite (anim `holding-right-shoot`), cadence, dégâts, dispersion, recul, son, flash de bouche (`muzzle_*`).

## Choix techniques

- **C++17**, **raylib 5.5** via CMake `FetchContent`, liée en statique.
  - Récupérer l'**archive**, pas le dépôt git (le clone échoue sous Windows, chemins trop longs) :
    `FetchContent_Declare(raylib URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP TRUE)`
  - Testé sous Windows : CMake 4.4 + Visual Studio Build Tools 2022.
- **Multiplateforme obligatoire** : aucun en-tête Windows/POSIX dans le code du jeu ; chemins avec `/` ; noms de fichiers en minuscules (Linux est sensible à la casse) ; chemins des assets relatifs à l'exécutable (`GetApplicationDirectory()`), et CMake copie `assets/` à côté de l'exécutable après le build.
- **CI GitHub Actions** (`.github/workflows/build.yml`) : build Linux (ubuntu-latest) + Windows (windows-latest) sur chaque PR. **Une PR dont la CI est rouge ne se fusionne pas.**
- **Docker** : `Dockerfile` multi-étapes qui compile la version Linux (image `ubuntu:24.04` + deps raylib) et produit l'exécutable ; `docker build -t lastlight . && docker run --rm -v "$PWD/dist:/out" lastlight` copie le binaire dans `dist/`. Doc pour lancer le jeu dans Docker avec X11 (`-e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix`).
- **Release** : workflow qui, sur un tag `v*`, publie `LastLight-linux.tar.gz` et `LastLight-windows.zip` (exécutable + `assets/`).
- Collisions : boîtes / sphères raylib et rayons (`GetRayCollision*`). Pas de moteur physique.
- États : `Menu`, `Playing`, `Paused`, `Shop` (entre vagues), `GameOver`, `Victory`, `Credits`.

### ⚠️ Animations des personnages Kenney (vérifié)

Les GLB Kenney utilisent des animations **par nœuds** (chaque membre est un nœud rigide : `root`, `torso`, `head`, `arm-left`, `arm-right`, `leg-left`, `leg-right`), **sans squelette**. `LoadModelAnimations` de raylib renvoie **0 animation** pour ces fichiers. Il faut notre propre lecteur :

- `#include "external/cgltf.h"` (sans `CGLTF_IMPLEMENTATION` : les symboles sont déjà dans la lib raylib) et ajouter `target_include_directories(<cible> PRIVATE ${raylib_SOURCE_DIR}/src)`.
- `LoadModel` crée un mesh par nœud qui a un mesh, **dans l'ordre de `data->nodes`**, et **applique déjà** la transformation monde du nœud aux sommets.
- Donc pour animer : au chargement, garder `invBind[m] = inverse(worldBind(node_m))` ; à chaque image, échantillonner les canaux (translation = lerp, rotation = slerp), écrire dans `node->translation/rotation`, puis dessiner chaque mesh avec `transformObjet * world(node_m) * invBind[m]` (en notation raylib : `MatrixMultiply(MatrixMultiply(invBind[m], worldAnim), objet)`).
- Les 32 animations de chaque personnage : `idle, walk, sprint, jump, fall, crouch, sit, drive, die, pick-up, emote-yes, emote-no, holding-right, holding-left, holding-both, holding-right-shoot, holding-left-shoot, holding-both-shoot, attack-melee-right, attack-melee-left, attack-kick-right, attack-kick-left, interact-right, interact-left, wheelchair-*`.
- **Prototype qui marche** (zombie texturé qui marche, capture vérifiée) : `docs/tech/node_animation_prototype.cpp`. Le partager entre instances : une seule `cgltf_data` par fichier + un état (anim, temps) par entité ; écrire un `Animator` qui calcule les matrices sans dépendre de l'état global des nœuds (copier TRS de base, appliquer l'anim, calculer les mondes soi-même) pour pouvoir animer 200 zombies avec le même modèle.

## Architecture

```
src/
  main.cpp            boucle, machine à états                    (leader)
  core/Assets.*       chargement centralisé (modèles, sons, shaders, textures) (C)
  core/Animator.*     lecteur d'animations par nœuds (cgltf)     (B)
  core/Audio.*        sons, sons 3D (volume/pan selon distance), musique (C)
  game/Player.*       déplacement, visée, vie, endurance, batterie (A)
  game/Weapon.*       armes, projectiles, recul, recharge        (A)
  game/Flashlight.*   cône de lumière, effet sur les ennemis     (A)
  game/Enemy.*        types, IA, états (poursuite, attaque, mort) (B)
  game/WaveManager.*  vagues, apparitions, difficulté, boss      (B)
  game/World.*        cimetière, décor, collisions, lampadaires  (C)
  game/Pickup.*       objets au sol                              (C)
  fx/Particles.*      système de particules (billboards)         (A ou C)
  fx/Lighting.*       shader d'éclairage nuit (lumières ponctuelles + cône) (C)
  fx/PostFx.*         vignette, grain, aberration chromatique, flash de dégâts (C)
  ui/Hud.*  ui/Menu.* interface, menus, crédits                  (C)
assets/  models/  textures/  sounds/  music/  shaders/
tools/fetch_assets.cmake
.github/workflows/build.yml   Dockerfile   CREDITS.md
```

## Étapes (chaque ligne ≈ un tour de 30 min ; dans l'ordre)

**Leader — socle et intégration**
1. Squelette : CMake (raylib par archive, C++17, `assets/` copié après build, warnings activés), fenêtre 1280×720, sol, caméra vue de dessus inclinée qui suit, cube joueur ZQSD, structure `src/`.
2. CI GitHub Actions Linux + Windows.
3. Machine à états + menu minimal + pause.
4. Ensuite : partie A, intégration des PR, équilibrage, Dockerfile si C est en retard.

**A — Joueur & combat (leader)**
1. Joueur `character-keeper` animé (idle/walk/sprint), visée souris
2. Tir : projectiles, munitions, recharge, flash de bouche, son
3. Vie, dégâts reçus (flash rouge, secousse), mort (anim `die`), game over
4. Lampe torche : cône, batterie, effet sur les ennemis (API `Flashlight::illuminates(pos)`)
5. Armes multiples (Blaster Kit), changement d'arme, recul
6. Améliorations entre les vagues, sprint/endurance

**B — Ennemis & vagues (Léandre)**
1. `Animator` (à partir du prototype), réutilisable par toutes les entités
2. Marcheur animé : poursuite, vie, mort (anim `die` puis disparition), dégâts au contact (`attack-melee-*`)
3. Séparation (pas de superposition), contournement simple des obstacles
4. `WaveManager` : vagues croissantes, apparition aux bords, pause de 20 s
5. Coureur, Squelette, Vampire, réaction à la lumière
6. Spectre, Boss, équilibrage, 200 ennemis à 60 FPS (instancing ou LOD si besoin)

**C — Monde & présentation (3ᵉ membre, sinon réparti)**
1. `tools/fetch_assets.cmake` + sélection commitée dans `assets/` + `CREDITS.md`
2. Cimetière : décor Graveyard Kit, murs/clôtures en bordure, collisions
3. HUD : vie, munitions, batterie, vague, score
4. Éclairage nuit (shader) : lampadaires, braseros, lampe du joueur, brouillard
5. Objets ramassables, lampadaires à rallumer, barricades
6. Particules (sang, étincelles, fumée), post-processing, sons 3D, musique
7. Menus (titre, options volume, crédits), écran de victoire avec lever du soleil
8. Dockerfile + workflow de release + README de lancement Linux/Windows

## Planning

- **Samedi matin** : socle + CI fusionnés, assets dans le repo.
- **Samedi soir** : jeu minimum jouable sur Linux et Windows (joueur, tir, marcheurs animés, vagues, HUD, game over).
- **Dimanche matin** : lumière, éclairage, nouveaux ennemis, armes, objets.
- **Dimanche après-midi** : particules, post-fx, son, boss, menus, équilibrage. **Plus de nouvelle fonctionnalité 2 h avant la fin** : corrections, release Linux + Windows, vidéo de démo.
