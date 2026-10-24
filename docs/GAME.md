# LAST LIGHT — design du jeu (v3)

> v2 : sections 1 à 6. **v3** : la base à fortifier (§7) et les pouvoirs actifs (§8), plus les tâches « v3 » de la roadmap. En cas de contradiction, la v3 l'emporte.

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
| E | Interagir : grille, marchand, boîte mystère, générateur (verser un bidon), établi, allumer une flaque d'huile |
| Tab | Récapitulatif (cartes, pouvoirs, stats) |
| Échap | Pause (en mode construction : quitte le mode) |
| Entrée | Pendant la pause entre vagues : lancer la vague tout de suite |
| **B** (v3) | Mode construction (§7.2) |
| **Espace / F / C** (v3) | Pouvoirs 1 / 2 / 3 (§8.1) |

En mode construction (v3) : clic gauche = poser, 1-4 = catégorie, molette = élément, **T** = tourner, **X** = revendre, **R** = réparer, **U** = améliorer, **Retour arrière** = annuler la dernière pose, clic droit / B / Échap = quitter.

> **Claviers** : raylib lit des positions **physiques QWERTY** (`KEY_W` = touche Z en AZERTY, `KEY_A` = Q, `KEY_Q` = A, `KEY_Z` = W). Toutes les touches v3 (B, C, F, R, T, U, X, Espace, Retour arrière) sont à la même place et portent la même lettre en AZERTY et en QWERTY : les libellés affichés sont donc toujours justes. Ne jamais lier une action à `KEY_Q`, `KEY_W`, `KEY_A`, `KEY_Z`, `KEY_M` ni à la ponctuation.

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

> **Remplacé par la v3 (§7)** : les constructions coûtent du bois, de la ferraille et de l'or, se posent en mode construction (B) et la tourelle-lanterne devient les trois tourelles du §7.3. Le marchand vend en plus (v3) : bidon de carburant **60**, 10 bois **80**, 5 ferrailles **100**, kit de réparation (toutes les structures +30 % PV) **120**.

| Construction (v2, obsolète) | Prix | Effet |
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
| 🪵 Termites (v3, vague 4+) | Les morts font ×2 aux structures, le bois lâché est doublé |

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
| Initié (v3) | 1 | 25 | 4 pouvoirs proposés au départ au lieu de 3 |
| Bâtisseur (v3) | 3 | 10, 20, 30 | +10 bois et +3 ferrailles au départ |

### Personnages jouables

| Personnage | Coût | Particularité |
|---|---|---|
| Gardien (`character-keeper`) | départ | Équilibré |
| Vampire (`character-vampire`) | 50 | Vitesse +20 %, vampirisme 2 % natif, ne supporte pas sa propre lampe (lampe −50 %) |
| Squelette (`character-skeleton`) | 80 | Insensible au poison et aux explosions de zombies, vie −20 % |
| Spectre (`character-ghost`) | 120 | Traverse les barricades, dash (Maj) au lieu du sprint |

---

## 7. La base (v3)

La cour du gardien devient **une base à fortifier**. Pendant les vagues, on ramasse du bois et de la ferraille ; entre les vagues, on construit des murs, des tourelles et des pièges. Les morts cassent ce qui leur barre la route. L'or sert aussi au marchand et aux grilles : chaque construction est un vrai choix.

### 7.1 Nouvelles ressources

| Ressource | Icône HUD (Board Game Icons) | Départ | Plafond | Source | Modèle au sol |
|---|---|---|---|---|---|
| 🪵 **Bois** | `resource_wood` | 20 | 200 | morts, caisses | `survival/resource-wood` ×0,6 |
| 🔩 **Ferraille** | `resource_iron` | 6 | 120 | morts, caisses | `survival/metal-panel-screws-half` ×0,4 |
| ⛽ **Bidon** (carburant) | `flask_full` | 1 | 6 | tonneaux, marchand (60 or) | `survival/bottle-large` teinté rouge |

- Ramassage comme l'or (`Pickups`, aimantation 3 m, jaillissement), mais **durée de vie 30 s** et **pas de multiplicateur de combo**. Événement `ResourcePicked` (value = quantité, direction.x = type).
- Compteurs dans le HUD sous l'or : `🪵 24  🔩 7  ⛽ 2` (icônes 20 px), qui sursautent au ramassage comme l'or.

#### Butin des morts

| Ennemi | Bois | Ferraille |
|---|---|---|
| Marcheur | 25 % : 1 | — |
| Coureur | — | 15 % : 1 |
| Squelette | 10 % : 1 | 30 % : 1 |
| Vampire | — | 35 % : 1 |
| Spectre | — | — |
| Brute | 100 % : 3 | 60 % : 2 |
| Élite | quantités ×2, chance 100 % | quantités ×2, chance 100 % |
| Le Fossoyeur / Le Rongé | 15 / 25 | 8 / 15 |

#### Caisses destructibles

Au début de chaque pause entre vagues, **2 + nombre de zones ouvertes** caisses apparaissent (au plus 10 au sol en même temps), sur des cases libres des zones ouvertes, à ≥ 4 m du joueur et ≥ 3 m d'un point d'apparition (petite chute depuis 3 m + nuage de poussière). Elles se cassent aux balles, aux explosions et aux pouvoirs, bloquent le passage et les balles (boîte de collision), et les morts les ignorent. Une caisse non cassée reste.

| Caisse | Modèle | Proportion | PV | Contenu |
|---|---|---|---|---|
| Caisse de bois | `survival/box` | 45 % | 25 | 3-5 bois |
| Grande caisse | `survival/box-large` | 25 % | 50 | 2-4 ferraille + 1-2 bois |
| Caisse de munitions | `blaster/crate-medium` | 15 % | 30 | 1 chargeur de réserve pour l'arme en main + 1-2 ferraille |
| Tonneau | `survival/barrel` | 10 % | 35 | 1 bidon (60 %) ou 10-20 or |
| Tonneau de poudre | `survival/barrel` teinté rouge | 5 % | 15 | **explose** : rayon 3 m, 60 dégâts aux morts, 20 au joueur |

Casse : 12 éclats (`dirt_01` brun pour le bois, `spark_01` pour le métal), son `hit_wood_*`, événement `CrateBroken`.

### 7.2 Mode construction

| Entrée | Action |
|---|---|
| **B** | Entrer / sortir du mode construction |
| 1 / 2 / 3 / 4 | Catégorie : Défenses / Tourelles / Pièges / Utilitaires |
| Molette | Élément suivant / précédent dans la catégorie |
| Clic gauche | Poser. **Maintenir et glisser** : ligne de palissades ou de barricades le long d'un axe (coût total affiché) |
| T | Tourner de 90° (projecteur : 45°) |
| X | Revendre la structure sous le curseur |
| R | Réparer la structure sous le curseur |
| U | Améliorer la structure sous le curseur (établi requis) |
| Retour arrière | Annuler la dernière pose (remboursement 100 %, seulement pendant la pause où elle a été posée) |
| Clic droit / B / Échap | Quitter le mode |

**Quand.**
- **Pendant la pause entre vagues** : tout le catalogue, prix normal, pose instantanée.
- **Pendant une vague : mode « urgence »** : seulement Défenses et Pièges, plus la réparation ; **prix ×1,5** (arrondi au-dessus) ; pose en **1 s** (barre de progression, annulée si le joueur prend un coup ou s'éloigne de plus de 0,5 m) ; revente à 25 %.
- La pause entre vagues passe de 15 s à **35 s** (première pause : **45 s**). **Entrée** lance la vague tout de suite et rapporte **+3 or par seconde restante** (« Témérité ! »). Le chrono est affiché en grand en mode construction.

**En mode construction** : arme rangée (pas de tir, pas de lampe), vitesse du joueur ×0,8, caméra reculée de 15 %, barre des pouvoirs remplacée par la barre de construction (catalogue de la catégorie, coût de chaque élément, compteurs de ressources).

**Où.**
- **Grille de 1 m** : cases centrées sur (i + 0,5 ; j + 0,5). Empreinte 1×1, 2×1 ou 3×3 cases. Les éléments 2×1 se centrent sur l'arête entre leurs deux cases.
- Constructible : case dont le centre est dans une **zone ouverte** (`Zones::zoneAt` + zone ouverte).
- Interdit : case occupée par le décor (boîtes de `World`) ou une structure ; à < 1,5 m d'une grille ; à < 3 m d'un point d'apparition ; à < 2 m du marchand, de l'autel ou de la boîte mystère ; sous le joueur ou un mort (sauf les pièges) ; hors carte.

**Aperçu.**
- Lignes de la grille (blanc, alpha 40) dans un rayon de 6 m autour du curseur ; cases interdites teintées rouge (alpha 60).
- Fantôme : le modèle de la structure teinté **vert** `{90, 255, 120, 150}` si la pose est valide, **rouge** `{255, 80, 80, 150}` sinon ; dessiné en `BLEND_ALPHA` sans écriture de profondeur (`rlDisableDepthMask`) ; rectangle de l'empreinte au sol.
- Tourelles : cercle de portée au sol ; générateur : cercle bleu de 15 m (zone alimentée) ; projecteur : cône.
- Coût au-dessus du fantôme : `4 🪵  2 🔩  150 or`, chaque nombre en rouge s'il manque ; en urgence, suffixe `×1,5`.
- Structure existante sous le curseur : contour blanc + infobulle (nom, niveau, PV, coûts de revente / réparation / amélioration).

**Revente, réparation, annulation.**
- Revente : **50 %** de chaque ressource investie (améliorations comprises, arrondi en dessous) × PV restants / PV max. Pas d'or rendu pour les pièges à usage unique déjà déclenchés.
- Réparation : 30 % du coût de base en bois et ferraille (pas d'or) × fraction de PV manquante, arrondi au-dessus, minimum 1 ressource ; instantanée pendant la pause, 2 s pendant une vague.
- Annulation (Retour arrière) : pile des poses de la pause en cours, remboursement 100 %.

**Limites.**

| Type | Maximum |
|---|---|
| Tourelles (toutes) | 2 + 1 par zone ouverte en plus de la cour (au plus 6) |
| Pièges posés | 12 |
| Lampadaires + projecteurs | 8 |
| Générateur | 1 |
| Établi | 1 |
| Palissades, barricades, portes | sans limite (le coût et l'IA, §7.4, empêchent les labyrinthes géants) |

### 7.3 Structures

Coût = bois / ferraille / or. Les **pièges** se traversent et les morts ne les ciblent pas. Une structure **électrique** ne marche que si un générateur qui tourne est à ≤ 15 m.

| Structure | Catégorie | Empreinte | Coût | PV | Effet | Modèle |
|---|---|---|---|---|---|---|
| Palissade | Défenses | 2×1 | 4 / 0 / 0 | 150 | Bloque morts, joueur **et balles** | `graveyard/fence` → abîmée `fence-damaged` |
| Barricade | Défenses | 2×1 | 6 / 2 / 0 | 300 | Bloque morts et joueur ; **les balles passent** (on tire par-dessus) | `survival/fence-fortified` |
| Porte | Défenses | 2×1 | 5 / 3 / 0 | 250 | Le **joueur passe**, pas les morts ; s'ouvre quand le joueur est à < 1,5 m (0,25 s), reste fermée pour les morts même ouverte | `graveyard/fence-gate` |
| Arbalète | Tourelles | 1×1 | 8 / 6 / 150 | 300 | Portée 12 m, 1 carreau / 1,2 s, 45 dégâts, perce 2 morts. **Sans courant** | `tower/tower-square-bottom-a` + `tower/weapon-ballista` |
| Mitrailleuse | Tourelles | 1×1 | 4 / 12 / 300 | 250 | Portée 10 m, 8 balles/s, 9 dégâts, dispersion 4°. **Électrique** | `tower/tower-square-bottom-b` + `tower/weapon-turret` |
| Lance-flammes | Tourelles | 1×1 | 2 / 10 / 400 | 280 | Cône 5 m × 40°, 30 dégâts/s + embrasement (6/s pendant 3 s). **Électrique**, compte double pour la consommation | `tower/tower-round-bottom-a` + `tower/weapon-cannon` teinté orange, particules `flame_01` |
| Pics | Pièges | 1×1 | 3 / 2 / 0 | 25 passages | 15 dégâts + ralenti 30 % pendant 1 s, au plus une fois par mort et par 0,5 s | `tower/detail-crystal` teinté gris acier |
| Mine | Pièges | 1×1 | 0 / 4 / 50 | 1 usage | Bip 0,4 s puis explosion : rayon 3 m, 120 dégâts, recul 6. Épargne le joueur | `blaster/grenade-b` posée à plat, voyant rouge clignotant |
| Flaque d'huile | Pièges | 3×3 | 0 / 2 / 30 | 1 usage | Ralentit de 40 %. **S'enflamme** (balle incendiaire, lance-flammes, fusée, lance-fusées, explosion, ou **E** à ≤ 1,5 m pendant 1 s) : brûle 8 s, 25 dégâts/s + embrasement, puis disparaît | décalque `circle_05` teinté noir (alpha 200), puis `fire_01` / `flame_01` |
| Piège à ours | Pièges | 1×1 | 2 / 3 / 0 | 10 prises | Immobilise le premier mort 3 s (brute 1,5 s, boss 0,5 s) + 30 dégâts ; se réarme en 8 s | procédural : anneau `GenMeshTorus` + dents en cubes, mâchoires qui se ferment |
| Générateur | Utilitaires | 2×1 | 6 / 10 / 200 | 400 | Alimente les structures électriques à ≤ 15 m **pendant les vagues**. Réservoir 3 bidons (E : verser 1 bidon) | `urban/detail-dumpster-closed` ×0,5 + `survival/barrel` + `urban/detail-cables-type-a` |
| Lampadaire | Utilitaires | 1×1 | 2 / 3 / 100 | 150 | Lumière rayon 6 m : les morts y sont ralentis de 40 %. Électrique (sans courant : lanterne de 3 m) | `graveyard/lightpost-single` |
| Projecteur | Utilitaires | 1×1 | 3 / 4 / 120 | 150 | Cône fixe 12 m × 35° (orienté avec T) : ralentit 40 % et brûle 8/s (lumière). Électrique | `urban/detail-light-double` |
| Établi | Utilitaires | 2×1 | 10 / 5 / 100 | 300 | Débloque les améliorations niv. 2 ; **E** dessus : établi niv. II (0 / 15 / 300) → améliorations niv. 3 | `survival/workbench` → niv. II `survival/workbench-anvil` |

**Lumière** (règle commune aux structures et aux pouvoirs) : les squelettes y sont insensibles (ni dégâts, ni ralenti) ; vampires et spectres subissent ×1,5.

**Carburant.** Le générateur consomme **1 bidon par 90 s de vague** pour 2 structures alimentées, **+20 % par structure en plus** (lance-flammes = 2). Il s'arrête pendant les pauses (pas de consommation). Réservoir vide : les structures électriques s'éteignent (voyant rouge, son de panne), annonce « Générateur à sec ! ». Pendant l'événement **Coupure**, les lampadaires de la carte s'éteignent mais **le générateur continue** : c'est le contre.

#### Améliorations (établi)

Règle générale, puis un bonus propre au niveau 3 :

| Niveau | Condition | Coût | Effet général |
|---|---|---|---|
| 2 | établi posé | coût de base + 50 or | PV ×2 ; tourelles : dégâts +30 % ; pièges : dégâts +30 %, utilisations ×2 |
| 3 | établi niv. II | 2 × coût de base + 150 or | PV ×3,5 ; tourelles : dégâts +70 %, portée +20 % ; pièges : dégâts +70 %, utilisations ×3 |

| Structure | Niveau 2 | Bonus du niveau 3 |
|---|---|---|
| Palissade | **Mur de pierre** (`graveyard/stone-wall`, abîmé `stone-wall-damaged`) | **Mur sanctifié** (teinte bleutée + `candle`) : bloque les spectres, brûle 5/s les morts au contact |
| Barricade | plaques de métal (`survival/metal-panel` par-dessus) | Épines : renvoie 8 dégâts par coup reçu |
| Porte | `survival/fence-doorway` | Grille de fer (`graveyard/iron-fence-border-gate`) : repousse (recul 4) les morts à < 1,5 m quand elle se referme |
| Arbalète | — | Carreaux enflammés (embrasement 6/s, 3 s) |
| Mitrailleuse | — | Balles perforantes (+1 mort traversé) |
| Lance-flammes | — | Cône 7 m, laisse un sol brûlant 2 s (10 dégâts/s) |
| Pics | — | Saignement 5/s pendant 3 s |
| Mine | — | Se réarme une fois (2 explosions) |
| Flaque d'huile | — | Brûle 14 s |
| Piège à ours | — | Électrifié : 20 dégâts en chaîne à 2 morts voisins (≤ 3 m) |
| Générateur | — | Réservoir 6, consommation −30 %, rayon 22 m |
| Lampadaire | — | Rayon 9 m, brûle 4/s |
| Projecteur | — | Pivote seul vers le groupe de morts le plus dense (60°/s) |

### 7.4 Les morts contre les structures

**Navigation** (`game/FlowField`) : champ de distances (Dijkstra) sur la grille de 1 m de toute la carte, calculé **depuis la case du joueur**.

| Case | Coût de traversée |
|---|---|
| Libre, piège | 1 (vampires : piège = 3, ils les évitent) |
| Décor, mur de la carte, grille fermée | infranchissable |
| Structure bloquante (palissade, barricade, porte, tourelle, générateur…) | `1 + PV restants / 25` (palissade neuve ≈ 7, mur sanctifié ≈ 22) ; coureurs : ×1,5 ; **spectres : 1** (ils traversent toutes les structures sauf le mur sanctifié, qui leur est infranchissable) |

- Recalcul quand une structure est posée / détruite / perd 25 % de PV, ou quand le joueur change de case ; au plus toutes les 0,25 s. ≈ 60 × 80 cases : < 1 ms. Remplace `Zones::steer` (les grilles fermées sont simplement infranchissables).
- Chaque mort se dirige vers la case voisine (8 voisins, pas de diagonale qui coupe un coin bloqué) de plus petite distance. Si cette case est une structure bloquante, il passe en état **`Breaking`** : il s'arrête à portée, se tourne vers elle et la frappe (`attack-melee-right`).
- Conséquence voulue : un détour de plus de ~7 m autour d'une palissade neuve coûte plus cher que de la casser, donc les morts la cassent. On ne peut pas faire de labyrinthe infini ni s'emmurer pour toujours.

**Priorités de cible** (dans l'ordre) :
1. Joueur à portée d'attaque + 0,5 m et visible (segment non bloqué) → le joueur.
2. **Spectre** : générateur, lampadaire ou projecteur à ≤ 15 m → il y va en traversant les structures (sauf le mur sanctifié) ; au contact, il ne frappe pas mais **vide 1 bidon toutes les 6 s** (générateur) ou **éteint** la lumière 10 s.
3. **Brute** : tourelle à ≤ 8 m → elle la prend pour cible.
4. Sinon le chemin du `FlowField` ; une structure sur le chemin est cassée.
- Au plus **6 attaquants** par structure ; les suivants contournent ou attendent.

**Dégâts aux structures**

| Ennemi | Dégâts / s aux structures |
|---|---|
| Marcheur | 10 |
| Coureur | 9 |
| Squelette | 10 |
| Vampire | 12 |
| Spectre | 0 (vide le carburant, éteint les lumières) |
| Brute | 50 (×3 par rapport à ses coups) |
| Le Fossoyeur | 80 ; sa pierre tombale fait 150 dans sa zone d'impact |
| Le Rongé | sa charge **détruit** toute structure de niveau ≤ 2 sur sa ligne et fait 200 aux niveaux 3 |

**Retour visuel et sonore**

| État | PV | Rendu |
|---|---|---|
| Intact | > 66 % | normal |
| Fissuré | 33-66 % | teinte ×0,8 ; variante `-damaged` du modèle quand elle existe ; poussière à chaque coup |
| En ruine | < 33 % | inclinaison 6°, fumée `smoke_01` toutes les 0,6 s, étincelles pour le métal, barre de PV qui clignote en rouge |

- Chaque coup : 4-6 éclats (bois : brun `dirt_01` ; pierre : gris ; métal : `spark_01`), son `hit_wood_*` / `hit_heavy_*` (métal : `impactMetal_*`), tremblement de la structure 0,1 s (4 cm). Événement `StructureDamaged`.
- Destruction : 20 éclats, `graveyard/debris-wood` ou `graveyard/debris` au sol pendant 8 s, secousse caméra 0,15, événement `StructureDestroyed`, annonce si c'est une tourelle ou le générateur (« Tourelle détruite ! »).
- Barre de PV au-dessus de la structure dès qu'elle est abîmée (à ≤ 12 m du joueur ; toujours en mode construction).
- Structure attaquée hors de l'écran : flèche rouge au bord de l'écran (3 au plus).

### 7.5 Équilibrage

**Budget moyen gagné par vague** (morts de la vague + caisses de la pause suivante, combo moyen ×1,5) :

| Vagues | Or | Bois | Ferraille | Bidons | Ce que ça permet |
|---|---|---|---|---|---|
| 1-2 | 120-200 | 10 | 3 | 0,6 | 2 palissades + 1 piège |
| 3-4 | 250-400 | 14 | 6 | 0,8 | une barricade et une porte, ou épargner pour l'arbalète |
| 5-6 | 450-700 | 20 | 10 | 1 | une tourelle toutes les 2 vagues |
| 7-8 | 700-1000 | 25 | 14 | 1,2 | améliorations niv. 2, générateur |
| 9-10 | 900-1300 | 30 | 18 | 1,4 | niv. 3 sur 2-3 pièces maîtresses |

L'or est partagé avec le marchand et les grilles : on ne peut pas tout avoir.

**Garde-fous** (les structures ne doivent pas jouer à la place du joueur) :
1. Un mort tué par une structure donne 100 % de son or, **50 % de son XP** et **ne fait pas monter le combo**.
2. Les tourelles ne tirent **qu'à vue** : une palissade ou un mur les aveugle (une barricade, non). On ne peut pas s'emmurer derrière des tourelles.
3. Brutes et boss cassent vite (×3, charge du Rongé) ; spectres sabotent l'électricité.
4. Limites du §7.2 ; rien à < 3 m d'un point d'apparition.
5. Nouvel **événement de vague « Termites »** (à partir de la vague 4, même probabilité que les autres) : les morts font ×2 aux structures, mais le bois lâché est doublé.
6. Objectif : **≤ 35 % des morts** d'une partie normale dues aux structures. L'écran de fin affiche « Tués par tes défenses : n (x %) » pour le mesurer.

---

## 8. Les pouvoirs actifs (v3)

Trois emplacements de pouvoir, huit pouvoirs, et pour chacun un **arbre à 3 paliers avec des choix exclusifs** : deux parties ne se jouent jamais pareil.

### 8.1 Touches

| Emplacement | Touche | raylib | Alias souris | Débloqué |
|---|---|---|---|---|
| 1 | **Espace** | `KEY_SPACE` | `MOUSE_BUTTON_SIDE` | départ |
| 2 | **F** | `KEY_F` | `MOUSE_BUTTON_EXTRA` | niveau 4 |
| 3 | **C** | `KEY_C` | — | niveau 8 |

Espace, F et C sont à la même place et portent la même lettre en AZERTY et QWERTY, et la main gauche les atteint sans lâcher ZQSD / WASD. (« A » a été écarté : c'est `KEY_Q`, qui s'affiche Q sur un clavier QWERTY.)

- Pouvoirs **visés** (fusée, nuée, piège, lanterne) : au point du sol sous le curseur (`mouseGroundPoint`), ramené à la portée max ; un marqueur de visée apparaît tant que la touche est enfoncée, le pouvoir part au relâchement.
- **Dash** : dans la direction de déplacement, ou vers le curseur si le joueur est immobile.
- Pas de pouvoirs en mode construction, en pause ni sur un écran de choix.

### 8.2 Obtention

| Moment | Ce qui se passe |
|---|---|
| Début de partie | Écran **« Choisis ton premier pouvoir »** : 3 pouvoirs tirés au hasard parmi les 8 (4 avec le talent **Initié**) |
| Niveau 4, niveau 8 | La montée de niveau propose **3 pouvoirs non possédés** au lieu de 3 cartes ; le choisi va dans l'emplacement libre |
| Autel de la chapelle | **E** : échanger un pouvoir contre 1 parmi 2 tirés au hasard, **250 or**, une fois par pause ; le pouvoir échangé perd ses paliers, le nouveau part de zéro |

Nouveau talent permanent (arbre du §6) : **Initié**, 1 rang, 25 ossements : 4 pouvoirs proposés au départ au lieu de 3.

### 8.3 Paliers et maîtrise

- **Maîtrise** d'un pouvoir = somme des recharges (en secondes) de toutes ses utilisations. Un dash (6 s) rapporte 6, une rage (40 s) rapporte 40 : tous les pouvoirs progressent au même rythme.

| Palier | Maîtrise | Niveau du joueur | Dash (6 s) | Rage (40 s) |
|---|---|---|---|---|
| 1 | 60 | ≥ 3 | 10 utilisations | 2 |
| 2 | 180 | ≥ 7 | 30 | 5 |
| 3 | 360 | ≥ 11 | 60 | 9 |

- Palier atteint : l'icône scintille en or (événement `PowerTierReady`). **Au début de la pause suivante**, le jeu se fige sur l'écran `PowerTier` : 2 ou 3 cartes d'options, **une seule** à choisir, définitif pour la partie. Plusieurs pouvoirs prêts : un écran par pouvoir, à la suite.
- Les choix se cumulent d'un palier à l'autre (1A + 2C + 3B…) : 27 variantes par pouvoir. Tab affiche les options prises et celles écartées (grisées).
- Deux nouvelles cartes (§2) : **#27 Concentration** (commune, max 3) : recharge des pouvoirs −10 % ; **#28 Surcharge** (rare, max 1) : 20 % de chances qu'un pouvoir ne se recharge pas.

### 8.4 Les huit pouvoirs

« Lumière » : voir la règle du §7.3 (squelettes insensibles, vampires et spectres ×1,5).

#### 1. Pas du spectre — dash
Dash de **6 m en 0,18 s**, traverse les morts (pas le décor ni les structures). **Recharge 6 s.** Silhouette bleutée transparente le long de la trajectoire.

| Palier | A | B | C |
|---|---|---|---|
| 1 | **Double charge** : 2 charges | **Traînée de flammes** : traînée 6 m × 1 m, 3 s, 20 dégâts/s + embrasement | **Intangible** : invulnérable pendant le dash et 0,3 s après |
| 2 | **Élan** : vitesse +40 % pendant 2 s après le dash | **Leurre** : une ombre reste 2,5 s et attire les morts à ≤ 8 m | **Onde de choc** : à l'arrivée, rayon 2,5 m, 35 dégâts, recul 5 |
| 3 | **Passe-muraille** : 9 m, traverse aussi les structures | **Faucheur** : chaque mort dans les 2 s après un dash rend 1 s de recharge | **Éclair blanc** : les morts traversés subissent 60 dégâts (lumière) |

#### 2. Fusée éclairante
Lancée au curseur (≤ 14 m, vol en cloche 0,6 s), brûle **10 s** : lumière rayon **8 m**, les morts y sont **ralentis de 40 %** et prennent **6 dégâts/s** (lumière). **Recharge 22 s.** Point lumineux rouge-orangé, étincelles `spark_04`, compte comme une source de lumière pour l'éclairage.

| Palier | A | B | C |
|---|---|---|---|
| 1 | **Magnésium** : 15 dégâts/s | **Longue combustion** : 18 s | **Deux fusées** : 2 charges |
| 2 | **Phare** : attire les morts à ≤ 12 m vers la fusée pendant 4 s | **Cendres ardentes** : enflamme les morts qui entrent dans la lumière | **Révélation** : les morts éclairés subissent +25 % de dégâts |
| 3 | **Soleil miniature** : à l'extinction, explosion rayon 6 m, 150 dégâts (lumière) | **Ancre de lumière** : dure 30 s et alimente les structures électriques à ≤ 8 m sans carburant | **Pluie d'étincelles** : 4 petites fusées en plus autour (4 m), 6 s chacune |

#### 3. Rage du gardien
Pendant **6 s** : cadence **+40 %**, vitesse **+20 %**, rechargement ×0,5 ; écran bordé de rouge (`PostFx`), battements de cœur. **Recharge 40 s.**

| Palier | A | B | C |
|---|---|---|---|
| 1 | **Sang chaud** : vol de vie 4 % des dégâts | **Munitions de colère** : aucune balle consommée | **Hurlement** : à l'activation, recul 6 sur les morts à ≤ 4 m + étourdis 1 s |
| 2 | **Prolongation** : chaque mort +0,5 s (au plus +6 s) | **Inarrêtable** : dégâts subis −50 % | **Combo furieux** : le combo ne retombe pas et compte un palier de multiplicateur de plus |
| 3 | **Berserk** : dégâts +60 %, mais vie max −30 % pendant la rage | **Seconde rage** : 2 charges | **Brasier intérieur** : aura de feu 3 m, 20 dégâts/s pendant la rage |

#### 4. Cierge de veille — totem de soin
Pose un totem aux pieds du joueur (`graveyard/altar-stone` ×0,5 + `graveyard/candle-multiple`) pour **10 s**, **120 PV** (les morts peuvent le frapper) : soigne **5 PV/s** à ≤ **4 m**, lumière 4 m. **Recharge 35 s.**

| Palier | A | B | C |
|---|---|---|---|
| 1 | **Bénédiction** : 10 PV/s | **Sanctuaire** : les morts dans le rayon sont ralentis de 30 % | **Cierge portatif** : pas de totem, aura sur le joueur pendant 8 s |
| 2 | **Réparateur** : répare aussi les structures, 20 PV/s | **Purge** : retire l'embrasement et immunise 3 s aux dégâts de zone | **Surcharge** : les soins au-delà du max donnent un bouclier (≤ 30) |
| 3 | **Cierge éternel** : 20 s | **Martyr** : à la fin, explose (5 m, 100 dégâts, lumière) | **Résurrection** : mourir dans le rayon fait revenir à 50 % de vie (1 fois par vague ; le totem est consumé) |

#### 5. Nuée de corbeaux
**8 corbeaux** s'envolent vers le curseur (≤ 14 m) et attaquent les morts dans un rayon de **6 m** pendant **6 s** : chaque corbeau fait **10 dégâts / 0,6 s** et change de cible à la mort de la sienne. Corbeaux procéduraux (deux triangles noirs qui battent + petite ombre), croassements (pitch aléatoire). **Recharge 25 s.**

| Palier | A | B | C |
|---|---|---|---|
| 1 | **Grande nuée** : 14 corbeaux | **Charognards** : ramassent or, bois et ferraille à ≤ 12 m pendant l'effet | **Aveuglement** : les morts touchés errent 1,5 s (perdent leur cible) |
| 2 | **Becs de fer** : 18 dégâts | **Peste** : saignement 4/s pendant 4 s | **Vigies** : défendent d'abord les structures attaquées à ≤ 20 m |
| 3 | **Murmuration** : 12 s | **Faim insatiable** : chaque mort tué par la nuée ajoute un corbeau (≤ 24) | **Œil du corbeau** : morts touchés marqués 5 s, +30 % de dégâts reçus, visibles dans le brouillard |

#### 6. Glas — onde sacrée
Un coup de cloche : anneau de lumière qui part du joueur jusqu'à **7 m** en 0,3 s, **50 dégâts sacrés** (vampires et spectres ×2, squelettes normaux), **recul 8**. **Recharge 18 s.** Son `wave_bell` grave, secousse 0,2.

| Palier | A | B | C |
|---|---|---|---|
| 1 | **Grand glas** : rayon 10 m | **Sidération** : étourdit 1,5 s | **Terre consacrée** : cercle de 5 m pendant 5 s, 15 dégâts/s |
| 2 | **Double tintement** : 2ᵉ onde 0,8 s après (60 %) | **Égide** : +10 de bouclier par mort touché (≤ 50, 8 s) | **Ralliement** : répare 20 % des PV max des structures dans le rayon |
| 3 | **Tocsin** : recharge 11 s | **Exorcisme** : tue net les non-boss sous 25 % de vie | **Écho de lumière** : rallume les lumières éteintes à ≤ 15 m (Coupure, spectres) et recharge la lampe de 40 % |

#### 7. Piège à loup lancé
Lancé au curseur (≤ 10 m), s'ouvre en 0,3 s ; le **premier mort** qui marche dessus est **immobilisé 3 s** (brute 1,5 s, boss 0,5 s) et prend **40 dégâts** ; disparaît après usage ou 30 s. **3 charges, recharge 8 s par charge.** Même modèle que le piège à ours du §7.3.

| Palier | A | B | C |
|---|---|---|---|
| 1 | **Dentelé** : saignement 8/s pendant 3 s | **Gros gibier** : brutes immobilisées 3 s, boss 1,5 s | **Aimanté** : à l'ouverture, attire les morts à ≤ 2,5 m |
| 2 | **Cartouchière** : 5 charges | **Ressort** : se réarme 2 fois (3 prises) | **Électrifié** : choc en chaîne sur 3 morts, 25 dégâts |
| 3 | **Mâchoires d'acier** : 150 dégâts | **Piège explosif** : explose à la libération (3 m, 80 dégâts) | **Appât** : les morts à ≤ 6 m s'acharnent sur le piégé pendant 3 s (12 dégâts/s entre eux) |

#### 8. Lanterne-sentinelle
Pose au curseur (≤ 8 m) une lanterne flottante (`graveyard/lantern-glass` à 1,8 m, qui ondule) pendant **12 s** : lumière **6 m** et un rayon sur le mort le plus proche à ≤ **9 m** : **18 dégâts / 0,5 s** (lumière). **Recharge 28 s.**

| Palier | A | B | C |
|---|---|---|---|
| 1 | **Paire** : 2 charges | **Faisceau perçant** : traverse 3 morts | **Porte-lanterne** : elle suit le joueur (orbite 2 m) |
| 2 | **Prisme** : 3 rayons sur 3 cibles, 12 dégâts chacun | **Halo** : ralentit de 40 % dans sa lumière | **Veilleuse** : 25 s |
| 3 | **Phare** : lumière 12 m, brûle 8/s tous les morts éclairés | **Arc de lumière** : deux lanternes actives sont reliées par un mur de lumière (40 dégâts/s à la traversée) | **Lanterne-mère** : à la fin, se divise en 3 petites lanternes de 6 s (50 % des dégâts) |

### 8.5 HUD des pouvoirs

- **Barre** en bas au centre : 3 cases de 64 × 64 px, espacées de 12 px, à 24 px du bord ; remplacée par la barre de construction en mode construction.
- **Fond** : `button_square_depth_flat` (UI Pack, gris) ; **icône** : Board Game Icons 64 px blanc, teinté par pouvoir :

| Pouvoir | Icône | Teinte |
|---|---|---|
| Pas du spectre | `arrow_right_curve` | bleu pâle |
| Fusée éclairante | `fire` | orange |
| Rage du gardien | `skull` | rouge |
| Cierge de veille | `flask_full` | vert |
| Nuée de corbeaux | `pawns` | violet |
| Glas | `exploding` | or |
| Piège à loup | `hexagon_in` | acier |
| Lanterne-sentinelle | `campfire` | jaune |

- **Recharge** : voile noir (alpha 160) en secteur circulaire (`DrawCircleSector`) qui se retire dans le sens horaire depuis midi ; secondes restantes au centre (avec une décimale sous 3 s).
- **Charges** : pastilles sous la case (pleine = disponible) ; les charges se rechargent l'une après l'autre.
- **Touche** : étiquette en bas à droite de la case (« ESPACE », « F », « C »).
- **Paliers** : 3 losanges au-dessus de la case (pleins = palier choisi) ; scintillent en or quand un palier est prêt.
- **Effet actif** (rage, totem, lanterne…) : contour coloré qui se vide pendant la durée.
- **Prêt** : flash blanc 0,15 s + rebond ×1,1 + `ui_confirm` (volume 0,4). **Refus** (en recharge) : la case tremble 0,15 s + `ui_error`.
- **Emplacement verrouillé** : icône `lock_closed` + « Niv. 4 » / « Niv. 8 ».

### 8.6 Notes d'implémentation

- `game/Powers.*` : catalogue **en données** (`PowerDef` : nom, icône, teinte, recharge, charges, portée, durée, options des paliers) et `PowerSlots` (3 emplacements : pouvoir, charges, recharge, maîtrise, `tier[3]` = option choisie ou −1). `tryActivate(slot, PowerContext&, EventQueue&)`.
- `game/PowerEffects.*` : objets actifs dans le monde (fusées, totems, corbeaux, pièges, lanternes, traînées, ondes) avec `update` / `draw`, qui frappent les morts via `Enemy::hit` et publient des événements.
- Événements (ajoutés en fin d'`EventType`) : `PowerUsed` (value = id), `PowerReady`, `PowerTierReady`, `ResourcePicked`, `CrateBroken`, `StructurePlaced`, `StructureDamaged`, `StructureDestroyed`.
- Nouveaux états de `Game` : `PowerChoice` (départ, niveaux 4 et 8, autel) et `PowerTier`. Le mode construction n'est **pas** un état : un booléen dans `Playing`.
- Dégâts « lumière » et « sacré » : un paramètre `DamageKind` sur `Enemy::hit` (défaut `Physical`), qui applique la règle du §7.3.

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
  game/Buildings.*         (v2, remplacé en v3 par les fichiers ci-dessous)
  game/Crates.*            v3 : caisses destructibles et leur butin
  game/BuildGrid.*         v3 : grille de 1 m, occupation des cases, validité d'une pose
  game/Structures.*        v3 : catalogue (données), instances, PV, dégâts, rendu abîmé, revente
  game/BuildMode.*         v3 : mode construction (entrées, fantôme, coût, annulation)
  game/FlowField.*         v3 : champ de distances (Dijkstra) suivi par les morts
  game/Turrets.*           v3 : arbalète, mitrailleuse, lance-flammes
  game/Traps.*             v3 : pics, mine, huile, piège à ours
  game/Generator.*         v3 : générateur, carburant, réseau électrique, lampadaire, projecteur, établi
  game/Powers.*            v3 : catalogue des pouvoirs, emplacements, recharges, maîtrise, paliers
  game/PowerEffects.*      v3 : effets actifs des pouvoirs dans le monde
  ui/BuildBar.*            v3 : barre de construction
  ui/PowerBar.*            v3 : barre des pouvoirs (§8.5)
  ui/PowerPicker.*         v3 : écrans de choix de pouvoir et de palier
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

### Roadmap v3 — base, pouvoirs, assets (chaque ligne ≈ 30-45 min = une PR)

Trois couloirs qui avancent en parallèle ; dans un couloir, on fait les tâches dans l'ordre. **Règles anti-conflits** :
- `game/Events.hpp` : on **ajoute** les nouveaux `EventType` à la fin, jamais au milieu, sans réordonner.
- `Game.cpp` / `Game.hpp` : chaque système s'intègre par **une méthode à lui** (`updateBase(dt)`, `updatePowers(dt)`, `drawBase()`, `drawPowerBar()`…) et une seule ligne d'appel ; pas de reformatage du code existant.
- `HudData` : champs ajoutés **à la fin** de la structure.
- Les catalogues (structures, pouvoirs, caisses) sont des **tableaux de données** dans leur `.cpp` : ajouter un élément ne touche qu'un fichier.

| # | Branche | Contenu | Fichiers | Dépend de |
|---|---|---|---|---|
| **V0** | `feature/assets-v3` | Télécharger et commiter la sélection « À intégrer (v3) » de `docs/ASSETS.md` ; crédits | `tools/fetch_assets.cmake`, `assets/models/tower/`, `assets/models/urban/`, `assets/fonts/`, `assets/ui/`, `assets/sounds/`, `assets/music/`, `CREDITS.md` | — |
| **V1** | `feature/resources` | Bois, ferraille, bidons (§7.1) : butin des morts, pickups, compteurs HUD | `game/Economy.*`, `game/Pickups.*`, `game/Events.hpp`, `ui/Hud.*`, `Game.cpp` (`rewardKill`) | — |

**Couloir Base**

| # | Branche | Contenu | Fichiers | Dépend de |
|---|---|---|---|---|
| B1 | `feature/build-grid` | Grille 1 m, mode construction (B, catégories, fantôme vert/rouge, coût, rotation, glisser, revente, réparation, annulation), palissade + barricade + porte ; obstacles de `World` avec un **masque** (joueur / morts / balles) | `game/BuildGrid.*`, `game/Structures.*`, `game/BuildMode.*`, `ui/BuildBar.*`, `game/World.*` (masque des obstacles), `Game.*` | V1, #18 (carte) |
| B2 | `feature/flowfield` | `FlowField` (Dijkstra), état `Breaking`, priorités de cible, dégâts aux structures, rendu fissuré / en ruine, débris, barres de PV, flèches hors écran | `game/FlowField.*`, `game/Enemy.*`, `game/WaveManager.cpp`, `game/Structures.*` | B1 |
| B3 | `feature/turrets` | Arbalète, mitrailleuse, lance-flammes ; tir à vue ; limite de tourelles ; XP ×0,5 et pas de combo pour leurs morts | `game/Turrets.*`, `game/Structures.cpp` (catalogue), `Game.cpp` | B1 (B2 conseillé) |
| B4 | `feature/traps` | Pics, mine, flaque d'huile inflammable, piège à ours | `game/Traps.*`, `game/Structures.cpp` (catalogue) | B1 ; en parallèle de B3 |
| B5 | `feature/crates` | Caisses destructibles (§7.1), apparition au début des pauses, tonneau de poudre | `game/Crates.*`, `Game.cpp` | V1 ; indépendante du reste du couloir |
| B6 | `feature/generator-workbench` | Générateur, carburant, réseau 15 m, lampadaire, projecteur ; établi et améliorations niv. 2-3 ; articles v3 du marchand | `game/Generator.*`, `game/Structures.*`, `game/Shop.*` | B3 |
| B7 | `feature/intermission-v3` | Pause 35 s / 45 s, bonus « Témérité », mode urgence ×1,5, événement Termites, stat « tués par tes défenses » | `game/WaveManager.*`, `game/BuildMode.cpp`, écran de fin | B2 |

**Couloir Pouvoirs**

| # | Branche | Contenu | Fichiers | Dépend de |
|---|---|---|---|---|
| P1 | `feature/powers-core` | Catalogue en données, 3 emplacements, touches Espace / F / C (+ boutons latéraux), recharges, charges, maîtrise ; `DamageKind` (lumière, sacré) ; **Pas du spectre** et **Glas** ; barre des pouvoirs (§8.5) | `game/Powers.*`, `ui/PowerBar.*`, `game/Enemy.*` (`DamageKind`), `game/Events.hpp`, `Game.*` | — (icônes de V0 si là, sinon lettres) |
| P2 | `feature/power-picker` | États `PowerChoice` : choix au départ, aux niveaux 4 et 8 ; échange à l'autel de la chapelle ; talent Initié | `ui/PowerPicker.*`, `Game.*`, `game/Zones.cpp` (autel) | P1 |
| P3 | `feature/powers-world` | `PowerEffects` + **Fusée éclairante**, **Lanterne-sentinelle**, **Cierge de veille** (sources de lumière branchées sur l'éclairage) | `game/PowerEffects.*`, `game/Powers.cpp` | P1 |
| P4 | `feature/powers-world-2` | **Nuée de corbeaux**, **Piège à loup lancé**, **Rage du gardien** (bord rouge `PostFx`) | `game/PowerEffects.*`, `game/Powers.cpp`, `fx/PostFx.*` | P3 |
| P5 | `feature/power-tiers-1` | Écran `PowerTier` (début de pause), options exclusives des paliers pour Pas du spectre, Glas, Fusée, Lanterne ; Tab affiche les choix | `ui/PowerPicker.*`, `game/Powers.*`, `game/PowerEffects.*` | P2, P3 |
| P6 | `feature/power-tiers-2` | Paliers de Cierge, Corbeaux, Piège, Rage ; cartes #27 Concentration et #28 Surcharge | `game/Powers.*`, `game/PowerEffects.*`, `game/Cards.cpp` | P4, P5 |

**Couloir Présentation**

| # | Branche | Contenu | Fichiers | Dépend de |
|---|---|---|---|---|
| C1 | `feature/font` | Police Kenney Future avec accents : `LoadFontEx` avec les codepoints 32-255 (sinon raylib ne charge que l'ASCII) ; une fonction `drawText` commune ; remplacer `DrawText` dans `ui/*`. **Annoncer dans l'issue avant** (touche toute l'UI) | `core/Fonts.*`, `ui/*` | V0 |
| C2 | `feature/music-intensity` | Musique adaptative : nuit calme (pause) → combat (≥ 8 morts en vie) → boss ; fondu 2 s ; nouveaux râles et cris (pitch 0,8-1,2) | `core/Audio.*`, `Game.cpp` (`handleEvents`) | V0 |
| C3 | `feature/ui-skin` | Panneaux et boutons du UI Pack pour le HUD, le marchand, les écrans de choix et les menus | `ui/*` | C1 |
| C4 | `feature/base-feel` | Sons de la base (`chop`, `metalClick`, `doorOpen` / `doorClose`, `engineCircular` en boucle pour le générateur, `thrusterFire` pour le lance-flammes, `explosionCrunch` pour la mine), poussière à la pose, secousse à la destruction, panne du générateur | `Game.cpp` (`handleEvents`), `fx/Particles.*` | B2, V0 |

## Planning

- **Samedi matin** : combat jouable + économie de base sur `main`.
- **Samedi soir** : cartes, marchand, cimetière, lumière : la boucle complète est jouable.
- **Dimanche matin** : élites, événements, boss, zones, progression permanente.
- **Dimanche après-midi** : effets, menus, équilibrage. **Plus de nouvelle fonctionnalité 2 h avant la fin** : corrections, release Linux + Windows, vidéo de démo.
