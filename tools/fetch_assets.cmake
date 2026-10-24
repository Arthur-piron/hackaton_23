# Télécharge les packs CC0 listés dans docs/ASSETS.md et copie la sélection dans assets/.
# Usage (Linux et Windows) : cmake -P tools/fetch_assets.cmake
#
# Le résultat est commité : le build, la CI et Docker n'ont pas besoin du réseau.
# Les sons de tir et les râles de zombie sont découpés avec ffmpeg s'il est installé,
# et les .wav / .mp3 convertis en .ogg (lu partout par raylib) ; sans ffmpeg, ces
# fichiers sont laissés tels quels dans assets/ (déjà commités).

cmake_minimum_required(VERSION 3.20)

get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(CACHE_DIR "${ROOT}/build/_assets_cache")
set(OUT "${ROOT}/assets")
file(MAKE_DIRECTORY "${CACHE_DIR}")

# fetch(<nom> <url>) : télécharge (une seule fois) et extrait dans ${CACHE_DIR}/<nom>/.
function(fetch name url)
    get_filename_component(ext "${url}" LAST_EXT)
    set(archive "${CACHE_DIR}/${name}${ext}")
    if(NOT EXISTS "${archive}")
        message(STATUS "Téléchargement de ${name}")
        file(DOWNLOAD "${url}" "${archive}" STATUS status TLS_VERIFY ON)
        list(GET status 0 code)
        if(NOT code EQUAL 0)
            file(REMOVE "${archive}")
            message(FATAL_ERROR "Échec du téléchargement de ${url} : ${status}")
        endif()
    endif()
    set(dir "${CACHE_DIR}/${name}")
    if(ext STREQUAL ".zip" AND NOT EXISTS "${dir}")
        file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${dir}")
    endif()
endfunction()

# copy_glb_pack(<nom> <dossier de sortie>) : tous les GLB du pack Kenney + leur texture.
function(copy_glb_pack name dest)
    file(GLOB glbs "${CACHE_DIR}/${name}/Models/GLB format/*.glb")
    file(MAKE_DIRECTORY "${OUT}/models/${dest}/Textures")
    file(COPY ${glbs} DESTINATION "${OUT}/models/${dest}")
    file(COPY "${CACHE_DIR}/${name}/Models/GLB format/Textures/colormap.png"
         DESTINATION "${OUT}/models/${dest}/Textures")
endfunction()

# copy_glb_selection(<nom> <dossier de sortie> <textures> <modèles...>) : quelques GLB du pack.
# textures : COLORMAP (Textures/colormap.png seule), ALL (tout Textures/, kits à plusieurs
# textures) ou NONE (couleurs dans les matériaux).
function(copy_glb_selection name dest textures)
    set(src "${CACHE_DIR}/${name}/Models/GLB format")
    file(MAKE_DIRECTORY "${OUT}/models/${dest}")
    foreach(model IN LISTS ARGN)
        file(COPY "${src}/${model}.glb" DESTINATION "${OUT}/models/${dest}")
    endforeach()
    if(textures STREQUAL "COLORMAP")
        file(COPY "${src}/Textures/colormap.png" DESTINATION "${OUT}/models/${dest}/Textures")
    elseif(textures STREQUAL "ALL")
        file(COPY "${src}/Textures" DESTINATION "${OUT}/models/${dest}")
    endif()
endfunction()

# copy_renamed(<source> <destination>)
function(copy_renamed src dst)
    get_filename_component(dir "${dst}" DIRECTORY)
    file(MAKE_DIRECTORY "${dir}")
    file(COPY_FILE "${src}" "${dst}")
endfunction()

# copy_pairs(<dossier source> <dossier destination> <extension> <source:destination...>)
# copie en renommant (noms en minuscules : la CI Linux distingue la casse).
function(copy_pairs src_dir dst_dir ext)
    foreach(pair IN LISTS ARGN)
        string(REPLACE ":" ";" pair "${pair}")
        list(GET pair 0 from)
        list(GET pair 1 to)
        copy_renamed("${src_dir}/${from}${ext}" "${dst_dir}/${to}${ext}")
    endforeach()
endfunction()

# --- Modèles ------------------------------------------------------------------
fetch(graveyard "https://kenney.nl/media/pages/assets/graveyard-kit/ba8d4b4517-1760691807/kenney_graveyard-kit_5.0.zip")
fetch(blaster   "https://kenney.nl/media/pages/assets/blaster-kit/261d80a716-1753959510/kenney_blaster-kit_2.1.zip")
fetch(survival  "https://kenney.nl/media/pages/assets/survival-kit/4065a8185b-1712149243/kenney_survival-kit.zip")
copy_glb_pack(graveyard graveyard)
copy_glb_pack(blaster blaster)
copy_glb_pack(survival survival)

# --- Modèles v3 : base, tourelles, décor ------------------------------------------
fetch(tower  "https://kenney.nl/media/pages/assets/tower-defense-kit/a402493eaa-1726471567/kenney_tower-defense-kit.zip")
fetch(urban  "https://kenney.nl/media/pages/assets/retro-urban-kit/8314d4db22-1738147509/kenney_retro-urban-kit.zip")
fetch(town   "https://kenney.nl/media/pages/assets/fantasy-town-kit/efe948d309-1754222374/kenney_fantasy-town-kit_2.0.zip")
fetch(castle "https://kenney.nl/media/pages/assets/castle-kit/a395102d20-1711543616/kenney_castle-kit.zip")
fetch(nature "https://kenney.nl/media/pages/assets/nature-kit/37ac38a37b-1677698939/kenney_nature-kit.zip")
copy_glb_selection(tower tower COLORMAP
    weapon-ballista weapon-turret weapon-cannon weapon-ammo-arrow weapon-ammo-bullet
    tower-square-bottom-a tower-square-bottom-b tower-round-bottom-a tower-round-build-a
    detail-crystal detail-crystal-large selection-a)
copy_glb_selection(urban urban ALL
    detail-dumpster-closed detail-cables-type-a detail-cables-type-b detail-light-single detail-light-double
    detail-barrier-strong-type-a detail-barrier-strong-damaged pallet planks)
copy_glb_selection(town town COLORMAP
    wall-wood wall-wood-broken fence fence-broken fence-gate lantern cart planks pillar-wood)
copy_glb_selection(castle castle COLORMAP
    siege-ballista siege-ballista-demolished wall-narrow-wood wall-narrow-wood-fence metal-gate door)
# Nature Kit : GLB autonomes (couleurs dans les matériaux), pas de Textures/.
copy_pairs("${CACHE_DIR}/nature/Models/GLTF format" "${OUT}/models/nature" ".glb"
    log:log log_stack:log_stack stump_old:stump_old tree_pineDefaultA:tree_pine_a
    tree_pineTallA_detailed:tree_pine_tall tree_thin_dark:tree_thin_dark
    mushroom_redGroup:mushroom_red_group grass_large:grass_large)

# --- Interface : police, icônes, panneaux -----------------------------------------
fetch(fonts       "https://kenney.nl/media/pages/assets/kenney-fonts/8d5435c213-1677661710/kenney_kenney-fonts.zip")
fetch(board_icons "https://kenney.nl/media/pages/assets/board-game-icons/19cae04050-1721645690/kenney_board-game-icons.zip")
fetch(ui_pack     "https://kenney.nl/media/pages/assets/ui-pack/f651646eab-1718203990/kenney_ui-pack.zip")
fetch(game_icons  "https://kenney.nl/media/pages/assets/game-icons/1ebf9c14af-1677661579/kenney_game-icons.zip")
copy_pairs("${CACHE_DIR}/fonts/Fonts" "${OUT}/fonts" ".ttf"
    "Kenney Future:kenney_future" "Kenney Future Narrow:kenney_future_narrow" "Kenney Pixel:kenney_pixel")
foreach(icon resource_wood resource_iron resource_planks flask_full structure_wall structure_gate
             structure_tower structure_watchtower fire skull exploding campfire pawns hexagon_in
             arrow_right_curve lock_closed shield hourglass sword bow)
    copy_renamed("${CACHE_DIR}/board_icons/PNG/Default (64px)/${icon}.png" "${OUT}/ui/icons/${icon}.png")
endforeach()
foreach(icon gear pause trophy wrench trashcan return star warning mouse)
    copy_renamed("${CACHE_DIR}/game_icons/PNG/White/2x/${icon}.png" "${OUT}/ui/icons/${icon}.png")
endforeach()
foreach(color Grey Red Yellow)
    string(TOLOWER "${color}" prefix)
    foreach(panel button_square_depth_flat button_rectangle_depth_flat button_rectangle_depth_border
                  slide_horizontal_color)
        copy_renamed("${CACHE_DIR}/ui_pack/PNG/${color}/Default/${panel}.png"
                     "${OUT}/ui/panels/${prefix}_${panel}.png")
    endforeach()
endforeach()

# --- Particules -----------------------------------------------------------------
fetch(particles "https://kenney.nl/media/pages/assets/particle-pack/f8fe0f8cb8-1677578741/kenney_particle-pack.zip")
set(PARTICLES
    circle_05 dirt_01 dirt_02 fire_01 fire_02 flame_01 flame_03 flare_01 light_01 light_02 light_03
    muzzle_01 muzzle_02 muzzle_03 muzzle_04 scorch_01 scorch_02 smoke_01 smoke_04 smoke_07
    spark_01 spark_02 spark_04 spark_05 star_06 trace_01 trace_02)
foreach(p IN LISTS PARTICLES)
    copy_renamed("${CACHE_DIR}/particles/PNG (Transparent)/${p}.png" "${OUT}/textures/particles/${p}.png")
endforeach()

# --- Sons Kenney ----------------------------------------------------------------
fetch(impact "https://kenney.nl/media/pages/assets/impact-sounds/87b4ddecda-1677589768/kenney_impact-sounds.zip")
fetch(ui     "https://kenney.nl/media/pages/assets/interface-sounds/fa43c1dd4d-1677589452/kenney_interface-sounds.zip")
foreach(i 000 001 002 003 004)
    copy_renamed("${CACHE_DIR}/impact/Audio/footstep_grass_${i}.ogg" "${OUT}/sounds/footstep_grass_${i}.ogg")
    copy_renamed("${CACHE_DIR}/impact/Audio/impactPunch_medium_${i}.ogg" "${OUT}/sounds/hit_flesh_${i}.ogg")
    copy_renamed("${CACHE_DIR}/impact/Audio/impactWood_medium_${i}.ogg" "${OUT}/sounds/hit_wood_${i}.ogg")
endforeach()
foreach(i 000 001 002)
    copy_renamed("${CACHE_DIR}/impact/Audio/impactPunch_heavy_${i}.ogg" "${OUT}/sounds/hit_heavy_${i}.ogg")
    copy_renamed("${CACHE_DIR}/impact/Audio/impactMetal_light_${i}.ogg" "${OUT}/sounds/reload_${i}.ogg")
    copy_renamed("${CACHE_DIR}/impact/Audio/impactSoft_heavy_${i}.ogg" "${OUT}/sounds/body_fall_${i}.ogg")
endforeach()
copy_renamed("${CACHE_DIR}/impact/Audio/impactBell_heavy_000.ogg" "${OUT}/sounds/wave_bell.ogg")
copy_renamed("${CACHE_DIR}/ui/Audio/click_002.ogg" "${OUT}/sounds/ui_click.ogg")
copy_renamed("${CACHE_DIR}/ui/Audio/confirmation_002.ogg" "${OUT}/sounds/ui_confirm.ogg")
copy_renamed("${CACHE_DIR}/ui/Audio/error_004.ogg" "${OUT}/sounds/ui_error.ogg")
copy_renamed("${CACHE_DIR}/ui/Audio/drop_002.ogg" "${OUT}/sounds/pickup.ogg")
copy_renamed("${CACHE_DIR}/ui/Audio/glitch_002.ogg" "${OUT}/sounds/light_flicker.ogg")

fetch(rpg     "https://kenney.nl/media/pages/assets/rpg-audio/8e99002d76-1677590336/kenney_rpg-audio.zip")
fetch(scifi   "https://kenney.nl/media/pages/assets/sci-fi-sounds/6b296f9ecf-1677589334/kenney_sci-fi-sounds.zip")
fetch(jingles "https://kenney.nl/media/pages/assets/music-jingles/f37e530b9e-1677590399/kenney_music-jingles.zip")
copy_pairs("${CACHE_DIR}/rpg/Audio" "${OUT}/sounds" ".ogg"
    chop:chop metalClick:metal_click metalLatch:metal_latch doorOpen_1:door_open doorClose_1:door_close
    creak1:creak handleCoins:coins drawKnife1:draw_knife)
copy_pairs("${CACHE_DIR}/scifi/Audio" "${OUT}/sounds" ".ogg"
    engineCircular_000:engine_loop thrusterFire_000:thruster_loop explosionCrunch_000:explosion_crunch
    lowFrequency_explosion_000:explosion_low forceField_000:force_field laserSmall_000:laser_small
    impactMetal_000:impact_metal)
copy_pairs("${CACHE_DIR}/jingles/Audio/Steel jingles" "${OUT}/sounds" ".ogg"
    jingles_STEEL00:jingle_steel_0 jingles_STEEL01:jingle_steel_1)

# --- Sons OpenGameArt -------------------------------------------------------------
fetch(znoise "https://opengameart.org/sites/default/files/zombienoises.zip")
copy_renamed("${CACHE_DIR}/znoise/zombienoise1.ogg" "${OUT}/sounds/zombie_groan_0.ogg")
copy_renamed("${CACHE_DIR}/znoise/zombienoise2.ogg" "${OUT}/sounds/zombie_groan_1.ogg")
copy_renamed("${CACHE_DIR}/znoise/zombienoise3.ogg" "${OUT}/sounds/zombie_attack.ogg")
copy_renamed("${CACHE_DIR}/znoise/fastzombie1.ogg" "${OUT}/sounds/zombie_runner.ogg")

fetch(moans "https://opengameart.org/sites/default/files/darsycho__zombie-moans_0.ogg")
fetch(guns  "https://opengameart.org/sites/default/files/sounds.zip")

fetch(music_menu "https://opengameart.org/sites/default/files/ambient_horror_track_01_0.zip")
fetch(music_night "https://opengameart.org/sites/default/files/freezer_0.ogg")
copy_renamed("${CACHE_DIR}/music_menu/Ambient Horror Track 01/ambient_horror_track01.ogg" "${OUT}/music/menu.ogg")
copy_renamed("${CACHE_DIR}/music_night.ogg" "${OUT}/music/night_ambience.ogg")

fetch(zscreams "https://opengameart.org/sites/default/files/zombies.zip")
set(ZOMBIE_CALLS 01_zombey 02_zombey 03_zombey 04_zombey_0 06_zombey 07_zombey)
foreach(call IN LISTS ZOMBIE_CALLS)
    fetch(zcall_${call} "https://opengameart.org/sites/default/files/${call}.mp3")
endforeach()

# --- Musique v3 : ambiance de nuit, combat, boss ----------------------------------
fetch(music_crickets "https://opengameart.org/sites/default/files/crickets_1.mp3")
fetch(music_wind     "https://opengameart.org/sites/default/files/wind%20woosh%20loop.ogg")
fetch(music_combat   "https://opengameart.org/sites/default/files/cynicbattleloop_0.ogg")
fetch(music_combat_2 "https://opengameart.org/sites/default/files/battleThemeA.mp3")
fetch(music_boss_1   "https://opengameart.org/sites/default/files/hard_boss_battle_1_bpm200_0.ogg")
fetch(music_boss_2   "https://opengameart.org/sites/default/files/hard_boss_battle_2_bpm210_0.ogg")
copy_renamed("${CACHE_DIR}/music_wind.ogg" "${OUT}/music/wind.ogg")
copy_renamed("${CACHE_DIR}/music_combat.ogg" "${OUT}/music/combat.ogg")
copy_renamed("${CACHE_DIR}/music_boss_1.ogg" "${OUT}/music/boss_1.ogg")
copy_renamed("${CACHE_DIR}/music_boss_2.ogg" "${OUT}/music/boss_2.ogg")

# --- Découpage et conversion avec ffmpeg (optionnel) ------------------------------
find_program(FFMPEG ffmpeg)
if(FFMPEG)
    # cut(<source> <début> <durée> <destination>) : extrait, normalise et ajoute un fondu de sortie.
    function(cut src start duration dst)
        execute_process(
            COMMAND "${FFMPEG}" -y -loglevel error -ss ${start} -t ${duration} -i "${src}"
                    -af "loudnorm=I=-16:TP=-1.5,afade=t=out:st=0:d=${duration}:curve=exp"
                    -ac 1 -ar 44100 -c:a libvorbis -q:a 5 "${dst}"
            RESULT_VARIABLE rc)
        if(NOT rc EQUAL 0)
            message(WARNING "ffmpeg a échoué pour ${dst}")
        endif()
    endfunction()
    # to_ogg(<source> <destination> <canaux>) : convertit en Ogg Vorbis (1 = mono pour les sons 3D).
    function(to_ogg src dst channels)
        execute_process(
            COMMAND "${FFMPEG}" -y -loglevel error -i "${src}" -ac ${channels} -ar 44100 -c:a libvorbis -q:a 4 "${dst}"
            RESULT_VARIABLE rc)
        if(NOT rc EQUAL 0)
            message(WARNING "ffmpeg a échoué pour ${dst}")
        endif()
    endfunction()
    cut("${CACHE_DIR}/guns/sounds/cz.wav"     0.20 0.7 "${OUT}/sounds/gun_pistol.ogg")
    cut("${CACHE_DIR}/guns/sounds/shotty.wav" 0.05 0.65 "${OUT}/sounds/gun_shotgun.ogg")
    cut("${CACHE_DIR}/guns/sounds/sks.wav"    0.30 0.6 "${OUT}/sounds/gun_rifle.ogg")
    cut("${CACHE_DIR}/moans.ogg" 0.0  3.0 "${OUT}/sounds/zombie_moan_0.ogg")
    cut("${CACHE_DIR}/moans.ogg" 3.65 2.6 "${OUT}/sounds/zombie_moan_1.ogg")
    cut("${CACHE_DIR}/moans.ogg" 6.6  2.3 "${OUT}/sounds/zombie_moan_2.ogg")
    cut("${CACHE_DIR}/moans.ogg" 9.3  3.0 "${OUT}/sounds/zombie_moan_3.ogg")
    # Cris d'attaque (artisticdude) : 8 variantes sur 24, de longueurs différentes.
    set(index 0)
    foreach(scream 1 4 8 12 16 18 20 23)
        to_ogg("${CACHE_DIR}/zscreams/zombies/zombie-${scream}.wav" "${OUT}/sounds/zombie_scream_${index}.ogg" 1)
        math(EXPR index "${index} + 1")
    endforeach()
    # Appels courts (saturn91).
    set(index 0)
    foreach(call IN LISTS ZOMBIE_CALLS)
        to_ogg("${CACHE_DIR}/zcall_${call}.mp3" "${OUT}/sounds/zombie_call_${index}.ogg" 1)
        math(EXPR index "${index} + 1")
    endforeach()
    to_ogg("${CACHE_DIR}/music_crickets.mp3" "${OUT}/music/crickets.ogg" 2)
    to_ogg("${CACHE_DIR}/music_combat_2.mp3" "${OUT}/music/combat_2.ogg" 2)
else()
    message(STATUS "ffmpeg introuvable : sons découpés et convertis non régénérés (versions commitées conservées)")
endif()

message(STATUS "Assets prêts dans ${OUT}")
