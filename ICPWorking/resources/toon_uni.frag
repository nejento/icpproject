varying vec3 normal, lightDir;

uniform vec4 toon_color;

vec4 toonify(in float intensity) {

    vec4 color = toon_color;

    if (intensity > 0.98)
        color = vec4(0.8,0.8,0.8,1.0);
    else if (intensity > 0.5)
        color = toon_color*vec4(0.4,0.4,0.4,1.0);  
    else if (intensity > 0.25)
        color = toon_color*vec4(0.2,0.2,0.2,1.0);
    else
        color = vec4(0.1,0.1,0.1,1.0);      

    return(color);
}

void main()
{
    float intensity;
    vec3 n;
    vec4 color;

    n = normalize(normal);
    intensity = max(dot(lightDir,n),0.0); 

    color = toonify(intensity);       
        
    gl_FragColor = color;
}