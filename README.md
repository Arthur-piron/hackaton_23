# LAST LIGHT (hackaton_23)

Survie zombie en 3D vue de dessus, C++17 / raylib 5.5, pour Linux et Windows. Design du jeu : `docs/GAME.md`.

> Panne générale, la nuit, dans un vieux cimetière. Tu es le gardien, dernier vivant. **Les morts craignent la lumière** : ta lampe les ralentit et les brûle, mais sa batterie se vide. Tiens jusqu'à l'aube : ramasse l'or des morts, ouvre les grilles du cimetière, choisis tes cartes… et ne te fais pas encercler.

## Jouer

Télécharger la dernière version dans **Releases** : `LastLight-windows.zip` ou `LastLight-linux.tar.gz`. Décompresser, puis lancer `lastlight.exe` (Windows) ou `./lastlight` (Linux). Le dossier `assets/` doit rester à côté de l'exécutable.

| Entrée | Action |
|---|---|
| ZQSD / WASD | Se déplacer |
| Maj | Sprint (endurance) |
| Souris | Viser |
| Clic gauche | Tirer |
| Clic droit | Lampe torche (vide la batterie) |
| 1-4 / molette | Changer d'arme |
| R | Recharger |
| E | Interagir (grille, marchand) |
| Entrée | Entre deux vagues : lancer la suivante tout de suite |
| Échap | Pause |

La progression (ossements, talents, records) est enregistrée dans `lastlight_save.txt`, à côté de l'exécutable ou à défaut dans le dossier utilisateur.

## Publier une version

Pousser un tag `v*` sur `main` : le workflow `.github/workflows/release.yml` compile Linux et Windows et publie la release avec les deux archives.

```
git tag v1.0.0
git push origin v1.0.0
```

## Docker (Linux)

```
docker build --target export --output dist .   # exécutable Linux + assets dans dist/
docker build -t lastlight .                     # image jouable (X11 de l'hôte, voir l'en-tête du Dockerfile)
```

## Compiler et lancer

```
cmake -S . -B build
cmake --build build --config Release
```

L'exécutable est dans `build/` (Linux) ou `build/Release/` (Windows), avec le dossier `assets/` copié à côté.
Prérequis : CMake 3.20+, un compilateur C++17 (Visual Studio Build Tools 2022 ou g++), et sous Linux les paquets de développement X11, OpenGL et ALSA (liste dans `.github/workflows/build.yml`).

Test automatique : `lastlight --smoke 120 capture.png` lance une partie, simule 120 images, enregistre une capture dans le dossier courant et quitte.

## Équipe

Projet d'équipe à 3. Chaque membre travaille avec son propre Claude (Claude Code terminal, web ou Remote Control depuis le téléphone).

## Règles de travail

1. **Personne ne pousse sur `main`.** `main` est protégée : PR obligatoire et approbation d'un code owner (`.github/CODEOWNERS`) requise.
2. **Une tâche = une branche `feature/<nom>` = une PR.**
3. **Chaque PR met à jour `docs/COMMUNICATION.md`** : statut de la tâche et une ligne dans le journal.
4. **On ne fusionne jamais sa propre PR.** Un autre membre (ou son Claude) la relit, l'approuve et la fusionne.
5. Les questions qui bloquent vont dans `docs/COMMUNICATION.md` (section « Questions ouvertes ») ou dans une issue GitHub.

## Communication entre les Claude

Les Claude ne se parlent pas directement. Le repo est le canal :

- `docs/COMMUNICATION.md` : tableau commun (membres, leader, tâches, décisions, questions, journal). Chaque Claude le lit au démarrage.
- Issues et PR GitHub : discussion sur une tâche précise.
- Discord (optionnel) : un webhook GitHub envoie les notifications du repo dans un salon pour que tout le monde suive.

## Démarrer une session

Colle ce brief dans ton Claude :

```
Projet d'équipe à 3 sur le repo GitHub privé https://github.com/Arthur-piron/hackaton_23.
Lis CLAUDE.md, README.md et docs/COMMUNICATION.md avant toute action.
Règles : ne jamais pousser sur main, une branche feature/<nom> par tâche,
une PR par tâche, mettre à jour docs/COMMUNICATION.md dans chaque PR,
ne jamais fusionner une PR soi-même (le leader valide).

Aujourd'hui, à faire : [décris la tâche ici].
```

## @claude dans les PR (optionnel)

Le workflow `.github/workflows/claude.yml` répond aux mentions `@claude` dans les issues et PR. Il demande le secret `ANTHROPIC_API_KEY` dans les paramètres du repo. Cette clé API est facturée à part et n'est pas incluse dans l'abonnement Pro.
