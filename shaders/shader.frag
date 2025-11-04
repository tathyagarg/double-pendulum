#version 330 core

uniform vec2 iResolution;
uniform vec3 iCenter;
uniform float iRadius;
uniform vec3 iColor;

out vec4 FragColor;

void main() {
  // discard all points gl_FragCoord outside the circle defined by iCenter and iRadius
  // centerX and centerY are screen relative, with (0,0) at the center of the screen
  // radius is also screen relative
  // gl_FragCoord is in screen coordinates, with (0,0) at the bottom left corner
  // iCenter.xy is in (-1, 1) range, so we need to convert it to screen coordinates
  
  vec2 center = (iCenter.xy * 0.5 + 0.5);
  float radius = iRadius;
  vec2 fragCoord = gl_FragCoord.xy / iResolution.xy * 2.0 - (1.5 + 0.5 * iResolution.y / iResolution.x);

  float dist = distance(fragCoord, iCenter.xy * 2);
  if (dist > 2 * radius) {
    discard;
  }

  FragColor = vec4(iColor, 1.0);
}
