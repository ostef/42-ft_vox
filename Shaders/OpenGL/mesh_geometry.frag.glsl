#version 450 core

uniform sampler2D my_texture;

out vec4 frag_color;

void main()
{
    frag_color = texture(my_texture, gl_FragCoord.xy);
}
