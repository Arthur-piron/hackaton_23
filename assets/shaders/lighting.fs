#version 330

// Éclairage de nuit de LAST LIGHT : ambiance bleutée, lune froide, lumières
// ponctuelles colorées (lampadaires, braseros, flash de tir), cône de la lampe
// torche, brume au ras du sol et brouillard qui noie le bord de la carte.

#define MAX_LIGHTS 24

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 ambientSky;      // ambiance venue du ciel (faces tournées vers le haut)
uniform vec3 ambientGround;   // ambiance venue du sol (faces tournées vers le bas)
uniform vec3 moonDirection;   // direction dans laquelle voyage la lumière de la lune
uniform vec3 moonColor;

uniform int lightCount;
uniform vec4 lightPositions[MAX_LIGHTS]; // xyz : position, w : portée
uniform vec3 lightColors[MAX_LIGHTS];    // couleur multipliée par l'intensité

uniform vec3 spotPosition;
uniform vec3 spotDirection;
uniform vec3 spotColor;       // noir quand la lampe est éteinte
uniform vec3 spotCone;        // cosinus extérieur, cosinus intérieur, portée

uniform vec3 fogColor;
uniform vec3 fogCenter;       // le brouillard se mesure depuis ce point, dans le plan du sol
uniform vec2 fogRange;        // début, fin
uniform vec3 mistColor;
uniform float time;

out vec4 finalColor;

float hash(vec2 p)
{
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Éclairage « enveloppant » : les faces de biais restent un peu éclairées (style low-poly).
float wrapDiffuse(vec3 n, vec3 l, float wrap)
{
    return clamp((dot(n, l) + wrap) / (1.0 + wrap), 0.0, 1.0);
}

// Atténuation douce, nulle à la portée de la lumière.
float attenuation(float distanceSq, float range)
{
    float x = distanceSq / (range * range);
    float window = clamp(1.0 - x * x, 0.0, 1.0);
    return window * window / (1.0 + 0.3 * distanceSq);
}

void main()
{
    vec4 texel = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec3 albedo = texel.rgb;
    vec3 n = normalize(fragNormal);

    // Ambiance : ciel nocturne au-dessus, sol presque noir en dessous.
    vec3 light = mix(ambientGround, ambientSky, n.y * 0.5 + 0.5);
    light += moonColor * wrapDiffuse(n, -moonDirection, 0.2);

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= lightCount) break;
        vec3 toLight = lightPositions[i].xyz - fragPosition;
        float distanceSq = dot(toLight, toLight);
        float range = lightPositions[i].w;
        if (distanceSq >= range * range) continue;
        vec3 l = toLight * inversesqrt(max(distanceSq, 1e-4));
        light += lightColors[i] * wrapDiffuse(n, l, 0.5) * attenuation(distanceSq, range);
    }

    // Lampe torche : cône doux, bord légèrement chaud.
    vec3 toSpot = spotPosition - fragPosition;
    float spotDistanceSq = dot(toSpot, toSpot);
    vec3 ls = toSpot * inversesqrt(max(spotDistanceSq, 1e-4));
    float cone = smoothstep(spotCone.x, spotCone.y, dot(-ls, spotDirection));
    float spotFalloff = clamp(1.0 - spotDistanceSq / (spotCone.z * spotCone.z), 0.0, 1.0);
    vec3 beam = mix(vec3(1.0, 0.8, 0.55), vec3(1.0), cone);
    light += spotColor * beam * cone * spotFalloff * spotFalloff * wrapDiffuse(n, ls, 0.6) * 2.2;

    vec3 color = albedo * light;
    // Les hautes lumières saturent en douceur au lieu de brûler.
    color = color / (1.0 + max(max(color.r, color.g), color.b) * 0.25);

    // Brume au ras du sol, qui dérive lentement.
    vec2 drift = fragPosition.xz * 0.18 + vec2(time * 0.05, time * 0.03);
    float mist = valueNoise(drift) * 0.6 + valueNoise(drift * 2.3 - time * 0.04) * 0.4;
    color = mix(color, mistColor, mist * exp(-max(fragPosition.y, 0.0) * 1.5) * 0.35);

    // Brouillard : le bord de la carte se fond dans la nuit.
    float horizontal = length(fragPosition.xz - fogCenter.xz);
    float fog = smoothstep(fogRange.x, fogRange.y, horizontal);
    color = mix(color, fogColor, fog);

    finalColor = vec4(color, texel.a);
}
