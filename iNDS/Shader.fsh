//http://sunandblackcat.com/tipFullView.php?l=eng&topicid=15

//Normal
uniform sampler2D inputImageTexture;
varying highp vec2 texCoord;

void main()
{
    highp vec4 color = texture2D(inputImageTexture, texCoord);
    gl_FragColor = color;
    //gl_FragColor = filter(inputImageTexture, texCoord, highp vec2(2, 2));
}

//http://stackoverflow.com/questions/13501081/efficient-bicubic-filtering-code-in-glsl
vec4 cubic(float v)
{
    highp vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
    highp vec4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4.0 * s.x;
    float z = s.z - 4.0 * s.y + 6.0 * s.x;
    float w = 6.0 - x - y - z;
    return highp vec4(x, y, z, w);
}

vec4 filter(sampler2D texture, vec2 texcoord, vec2 texscale)
{
    float fx = fract(texcoord.x);
    float fy = fract(texcoord.y);
    texcoord.x -= fx;
    texcoord.y -= fy;
    
    highp vec4 xcubic = cubic(fx);
    highp vec4 ycubic = cubic(fy);
    
    highp vec4 c = vec4(texcoord.x - 0.5, texcoord.x + 1.5, texcoord.y -
                  0.5, texcoord.y + 1.5);
    highp vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x +
                  ycubic.y, ycubic.z + ycubic.w);
    highp vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) /
    s;
    
    highp vec4 sample0 = texture2D(texture, vec2(offset.x, offset.z) *
                             texscale);
    highp vec4 sample1 = texture2D(texture, vec2(offset.y, offset.z) *
                             texscale);
    highp vec4 sample2 = texture2D(texture, vec2(offset.x, offset.w) *
                             texscale);
    highp vec4 sample3 = texture2D(texture, vec2(offset.y, offset.w) *
                             texscale);
    
    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);
    
    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}



/*
//Sepia
uniform sampler2D inputImageTexture;
varying highp vec2 texCoord;

void main()
{
    highp vec4 textureColor = texture2D(inputImageTexture, texCoord);
    lowp vec4 outputColor;
    outputColor.r = (textureColor.r * 0.393) + (textureColor.g * 0.769) + (textureColor.b * 0.189);
    outputColor.g = (textureColor.r * 0.349) + (textureColor.g * 0.686) + (textureColor.b * 0.168);
    outputColor.b = (textureColor.r * 0.272) + (textureColor.g * 0.534) + (textureColor.b * 0.131);
    outputColor.a = 1.0;
    
    gl_FragColor = outputColor;
}*/

/*
 float4 SampleBicubic( Texture2D tex, sampler texSampler, float2 uv )
 {
 //--------------------------------------------------------------------------------------
 // Calculate the center of the texel to avoid any filtering
 
 float2 textureDimensions    = GetTextureDimensions( tex );
 float2 invTextureDimensions = 1.f / textureDimensions;
 
 uv *= textureDimensions;
 
 float2 texelCenter   = floor( uv - 0.5f ) + 0.5f;
 float2 fracOffset    = uv - texelCenter;
 float2 fracOffset_x2 = fracOffset * fracOffset;
 float2 fracOffset_x3 = fracOffset * fracOffset_x2;
 
 //--------------------------------------------------------------------------------------
 // Calculate the filter weights (B-Spline Weighting Function)
 
 float2 weight0 = fracOffset_x2 - 0.5f * ( fracOffset_x3 + fracOffset );
 float2 weight1 = 1.5f * fracOffset_x3 - 2.5f * fracOffset_x2 + 1.f;
 float2 weight3 = 0.5f * ( fracOffset_x3 - fracOffset_x2 );
 float2 weight2 = 1.f - weight0 - weight1 - weight3;
 
 //--------------------------------------------------------------------------------------
 // Calculate the texture coordinates
 
 float2 scalingFactor0 = weight0 + weight1;
 float2 scalingFactor1 = weight2 + weight3;
 
 float2 f0 = weight1 / ( weight0 + weight1 );
 float2 f1 = weight3 / ( weight2 + weight3 );
 
 float2 texCoord0 = texelCenter - 1.f + f0;
 float2 texCoord1 = texelCenter + 1.f + f1;
 
 texCoord0 *= invTextureDimensions;
 texCoord1 *= invTextureDimensions;
 
 //--------------------------------------------------------------------------------------
 // Sample the texture
 
 return tex.Sample( texSampler, float2( texCoord0.x, texCoord0.y ) ) * scalingFactor0.x * scalingFactor0.y +
 tex.Sample( texSampler, float2( texCoord1.x, texCoord0.y ) ) * scalingFactor1.x * scalingFactor0.y +
 tex.Sample( texSampler, float2( texCoord0.x, texCoord1.y ) ) * scalingFactor0.x * scalingFactor1.y +
 tex.Sample( texSampler, float2( texCoord1.x, texCoord1.y ) ) * scalingFactor1.x * scalingFactor1.y;
 }
*/
