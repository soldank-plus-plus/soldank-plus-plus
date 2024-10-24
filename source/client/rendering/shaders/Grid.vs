R"(
#version 120
attribute vec3 position;

varying vec3 vertexPosition;

void main()
{
    vertexPosition = position;
    gl_Position = vec4(position, 1.0);
}
)"