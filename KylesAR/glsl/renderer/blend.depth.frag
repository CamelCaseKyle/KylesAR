#version 330 core

layout(location = 0) out vec3 color;

uniform int mode;
uniform float warpFactor;
uniform vec2 frameSize;
uniform vec2 depthSize;
uniform vec2 bal;
uniform sampler2D cameraFrame;
uniform sampler2D vertFrame;
uniform sampler2D depthFrame;

in vec2 tc;

//shadertoy.com/view/Ms2SD1 thanks TDM
float hash(in vec2 p) {
	float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*43758.5453123);
}

void main() {
	float offset = float(int(gl_FragCoord.x*frameSize.x > 1.));

	//where to sample textures
	vec2 smp = (gl_FragCoord.xy * frameSize.xy),
		 smp_inv_y = vec2(smp.x, 1.f - smp.y);

	if (mode == 2) {
		//input in screen space [0, 1]
		smp = vec2(tc.x, 1.-tc.y); //getDistortion(smp, offset);
		smp_inv_y = tc;
		if (any(lessThan(smp, vec2(0.))) || any(greaterThan(smp, vec2(1.)))) {
			//default to black
			color = vec3(0.);
			return;
		}
	}
		
	vec4 vertCol = texture2D(vertFrame, smp);
	vec3 forecolor = vertCol.rgb;
	vec3 backcolor = vec3(0.);

	//depth frame
	if (mode == 0) {
		float d11 = texture2D(depthFrame, smp).r;
		backcolor = vec3(pow(d11, 255.));
		vertCol.a = 0.;

	//depth frame normal
	} else if (mode == 1) {

		vec3 nrm = vec3(0.);
		for (int i = 5; i > 0; --i) {
			//sample points o pixels away to avoid fine grain noise
			vec3 s1 = vec3(smp, 0.), s2, s3, s4, s5;
			s1.z = texture2D(vertFrame, s1.xy).r;
			float o = float(i) * (s1.z + 1.) * 2.;
			//s1.z *= o;

			if ((i & 1) == 0) {
				s2 = vec3(smp + frameSize * vec2(o, 0.), 0.);
				s3 = vec3(smp + frameSize * vec2(-o, 0.), 0.);
				s4 = vec3(smp + frameSize * vec2(0., o), 0.);
				s5 = vec3(smp + frameSize * vec2(0., -o), 0.);
			} else {
				s2 = vec3(smp + frameSize * vec2(o, o), 0.);
				s3 = vec3(smp + frameSize * vec2(o, -o), 0.);
				s4 = vec3(smp + frameSize * vec2(-o, -o), 0.);
				s5 = vec3(smp + frameSize * vec2(-o, o), 0.);
			}
			//do the sample, multiply by the sample size
			s2.z = texture2D(vertFrame, s2.xy).r;
			s3.z = texture2D(vertFrame, s3.xy).r;
			s4.z = texture2D(vertFrame, s4.xy).r;
			s5.z = texture2D(vertFrame, s5.xy).r;

			if (s1.z < .0001) {
				if (s2.z + s3.z + s4.z + s5.z < .0001) continue;
				//calc norm
				nrm = normalize(cross(s2-s5, s2-s3));
				vec3 nm2 = normalize(cross(s3-s2, s3-s4)),
					 nm3 = normalize(cross(s4-s3, s4-s5)),
					 nm4 = normalize(cross(s5-s4, s5-s2));
				//if they are similar
				if (length(nrm + nm2 + nm3 + nm4) > 3. * float(i != 1)) {
					//find holes in data
					float dat = step(.0001, abs(s5.z * s2.z * s3.z)),
						  dt2 = step(.0001, abs(s2.z * s3.z * s4.z)),
						  dt3 = step(.0001, abs(s3.z * s4.z * s5.z)),
						  dt4 = step(.0001, abs(s4.z * s5.z * s2.z));
					backcolor = normalize(nrm*dat + nm2*dt2 + nm3*dt3 + nm4*dt4) * .5 + .5;
					break;
				}
			} else {
				//calc norm
				nrm = normalize(cross(s1-s2, s1-s3));
				vec3 nm2 = normalize(cross(s1-s3, s1-s4)),
					 nm3 = normalize(cross(s1-s4, s1-s5)),
					 nm4 = normalize(cross(s1-s5, s1-s2));
				//if they are similar
				if (length(nrm + nm2 + nm3 + nm4) > 3. * float(i != 1)) {
					//find holes in data
					float dat = step(.0001, abs(s2.z * s3.z)),
						  dt2 = step(.0001, abs(s3.z * s4.z)),
						  dt3 = step(.0001, abs(s4.z * s5.z)),
						  dt4 = step(.0001, abs(s5.z * s2.z));
					backcolor = normalize(nrm*dat + nm2*dt2 + nm3*dt3 + nm4*dt4) * .5 + .5;
					break;
				}
			}
		}
		if (backcolor.r + backcolor.g + backcolor.b > .001)
			backcolor.b = 1.-backcolor.b;
		vertCol.a = 0.;
	//camera frame
	} else {
		backcolor = texture2D(cameraFrame, smp_inv_y).rgb;
	}

	//final
	color = mix(backcolor, forecolor, vertCol.a);	
}
