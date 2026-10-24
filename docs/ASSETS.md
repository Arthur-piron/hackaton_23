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

---

## À intégrer (v3)

Vérifié le 2026-10-24 : chaque lien ci-dessous a été **réellement téléchargé avec `curl -L`** (HTTP 200, taille indiquée) et le contenu de l'archive inspecté. Licence **CC0** (fichier `License.txt` des zips Kenney ; mention CC0 sur la page OpenGameArt).
À faire par la tâche **V0 `feature/assets-v3`** (roadmap v3 de `docs/GAME.md`) : ajouter ces packs à `tools/fetch_assets.cmake` et ne commiter que la sélection.

### Modèles 3D (GLB)

| Pack | URL directe | Taille | Dossier cible | Sélection (dans `Models/GLB format/`) | Usage |
|---|---|---|---|---|---|
| Kenney Tower Defense Kit | https://kenney.nl/media/pages/assets/tower-defense-kit/a402493eaa-1726471567/kenney_tower-defense-kit.zip | 5,4 Mo | `assets/models/tower/` | `weapon-ballista`, `weapon-turret`, `weapon-cannon`, `weapon-ammo-arrow`, `weapon-ammo-bullet`, `tower-square-bottom-a`, `tower-square-bottom-b`, `tower-round-bottom-a`, `tower-round-build-a`, `detail-crystal`, `detail-crystal-large`, `selection-a` | tourelles (arbalète, mitrailleuse, lance-flammes), carreaux, pics, marqueur de sélection au sol |
| Kenney Retro Urban Kit | https://kenney.nl/media/pages/assets/retro-urban-kit/8314d4db22-1738147509/kenney_retro-urban-kit.zip | 2,2 Mo | `assets/models/urban/` | `detail-dumpster-closed`, `detail-cables-type-a`, `detail-cables-type-b`, `detail-light-single`, `detail-light-double`, `detail-barrier-strong-type-a`, `detail-barrier-strong-damaged`, `pallet`, `planks` | générateur, câbles, projecteur, barricade lourde, caisses de bois |
| Kenney Fantasy Town Kit 2.0 | https://kenney.nl/media/pages/assets/fantasy-town-kit/efe948d309-1754222374/kenney_fantasy-town-kit_2.0.zip | 3,9 Mo | `assets/models/town/` (optionnel) | `wall-wood`, `wall-wood-broken`, `fence`, `fence-broken`, `fence-gate`, `lantern`, `cart`, `planks`, `pillar-wood` | variantes de palissade et de porte, décor de la base |
| Kenney Castle Kit | https://kenney.nl/media/pages/assets/castle-kit/a395102d20-1711543616/kenney_castle-kit.zip | 2,2 Mo | `assets/models/castle/` (optionnel) | `siege-ballista`, `siege-ballista-demolished`, `wall-narrow-wood`, `wall-narrow-wood-fence`, `metal-gate`, `door` | arbalète lourde (variante), porte niv. 3, murs |
| Kenney Nature Kit | https://kenney.nl/media/pages/assets/nature-kit/37ac38a37b-1677698939/kenney_nature-kit.zip | 10,5 Mo | `assets/models/nature/` (optionnel) | dans `Models/GLTF format/` : `log`, `log_stack`, `stump_old`, `tree_pineDefaultA`, `tree_pineTallA_detailed`, `tree_thin_dark`, `mushroom_redGroup`, `grass_large` | pile de bois (butin), souches, forêt plus dense |

⚠️ Textures :
- Tower Defense, Fantasy Town et Castle : comme les autres kits, copier `Models/GLB format/Textures/colormap.png` dans `Textures/` à côté des `.glb`.
- **Retro Urban** utilise **22 textures** (`asphalt.png`, `metal.png`, `planks.png`, `wall.png`…), pas une seule `colormap` : copier **tout** `Models/GLB format/Textures/`. Le cache de `World` (« une texture `colormap` par kit ») ne s'applique pas à ce kit : laisser raylib charger ses matériaux normalement.
- **Nature Kit** : les `.glb` sont autonomes (couleurs dans les matériaux), pas de dossier `Textures/`.

Écartés : Kenney Holiday Kit (thème Noël) ; il n'existe pas de « Halloween Kit » Kenney (c'est le Graveyard Kit, déjà intégré).

### Interface et police

| Pack | URL directe | Taille | Dossier cible | Sélection | Usage |
|---|---|---|---|---|---|
| Kenney Fonts | https://kenney.nl/media/pages/assets/kenney-fonts/8d5435c213-1677661710/kenney_kenney-fonts.zip | 58 Ko | `assets/fonts/` | `Fonts/Kenney Future.ttf` → `kenney_future.ttf`, `Fonts/Kenney Future Narrow.ttf` → `kenney_future_narrow.ttf`, `Fonts/Kenney Pixel.ttf` → `kenney_pixel.ttf` | police du jeu (titres, HUD, chiffres de dégâts) |
| Kenney Board Game Icons | https://kenney.nl/media/pages/assets/board-game-icons/19cae04050-1721645690/kenney_board-game-icons.zip | 1,0 Mo | `assets/ui/icons/` | dans `PNG/Default (64px)/` : `resource_wood`, `resource_iron`, `resource_planks`, `flask_full`, `structure_wall`, `structure_gate`, `structure_tower`, `structure_watchtower`, `fire`, `skull`, `exploding`, `campfire`, `pawns`, `hexagon_in`, `arrow_right_curve`, `lock_closed`, `shield`, `hourglass`, `sword`, `bow` | icônes **blanches** (à teinter avec `DrawTexture` + couleur) : ressources, catégories de construction, pouvoirs |
| Kenney UI Pack | https://kenney.nl/media/pages/assets/ui-pack/f651646eab-1718203990/kenney_ui-pack.zip | 1,2 Mo | `assets/ui/panels/` | dans `PNG/Grey/Default/` : `button_square_depth_flat`, `button_rectangle_depth_flat`, `button_rectangle_depth_border`, `slide_horizontal_color` (+ les mêmes dans `PNG/Red/Default/` et `PNG/Yellow/Default/`) | fond des cases de pouvoir, barre de construction, boutons des menus et du marchand |
| Kenney Game Icons | https://kenney.nl/media/pages/assets/game-icons/1ebf9c14af-1677661579/kenney_game-icons.zip | 1,0 Mo | `assets/ui/icons/` (optionnel) | dans `PNG/White/2x/` : `gear`, `pause`, `trophy`, `wrench`, `trashcan`, `return`, `star`, `warning`, `mouse` | menus, options, revente (`trashcan`), réparation (`wrench`), annulation (`return`) |

**Accents** (vérifié dans la table `cmap` des TTF) : **Kenney Future** et **Kenney Pixel** couvrent U+0020-007E et **U+00A1-00FF** (é è ê à â ç ù ô î ï ë É È À Ç « »), ainsi que U+2018-201E (apostrophe ’). **Pas de « œ »** (U+0153) : écrire « oe ». raylib ne charge que l'ASCII par défaut : `LoadFontEx(path, 32, codepoints, n)` avec les codepoints 32-255 (+ 0x2019), puis `DrawTextEx` ; les chaînes UTF-8 du code source passent telles quelles.

### Sons

| Pack | URL directe | Taille | Sélection → `assets/sounds/` | Usage |
|---|---|---|---|---|
| Zombies Sound Pack (artisticdude, OGA) | https://opengameart.org/sites/default/files/zombies.zip | 4,7 Mo | `zombies/zombie-1.wav` … `zombie-24.wav` : garder 8 cris variés → `zombie_scream_0..7.wav` (ignorer `__MACOSX/`) | cris d'attaque, coureurs, élites |
| Zomby sfx pack (saturn91, OGA) | https://opengameart.org/sites/default/files/01_zombey.mp3 (et `02_zombey.mp3`, `03_zombey.mp3`, `04_zombey_0.mp3`, `06_zombey.mp3`, `07_zombey.mp3`, même dossier) | 22 Ko chacun | `zombie_call_0..5.mp3` | appels courts ; pitch aléatoire 0,5-1,5 conseillé par l'auteur |
| Kenney RPG Audio | https://kenney.nl/media/pages/assets/rpg-audio/8e99002d76-1677590336/kenney_rpg-audio.zip | 965 Ko | `Audio/chop.ogg`, `Audio/metalClick.ogg`, `Audio/metalLatch.ogg`, `Audio/doorOpen_1.ogg`, `Audio/doorClose_1.ogg`, `Audio/creak1.ogg`, `Audio/handleCoins.ogg`, `Audio/drawKnife1.ogg` | pose / réparation (bois, métal), porte, achat, piège qui claque |
| Kenney Sci-fi Sounds | https://kenney.nl/media/pages/assets/sci-fi-sounds/6b296f9ecf-1677589334/kenney_sci-fi-sounds.zip | 5,9 Mo | `Audio/engineCircular_000.ogg`, `Audio/thrusterFire_000.ogg`, `Audio/explosionCrunch_000.ogg`, `Audio/lowFrequency_explosion_000.ogg`, `Audio/forceField_000.ogg`, `Audio/laserSmall_000.ogg`, `Audio/impactMetal_000.ogg` | générateur (boucle), lance-flammes (boucle), mine, tonneau de poudre, Glas, rayon de la lanterne, coups sur le métal |
| Kenney Music Jingles | https://kenney.nl/media/pages/assets/music-jingles/f37e530b9e-1677590399/kenney_music-jingles.zip | 1,2 Mo | `Audio/Steel jingles/jingles_STEEL00.ogg`, `jingles_STEEL01.ogg` | palier de pouvoir, nouveau pouvoir |

### Musique / ambiance

| Piste | URL directe | Taille | Fichier cible (`assets/music/`) | Usage |
|---|---|---|---|---|
| Crickets Ambient Noise, loopable (wolfgang, OGA) | https://opengameart.org/sites/default/files/crickets_1.mp3 | 184 Ko | `crickets.mp3` | grillons pendant les pauses (boucle de 11 s) |
| Wind whoosh loop (sketchman3, OGA) | https://opengameart.org/sites/default/files/wind%20woosh%20loop.ogg | 252 Ko | `wind.ogg` | vent de nuit en boucle, sous la musique |
| Cynic Battle Loop (ferk, OGA) | https://opengameart.org/sites/default/files/cynicbattleloop_0.ogg | 1,9 Mo | `combat.ogg` | combat (≥ 8 morts en vie) |
| Battle Theme A (cynicmusic, OGA) | https://opengameart.org/sites/default/files/battleThemeA.mp3 | 3,3 Mo | `combat_2.mp3` | variante de combat, vagues 6+ |
| Hard Boss Battle 1 (MintoDog, OGA) | https://opengameart.org/sites/default/files/hard_boss_battle_1_bpm200_0.ogg | 2,8 Mo | `boss_1.ogg` | Le Fossoyeur (vague 5) |
| Hard Boss Battle 2 (MintoDog, OGA) | https://opengameart.org/sites/default/files/hard_boss_battle_2_bpm210_0.ogg | 3,9 Mo | `boss_2.ogg` | Le Rongé (vague 10) |

**Musique qui monte en intensité** (tâche C2) : pendant les pauses `night_ambience.ogg` + `crickets` + `wind` → fondu de 2 s vers `combat` quand ≥ 8 morts sont en vie → `boss_*` pendant un boss → retour au calme 4 s après la fin de la vague. raylib lit l'OGG, le MP3 (dr_mp3) et le WAV nativement.

Écartés pour la licence : *ULTIMATE ZOMBIE TAUNT SCREAM PACK* (affiché CC0 mais généré par IA à partir de sources non libres), *Horror Sound Effects Library* et *ZombiePackSFX 3* (CC-BY). On reste 100 % CC0.

Crédits à ajouter dans `CREDITS.md` : artisticdude, saturn91, wolfgang, sketchman3, ferk, cynicmusic, MintoDog (OpenGameArt) ; Kenney pour tous les packs Kenney.
