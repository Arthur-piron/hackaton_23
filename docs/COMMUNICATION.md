# Tableau de communication

Membres, décisions et questions de fond. **Seul le leader le modifie** (dans ses PR), pour éviter les conflits. Les ordres et les rapports de chaque tour passent par l'issue « 📋 Ordres du leader ». La procédure d'un tour est dans `docs/LOOP.md`, le jeu dans `docs/GAME.md`.

## Membres

| Membre | GitHub | Rôle | Partie du jeu |
|---|---|---|---|
| Arthur | @Arthur-piron | Leader | Socle, intégration, A (joueur & combat) |
| Léandre | @T3a1 | Membre | B (zombies & vagues) |
| ? | @? | Membre | C (monde & présentation) |

## Leader

**Arthur (@Arthur-piron)** : donne les ordres dans l'issue « 📋 Ordres du leader ». Chaque Claude reviewe et fusionne les PR des autres, jamais la sienne.

## Tâches en cours

| Tâche | Branche | Responsable | Statut |
|---|---|---|---|
| Onboarding de Léandre | `feature/onboarding-leandre` | Léandre (@T3a1) | fusionnée |
| Workflow /loop + design du jeu | `feature/workflow-loop` | Arthur (@Arthur-piron) | fusionnée (#5) |
| Squelette CMake + raylib | `feature/squelette` | Arthur (@Arthur-piron) | fusionnée (#7) |
| CI Linux + Windows | `feature/ci` | Arthur (@Arthur-piron) | fusionnée (#8) |
| Assets CC0 | `feature/assets` | Arthur (@Arthur-piron) | fusionnée (#9) |
| AnimatedModel + gardien animé | `feature/animator` | Arthur (@Arthur-piron) | fusionnée (#10) |
| Design v2 | `feature/design-v2` | Arthur (@Arthur-piron) | PR ouverte |
| Combat (armes, zombies, vagues, HUD) | `feature/combat` | Arthur (@Arthur-piron) | en cours |

Statuts : `à faire`, `en cours`, `PR ouverte`, `fusionnée`, `bloquée`. Le suivi tour par tour se fait dans l'issue.

## Décisions

| Date | Décision | Par |
|---|---|---|
| 2026-10-23 | Le repo sert de canal entre les Claude ; `main` protégée. | Arthur |
| 2026-10-23 | Jeu : **LAST LIGHT**, survie zombie 3D vue de dessus en C++17 / raylib, livré en `.exe` (voir `docs/GAME.md`). | Arthur |
| 2026-10-23 | Travail autonome en `/loop` de 30 min : chaque Claude reviewe et fusionne les PR des autres, jamais la sienne, sans validation humaine. Remplace « seul le leader fusionne ». | Arthur |
| 2026-10-23 | Ordres et rapports dans l'issue « 📋 Ordres du leader » ; seul le leader modifie ce fichier. | Arthur |
| 2026-10-24 | Ambition maximale ; Linux + Windows obligatoires (CI sur chaque PR, Docker, release). Assets Kenney / OpenGameArt CC0 listés dans `docs/ASSETS.md`. | Arthur |
| 2026-10-24 | **Design v2** : or + combo, XP et cartes (26), marchand, cimetière à débloquer avec grilles payantes et boîte mystère, élites, événements de vague, boss, modes Nuit / Sans fin / Cauchemar, progression permanente (ossements, talents, personnages). Voir `docs/GAME.md`. | Arthur |
| 2026-10-24 | Rôles v2 : leader = joueur, combat, économie, cartes, marchand, progression ; Léandre = ennemis, élites, événements, boss ; 3ᵉ membre = monde, zones, lumière, effets, menus. | Arthur |

## Questions ouvertes

- Pseudo GitHub du 3ᵉ membre (à ajouter ici et dans `.github/CODEOWNERS`).

## Journal

| Date | Qui | Quoi |
|---|---|---|
| 2026-10-23 | Arthur | Mise en place du repo : règles, consignes Claude, permissions, workflow @claude, template de PR. |
| 2026-10-23 | Léandre | Onboarding : pseudo GitHub @T3a1 ajouté dans « Membres ». |
| 2026-10-23 | Arthur | Workflow /loop (`docs/LOOP.md`), design du jeu (`docs/GAME.md`), CODEOWNERS ouvert aux membres. |
| 2026-10-24 | Arthur | Mode solo de nuit (fusion de ses propres PR jusqu'à 7h, approbations à 0 sur main). Squelette : CMake + raylib 5.5, machine à états, caméra qui suit, joueur ZQSD, mode `--smoke` de test automatique. |
| 2026-10-24 | Arthur | CI Linux + Windows (#8), assets CC0 (#9), AnimatedModel + gardien animé (#10). Design v2 rédigé dans `docs/GAME.md`. |
