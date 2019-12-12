#version 330 core

layout(location = 0) out vec3 color;

uniform sampler2D vertFrame;
uniform vec2 frameSize;

void main() {
	//where to sample textures
	vec2 smp = (gl_FragCoord.xy * frameSize);
	smp.y = 1. - smp.y;
	vec3 backcolor = vec3(0.);

	for (int i = 5; i > 0; --i) {
		//sample points o pixels away to avoid fine grain noise
		vec3 s1 = vec3(smp, 0.), s2, s3, s4, s5;
		s1.z = texture2D(vertFrame, s1.xy).r;
		float o = float(i) * (s1.z + 1.) * 2.;
		s1.z *= o;

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
		s2.z = o * texture2D(vertFrame, s2.xy).r;
		s3.z = o * texture2D(vertFrame, s3.xy).r;
		s4.z = o * texture2D(vertFrame, s4.xy).r;
		s5.z = o * texture2D(vertFrame, s5.xy).r;

		if (s1.z < .0001) {
			if (s2.z + s3.z + s4.z + s5.z < .0001) continue;
			//calc norm
			vec3 nrm = normalize(cross(s2-s5, s2-s3)),
				 nm2 = normalize(cross(s3-s2, s3-s4)),
				 nm3 = normalize(cross(s4-s3, s4-s5)),
				 nm4 = normalize(cross(s5-s4, s5-s2));
			//if they are similar
			if (dot(nrm, nm2) + dot(nm2, nm3) + dot(nm3, nm4) + dot(nm4, nrm) > 3. * float(i != 1)) {
				//find holes in data
				float dat = step(.0001, abs(s5.z * s2.z * s3.z)),
					  dt2 = step(.0001, abs(s2.z * s3.z * s4.z)),
					  dt3 = step(.0001, abs(s3.z * s4.z * s5.z)),
					  dt4 = step(.0001, abs(s4.z * s5.z * s2.z));
				backcolor = -normalize(nrm*dat + nm2*dt2 + nm3*dt3 + nm4*dt4);
				break;
			}
		} else {
			//calc norm
			vec3 nrm = normalize(cross(s1-s2, s1-s3)),
				 nm2 = normalize(cross(s1-s3, s1-s4)),
				 nm3 = normalize(cross(s1-s4, s1-s5)),
				 nm4 = normalize(cross(s1-s5, s1-s2));
			//if they are similar
			if (dot(nrm, nm2) + dot(nm2, nm3) + dot(nm3, nm4) + dot(nm4, nrm) > 3. * float(i != 1)) {
				//find holes in data
				float dat = step(.0001, abs(s2.z * s3.z)),
					  dt2 = step(.0001, abs(s3.z * s4.z)),
					  dt3 = step(.0001, abs(s4.z * s5.z)),
					  dt4 = step(.0001, abs(s5.z * s2.z));
				backcolor = -normalize(nrm*dat + nm2*dt2 + nm3*dt3 + nm4*dt4);
				break;
			}
		}
	}

	color = backcolor * .5 + .5;
}
