attribute vec3 pos;
attribute vec2 tex;
attribute vec3 nor;
varying vec2 texCoord;
varying vec3 normal;
uniform mat4 PV;
uniform mat4 M;

void kore() {
	//gl_Position = vec4(pos.x, pos.y, pos.z, 1.0);
	gl_Position = PV * M * vec4(pos.x, pos.y, pos.z, 1.0);
	texCoord = tex;
	normal = nor;
}
