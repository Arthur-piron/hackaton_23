# Assets (tous CC0, vérifiés le 2026-10-24)

Tous les liens ci-dessous se téléchargent directement avec `curl` / `file(DOWNLOAD)` (pas de compte, pas de navigateur).
On **commite dans `assets/` uniquement les fichiers utilisés** (pas les zips entiers), pour que le build, la CI et Docker ne dépendent pas du réseau.
Le script `tools/fetch_assets.cmake` télécharge les zips dans `build/_assets_cache/` et copie la sélection dans `assets/`. Il se lance pareil sous Linux et Windows : `cmake -P tools/fetch_assets.cmake`.

## Modèles 3D (GLB)

| Pack | URL directe | Contenu utile |
|---|---|---|
| Kenney Graveyard Kit 5.0 | https://kenney.nl/media/pages/assets/graveyard-kit/ba8d4b4517-1760691807/kenney_graveyard-kit_5.0.zip | **Personnages animés** : `character-zombie`, `character-skeleton`, `character-vampire`, `character-ghost`, `character-keeper` (= le joueur). Décor : `crypt*`, `gravestone-*`, `iron-fence*`, `stone-wall*`, `lightpost-*`, `lantern-*`, `pine*`, `coffin*`, `pumpkin*`, `fire-basket`, `road`, `debris*` |
| Kenney Blaster Kit 2.1 | https://kenney.nl/media/pages/assets/blaster-kit/261d80a716-1753959510/kenney_blaster-kit_2.1.zip | 18 armes `blaster-a..r`, `grenade-a/b`, `clip-*`, `crate-*`, `smoke` |
| Kenney Survival Kit 2.0 | https://kenney.nl/media/pages/assets/survival-kit/4065a8185b-1712149243/kenney_survival-kit.zip | Barricades `fence-fortified`, `barrel`, `box*`, `chest`, `campfire-*`, `tent*`, `structure-metal-*`, `tree*`, `rock-*`, `workbench*` |

Les GLB référencent une texture externe `Textures/colormap.png` : **copier le dossier `Textures/` à côté des `.glb`** de chaque pack (un sous-dossier par pack : `assets/models/graveyard/`, `assets/models/blaster/`, `assets/models/survival/`).

## Textures 2D

| Pack | URL directe | Contenu utile |
|---|---|---|
| Kenney Particle Pack | https://kenney.nl/media/pages/assets/particle-pack/f8fe0f8cb8-1677578741/kenney_particle-pack.zip | Dossier `PNG (Transparent)` : `muzzle_0*`, `spark_0*`, `smoke_0*`, `fire_0*`, `flame_0*`, `light_0*`, `scorch_0*`, `dirt_0*`, `circle_0*`, `trace_0*` |

## Sons

| Pack | URL directe | Contenu utile |
|---|---|---|
| Kenney Impact Sounds | https://kenney.nl/media/pages/assets/impact-sounds/87b4ddecda-1677589768/kenney_impact-sounds.zip | `footstep_grass_*`, `footstep_concrete_*`, `impactPunch_*`, `impactSoft_*`, `impactWood_*`, `impactMetal_*` |
| Kenney Interface Sounds | https://kenney.nl/media/pages/assets/interface-sounds/fa43c1dd4d-1677589452/kenney_interface-sounds.zip | `click_*`, `confirmation_*`, `error_*`, `drop_*`, `glitch_*` |
| Zombie noises (ianzazz, OGA) | https://opengameart.org/sites/default/files/zombienoises.zip | `zombienoise1..3.ogg`, `fastzombie1.ogg` |
| Zombie moans (Darsycho, OGA) | https://opengameart.org/sites/default/files/darsycho__zombie-moans_0.ogg | plusieurs râles à découper (ou jouer avec pitch aléatoire) |
| Gunshot sounds (Tabasco, OGA) | https://opengameart.org/sites/default/files/sounds.zip | pistolet, fusil, fusil à pompe (enregistrements bruités : couper le début/fin, baisser le volume) |

## Musique / ambiance

| Piste | URL directe |
|---|---|
| Ambient Horror Track 01 (Cleyton Kauffman) — menu / jeu | https://opengameart.org/sites/default/files/ambient_horror_track_01_0.zip (garder seulement le `.ogg`) |
| Abandoned passages (congusbongus) — ambiance de nuit | https://opengameart.org/sites/default/files/freezer_0.ogg |

## Crédits

CC0 = aucune attribution obligatoire, mais on crédite quand même dans `CREDITS.md` et dans l'écran de crédits : Kenney (kenney.nl), ianzazz, Darsycho, Tabasco, Cleyton Kauffman, congusbongus (OpenGameArt).
