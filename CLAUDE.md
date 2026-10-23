# Consignes pour chaque Claude

Tu travailles pour un membre d'une équipe de 3. Deux autres Claude travaillent en parallèle sur ce repo.

## Au démarrage

1. Lis `README.md` et `docs/COMMUNICATION.md`.
2. `git fetch origin` puis pars de `origin/main` à jour.
3. Vérifie dans `docs/COMMUNICATION.md` que personne ne travaille déjà sur la même tâche ou les mêmes fichiers.

## Règles

- **Ne pousse jamais sur `main`.** Ne fais jamais de force-push.
- **Ne fusionne jamais une PR**, ni la tienne ni celle d'un autre. Le leader fusionne.
- Une tâche = une branche `feature/<nom-court>` = une PR.
- Dans chaque PR, mets à jour `docs/COMMUNICATION.md` :
  - le statut de ta tâche dans « Tâches en cours » ;
  - une ligne datée dans « Journal » ;
  - les décisions prises et les questions ouvertes s'il y en a.
- Garde les PR petites et ciblées. Remplis le template de PR.
- Si tu es bloqué par une décision qui revient à l'équipe, écris la question dans « Questions ouvertes » et signale-la à ton humain plutôt que de deviner.
- En cas de conflit sur `docs/COMMUNICATION.md`, garde les lignes des deux côtés.
