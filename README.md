# hackaton_23

Projet d'équipe à 3. Chaque membre travaille avec son propre Claude (Claude Code terminal, web ou Remote Control depuis le téléphone).

## Règles de travail

1. **Personne ne pousse sur `main`.** `main` est protégée : PR obligatoire et approbation du leader requise.
2. **Une tâche = une branche `feature/<nom>` = une PR.**
3. **Chaque PR met à jour `docs/COMMUNICATION.md`** : statut de la tâche et une ligne dans le journal.
4. **Seul le leader fusionne.** Aucun membre, et aucun Claude, ne fusionne sa propre PR.
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
