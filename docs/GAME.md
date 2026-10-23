# LAST LIGHT — design du jeu

Survie zombie en 3D vue de dessus, C++ / raylib, livré en `.exe` Windows.

## Pitch

Panne générale, la nuit. Tu es le dernier survivant d'un quartier envahi. **Les zombies craignent la lumière** : ta lampe torche les ralentit et les brûle, mais sa batterie se vide. Tiens 10 vagues jusqu'à l'aube en ramassant piles, munitions et ferraille, et en rallumant des groupes électrogènes qui créent des zones sûres.

## Contrôles

| Touche | Action |
|---|---|
| ZQSD (et WASD) | Se déplacer |
| Souris | Viser (rayon caméra → sol) |
| Clic gauche | Tirer |
| Clic droit | Lampe torche (vide la batterie) |
| E | Activer un groupe électrogène / ramasser |
| R | Recharger |
| Échap | Pause |

## Boucle de jeu

1. Une vague sort du noir par les bords de la map.
2. Survivre : balles, lumière, déplacement.
3. Les zombies lâchent piles, munitions, ferraille.
4. Entre deux vagues (20 s) : rallumer un groupe électrogène (ferraille) ou choisir 1 amélioration parmi 3.
5. Vague 10 : boss. Puis l'aube : victoire.

## Zombies

| Type | Comportement |
|---|---|
| Marcheur | Lent, en groupe |
| Coureur | Rapide, fragile, fonce en ligne droite |
| Brute | Beaucoup de vie, insensible à la lumière |
| Hurleur | Éteint la lampe et les groupes proches en criant |
| Boss « Le Rongé » | Énorme, fait apparaître des marcheurs |

## Choix techniques

- C++17, raylib 5.5 récupérée par CMake (`FetchContent`), liée en statique → un seul `.exe`.
- Pas de moteur maison : des classes simples appelées depuis la boucle de `main.cpp`.
- Collisions : boîtes et sphères raylib (`CheckCollisionBoxes`, `GetRayCollision*`). Pas de moteur physique.
- **Graphismes : d'abord des primitives raylib** (cubes, cylindres, sphères, style low-poly). Les modèles `.glb` (Quaternius, Kenney, CC0) viennent en bonus et vont dans `assets/`.
- États du jeu : `Menu`, `Playing`, `Paused`, `GameOver`, `Victory`.

## Architecture

```
src/
  main.cpp          boucle, états du jeu                   (leader)
  Player.*          déplacement, visée, vie, batterie       (A)
  Weapon.*          tirs, projectiles, recharge             (A)
  Flashlight.*      cône de lumière, effet sur les zombies  (A)
  Zombie.*          types, IA, animations                   (B)
  WaveManager.*     vagues, apparition, difficulté          (B)
  World.*           map, décor, collisions, groupes élec.   (C)
  Pickup.*          objets au sol                           (C)
  Hud.*             interface, menus                        (C)
  Assets.*          chargement centralisé modèles/sons/shaders (C)
assets/
CMakeLists.txt
```

## Étapes (chaque ligne ≈ un tour de 30 min)

**Socle (leader)** : squelette CMake + fenêtre + sol + caméra vue de dessus + cube joueur qui bouge.

**A — Joueur & combat**
1. Player : déplacement ZQSD, rotation vers la souris
2. Weapon : tir de projectiles, munitions, recharge
3. Vie du joueur, dégâts reçus, game over
4. Flashlight : cône, batterie, effet ralentissement/brûlure (API exposée aux zombies)
5. Améliorations entre les vagues

**B — Zombies & vagues**
1. Zombie marcheur : poursuite du joueur, vie, mort
2. Séparation entre zombies (ils ne se superposent pas), dégâts au contact
3. WaveManager : vagues croissantes, apparition aux bords, pause de 20 s
4. Coureur et Brute
5. Réaction à la lumière, Hurleur
6. Boss

**C — Monde & présentation**
1. World : map fermée (rue, maisons, voitures en cubes), collisions joueur/décor
2. Hud : vie, munitions, batterie, vague, écran game over
3. Pickup : piles, munitions, ferraille lâchées par les zombies
4. Groupes électrogènes et zones de lumière
5. Éclairage nuit (shader), menus, son
6. Build release de l'`.exe` + README de lancement

## Planning du weekend

- **Samedi matin** : socle fusionné.
- **Samedi soir** : jeu minimum jouable (joueur, tir, marcheurs, vagues, HUD, game over).
- **Dimanche matin** : lampe, éclairage, nouveaux zombies, objets.
- **Dimanche après-midi** : effets, son, équilibrage. **Plus de nouvelle fonctionnalité 2 h avant la fin** : uniquement corrections, build et démo.
