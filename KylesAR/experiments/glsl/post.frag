// calculate depth from stereo disparity
// additional constraint of parallax (being signed distnace from registered plane)
#version 330 core

layout(location = 0) out vec3 color;

uniform sampler2D disparity;

in vec2 uv;

void main() {
    vec2 uv = fragCoord / iResolution.xy,
         disparity = textureLod(iChannel0, uv, 0.).rg;
    vec3 camLoc = vec3( 1., 2., -8.),
         rd = normalize(vec3(uv * 2. - 1., 1.)),
         markerNorm = vec3(0.,1.,0.);
    float time = fract(iTime * .05) * 5. + .5,
          invNormTerm = iResolution.x * .333333,
    
    //DEPTH CALC: stereo depth
    baseline = distance(camLoc, vec3(-1., 2., -8.)),
    //focal length
    focal = iResolution.x * (iResolution.y / iResolution.x) * .885,
    //get the actual stereo disparity back, get depth to image plane instead of focal point with * rd.z
    stereo = disparity.y * invNormTerm * rd.z,
    //to depth if we have disparity
    d = step(.2, uv.y) * step(eps, disparity.y) * (baseline * focal) / stereo,
    
    //
    para = disparity.x * invNormTerm;
    
    // add 'depth' message
    if (time < 4.) fragColor = max(vec4(d  * .1 - .4),
                                   vec4(0., .4, .8, 1.) * msg_depth(iChannel1, uv * 5. - vec2(.3, 0.)));
    else {
        // Calculate ground truth
        sph1.l.xz += vec2(cos(iTime) * .5, sin(iTime) + 1.);
        sph2.r += .25 * cos(iTime) + .25;
        box2.s.x += sin(iTime) * .25;
        ray r = ray(camLoc, rd, v31, nullMat);
        seg s = nullSeg;
        lt(s, rs(r, box1));
    	lt(s, rs(r, box2));
        lt(s, rs(r, sph1));
        lt(s, rs(r, sph2));
        float groundTruthD = minT(s.t),
              groundTruthParallax = dot(markerNorm, (camLoc + rd * groundTruthD) - box1.c);

        if (uv.y > .2 && d > 0.) fragColor = mix(vec4(d  * .1 - .4),
                                                 vec4(d - groundTruthD, 0., groundTruthD - d, 0.) * 5.,
                                                 cos(time * pi * .9 + .9) * -.5 + .5);
		// add messages
        fragColor = mix(max(fragColor, vec4(0., .4, .8, 1.) * msg_error(iChannel1, uv * 5. - vec2(.3, 0.))),
                        max(fragColor, vec4(0., .4, .8, 1.) * msg_depth(iChannel1, uv * 5. - vec2(.3, 0.))),
                        cos(time * pi * .9 + .9) * .5 + .5);
    }
}