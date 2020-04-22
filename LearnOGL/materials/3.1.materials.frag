#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

struct Material
{
	vec3 ambient, diffuse, specular;
	float shininess;
};

struct Light
{
	vec3 position;
	vec3 ambient, diffuse, specular;
};

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
	vec3 ambient = light.ambient * material.ambient;
	
	vec3 
		norm = normalize(Normal),
		lightDir = normalize(light.position - FragPos);
	float dif = max(dot(norm, lightDir), 0);
	vec3 diffuse = light.diffuse * (dif * material.diffuse);
	
	vec3 
		viewDir = normalize(viewPos - FragPos),
		reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0), material.shininess);
	vec3 specular = light.specular * (spec * material.specular);
	
	vec3 result = ambient + diffuse + specular;
	FragColor = vec4(result, 1.0);	
}



