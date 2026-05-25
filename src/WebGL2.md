я использую OpenGL 4 с DSA.
для WebGL нужно OpenGL 3.3.
здесь
https://github.com/g-truc/ogl-samples/blob/master/framework/dsa.cpp
есть пример эмуляции DSA который можно использовать для этого

ну и недостающее доделать
например
	glProgramUniform3fv
как
	BindShaderProgram(program);
	glUniform3fv(location, static_cast<GLint>(value.size()), reinterpret_cast<const float*>(value.data()));