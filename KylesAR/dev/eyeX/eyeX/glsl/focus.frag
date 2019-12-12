#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;

// Ouput data, make focus out float
layout(location = 0) out vec3 color;
layout(location = 1) out vec3 focus;

// Values that stay constant for the whole mesh.
uniform mat4 MV;
//uniform mat4 clipNDC;
uniform mat4 clipPrecomp;
uniform sampler2D myTextureSampler;
uniform vec3 LightPosition_worldspace;
uniform vec3 EyePosition_worldspace;

bool uvwCramer(vec4 pt) { //013 123
	vec4 e1 = pt - clipPrecomp[2];
	float d20 = dot(e1, clipPrecomp[0]),
		  d21 = dot(e1, clipPrecomp[1]);
		  
	float v = (clipPrecomp[3][2] * d20 - clipPrecomp[3][1] * d21) * clipPrecomp[3][3];
	if (v < -1.f || v > 1.f) return false;
	
	float w = (clipPrecomp[3][0] * d21 - clipPrecomp[3][1] * d20) * clipPrecomp[3][3];
	if (w < 0.f || w > 1.f) return false;

	float u = 1.f - v - w;
	if (u < 0.f || u > 1.f) return false;

	return true;
}

void main() {
	vec2 tUV = UV;

	// Enable mirror repeat
	if (UV.x > 1.f) {
		int ux = int(UV.x);
		float s = UV.x - int(UV.x); //rounds down
		tUV.x = s * (2 * int(! bool(ux & 1)) - 1); //should flip if odd
	}
	if (UV.y > 1.f) {
		int vy = int(UV.y);
		float t = UV.y - int(UV.y); //rounds down
		tUV.y = t * (2 * int(! bool(vy & 1)) - 1); //should flip if odd
	}
	
	//Light emission properties
	vec3 LightColor = vec3(0.9f,0.9f,1.0f);
	float LightPower = 10.f;

	//Material properties
	vec3 MaterialDiffuseColor = texture2D( myTextureSampler, tUV ).rgb;
	vec3 MaterialAmbientColor = vec3(0.2f,0.2f,0.1f) * MaterialDiffuseColor;
	vec3 MaterialSpecularColor = vec3(.9f,.9f,1.f);

	//Distance to the light
	float distance = length(LightPosition_worldspace - Position_worldspace);
	
	//Normal of the computed fragment, in camera space
	vec3 n = normalize( Normal_cameraspace );
	//Direction of the light (from the fragment to the light)
	vec3 l = normalize( LightDirection_cameraspace );

	//Cosine of the angle between the normal and the light direction, clamped above 0
	float cosTheta = clamp( dot( n,l ), 0, 1.f);
	
	//Eye vector (towards the camera)
	vec3 E = normalize(EyeDirection_cameraspace);
	//Direction in which the triangle reflects the light
	vec3 R = reflect(-l,n);
	//Cosine of the angle between the Eye vector and the Reflect vector, clamped to 0
	float cosAlpha = clamp( dot( E,R ), 0, 1.f);
	
	//Diffuse : "color" of the object
	MaterialDiffuseColor = MaterialDiffuseColor * LightColor * LightPower * cosTheta / (distance);
	//Specular : reflective highlight, like a mirror
	MaterialSpecularColor = MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha,3) / (distance*distance);	
	
	//parallelogram clip
	//if( uvwCramer(vec4(Position_worldspace,1)) )
		color = MaterialAmbientColor + MaterialDiffuseColor + MaterialSpecularColor;
	
	//get the distance from the eye focus
	float mag = length(EyePosition_worldspace - Position_worldspace) * 0.08f;
	//widen field of view and clamp result
	mag = clamp(mag * mag - 0.1f, 0.f, 1.f);

	//encode focus and specular bloom in output
	focus = vec3(mag, 0.f, 0.f); //dot(MaterialSpecularColor, MaterialSpecularColor)

}