#version 120

uniform sampler2D		u_ShadowMap;
uniform float               u_LinearDepthConstant;

varying vec3    v_Normal;
varying vec4	v_VertInLightSpace;
varying vec3    v_Vertex;
varying vec3    v_LightDir;

// can hardcode near/far/depth constant for speed, but passed in as uniform for convenience (u_LinearDepthConstant)
//const float Near = 0.1; // camera z near
//const float Far = 300.0; // camera z far
//const float LinearDepthConstant = 1.0 / (Far - Near);

struct material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
};

const material material1 = material(
    vec4(0.075, 0.075, 0.075, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0),
    250.0
);

void main(void)
{
    vec3 lightDir = normalize(v_Vertex.xyz - gl_LightSource[0].position.xyz);
	vec3 normal = normalize(v_Normal);
    
    vec3 R = -normalize( reflect( lightDir, normal ) );
    vec3 V = normalize(v_Vertex);

    float lambert = max(dot(normal, v_LightDir), 0.0);
    
    vec4 ambient = material1.ambient;
    vec4 diffuse = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 specular = vec4(0.0, 0.0, 0.0, 1.0);
    
    if ( lambert > 0.0 ) {
        ambient += gl_LightSource[0].ambient;
        diffuse += gl_LightSource[0].diffuse * material1.diffuse * lambert;
        specular += material1.specular * pow(max(dot(R, V), 0.0), material1.shininess);
    }

    // get projected shadow value
    vec3 depth = v_VertInLightSpace.xyz / v_VertInLightSpace.w;
    float lightDepth = length(v_Vertex.xyz - gl_LightSource[0].position.xyz) * u_LinearDepthConstant;

    float shadow = 1.0;
    
    if ( depth.z > 0.0 ) {
        float c = 10.0; // shadow coeffecient - change this to to affect shadow darkness/fade
        float texel = texture2D( u_ShadowMap, depth.xy ).r;
        shadow = clamp( exp( -c * (lightDepth - texel)), 0.0, 1.0 );
    }
    
    vec4 final_color = ambient + diffuse * shadow + specular * shadow;
    
	gl_FragColor = final_color;
}
