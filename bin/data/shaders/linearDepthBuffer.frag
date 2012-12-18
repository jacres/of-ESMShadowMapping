uniform float u_LinearDepthConstant;
varying vec4 v_Position;

// can hardcode near/far/depth constant for speed, but passed in as uniform for convenience (u_LinearDepthConstant)
//const float Near = 0.1; // camera z near
//const float Far = 300.0; // camera z far
//const float LinearDepthConstant = 1.0 / (Far - Near);

void main() 
{ 
    float linearDepth = length(v_Position) * u_LinearDepthConstant;
    gl_FragColor.r = linearDepth;
}
