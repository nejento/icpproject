varying vec3 normal, lightDir;

void main()
{   
    vec4 p;

    p.yxz = gl_Vertex.xyz;
    p.w = 1.0;

    lightDir = normalize(vec3(gl_LightSource[0].position));
    normal = normalize(gl_NormalMatrix * gl_Normal);
        
    gl_Position = gl_ModelViewProjectionMatrix * p;
}