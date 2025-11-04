#version 330 core

uniform vec2 iResolution;
uniform vec3 iCenter;
uniform float iRadius;
uniform vec3 iColor;

out vec4 FragColor;

void main() {
  vec2 center = (iCenter.xy * 0.5 + 0.5);
  float radius = iRadius;
  vec2 fragCoord = gl_FragCoord.xy / iResolution.xy * 2.0 - (1.5 + 0.5 * iResolution.y / iResolution.x);

  float dist = distance(fragCoord, iCenter.xy * 2);
  if (dist > 2 * radius) {
    discard;
  }

  FragColor = vec4(iColor, 1.0);
}
