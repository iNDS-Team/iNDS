/*uniform sampler2D inputImageTexture;
varying highp vec2 texCoord;

void main()
{
    highp vec4 color = texture2D(inputImageTexture, texCoord);
    gl_FragColor = color;
}*/

uniform sampler2D inputImageTexture;
varying highp vec2 texCoord;

void main()
{
    highp vec4 color = texture2D(inputImageTexture, texCoord);
    gl_FragColor = color;
}