layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragGeometryNormal;
layout(location = 1) out vec3 fragTangent;
layout(location = 2) out vec3 fragBitangent;
layout(location = 3) out vec2 fragTexCoord;
