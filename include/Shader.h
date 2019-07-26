/*
	GLSL Shader abstraction

	MIT License

	Copyright (c) 2009--2014, Markus Broecker <mbrckr@gmail.com>

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
 */

#ifndef SHADER_INCLUDED
#define SHADER_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>


/// An OpenGL3 shader
/**	Update 2014: Simplified error reporting, added geometry shaders.
*/
class Shader
{
public:
	
	/**	Loads, compiles and validates the shader.
		@param vpFile vertex shader file
		@param gpFile geometry shader file
		@param fpFile fragment shader file
		@param defines a list of defines which whill be added before the first line of the source code
	*/
	Shader(const std::string& vpFile, const std::string& fpFile);
	Shader(const std::string& vpFile, const std::string& gpFile, const std::string& fpFile);
	
	Shader(const std::string& vpFile, const std::string& fpFile, const std::vector<std::pair<std::string, std::string> >& defines);
	Shader(const std::string& vpFile, const std::string& gpFile, const std::string& fpFile, const std::vector<std::pair<std::string, std::string> >& defines);

	/// DTor
	~Shader();

	inline bool isReady() const { return linkedSuccessfully; }

	void reload();


	/// Bind this shader for rendering
	void bind() const;
	/// Returns to the fixed OpenGL pipeline
	void disable() const;

	/// \name uniform variable acces
	/// \{

	void setAttributeLocation(unsigned int attributeLocation, const std::string& name);
	int getAttributeLocation(const std::string& name);

	/// returns the location of a uniform variable within the shader
	int getUniform(const std::string& name) const;

	/// returns the location of a uniform variable
	inline int operator [] (const std::string& name) const { return getUniform(name); }  

	void setUniform(const std::string& name, int i) const;
	void setUniform(const std::string& name, float f) const;
	void setUniform(const std::string& name, float x, float y) const;
	void setUniform(const std::string& name, float x, float y, float z) const;
	void setUniform(const std::string& name, float x, float y, float z, float w) const;
	void setUniform(const std::string& name, const glm::vec2& v) const;
	void setUniform(const std::string& name, const glm::vec3& v) const;
	void setUniform(const std::string& name, const glm::vec4& v) const;
	void setUniform(const std::string& name, const glm::ivec2& v) const;
	void setUniform(const std::string& name, int i, int j) const;

	/// Note that the matrix must have 16 elements!
	void setMatrix4(const std::string& name, const float* matrix) const;
	void setMatrix4(const std::string& name, const glm::mat4& m) const;
	void setMatrix4(const std::string& name, const glm::dmat4& m) const;

	void setMatrices4(const std::string& name, const std::vector<glm::mat4>& mats) const;
		
	inline void setUniform(const std::string& name, const float* matrix) const { setMatrix4(name, matrix); }
	inline void setUniform(const std::string& name, const glm::mat4& m) const { setMatrix4(name, m); }

	/** Sets a 1D texture. 
		@param textureName sampler name in the shader
		@param textureId OpenGL texture id
		@param textureUnit on which unit (usually 0-4) should the texture be bound?
	*/
	void setTexture1D(const std::string& textureName, unsigned int textureId, unsigned int textureUnit=0) const;


	/** Sets a 2D texture. 
		@param textureName sampler name in the shader
		@param textureId OpenGL texture id
		@param textureUnit on which unit (usually 0-4) should the texture be bound?
	*/
	void setTexture2D(const std::string& textureName, unsigned int textureId, unsigned int textureUnit=0) const;

	/** Sets a 3D texture.
	@param textureName sampler name in the shader
	@param textureId OpenGL texture id
	@param textureUnit on which unit (usually 0-4) should the texture be bound?
	*/
	void setTexture3D(const std::string& textureName, unsigned int textureId, unsigned int textureUnit = 0) const;


	/** Sets a cubemap texture. Note that you have to disable texturing manually
		after you're done.

		@param textureName sampler name in the shader
		@param textureId OpenGL texture id
		@param textureUnit on which unit (usually 0-4) should the texture be bound?
	*/
	void setTextureCube(const std::string& textureName, unsigned int textureId, unsigned int textureUnit=0) const;

	/// \}


private:
	unsigned int	mVertexShader, mGeometryShader, mFragmentShader;
	unsigned int	mProgram;

	std::string		mVertexSource, mGeometrySource, mFragmentSource;
	std::map<std::string, std::string>	mDefines;


	typedef std::map<std::string, int> IntMap;
	mutable IntMap	mUniformLocations;
	mutable IntMap	mAttributeLocations;

	bool			linkedSuccessfully;

	/// Links the loaded shaders
	void link();

};

#endif
