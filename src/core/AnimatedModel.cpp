#include "core/AnimatedModel.hpp"

#include "external/cgltf.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {
Matrix composeTrs(Vector3 t, Quaternion r, Vector3 s)
{
    return MatrixMultiply(MatrixMultiply(MatrixScale(s.x, s.y, s.z), QuaternionToMatrix(r)),
                          MatrixTranslate(t.x, t.y, t.z));
}

// Indice de la dernière clé dont le temps est <= t (recherche dichotomique).
size_t keyBefore(const std::vector<float>& times, float t)
{
    const auto it = std::upper_bound(times.begin(), times.end(), t);
    return it == times.begin() ? 0 : static_cast<size_t>(it - times.begin()) - 1;
}

float wrapTime(float time, float duration, bool loop)
{
    if (duration <= 0.0f) return 0.0f;
    if (loop) return std::fmod(std::fmod(time, duration) + duration, duration);
    return std::clamp(time, 0.0f, duration);
}
}

AnimatedModel::AnimatedModel(const std::string& path)
{
    model_ = LoadModel(path.c_str());

    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        TraceLog(LOG_WARNING, "ANIM: impossible de lire %s", path.c_str());
        return;
    }
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        TraceLog(LOG_WARNING, "ANIM: buffers introuvables pour %s", path.c_str());
        cgltf_free(data);
        return;
    }

    const size_t nodeCount = data->nodes_count;
    parents_.assign(nodeCount, -1);
    basePose_.resize(nodeCount);
    for (size_t i = 0; i < nodeCount; ++i) {
        const cgltf_node& node = data->nodes[i];
        if (node.parent) parents_[i] = static_cast<int>(node.parent - data->nodes);
        NodePose& pose = basePose_[i];
        if (node.has_matrix) {
            Matrix m = {node.matrix[0], node.matrix[4], node.matrix[8], node.matrix[12],
                        node.matrix[1], node.matrix[5], node.matrix[9], node.matrix[13],
                        node.matrix[2], node.matrix[6], node.matrix[10], node.matrix[14],
                        node.matrix[3], node.matrix[7], node.matrix[11], node.matrix[15]};
            MatrixDecompose(m, &pose.translation, &pose.rotation, &pose.scale);
        } else {
            if (node.has_translation) pose.translation = {node.translation[0], node.translation[1], node.translation[2]};
            if (node.has_rotation) pose.rotation = {node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]};
            if (node.has_scale) pose.scale = {node.scale[0], node.scale[1], node.scale[2]};
        }
    }

    // Ordre de calcul : chaque parent avant ses enfants.
    std::vector<bool> placed(nodeCount, false);
    std::function<void(int)> place = [&](int n) {
        if (placed[n]) return;
        if (parents_[n] >= 0) place(parents_[n]);
        placed[n] = true;
        order_.push_back(n);
    };
    for (size_t i = 0; i < nodeCount; ++i) place(static_cast<int>(i));

    // raylib crée un mesh par primitive triangulaire, en parcourant les nœuds dans l'ordre,
    // et y applique déjà la transformation de repos du nœud.
    std::vector<Matrix> bindWorld;
    computeWorld(basePose_, bindWorld);
    for (size_t i = 0; i < nodeCount; ++i) {
        const cgltf_mesh* mesh = data->nodes[i].mesh;
        if (!mesh) continue;
        for (size_t p = 0; p < mesh->primitives_count; ++p) {
            if (mesh->primitives[p].type != cgltf_primitive_type_triangles) continue;
            meshNodes_.push_back(static_cast<int>(i));
            inverseBind_.push_back(MatrixInvert(bindWorld[i]));
        }
    }
    if (static_cast<int>(meshNodes_.size()) != model_.meshCount) {
        TraceLog(LOG_WARNING, "ANIM: %s : %d meshes raylib pour %d primitives glTF, animation désactivée",
                 path.c_str(), model_.meshCount, static_cast<int>(meshNodes_.size()));
        meshNodes_.clear();
        inverseBind_.clear();
    }

    for (size_t a = 0; a < data->animations_count; ++a) {
        const cgltf_animation& anim = data->animations[a];
        Clip clip;
        clip.name = anim.name ? anim.name : "";
        for (size_t c = 0; c < anim.channels_count; ++c) {
            const cgltf_animation_channel& src = anim.channels[c];
            if (!src.target_node || !src.sampler) continue;
            Channel channel;
            channel.node = static_cast<int>(src.target_node - data->nodes);
            int components = 3;
            switch (src.target_path) {
            case cgltf_animation_path_type_translation: channel.path = Path::Translation; break;
            case cgltf_animation_path_type_rotation: channel.path = Path::Rotation; components = 4; break;
            case cgltf_animation_path_type_scale: channel.path = Path::Scale; break;
            default: continue; // poids de morphing : non utilisés
            }
            const cgltf_accessor* input = src.sampler->input;
            const cgltf_accessor* output = src.sampler->output;
            channel.step = src.sampler->interpolation == cgltf_interpolation_type_step;
            // En CUBICSPLINE, chaque clé contient (tangente entrée, valeur, tangente sortie) :
            // on ne garde que la valeur.
            const bool cubic = src.sampler->interpolation == cgltf_interpolation_type_cubic_spline;
            channel.times.resize(input->count);
            channel.values.resize(input->count * components);
            for (size_t k = 0; k < input->count; ++k) {
                cgltf_accessor_read_float(input, k, &channel.times[k], 1);
                const size_t outIndex = cubic ? k * 3 + 1 : k;
                cgltf_accessor_read_float(output, outIndex, &channel.values[k * components], components);
            }
            if (!channel.times.empty()) clip.duration = std::max(clip.duration, channel.times.back());
            clip.channels.push_back(std::move(channel));
        }
        clips_.push_back(std::move(clip));
    }

    cgltf_free(data);
}

AnimatedModel::~AnimatedModel()
{
    UnloadModel(model_);
}

int AnimatedModel::findClip(const std::string& name) const
{
    for (size_t i = 0; i < clips_.size(); ++i) {
        if (clips_[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

float AnimatedModel::clipDuration(int clip) const
{
    return (clip >= 0 && clip < static_cast<int>(clips_.size())) ? clips_[clip].duration : 0.0f;
}

void AnimatedModel::play(AnimationState& state, int clip, bool loop, float speed, bool restart) const
{
    state.speed = speed;
    if (clip == state.clip && !restart) {
        state.loop = loop;
        return;
    }
    state.previousClip = state.clip;
    state.previousTime = state.time;
    state.clip = clip;
    state.time = 0.0f;
    state.loop = loop;
    state.blend = state.previousClip >= 0 ? 0.0f : 1.0f;
}

void AnimatedModel::update(AnimationState& state, float dt) const
{
    state.time += dt * state.speed;
    state.previousTime += dt * state.speed;
    if (state.blend < 1.0f) {
        state.blend = state.blendDuration > 0.0f ? std::min(1.0f, state.blend + dt / state.blendDuration) : 1.0f;
    }
}

bool AnimatedModel::isFinished(const AnimationState& state) const
{
    return !state.loop && state.time >= clipDuration(state.clip);
}

void AnimatedModel::samplePose(int clipIndex, float time, std::vector<NodePose>& pose) const
{
    pose = basePose_;
    if (clipIndex < 0 || clipIndex >= static_cast<int>(clips_.size())) return;

    for (const Channel& ch : clips_[clipIndex].channels) {
        const size_t k0 = keyBefore(ch.times, time);
        const size_t k1 = std::min(k0 + 1, ch.times.size() - 1);
        float a = 0.0f;
        if (!ch.step && k1 != k0) {
            const float span = ch.times[k1] - ch.times[k0];
            a = span > 0.0f ? std::clamp((time - ch.times[k0]) / span, 0.0f, 1.0f) : 0.0f;
        }
        NodePose& target = pose[ch.node];
        if (ch.path == Path::Rotation) {
            const float* v0 = &ch.values[k0 * 4];
            const float* v1 = &ch.values[k1 * 4];
            target.rotation = QuaternionSlerp({v0[0], v0[1], v0[2], v0[3]}, {v1[0], v1[1], v1[2], v1[3]}, a);
        } else {
            const float* v0 = &ch.values[k0 * 3];
            const float* v1 = &ch.values[k1 * 3];
            const Vector3 value = Vector3Lerp({v0[0], v0[1], v0[2]}, {v1[0], v1[1], v1[2]}, a);
            if (ch.path == Path::Translation) target.translation = value;
            else target.scale = value;
        }
    }
}

void AnimatedModel::computeWorld(const std::vector<NodePose>& pose, std::vector<Matrix>& world) const
{
    world.resize(pose.size());
    for (const int n : order_) {
        const Matrix local = composeTrs(pose[n].translation, pose[n].rotation, pose[n].scale);
        world[n] = parents_[n] >= 0 ? MatrixMultiply(local, world[parents_[n]]) : local;
    }
}

void AnimatedModel::draw(const AnimationState& state, const Matrix& transform, Color tint) const
{
    if (meshNodes_.empty()) {
        // Animation indisponible : on dessine le modèle tel que raylib l'a chargé.
        for (int m = 0; m < model_.meshCount; ++m) {
            Material material = model_.materials[model_.meshMaterial[m]];
            material.maps[MATERIAL_MAP_DIFFUSE].color = tint;
            DrawMesh(model_.meshes[m], material, transform);
        }
        return;
    }

    const int clipCount = static_cast<int>(clips_.size());
    const auto clipTime = [&](int clip, float time, bool loop) {
        return wrapTime(time, clip >= 0 && clip < clipCount ? clips_[clip].duration : 0.0f, loop);
    };

    samplePose(state.clip, clipTime(state.clip, state.time, state.loop), scratchPose_);
    if (state.blend < 1.0f && state.previousClip >= 0) {
        samplePose(state.previousClip, clipTime(state.previousClip, state.previousTime, true), scratchPrevious_);
        for (size_t n = 0; n < scratchPose_.size(); ++n) {
            NodePose& to = scratchPose_[n];
            const NodePose& from = scratchPrevious_[n];
            to.translation = Vector3Lerp(from.translation, to.translation, state.blend);
            to.rotation = QuaternionSlerp(from.rotation, to.rotation, state.blend);
            to.scale = Vector3Lerp(from.scale, to.scale, state.blend);
        }
    }
    computeWorld(scratchPose_, scratchWorld_);

    for (int m = 0; m < model_.meshCount; ++m) {
        const int node = meshNodes_[m];
        const Matrix meshTransform = MatrixMultiply(MatrixMultiply(inverseBind_[m], scratchWorld_[node]), transform);
        Material material = model_.materials[model_.meshMaterial[m]];
        material.maps[MATERIAL_MAP_DIFFUSE].color = tint;
        DrawMesh(model_.meshes[m], material, meshTransform);
    }
}
