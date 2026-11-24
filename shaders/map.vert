#version 450
layout(location=0) in vec2 pos;
layout(location=1) in vec2 uv;
layout(location=2) in vec4 color;
layout(location=0) out vec2 texcoord;
layout(location=1) out vec4 tint;
layout(push_constant) uniform View { vec2 size; } view;
void main() {
    gl_Position = vec4(pos / view.size * 2.0 - 1.0, 0, 1);
    texcoord = uv;
    tint = color;
}
