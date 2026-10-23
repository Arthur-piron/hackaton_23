# LAST LIGHT — design du jeu (v2)

Survie zombie en 3D vue de dessus, C++17 / raylib 5.5. **Doit tourner sous Linux ET Windows**, livré en exécutable sur les deux.

## Ambition

Pas « un jeu qui marche » : **un jeu profond qui en met plein la vue**. Rendu nocturne avec vraies lumières, particules, sang, secousses d'écran, son spatialisé, animations ; et surtout **des choix à chaque instant** : économie, cartes, marchand, zones à ouvrir, progression entre les parties.
Priorités, dans l'ordre : **(1) ça compile partout, (2) c'est jouable, (3) c'est profond, (4) c'est beau, (5) il y a du contenu**. On ne casse jamais `main`.

## Pitch

Panne générale, la nuit, dans un vieux cimetière. Tu es le gardien (`character-keeper`), dernier vivant. **Les morts craignent la lumière** : ta lampe torche les ralentit et les brûle, mais sa batterie se vide. Tiens jusqu'à l'aube : ramasse l'or des morts, achète au marchand, ouvre les grilles du cimetière, choisis tes cartes… et ne te fais pas encercler.

## Contrôles

| Entrée | Action |
|---|---|
| ZQSD / WASD | Se déplacer |
| Maj | Sprint (endurance) |
| Souris | Viser (rayon caméra → sol) |
| Clic gauche | Tirer |
| Clic droit | Lampe torche (vide la batterie) |
| 1-4 / molette | Changer d'arme |
| R | Recharger |
| E | Interagir : grille, marchand, boîte mystère, lampadaire, construction |
| Tab | Récapitulatif (cartes, stats) |
| Échap | Pause |

---

## 1. Les trois ressources

| Ressource | Source | Usage |
|---|---|---|
| 🪙 **Or** | Lâché par chaque mort (pièces au sol, aimantées dans un rayon de 3 m, disparaissent après 20 s) | Marchand, grilles, boîte mystère, constructions |
| ✨ **XP** | Chaque mort (orbe aimanté avec l'or) | Montée de niveau → **choix d'1 carte parmi 3** |
| 🦴 **Ossements** | Élites (1-3), mini-boss (10), boss (25), fin de partie (1 par vague survécue) | Progression permanente (sauvegardée) |

### Gains

| Ennemi | Or | XP |
|---|---|---|
| Marcheur | 5 | 3 |
| Coureur | 6 | 4 |
| Squelette | 7 | 5 |
| Vampire | 10 | 7 |
| Spectre | 12 | 8 |
| Brute | 30 | 20 |
| Élite | ×3 | ×3 |
| Mini-boss / Boss | 200 / 500 | 100 / 250 |

### Combo

Chaque mort dans les **2 s** qui suivent la précédente augmente le combo. Multiplicateur d'or : **×1** (0-4), **×2** (5-9), **×3** (10-19), **×4** (20-34), **×5** (35+). Affiché en grand près du HUD, avec une jauge qui se vide. Prendre des dégâts divise le combo par deux.

### Niveaux

XP nécessaire pour passer du niveau n à n+1 : `20 + 15 × n`. À la montée de niveau, le temps se fige (ralenti puis pause), 3 cartes sont tirées au sort selon leur rareté : **commune 70 %, rare 25 %, légendaire 5 %** (+1 % par niveau de talent « Chance »). Une carte déjà prise peut revenir et s'**empile** (jusqu'à 3 rangs, sauf indication).

---

## 2. Les cartes (26)

| # | Carte | Rareté | Effet (par rang) | Max |
|---|---|---|---|---|
| 1 | Main sûre | Commune | Dégâts +12 % | 3 |
| 2 | Gâchette nerveuse | Commune | Cadence +10 % | 3 |
| 3 | Pas de loup | Commune | Vitesse +15 % | 3 |
| 4 | Peau dure | Commune | Vie max +25 (et soigne 25) | 3 |
| 5 | Poches profondes | Commune | Chargeurs +25 % | 3 |
| 6 | Main leste | Commune | Rechargement −15 % | 3 |
| 7 | Avare | Commune | Or +15 % | 3 |
| 8 | Aimant | Commune | Rayon d'aimantation +50 % | 2 |
| 9 | Pile longue durée | Commune | Batterie de lampe +30 % | 3 |
| 10 | Second souffle | Commune | Endurance +30 %, récupération +20 % | 2 |
| 11 | Balles incendiaires | Rare | Les balles enflamment : 6 dégâts/s pendant 3 s | 1 |
| 12 | Ricochet | Rare | Une balle qui tue rebondit vers l'ennemi le plus proche (8 m) | 2 |
| 13 | Perforation | Rare | Les balles traversent +1 ennemi | 2 |
| 14 | Lampe UV | Rare | Le cône de lumière inflige 15 dégâts/s | 2 |
| 15 | Vampirisme | Rare | Chaque mort rend 1 % de la vie max | 3 |
| 16 | Chargeur sans fond | Rare | 10 % de chances de ne pas consommer de balle | 3 |
| 17 | Lanterne volante | Rare | Une lanterne orbite autour de toi (rayon 2,5 m), éclaire et brûle | 2 |
| 18 | Explosion nécrotique | Rare | 20 % des morts explosent (rayon 2,5 m, 40 dégâts) | 2 |
| 19 | Gel | Rare | Les balles ralentissent de 30 % pendant 1,5 s | 1 |
| 20 | Coup critique | Rare | 10 % de chances de dégâts ×3 (texte jaune) | 3 |
| 21 | Dernier rempart | Légendaire | Une fois par vague, un coup mortel laisse à 1 PV et repousse tout le monde | 1 |
| 22 | Chaîne de feu | Légendaire | **Synergie** (Incendiaires + Ricochet requises) : les rebonds enflamment et se propagent à 3 cibles | 1 |
| 23 | Soleil de poche | Légendaire | Toutes les 20 s, un flash de lumière brûle tout dans un rayon de 8 m | 1 |
| 24 | Pluie de plomb | Légendaire | Chaque tir part en double, dispersion +5° | 1 |
| 25 | Faucheuse | Légendaire | Les ennemis sous 15 % de vie meurent au contact d'une balle | 1 |
| 26 | Fortune du mort | Légendaire | Chaque combo ×5 fait pleuvoir 50 pièces | 1 |

Les cartes sont **des données** (table dans `game/Cards.cpp`) et des **crochets** dans le combat (`onHit`, `onKill`, `onShot`, `onDamaged`, `onWaveStart`), pour en ajouter facilement.

---

## 3. Le marchand

Accessible **entre les vagues** (écran `Shop`, touche E près de la cabane du gardien ; la chapelle débloque un 2ᵉ marchand avec des articles rares). Le temps de pause (20 s) continue de s'écouler pendant les achats.

### Armes

| Arme | Prix | Modèle | Rôle |
|---|---|---|---|
| Revolver | départ | `blaster-a` | munitions infinies, précis |
| Fusil à pompe | 400 | `blaster-d` | 9 plombs, recul, contrôle de foule |
| Mitraillette | 650 | `blaster-h` | automatique, 40 balles |
| Fusil de chasse | 900 | `blaster-k` | perforant ×4, 120 dégâts |
| Lance-fusées | 1500 (chapelle) | `blaster-q` | explosion de zone, enflamme |

### Améliorations d'arme (par arme, 3 niveaux)

| Amélioration | Niv. 1 | Niv. 2 | Niv. 3 | Effet par niveau |
|---|---|---|---|---|
| Dégâts | 200 | 450 | 800 | +20 % |
| Cadence | 200 | 450 | 800 | +12 % |
| Chargeur | 150 | 350 | 600 | +30 % |

### Consommables

| Article | Prix |
|---|---|
| Soin (+50 PV) | 120 |
| Pile de lampe (recharge complète) | 80 |
| Munitions (remplit l'arme courante) | 100 |
| Relance des cartes (prochaine montée de niveau) | 150 |

### Constructions (placées avec E, aperçu fantôme)

| Construction | Prix | Effet |
|---|---|---|
| Lampadaire | 250 | Zone de lumière (rayon 6 m) : les morts y sont ralentis de 40 % |
| Barricade | 150 | Bloque le passage (300 PV), les zombies doivent la détruire |
| Tourelle-lanterne | 600 | Tire sur l'ennemi le plus proche (portée 10 m) et éclaire |

---

## 4. Le cimetière à débloquer

Carte construite avec le Graveyard Kit (+ Survival Kit). On démarre dans la **cour du gardien** (petite zone, une seule entrée pour les morts). Des **grilles** (`iron-fence-border-gate`) s'ouvrent avec E contre de l'or.

| Zone | Prix de la grille | Contenu |
|---|---|---|
| Cour du gardien | départ | Cabane + marchand, 2 lampadaires |
| Allée des tombes | 500 | Espace pour manœuvrer, 2 entrées de zombies en plus |
| Chapelle | 1000 | Marchand rare (lance-fusées, cartes), autel de soin (1 fois par vague) |
| Crypte | 1250 | Armes rares au sol, beaucoup de squelettes, coffres d'or |
| Mausolée | 1500 | **Boîte mystère** |

**Boîte mystère** (950 or) : tire une arme au hasard (y compris des variantes **légendaires** dorées : dégâts ×1,5, effet spécial). Après 4 utilisations, elle disparaît et réapparaît ailleurs (moment drôle et frustrant façon *CoD Zombies*).

Chaque zone ouverte **ajoute des points d'apparition** : plus de place pour fuir, mais plus de côtés d'où les morts arrivent.

---

## 5. Le défi

### Ennemis (Kenney Graveyard Kit)

| Type | Modèle | Comportement | Vie |
|---|---|---|---|
| Marcheur | `character-zombie` | Lent, en groupe | 60 |
| Coureur | `character-zombie` teinté rouge, ×0,92 | Rapide, fragile, `sprint` | 35 |
| Squelette | `character-skeleton` | **Insensible à la lumière** | 45 |
| Vampire | `character-vampire` | Fuit la lumière, contourne pour attaquer de dos | 80 |
| Spectre | `character-ghost` semi-transparent | Traverse les barricades, **éteint les lumières** proches | 70 |
| Brute | `character-zombie` ×1,6 teinté | Encaisse, frappe fort, insensible au recul | 420 |

### Élites

À partir de la vague 3, 5 % des ennemis (+1 % par vague) reçoivent un modificateur, un **halo coloré** et un nom :

| Modificateur | Couleur | Effet |
|---|---|---|
| Explosif | orange | Explose à sa mort (rayon 3 m, 35 dégâts, blesse aussi les zombies) |
| Blindé | gris acier | −50 % de dégâts reçus de face |
| Rapide | jaune | Vitesse ×1,8 |
| Régénérant | vert | Récupère 5 % de vie/s s'il n'est pas touché pendant 2 s |

Les élites rapportent ×3 or/XP et 1-3 ossements.

### Événements de vague (à partir de la vague 3, 1 chance sur 3, annoncés en grand)

| Événement | Effet |
|---|---|
| 🌕 Lune de sang | Ciel rouge, zombies +30 % de vitesse, **or ×2** |
| 🌫️ Brouillard | Visibilité réduite à 8 m, sons étouffés |
| 🔌 Coupure | Tous les lampadaires s'éteignent pour la vague |
| 🧟 Horde | ×3 marcheurs, plus faibles (−40 % de vie) |

### Boss

- **Mini-boss** (vague 5) : *Le Fossoyeur*, brute ×2 qui lance des pierres tombales (projectile au sol avec zone d'impact visible).
- **Boss** (vague 10) : *Le Rongé*, zombie ×3 : charge (télégraphiée par une ligne au sol), frappe au sol (onde de choc à esquiver), invoque des marcheurs. Barre de vie en haut de l'écran.

### Modes

| Mode | Règle | Déblocage |
|---|---|---|
| Nuit | 10 vagues, victoire à l'aube (le ciel s'éclaircit, les morts brûlent) | départ |
| Sans fin | Vagues infinies, score, **tableau des scores local** (top 10) | départ |
| Cauchemar | Ennemis +50 % de vie, élites ×2, or −25 % | après une victoire en Nuit |

### Difficulté

Quota de la vague n : `6 + 5n` ennemis ; maximum en même temps : `min(20 + 6n, 110)` ; intervalle d'apparition : `max(0,18 ; 0,9 − 0,07n)` s. Vie des ennemis +8 % par vague à partir de la vague 6.

---

## 6. Progression permanente

Sauvegarde : fichier texte `lastlight_save.txt` (clé=valeur) dans le dossier de l'exécutable, ou à défaut dans le dossier utilisateur (`$HOME` / `%APPDATA%`, via `getenv`). Portable Linux/Windows, sans dépendance.

### Arbre de talents (ossements)

| Talent | Rangs | Coût par rang | Effet par rang |
|---|---|---|---|
| Constitution | 5 | 5, 10, 15, 20, 25 | Vie de départ +10 |
| Héritage | 5 | 5, 10, 15, 20, 25 | Or de départ +50 |
| Chance | 3 | 10, 20, 30 | Rareté des cartes +1 % |
| Relance | 3 | 15, 30, 45 | +1 relance de cartes gratuite par partie |
| Arsenal | 1 | 40 | Commence avec le fusil à pompe |
| Lumière tenace | 3 | 10, 20, 30 | Batterie +15 % |
| Lucidité | 1 | 60 | 4 cartes au choix au lieu de 3 |

### Personnages jouables

| Personnage | Coût | Particularité |
|---|---|---|
| Gardien (`character-keeper`) | départ | Équilibré |
| Vampire (`character-vampire`) | 50 | Vitesse +20 %, vampirisme 2 % natif, ne supporte pas sa propre lampe (lampe −50 %) |
| Squelette (`character-skeleton`) | 80 | Insensible au poison et aux explosions de zombies, vie −20 % |
| Spectre (`character-ghost`) | 120 | Traverse les barricades, dash (Maj) au lieu du sprint |

---

## Choix techniques

- **C++17**, **raylib 5.5** via CMake `FetchContent`, liée en statique, runtime MSVC statique (un seul `.exe`).
  - Récupérer l'**archive**, pas le dépôt git (le clone échoue sous Windows, chemins trop longs).
- **Multiplateforme obligatoire** : aucun en-tête Windows/POSIX dans le code ; chemins avec `/` ; noms de fichiers en minuscules ; assets relatifs à l'exécutable (`assetPath()` dans `core/AssetPath`).
- **CI GitHub Actions** : build Linux (gcc) + Windows (MSVC) sur chaque PR ; sous Linux le jeu est **réellement lancé** (`--smoke` sous Xvfb) et la capture est publiée en artefact `smoke-linux`. **Une PR dont la CI est rouge ne se fusionne pas.**
- **Test automatique** : `lastlight --smoke <images> <capture.png>` lance une partie, simule, capture l'écran et quitte. À utiliser avant chaque push.
- **Échelle** : les kits Kenney sont petits (personnage ≈ 0,86 unité) → `kKenneyScale = 2` (`game/Scale.hpp`), 1 unité ≈ 1 m.
- **Événements** : les systèmes de jeu publient des `Event` (`game/Events.hpp`) ; le son, les particules, la caméra et le HUD les consomment. Pas de dépendance directe entre gameplay et présentation.
- **Docker** : `Dockerfile` multi-étapes qui compile la version Linux ; **Release** sur tag `v*` : `LastLight-linux.tar.gz` et `LastLight-windows.zip`.
- Collisions : sphères/cercles (personnages), boîtes alignées (décor), segments (balles, `segmentHitsSphere`). Pas de moteur physique.
- États de `Game` : `Menu`, `Playing`, `Paused`, `LevelUp` (choix de carte), `Shop`, `GameOver`, `Victory`, plus les écrans `Talents`, `Characters`, `Scores`, `Options`, `Credits`.

### ⚠️ Animations des personnages Kenney (vérifié)

Les GLB Kenney utilisent des animations **par nœuds** (`root`, `torso`, `head`, `arm-left`, `arm-right`, `leg-left`, `leg-right`), **sans squelette** : `LoadModelAnimations` renvoie 0. `core/AnimatedModel` les lit avec cgltf :
- `#include "external/cgltf.h"` (symboles déjà dans raylib), include de `${raylib_SOURCE_DIR}/src` (fait dans CMake).
- raylib crée un mesh par primitive, dans l'ordre des nœuds, en appliquant déjà la transformation du nœud : on dessine chaque mesh avec `inverseBind × worldAnim × objet`.
- Un `AnimatedModel` est partagé par toutes les entités du même type ; chacune garde son `AnimationState` (animation, temps, fondu, **couche partielle** `overlayClip`/`overlayMask` pour par ex. tenir une arme en marchant).
- 32 animations par personnage : `idle, walk, sprint, jump, fall, crouch, sit, drive, die, pick-up, emote-yes, emote-no, holding-right, holding-left, holding-both, holding-right-shoot, holding-left-shoot, holding-both-shoot, attack-melee-right, attack-melee-left, attack-kick-right, attack-kick-left, interact-right, interact-left, wheelchair-*`.
- Main droite ≈ (-0,26 ; 0 ; 0) dans le repère du nœud `arm-right`. Les blasters font ≈ 0,8 unité : les afficher à 0,5 × l'échelle Kenney.

## Architecture

```
src/
  main.cpp                 options (--smoke)
  Game.*                   machine à états, possède les systèmes, consomme les événements   (leader)
  core/AssetPath.*         chemins des assets                                               (leader)
  core/AnimatedModel.*     animations par nœuds (cgltf)                                     (leader)
  core/Audio.*             groupes de sons, sons 3D, musique en fondu                       (leader)
  core/Save.*              sauvegarde clé=valeur                                            (leader)
  game/Scale.hpp  game/Events.hpp
  game/Player.*            déplacement, visée, vie, endurance, animations                   (A)
  game/Weapon.*            Arsenal : armes, munitions, améliorations                        (A)
  game/Projectile.*        balles traçantes, collisions segment/sphère                      (A)
  game/Flashlight.*        cône, batterie, effet sur les ennemis                            (A)
  game/Economy.*           or, XP, niveaux, combo                                           (A)
  game/Pickups.*           pièces, orbes, objets au sol, aimantation                        (A)
  game/Cards.*             catalogue des cartes, tirage, effets (crochets)                  (A)
  game/Shop.*              catalogue et achats du marchand                                  (A)
  game/Enemy.*             types, IA, attaque, mort                                         (B)
  game/Elites.*            modificateurs d'élite                                            (B)
  game/WaveManager.*       vagues, apparitions, événements de vague                         (B)
  game/Boss.*              mini-boss et boss                                                (B)
  game/World.*             carte du cimetière, décor, collisions                            (C)
  game/Zones.*             zones, grilles payantes, points d'apparition                     (C)
  game/MysteryBox.*        boîte mystère                                                    (C)
  game/Buildings.*         lampadaires, barricades, tourelles                               (C)
  game/Progression.*       talents, personnages, ossements                                  (leader)
  fx/Lighting.*            shader nuit : lumières ponctuelles, cône, brouillard             (C)
  fx/Particles.*           particules (billboards)                                          (C)
  fx/PostFx.*              vignette, grain, aberration, flash de dégâts                     (C)
  fx/CameraRig.*           (actuellement game/CameraRig) suivi + secousse                   (leader)
  ui/Hud.*                 vie, munitions, or, combo, XP, vague                             (C)
  ui/CardPicker.*          écran de choix de cartes                                         (A)
  ui/ShopScreen.*          écran du marchand                                                (A)
  ui/Menus.*               titre, modes, talents, personnages, scores, options, crédits     (C)
assets/  tools/fetch_assets.cmake  .github/workflows/  Dockerfile  CREDITS.md
```

## Roadmap (chaque ligne ≈ une tâche de 30 min = une PR)

**Fait** : squelette (#7), CI (#8), assets (#9), AnimatedModel + gardien animé (#10).

**Leader / A — joueur, combat, économie**
1. Combat : armes + balles + collisions, vie du joueur, Game Over, HUD minimal, sons, secousse, hit-stop, arme en main, `--smoke` qui teste le combat
2. Or + XP + combo (Economy, Pickups) + affichage HUD
3. Cartes : catalogue, tirage par rareté, écran de choix, 10 premières cartes
4. Cartes : les 16 suivantes et les synergies
5. Marchand : écran Shop, armes, améliorations, consommables
6. Lampe torche : cône, batterie, effet sur les ennemis
7. Progression permanente : sauvegarde, ossements, talents, personnages

**B — ennemis, vagues, défi (Léandre)**
1. Vampire et Spectre (comportements liés à la lumière)
2. Élites : modificateurs, halo, noms
3. Événements de vague (Lune de sang, Brouillard, Coupure, Horde)
4. Mini-boss Le Fossoyeur
5. Boss Le Rongé (charge, onde de choc, invocations, barre de vie)
6. Équilibrage, modes Sans fin et Cauchemar, performance (110 ennemis à 60 FPS)

**C — monde & présentation**
1. Cimetière : carte de la cour + allée, décor, collisions
2. Zones et grilles payantes, nouveaux points d'apparition
3. Chapelle, crypte, mausolée, boîte mystère
4. Éclairage nuit (shader) : lampadaires, braseros, brouillard
5. Constructions (lampadaire, barricade, tourelle)
6. Particules (sang, étincelles, flash, pièces), post-fx
7. Menus (titre, modes, talents, personnages, scores, options, crédits)
8. Dockerfile + release + README Linux/Windows

## Planning

- **Samedi matin** : combat jouable + économie de base sur `main`.
- **Samedi soir** : cartes, marchand, cimetière, lumière : la boucle complète est jouable.
- **Dimanche matin** : élites, événements, boss, zones, progression permanente.
- **Dimanche après-midi** : effets, menus, équilibrage. **Plus de nouvelle fonctionnalité 2 h avant la fin** : corrections, release Linux + Windows, vidéo de démo.
