varying vec3		v_Normal;
varying vec4		v_VertInLightSpace;
varying vec3        v_Vertex;
varying vec3        v_LightDir;

uniform mat4		u_ShadowTransMatrix;

void main(void)
{
    vec4 vertInViewSpace = gl_ModelViewMatrix * gl_Vertex;

    v_Vertex = vertInViewSpace.xyz;
	v_Normal = gl_NormalMatrix * gl_Normal;
    v_LightDir = normalize( gl_LightSource[0].position.xyz - vertInViewSpace.xyz );
    
	v_VertInLightSpace = u_ShadowTransMatrix * vertInViewSpace;

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
