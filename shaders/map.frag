#version 450
layout(binding=0) uniform sampler2D atlas;
layout(location=0) in vec2 texcoord;
layout(location=1) in vec4 tint;
layout(location=0) out vec4 outColor;
void main() {
    outColor = vec4(tint.rgb, tint.a * texture(atlas, texcoord).r);
}
