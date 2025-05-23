#version 460

uniform sampler2D main_texture;

in vec2 position;

out vec4 frag_color;

void main()
{
    frag_color = texture(main_texture, position);
}
