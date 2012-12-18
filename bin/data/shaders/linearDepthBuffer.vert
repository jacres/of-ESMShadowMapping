varying vec4 v_Position;

void main( void )
{
    v_Position = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = ftransform();
}