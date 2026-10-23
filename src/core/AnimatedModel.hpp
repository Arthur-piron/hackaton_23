#pragma once

#include "raylib.h"

#include <string>
#include <vector>

// État d'animation propre à une entité. Plusieurs entités partagent le même
// AnimatedModel, chacune avec son AnimationState.
struct AnimationState {
    int clip = -1;          // animation courante (-1 : pose de repos)
    float time = 0.0f;      // temps dans l'animation courante, en secondes
    float speed = 1.0f;     // multiplicateur de vitesse de lecture
    bool loop = true;

    int previousClip = -1;  // animation dont on sort (fondu enchaîné)
    float previousTime = 0.0f;
    float blend = 1.0f;     // 0 = previousClip, 1 = clip
    float blendDuration = 0.15f;
};

// Modèle GLB animé par nœuds (personnages Kenney : chaque membre est un nœud rigide).
// raylib ne lit pas ce type d'animation : on relit le fichier avec cgltf.
// Voir docs/GAME.md, section « Animations des personnages Kenney ».
class AnimatedModel {
public:
    explicit AnimatedModel(const std::string& path);
    ~AnimatedModel();
    AnimatedModel(const AnimatedModel&) = delete;
    AnimatedModel& operator=(const AnimatedModel&) = delete;

    bool isValid() const { return model_.meshCount > 0; }

    // Indice d'une animation par son nom (« walk », « die »...), -1 si absente.
    int findClip(const std::string& name) const;
    float clipDuration(int clip) const;

    // Lance une animation avec un fondu depuis l'animation courante.
    // Ne fait rien si elle est déjà en cours (sauf restart = true).
    void play(AnimationState& state, int clip, bool loop = true, float speed = 1.0f,
              bool restart = false) const;
    void update(AnimationState& state, float dt) const;
    // Vrai quand une animation non bouclée est arrivée au bout.
    bool isFinished(const AnimationState& state) const;

    // Dessine le modèle dans la pose de l'état, avec une transformation monde.
    void draw(const AnimationState& state, const Matrix& transform, Color tint = WHITE) const;

    // Accès aux matériaux, par exemple pour leur affecter un shader d'éclairage.
    Model& model() { return model_; }

private:
    enum class Path { Translation, Rotation, Scale };

    struct Channel {
        int node = -1;
        Path path = Path::Translation;
        bool step = false;            // interpolation STEP au lieu de LINEAR
        std::vector<float> times;
        std::vector<float> values;    // 3 ou 4 composantes par clé
    };

    struct Clip {
        std::string name;
        float duration = 0.0f;
        std::vector<Channel> channels;
    };

    struct NodePose {
        Vector3 translation = {0.0f, 0.0f, 0.0f};
        Quaternion rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        Vector3 scale = {1.0f, 1.0f, 1.0f};
    };

    void samplePose(int clip, float time, std::vector<NodePose>& pose) const;
    void computeWorld(const std::vector<NodePose>& pose, std::vector<Matrix>& world) const;

    Model model_{};
    std::vector<int> parents_;          // parent de chaque nœud (-1 : racine)
    std::vector<int> order_;            // nœuds triés parents d'abord
    std::vector<NodePose> basePose_;    // pose de repos de chaque nœud
    std::vector<int> meshNodes_;        // nœud porteur de chaque mesh raylib
    std::vector<Matrix> inverseBind_;   // inverse de la transformation de repos de ce nœud
    std::vector<Clip> clips_;

    // Tampons réutilisés à chaque dessin (le rendu est mono-thread).
    mutable std::vector<NodePose> scratchPose_;
    mutable std::vector<NodePose> scratchPrevious_;
    mutable std::vector<Matrix> scratchWorld_;
};
