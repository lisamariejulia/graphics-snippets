/******************************************************************//**
* \brief Implementation of OpenGL shader programs.
* 
* \author  gernot
* \date    2018-08-01
* \version 1.0
**********************************************************************/#

#include <stdafx.h>


// includes

#include <OpenGLError.h>
#include <OpenGLProgram.h>


// OpenGL wrapper

#include <GL/glew.h>
//#include <GL/gl.h> not necessary because of glew 
#include <GL/glu.h>


// stl

#include <algorithm>
#include <unordered_map>
#include <cassert>
#include <sstream>


// class implementations


namespace OpenGL
{


//*********************************************************************
// CShaderObject
//*********************************************************************


const std::vector<std::tuple<Render::Program::TShaderType, size_t>> CShaderObject::c_type_map
{
  { Render::Program::TShaderType::vertex,          GL_VERTEX_SHADER },
  { Render::Program::TShaderType::tess_control,    GL_TESS_CONTROL_SHADER },
  { Render::Program::TShaderType::tess_evaluation, GL_TESS_EVALUATION_SHADER },
  { Render::Program::TShaderType::geometry,        GL_GEOMETRY_SHADER },
  { Render::Program::TShaderType::fragment,        GL_FRAGMENT_SHADER },
  { Render::Program::TShaderType::compute,         GL_COMPUTE_SHADER }
};


/******************************************************************//**
* \brief ctor  
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
CShaderObject::CShaderObject( 
  Render::Program::TShaderType type ) //!< I - shader type
  : _type( type )
{}
  

/******************************************************************//**
* \brief ctor  
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
CShaderObject::CShaderObject(
  CShaderObject && source_objet ) //!< I - source object
{
  *this = std::move( source_objet );
}
 

/******************************************************************//**
* \brief dotr   
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
CShaderObject::~CShaderObject()
{
  if ( _object != 0 )
    glDeleteShader( (GLuint)_object );
}


/******************************************************************//**
* \brief Move operator  
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
CShaderObject & CShaderObject::operator =( 
  CShaderObject && source_objet ) //!< soure object
{
  _type   = source_objet._type;
  _code   = std::move( source_objet._code );
  _object = source_objet._object;
  source_objet._object = 0;

  return *this;
}


/******************************************************************//**
* \brief Append source code to the shader.    
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
CShaderObject & CShaderObject::operator << ( 
    const std::string & code ) //!< I - source code
{
  _code.emplace_back( code );
  return *this;
}


/******************************************************************//**
* \brief Append source code to the shader.    
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
CShaderObject & CShaderObject::operator << ( 
  std::string && code ) //!< I - source code
{
  _code.emplace_back( std::move( code ) );
  return *this;
}


/******************************************************************//**
* \brief Clear internal source code.    
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
CShaderObject & CShaderObject::ClearCode( void )
{
  _code.clear();
  return *this;
}
  

/******************************************************************//**
* \brief Get the OpenGL enum constant for the shader type. 
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
size_t CShaderObject::ShaderEnum( 
  Render::Program::TShaderType type )  //!< I - shader type
{
  auto it = std::find_if( c_type_map.begin(), c_type_map.end(), [type]( auto &t ) -> bool
  {
    return std::get<0>(t) == type;
  } );
  assert( it != c_type_map.end() );
  return it != c_type_map.end() ? std::get<1>(*it ) : GL_VERTEX_SHADER;
}


/******************************************************************//**
* \brief Get the shader type from the OpenGL enum constant. 
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
Render::Program::TShaderType CShaderObject::ShaderType( 
  size_t stage )  //!< I - shader stage
{
  auto it = std::find_if( c_type_map.begin(), c_type_map.end(), [stage]( auto &t ) -> bool
  {
    return std::get<1>(t) == stage;
  } );
  assert( it != c_type_map.end() );
  return it != c_type_map.end() ? std::get<0>(*it ) : Render::Program::TShaderType::vertex;
}


/******************************************************************//**
* \brief Get the a user friendl name for the shader type. 
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
const char *CShaderObject:: ShaderTypeName( 
  Render::Program::TShaderType type )   //!< I - shader type
{
  const std::unordered_map<Render::Program::TShaderType, const char*> c_name_map
  {
    { Render::Program::TShaderType::vertex,          "vertex" },
    { Render::Program::TShaderType::tess_control,    "tessellation control" },
    { Render::Program::TShaderType::tess_evaluation, "tessellation evaluation" },
    { Render::Program::TShaderType::geometry,        "geometry" },
    { Render::Program::TShaderType::fragment,        "fragment" },
    { Render::Program::TShaderType::compute,         "compute" }
  };

  auto it = c_name_map.find( type );
  assert( it != c_name_map.end() );
  return it != c_name_map.end() ? it->second : "";
}


/******************************************************************//**
* \brief Compile the shader stage, the function succeeds, even
* if the compilation fails, but it fails if the stage was not properly
* initialized.
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
bool CShaderObject::Compile( void )
{
  if ( _object != 0 )
    glDeleteShader( (GLuint)_object );
  
  GLenum stage = (GLenum)ShaderEnum( _type );
  _object = glCreateShader( stage );
  
  std::vector<const char *> code_ptr( _code.size() );
  std::transform( _code.begin(), _code.end(), code_ptr.begin(), []( auto &str ) -> const char * { return str.c_str(); } );
  glShaderSource( (GLuint)_object, (GLsizei)code_ptr.size(), code_ptr.data(), nullptr );
  
  glCompileShader( (GLuint)_object );

  return true;
}

  
/******************************************************************//**
* \brief Verifies the compilation result.    
* 
* \author  gernot
* \date    2018-08-02
* \version 1.0
**********************************************************************/
bool CShaderObject::Verify( 
  std::string &message ) //! O - error messages
{
  message.clear();
  if ( _object == 0 )
    return false;

  GLint status = GL_TRUE;
  glGetShaderiv( _object, GL_COMPILE_STATUS, &status );
  if ( status != GL_FALSE )
    return true;
  
  GLint maxLen;
	glGetShaderiv( _object, GL_INFO_LOG_LENGTH, &maxLen );
  std::vector< char >log( maxLen );
	GLsizei len;
	glGetShaderInfoLog( _object, maxLen, &len, log.data() );
  
  std::stringstream str_stream;
  str_stream << "compile error:" << std::endl << log.data() << std::endl;
  message = str_stream.str();

  assert( false );
  return false;
}


} // OpenGL
