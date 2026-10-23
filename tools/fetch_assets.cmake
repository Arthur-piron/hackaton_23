# Télécharge les packs CC0 listés dans docs/ASSETS.md et copie la sélection dans assets/.
# Usage (Linux et Windows) : cmake -P tools/fetch_assets.cmake
#
# Le résultat est commité : le build, la CI et Docker n'ont pas besoin du réseau.
# Les sons de tir et les râles de zombie sont découpés avec ffmpeg s'il est installé ;
# sinon ces fichiers sont laissés tels quels dans assets/ (déjà commités).

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

# copy_renamed(<source> <destination>)
function(copy_renamed src dst)
    get_filename_component(dir "${dst}" DIRECTORY)
    file(MAKE_DIRECTORY "${dir}")
    file(COPY_FILE "${src}" "${dst}")
endfunction()

# --- Modèles ------------------------------------------------------------------
fetch(graveyard "https://kenney.nl/media/pages/assets/graveyard-kit/ba8d4b4517-1760691807/kenney_graveyard-kit_5.0.zip")
fetch(blaster   "https://kenney.nl/media/pages/assets/blaster-kit/261d80a716-1753959510/kenney_blaster-kit_2.1.zip")
fetch(survival  "https://kenney.nl/media/pages/assets/survival-kit/4065a8185b-1712149243/kenney_survival-kit.zip")
copy_glb_pack(graveyard graveyard)
copy_glb_pack(blaster blaster)
copy_glb_pack(survival survival)

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

# --- Découpage avec ffmpeg (optionnel) --------------------------------------------
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
    cut("${CACHE_DIR}/guns/sounds/cz.wav"     0.20 0.7 "${OUT}/sounds/gun_pistol.ogg")
    cut("${CACHE_DIR}/guns/sounds/shotty.wav" 0.05 0.65 "${OUT}/sounds/gun_shotgun.ogg")
    cut("${CACHE_DIR}/guns/sounds/sks.wav"    0.30 0.6 "${OUT}/sounds/gun_rifle.ogg")
    cut("${CACHE_DIR}/moans.ogg" 0.0  3.0 "${OUT}/sounds/zombie_moan_0.ogg")
    cut("${CACHE_DIR}/moans.ogg" 3.65 2.6 "${OUT}/sounds/zombie_moan_1.ogg")
    cut("${CACHE_DIR}/moans.ogg" 6.6  2.3 "${OUT}/sounds/zombie_moan_2.ogg")
    cut("${CACHE_DIR}/moans.ogg" 9.3  3.0 "${OUT}/sounds/zombie_moan_3.ogg")
else()
    message(STATUS "ffmpeg introuvable : sons de tir et râles non régénérés (versions commitées conservées)")
endif()

message(STATUS "Assets prêts dans ${OUT}")
