#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ScreenTex;

const float u_ContrastThreshold = 0.083;
const float u_RelativeThreshold = 0.06;
const int iteration_count = 35;

struct luminance_data
{
  float m;
  float n, s, e, w;
  float nw, ne, sw, se;
  float highest, lowest, contrast;
}; 

struct edge_info
{
  bool is_horizontal;
  float step_value;
  float opposite_luminance, gradient;
};

float SampleLuminance(vec2 uv)
{
  vec3 color_sample = texture(ScreenTex, uv).rgb;
  return dot(color_sample, vec3(0.2, 0.7, 0.1));
}

float SampleLuminance(vec2 uv, vec2 offset, vec2 texel_size)
{
  uv += vec2(offset.x * texel_size.x, offset.y*texel_size.y);
  return SampleLuminance(uv);
}

luminance_data SampleLuminance3x3(vec2 uv, vec2 texel_size)
{
  luminance_data l;
  l.m  = SampleLuminance(uv, vec2( 0,  0), texel_size);
  l.n  = SampleLuminance(uv, vec2( 0,  1), texel_size);
  l.s  = SampleLuminance(uv, vec2( 0, -1), texel_size);
  l.w  = SampleLuminance(uv, vec2(-1,  0), texel_size);
  l.e  = SampleLuminance(uv, vec2( 1,  0), texel_size);
  l.nw = SampleLuminance(uv, vec2(-1,  1), texel_size);
  l.ne = SampleLuminance(uv, vec2( 1,  1), texel_size);
  l.sw = SampleLuminance(uv, vec2(-1, -1), texel_size);
  l.se = SampleLuminance(uv, vec2( 1, -1), texel_size);

  l.highest = max(max(max(max(l.n, l.s), l.w), l.e), l.m);
  l.lowest = min(min(min(min(l.n, l.s), l.w), l.e), l.m); 
  l.contrast = l.highest - l.lowest;	 
  return l;
}

bool ShouldSkipPixel(luminance_data l)
{
  if(l.contrast < max(u_ContrastThreshold, u_RelativeThreshold* l.highest))
  {
    return true;
  }
  return false;
}

float GetPixelBlendFactor(luminance_data l)
{
  float activation = 2*(l.n + l.s + l.w + l.e);
  activation += l.nw + l.ne + l.sw + l.se;
  activation *= 1.0f / 12;
  activation = abs(activation - l.m);
  activation = clamp(activation/l.contrast, 0.0, 1.0);
  activation = smoothstep(0, 1, activation);
  activation *= activation;
  return activation;
}

edge_info GetEdgeInfo(luminance_data l, vec2 texel_size)
{
  edge_info edge;
  float horizontal = abs(l.n + l.s - 2*l.m) + abs(l.nw + l.sw - 2*l.w) + abs(l.ne + l.se - 2*l.e);
  float vertical   = abs(l.w + l.e - 2*l.m) + abs(l.nw + l.ne - 2*l.n) + abs(l.sw + l.se - 2*l.s);
  edge.is_horizontal = horizontal >= vertical;

  float neg_luminance = edge.is_horizontal ? l.s : l.w;
  float pos_luminance = edge.is_horizontal ? l.n : l.e;
  float neg_abs_delta = abs(neg_luminance - l.m);
  float pos_abs_delta = abs(pos_luminance - l.m);

  edge.step_value = edge.is_horizontal ? texel_size.y : texel_size.x;
  if(pos_abs_delta < neg_abs_delta){
    edge.step_value = -edge.step_value;
    edge.opposite_luminance = neg_luminance;
    edge.gradient = neg_abs_delta;  
  }else {
    edge.opposite_luminance = pos_luminance;
    edge.gradient = pos_abs_delta; 
  }
  return edge;
}

float GetEdgeBlendFactor(luminance_data l, edge_info edge, vec2 uv, vec2 texel_size)
{
  vec2 edge_uv = uv;
  vec2 edge_step;
  if(edge.is_horizontal) {
    edge_uv.y += edge.step_value * 0.5;
    edge_step = vec2(texel_size.x, 0);
  } else {
    edge_uv.x += edge.step_value * 0.5;
    edge_step = vec2(0, texel_size.y);
  }

  float edge_luminance = (l.m + edge.opposite_luminance) * 0.5;
  float gradient_threshold = edge.gradient * 0.25;
  

  {
    vec2 puv = edge_uv + edge_step;
    float p_luminance_delta = SampleLuminance(puv) - edge_luminance;
    bool p_at_end = abs(p_luminance_delta) >= gradient_threshold;

    for(int i = 0; i < iteration_count-1 && !p_at_end; i++) {
      puv += edge_step;
      p_luminance_delta = SampleLuminance(puv) - edge_luminance;
      p_at_end = abs(p_luminance_delta) >= gradient_threshold;
    }

    vec2 nuv = edge_uv - edge_step;
    float n_luminance_delta = SampleLuminance(nuv) - edge_luminance;
    bool n_at_end = abs(n_luminance_delta) >= gradient_threshold;

    
    for(int i = 0; i < iteration_count-1 && !n_at_end; i++) {
      nuv -= edge_step;
      n_luminance_delta = SampleLuminance(nuv) - edge_luminance;
      n_at_end = abs(n_luminance_delta) >= gradient_threshold;
    }

    float p_distance, n_distance;
    if(edge.is_horizontal){
      p_distance = puv.x - uv.x;
      n_distance = uv.x  - nuv.x; 
    }else{
      p_distance = puv.y - uv.y;
      n_distance = uv.y  - nuv.y; 
    }

    float shortest_distance;
    bool delta_sign;
    if(p_distance <= n_distance) {
      shortest_distance = p_distance;
      delta_sign = p_luminance_delta >= 0;
    } else {
      shortest_distance = n_distance;
      delta_sign = n_luminance_delta >= 0;
    }
    
    if(delta_sign == (l.m - edge_luminance >= 0)) { 
       return 0;
    }
    return 0.5 - shortest_distance / (p_distance + n_distance)			;
  }  
}

void main()
{
  vec2 texel_size = 1.0f / vec2(textureSize(ScreenTex, 0));

  luminance_data l = SampleLuminance3x3(TexCoords, texel_size);

  bool skipped = false;
  FragColor = vec4(0, 0, 0, 1);

  if(ShouldSkipPixel(l))
  {
    skipped = true;
  }
  
  vec2 blend_uv = TexCoords;
  
  if(!skipped)
  {
    float blend_factor = GetPixelBlendFactor(l);
    edge_info edge = GetEdgeInfo(l, texel_size);
    //FragColor = (edge.is_horizontal) ? vec4(1, 0, 0, 1) : vec4(1);
    //FragColor = (edge.step_value < 0) ? vec4(1, 0, 0, 1) : vec4(1);
    float edge_blend_factor = GetEdgeBlendFactor(l, edge, TexCoords, texel_size);
    FragColor = vec4(vec3(edge_blend_factor), 1);

    if(edge.is_horizontal)
    {
      blend_uv.y += edge_blend_factor*edge.step_value;
    }
    else
    {
      blend_uv.x += edge_blend_factor*edge.step_value;
    }
    //FragColor = vec4(vec3(edge.gradient), 1);

  }
	
  FragColor = vec4(texture(ScreenTex, blend_uv).rgb, 1);
}
