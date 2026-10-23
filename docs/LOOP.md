# Procédure d'un tour de /loop (toutes les 30 min)

Chaque Claude tourne seul, sans humain devant l'écran. **Ne demande jamais de validation à ton humain** : il n'est pas là. Décide, documente, avance.

Le canal entre les Claude est GitHub :

- **Issue « 📋 Ordres du leader »** (titre exact) : le leader y poste les ordres à chaque tour, les membres y répondent.
- **Commentaires de PR** : reviews et rapports de fin de tour.
- `docs/GAME.md` : le design du jeu (référence commune), avec les pièges techniques déjà vérifiés.
- `docs/ASSETS.md` : les assets CC0 et leurs liens directs.
- `docs/COMMUNICATION.md` : membres, décisions, questions ouvertes. Seul le leader le modifie (évite les conflits).

## Les rôles

| Rôle | Qui | Partie du jeu (voir `docs/GAME.md`) |
|---|---|---|
| Leader | Arthur (@Arthur-piron) | Squelette, intégration, Joueur & combat (A) |
| Membre | Léandre (@T3a1) | Zombies & vagues (B) |
| Membre | 3ᵉ membre | Monde & présentation (C). S'il n'y a que 2 membres actifs, le leader répartit C. |

Le leader donne les ordres. **Un ordre du leader passe avant ta propre roadmap.**

## Déroulé d'un tour (≈ 25 min de travail max)

### 1. Synchroniser (2 min)
```
git fetch origin
gh issue list --state open
gh pr list --state open
```
Lis le **dernier commentaire du leader** dans l'issue « 📋 Ordres du leader » et les commentaires récents sur les PR ouvertes.

### 2. Reviewer les PR des autres (5 min max par PR)
Pour chaque PR ouverte **qui n'est pas la tienne** :
1. `gh pr diff <n>`, `gh pr checks <n>` (CI Linux + Windows, dès qu'elle existe), puis `gh pr checkout <n>` et **compile + lance le jeu quelques secondes**.
2. Vérifie aussi : rien de spécifique à Windows (Linux doit compiler), chemins d'assets relatifs, pas de régression visible.
3. Si c'est bon et la CI verte : `gh pr review <n> --approve -b "<résumé de la review>"` puis `gh pr merge <n> --squash --delete-branch`. Si la CI tourne encore, repasse au tour suivant.
4. Si ça ne compile pas ou si c'est faux : `gh pr review <n> --request-changes -b "<problème précis + correction attendue>"`. Ne fusionne pas.
5. Petit défaut (nommage, oubli mineur) : approuve et fusionne quand même, et signale-le en commentaire.

**Ne fusionne jamais ta propre PR.** Ne demande pas à un humain de la valider.

### 3. Traiter les retours sur tes PR
Si on t'a demandé des changements : corrige sur la même branche, pousse, réponds au commentaire. C'est prioritaire sur le nouveau code.

### 4. Mini-roadmap (1 min)
Choisis **une seule** tâche faisable en ~20 min : l'ordre du leader en priorité, sinon la prochaine étape de ta partie dans `docs/GAME.md`. Si ta tâche dépend d'une PR pas encore fusionnée, pars de cette branche ou prends une autre tâche.

### 5. Coder (≈ 15-20 min)
```
git switch -c feature/<nom-court> origin/main
```
- Code propre, dans le style existant. Un fichier par classe (`src/Zombie.hpp/.cpp`…).
- **Ne touche pas aux fichiers d'un autre rôle** sans ordre du leader. `main.cpp` et `CMakeLists.txt` : modifs minimales et signalées.
- **Ça doit compiler** avant de pousser : `cmake -S . -B build && cmake --build build`.

### 6. Commit, push, PR, rapport (3 min)
Commits au format **Conventional Commits**, en français :
```
feat(zombie): ajoute la poursuite du joueur
fix(player): corrige la visée quand la caméra tourne
refactor(world): extrait le chargement des assets
```
Types : `feat`, `fix`, `refactor`, `docs`, `chore`, `build`, `style`.

Pousse (`git push -u origin feature/<nom>`), ouvre la PR avec le template, puis **commente dans l'issue « 📋 Ordres du leader »** :
```
**Tour <heure> — <ton rôle>**
Fait : …
PR : #<n> (à reviewer)
Reviewé/fusionné : #…
Prochaine étape pour moi : …
Besoin des autres : …   (ex. « B : il me faut Zombie::isAlive() »)
```

Si tu n'as pas fini en 25 min : pousse quand même ce qui compile, ouvre la PR en **draft** (`gh pr create --draft`) et dis-le dans le rapport.

## Spécifique au leader

À chaque tour, en plus de ce qui précède :
- Si l'issue « 📋 Ordres du leader » n'existe pas, crée-la.
- Après la review, poste **un commentaire d'ordres** dans l'issue, avec une tâche de ~20 min par membre :
  ```
  **Ordres — tour <heure>**
  @T3a1 (B) : …
  @<membre C> (C) : …
  Leader (A) : …
  Priorité générale : …
  ```
- Tiens `docs/COMMUNICATION.md` à jour (décisions, membres, journal) dans tes propres PR.
- Surveille le planning de `docs/GAME.md` : on arrête les nouvelles fonctionnalités 2 h avant la fin.

## Interdits

- Pousser sur `main`, force-push, fusionner sa propre PR.
- Supprimer ou réécrire le code d'un autre sans le dire.
- Demander une validation humaine : tu es seul, décide.
