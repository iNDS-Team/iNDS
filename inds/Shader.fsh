//http://sunandblackcat.com/tipFullView.php?l=eng&topicid=15

//Normal
uniform sampler2D inputImageTexture;
varying highp vec2 texCoord;

highp float Triangular( highp float f )
{
    f = f / 2.0;
    if( f < 0.0 )
    {
        return ( f + 1.0 );
    }
    else
    {
        return ( 1.0 - f );
    }
    return 0.0;
}

highp float BellFunc( highp float x )
{
    highp float f = ( x / 2.0 ) * 1.5; // Converting -2 to +2 to -1.5 to +1.5
    if( f > -1.5 && f < -0.5 )
    {
        return( 0.5 * pow(f + 1.5, 2.0));
    }
    else if( f > -0.5 && f < 0.5 )
    {
        return 3.0 / 4.0 - ( f * f );
    }
    else if( ( f > 0.5 && f < 1.5 ) )
    {
        return( 0.5 * pow(f - 1.5, 2.0));
    }
    return 0.0;
}

highp float BSpline( highp float x )
{
    highp float f = x;
    if( f < 0.0 )
    {
        f = -f;
    }
    
    if( f >= 0.0 && f <= 1.0 )
    {
        return ( 2.0 / 3.0 ) + ( 0.5 ) * ( f* f * f ) - (f*f);
    }
    else if( f > 1.0 && f <= 2.0 )
    {
        return 1.0 / 6.0 * pow( ( 2.0 - f  ), 3.0 );
    }
    return 1.0;
}

highp float CatMullRom( highp float x )
{
    const highp float B = 0.0;
    const highp float C = 0.5;
    highp float f = x;
    if( f < 0.0 )
    {
        f = -f;
    }
    if( f < 1.0 )
    {
        return ( ( 12.0 - 9.0 * B - 6.0 * C ) * ( f * f * f ) +
                ( -18.0 + 12.0 * B + 6.0 *C ) * ( f * f ) +
                ( 6.0 - 2.0 * B ) ) / 6.0;
    }
    else if( f >= 1.0 && f < 2.0 )
    {
        return ( ( -B - 6.0 * C ) * ( f * f * f )
                + ( 6.0 * B + 30.0 * C ) * ( f *f ) +
                ( - ( 12.0 * B ) - 48.0 * C  ) * f +
                8.0 * B + 24.0 * C)/ 6.0;
    }
    else
    {
        return 0.0;
    }
}

highp vec4 BiCubic( sampler2D textureSampler, highp vec2 TexCoord )
{
    highp float fWidth = 256.0 * 2.0;
    highp float fHeight = 192.0 * 2.0;
    highp float texelSizeX = 1.0 / fWidth; //size of one texel
    highp float texelSizeY = 1.0 / fHeight; //size of one texel
    highp vec4 nSum = vec4( 0.0, 0.0, 0.0, 0.0 );
    highp vec4 nDenom = vec4( 0.0, 0.0, 0.0, 0.0 );
    highp float a = fract( TexCoord.x * fWidth ); // get the decimal part
    highp float b = fract( TexCoord.y * fHeight ); // get the decimal part
    for( int m = -1; m <=2; m++ )
    {
        for( int n =-1; n<= 2; n++)
        {
            highp vec4 vecData = texture2D(textureSampler,
                                     TexCoord + vec2(texelSizeX * float( m ),
                                                     texelSizeY * float( n )));
            highp float f  = BSpline( float( m ) - a );
            highp vec4 vecCooef1 = vec4( f,f,f,f );
            highp float f1 = BSpline( -( float( n ) - b ) );
            highp vec4 vecCoeef2 = vec4( f1, f1, f1, f1 );
            nSum = nSum + ( vecData * vecCoeef2 * vecCooef1  );
            nDenom = nDenom + (( vecCoeef2 * vecCooef1 ));
        }
    }
    return nSum / nDenom;
}

void main()
{
    highp vec4 color = texture2D(inputImageTexture, texCoord);
    //gl_FragColor = color;
    gl_FragColor = BiCubic(inputImageTexture, texCoord);
}

//http://stackoverflow.com/questions/13501081/efficient-bicubic-filtering-code-in-glsl
/*
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
}*/



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
