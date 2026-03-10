#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

void main(){
  //Ambient
  float ambientStrength = 0.05;
  vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

  //Diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(vec3(1.0,1.0,1.0));
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * vec3(1.0, 0.95, 0.9);

  vec3 objectColor = vec3(0.2, 0.6, 0.3);
  vec3 result = (ambient + diffuse) * objectColor;
  FragColor = vec4(result, 1.0);
  
}

