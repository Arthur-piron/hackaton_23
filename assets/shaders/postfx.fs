#version 330

// Post-traitement de LAST LIGHT : aberration chromatique, désaturation rouge
// selon la vie perdue, flash de mort, vignette et grain de film.

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform float time;      // secondes
uniform float hurt;      // 1 juste après un coup, retombe à 0
uniform float lowHealth; // 0 pleine vie, 1 à l'agonie
uniform float flash;     // flash bref à chaque mort de zombie

out vec4 finalColor;

float hash(vec2 p)
{
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 centered = uv - 0.5;
    float dist = length(centered);

    // Aberration chromatique radiale, nulle au centre, plus forte quand on est touché.
    vec2 offset = centered * (0.003 + 0.007 * hurt);
    vec3 color;
    color.r = texture(texture0, uv + offset).r;
    color.g = texture(texture0, uv).g;
    color.b = texture(texture0, uv - offset).b;

    // Vie perdue : les couleurs se vident et virent au rouge.
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    vec3 drained = vec3(luma * 1.15, luma * 0.8, luma * 0.8);
    color = mix(color, drained, 0.55 * lowHealth);

    // Flash très léger à chaque mort.
    color += vec3(1.0, 0.95, 0.85) * (0.08 * flash);

    // Vignette, assombrie et rougie quand on saigne.
    float vignette = smoothstep(0.85, 0.3, dist);
    color *= mix(0.45, 1.0, vignette);
    float edge = 1.0 - vignette;
    color = mix(color, vec3(0.35, 0.0, 0.02), edge * clamp(0.5 * lowHealth + 0.15 * hurt, 0.0, 0.7));

    // Grain de film animé (change à chaque image, plus visible dans les ombres).
    float grain = hash(gl_FragCoord.xy + fract(time * 7.13) * 1000.0) - 0.5;
    color += grain * mix(0.06, 0.03, luma);

    finalColor = vec4(clamp(color, 0.0, 1.0), 1.0) * colDiffuse * fragColor;
}
