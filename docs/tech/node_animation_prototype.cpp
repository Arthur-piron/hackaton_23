// Prototype vérifié (Windows, raylib 5.5) : lecture des animations par nœuds des GLB Kenney.
// Build : target_include_directories(<cible> PRIVATE ${raylib_SOURCE_DIR}/src). Voir docs/GAME.md.
#include "raylib.h"
#include "raymath.h"
#include "external/cgltf.h"
#include <vector>
#include <cstring>
#include <cstdio>

static Matrix ToMatrix(const cgltf_float* m) {
    return { m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13],
             m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15] };
}

int main() {
    InitWindow(640, 480, "anim");
    const char* path = "glb/character-zombie.glb";
    Model model = LoadModel(path);
    cgltf_options opt{}; cgltf_data* data = nullptr;
    cgltf_parse_file(&opt, path, &data); cgltf_load_buffers(&opt, data, path);

    std::vector<cgltf_node*> meshNodes; std::vector<Matrix> invBind;
    for (size_t i = 0; i < data->nodes_count; i++) if (data->nodes[i].mesh) {
        cgltf_float w[16]; cgltf_node_transform_world(&data->nodes[i], w);
        meshNodes.push_back(&data->nodes[i]); invBind.push_back(MatrixInvert(ToMatrix(w)));
    }
    cgltf_animation* walk = nullptr;
    for (size_t i = 0; i < data->animations_count; i++) if (!strcmp(data->animations[i].name, "walk")) walk = &data->animations[i];

    Camera3D cam{ {3, 3, 3}, {0, 0.8f, 0}, {0, 1, 0}, 45, CAMERA_PERSPECTIVE };
    float t = 0; int frame = 0;
    while (!WindowShouldClose() && frame < 40) {
        t += 0.033f; frame++;
        for (size_t c = 0; c < walk->channels_count; c++) {
            cgltf_animation_channel& ch = walk->channels[c];
            cgltf_accessor* in = ch.sampler->input;
            float tmax; cgltf_accessor_read_float(in, in->count - 1, &tmax, 1);
            float tt = fmodf(t, tmax);
            size_t k = 0; float t0 = 0, t1 = 0;
            for (k = 0; k + 1 < in->count; k++) { cgltf_accessor_read_float(in, k + 1, &t1, 1); if (t1 >= tt) break; }
            cgltf_accessor_read_float(in, k, &t0, 1);
            float a = (t1 > t0) ? (tt - t0) / (t1 - t0) : 0;
            size_t k1 = (k + 1 < in->count) ? k + 1 : k;
            if (ch.target_path == cgltf_animation_path_type_rotation) {
                Quaternion q0, q1; cgltf_accessor_read_float(ch.sampler->output, k, &q0.x, 4); cgltf_accessor_read_float(ch.sampler->output, k1, &q1.x, 4);
                Quaternion q = QuaternionSlerp(q0, q1, a); memcpy(ch.target_node->rotation, &q, 16); ch.target_node->has_rotation = 1;
            } else if (ch.target_path == cgltf_animation_path_type_translation) {
                Vector3 p0, p1; cgltf_accessor_read_float(ch.sampler->output, k, &p0.x, 3); cgltf_accessor_read_float(ch.sampler->output, k1, &p1.x, 3);
                Vector3 p = Vector3Lerp(p0, p1, a); memcpy(ch.target_node->translation, &p, 12); ch.target_node->has_translation = 1;
            }
        }
        BeginDrawing(); ClearBackground({20, 20, 30, 255}); BeginMode3D(cam);
        DrawGrid(10, 1);
        for (int m = 0; m < model.meshCount; m++) {
            cgltf_float w[16]; cgltf_node_transform_world(meshNodes[m], w);
            Matrix xf = MatrixMultiply(invBind[m], ToMatrix(w));
            DrawMesh(model.meshes[m], model.materials[model.meshMaterial[m]], xf);
        }
        EndMode3D(); EndDrawing();
        if (frame == 12) TakeScreenshot("shot.png");
    }
    printf("meshes=%d nodes=%zu ok\n", model.meshCount, meshNodes.size());
    cgltf_free(data); CloseWindow(); return 0;
}
