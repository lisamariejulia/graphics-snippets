/******************************************************************//**
* \brief   Implementation og generic interface for basic darwing.
* 
* \author  gernot
* \date    2018-02-04
* \version 1.0
**********************************************************************/

// includes

#include <stdafx.h>

// OpenGL

#include <OpenGLBasicDraw.h>
#include <OpenGLVertexBuffer.h>
#include <OpenGLFrameBuffer.h>

// OpenGL wrapper

#include <GL/glew.h>
//#include <GL/gl.h> not necessary because of glew 
#include <GL/glu.h>

// freetype

#include <ft2build.h>
#include FT_FREETYPE_H

// stl

#include <cassert>
#include <algorithm>


// class implementation

/******************************************************************//**
* \brief General OpenGL namespace
**********************************************************************/
namespace OpenGL
{

/******************************************************************//**
* \class OpenGL::CBasicDraw  
*
* Opaque and transparent shader:
* 
* If a rendered primitive hasn't a texture, a "white" 1*1 texture is
* set, this causes tha the shader has not to be changed when switching
* between color and texture.
* Since the texture color and the color are modulated (multiplied), a
* texture can be tint by a color.
*
**********************************************************************/


//---------------------------------------------------------------------
// qpaque shader
//---------------------------------------------------------------------

  std::string opaque_sh_vert = R"(
#version 460

layout (location = 0) in vec4 in_pos;
layout (location = 1) in vec2 in_tex;

out TVertexData
{
    vec3 pos;
    vec2 tex;
} out_data;


uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

void main()
{
    vec4 view_pos = u_view * u_model * in_pos; 
    out_data.pos  = view_pos.xyz / view_pos.w;
    out_data.tex  = in_tex;
    gl_Position   = u_proj * view_pos;
}
)";

std::string opaque_sh_frag = R"(
#version 460

in TVertexData
{
    vec3 pos;
    vec2 tex;
} in_data;

out vec4 frag_color;

uniform vec4 u_color;

layout (binding = 0) uniform sampler2D u_sampler_texture;

void main()
{
    vec4 col_texture  = texture(u_sampler_texture, in_data.tex.st); 
    vec4 col_modulate = u_color * col_texture;
    frag_color        = col_modulate;
}
)";


//---------------------------------------------------------------------
// transparent shader
//---------------------------------------------------------------------


std::string transp_sh_vert = R"(
#version 460

layout (location = 0) in vec4 in_pos;
layout (location = 1) in vec2 in_tex;

out TVertexData
{
    vec3 pos;
    vec2 tex;
} out_data;


uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

void main()
{
    vec4 view_pos = u_view * u_model * in_pos; 
    out_data.pos  = view_pos.xyz / view_pos.w;
    out_data.tex  = in_tex;
    gl_Position   = u_proj * view_pos;
}
)";

std::string transp_sh_frag = R"(
#version 460

in TVertexData
{
    vec3 pos;
    vec2 tex;
} in_data;

layout (location = 0) out vec4 transp_color;
layout (location = 1) out vec4 transp_attrib;

uniform vec4 u_color;

layout (binding = 0) uniform sampler2D u_sampler_texture;

void main()
{                      
    vec4 col_texture  = texture(u_sampler_texture, in_data.tex.st); 
    vec4 col_modulate = u_color * col_texture;

    float weight      = col_modulate.a * (1.0 - gl_FragCoord.z);
    transp_color      = vec4(col_modulate.rgb * weight, weight);
    transp_attrib     = vec4(1.0, 0.0, 0.0, col_modulate.a);
}
)";


//---------------------------------------------------------------------
// finish shader
//---------------------------------------------------------------------


std::string finish_sh_vert = R"(
#version 460

layout (location = 0) in vec2 in_pos;

out TVertexData
{
    vec2 pos;
} out_data;

void main()
{
    out_data.pos = in_pos;
    gl_Position  = vec4(in_pos, 0.0, 1.0);
}
)";

std::string finish_sh_frag = R"(
#version 460

in TVertexData
{
    vec2 pos;
} in_data;

out vec4 frag_color;

layout (binding = 1) uniform sampler2D u_sampler_color;
layout (binding = 2) uniform sampler2D u_sampler_transp;
layout (binding = 3) uniform sampler2D u_sampler_transp_attr;

void main()
{
    vec2 tex_st = in_data.pos.xy * 0.5 + 0.5;    
    vec4 col    = texture(u_sampler_color, tex_st);
    vec4 transp = texture(u_sampler_transp, tex_st);
    vec4 t_attr = texture(u_sampler_transp_attr, tex_st);

    //col.rgb /= (col.a > 1.0/255.0 ? col.a : 1.0); // resolve premultiplied alpha

    vec4 col_transp = vec4(0.0);
    if ( t_attr.x > 0.0 && t_attr.a > 0.0 )
    {
        vec3 colApprox      = transp.rgb / transp.a;
        float averageTransp = t_attr.w / t_attr.x;
        float alpha         = 1.0 - pow(1.0 - averageTransp, t_attr.x);
        col_transp          = vec4(colApprox, alpha); 
    }
    
    vec3 mix_col = mix(col.rgb, col_transp.rgb, col_transp.a);   
    frag_color   = vec4(mix_col.rgb, max(col.a, col_transp.a));
}
)";


//---------------------------------------------------------------------
// CBasicDraw
//---------------------------------------------------------------------


/******************************************************************//**
* @brief   ctor
*
* @author  gernot
* @date    2018-02-06
* @version 1.0
**********************************************************************/
CBasicDraw::CBasicDraw( void )
{}


/******************************************************************//**
* @brief   dtor
*
* @author  gernot
* @date    2018-02-06
* @version 1.0
*********************************************************************/
CBasicDraw::~CBasicDraw()
{
  Destroy();
}


/******************************************************************//**
* \brief   Create new or retrun existion default draw buffer
* 
* \author  gernot
* \date    2017-11-27
* \version 1.0
**********************************************************************/
Render::IDrawBuffer & CBasicDraw::DrawBuffer( void )
{
  bool cached = false;
  return DrawBuffer( nullptr, cached );
}


/******************************************************************//**
* \brief   Create new or retrun existion default draw buffer
* 
* \author  gernot
* \date    2017-11-27
* \version 1.0
**********************************************************************/
Render::IDrawBuffer & CBasicDraw::DrawBuffer( 
  const void *key,     //!< I - key for the temporary cache
  bool       &cached ) //!< O - object was found in the cache
{
  if ( key != nullptr )
  {
    // try to find temporary buffer which already has the data
    auto keyIt = std::find( _buffer_keys.begin(), _buffer_keys.end(), key );
    if ( keyIt != _buffer_keys.end() )
    {
      size_t inx = keyIt - _buffer_keys.begin();
      if ( _nextBufferI == inx )
        _nextBufferI = _nextBufferI < _max_buffers - 1 ? _nextBufferI + 1 : 0;
      
      Render::IDrawBuffer *foundBuffer = _draw_buffers[inx];
      cached = true;
      return *foundBuffer;
    }
  }

  // ensure the buffer object is allocated
  if ( _draw_buffers[_nextBufferI] == nullptr )
    _draw_buffers[_nextBufferI]= NewDrawBuffer( Render::TDrawBufferUsage::stream_draw );
  
  // get buffer object
  Render::IDrawBuffer *currentBuffer = _draw_buffers[_nextBufferI];
  _buffer_keys[_nextBufferI] = key;
  _nextBufferI = _nextBufferI < _max_buffers - 1 ? _nextBufferI + 1 : 0;

  cached = false;
  return *currentBuffer;
}


/******************************************************************//**
* \brief   Create a new and empty draw buffer object.
* 
* \author  gernot
* \date    2017-11-26
* \version 1.0
**********************************************************************/
Render::IDrawBuffer * CBasicDraw::NewDrawBuffer( 
  Render::TDrawBufferUsage usage ) //!< I - usage of draw buffer : static_draw, dynamic_draw or stream_draw
{
  return new OpenGL::CDrawBuffer( usage, 1024 ); 
}


/******************************************************************//**
* @brief   destroy internal GPU objects
*
* @author  gernot
* @date    2018-02-06
* @version 1.0
**********************************************************************/
void CBasicDraw::Destroy( void )
{
  _initialized = false;
  _drawing     = false;

  _opaque_prog.reset( nullptr );
  _transp_prog.reset( nullptr );
  _finish_prog.reset( nullptr );

  for ( auto & buffer : _draw_buffers )
  {
    delete buffer;
    buffer = nullptr;
  }
  for ( auto & key : _buffer_keys )
    key = nullptr;
  _nextBufferI = 0;

  glDeleteTextures( 1, &_color_texture );
  _color_texture = 0;

  _fonts.clear();
}


/******************************************************************//**
* \brief   Returns a font by its id.
* 
* The font is loaded, if not loaded yet.
*
* Most Popular Fonts [https://www.fontsquirrel.com/fonts/list/popular]
* Fonts [https://www.fontsquirrel.com/]
* 
* \author  gernot
* \date    2018-03-18
* \version 1.0
**********************************************************************/
bool CBasicDraw::LoadFont( 
  TFontId         font_id, //!< in: id of the font
  Render::IFont *&font )   //!< in: the font
{
  auto it = _fonts.find( font_id );
  if ( it != _fonts.end() )
  {
    font = it->second.get();
    return true;
  }

  std::string font_finename;
  switch( font_id )
  {
    default: break;

    case font_sans:        font_finename = "../resource/font/FreeSans.ttf"; break;
    case font_symbol:      font_finename = "../resource/font/symbol.ttf"; break;
    case font_pcifico:     font_finename = "../resource/font/Pacifico.ttf"; break;
    case font_alura:       font_finename = "../resource/font/Allura-Regular.otf"; break;
    case font_grandhotel:  font_finename = "../resource/font/GrandHotel-Regular.otf"; break;
    case font_greatevibes: font_finename = "../resource/font/GreatVibes-Regular.otf"; break;
  }

  if ( font_finename.empty() )
  {
    assert( false );
    return false;
  }

  Render::IFont *newFont = nullptr;
  try
  {
    newFont = new CFreetypeTexturedFont( font_finename.c_str() );
    newFont->Load();
    _fonts[font_id].reset( newFont );
  }
  catch (...)
  {
    return false;
  }

  font = newFont;
  return newFont != nullptr;
}


/******************************************************************//**
* \brief   General initializations.
*
* Specify the render buffers
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::Init( void )
{
  if ( _initialized )
    return true;

  // specify render process
  SpecifyRenderProcess();

  // draw shader
  try
  {
    _opaque_prog = std::make_unique<OpenGL::ShaderProgram>(
      std::vector< TShaderInfo >{
        { opaque_sh_vert, GL_VERTEX_SHADER },
        { opaque_sh_frag, GL_FRAGMENT_SHADER }
      } );
  }
  catch (...)
  {}

   // transparent shader
  try
  {
    _transp_prog = std::make_unique<OpenGL::ShaderProgram>(
      std::vector< TShaderInfo >{
        { transp_sh_vert, GL_VERTEX_SHADER },
        { transp_sh_frag, GL_FRAGMENT_SHADER }
      } );
  }
  catch (...)
  {}

  // finish shader
  try
  {
    _finish_prog.reset( new OpenGL::ShaderProgram(
      {
        { finish_sh_vert, GL_VERTEX_SHADER },
        { finish_sh_frag, GL_FRAGMENT_SHADER }
      } ) );
  }
  catch (...)
  {}

  // setup color texture
  glActiveTexture( GL_TEXTURE0 );
  glGenTextures( 1, &_color_texture );
  glBindTexture( GL_TEXTURE_2D, _color_texture );      
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

  unsigned char white[]{ 255, 255, 255, 255 };
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white );

  glBindTexture( GL_TEXTURE_2D, 0 );  


  // TODO $$$ uniform block model, view, projection
  
  _initialized = true;
  return true;
}


/******************************************************************//**
* \brief   Causes update of the render process
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
void CBasicDraw::InvalidateProcess( void )
{
  if ( _process != nullptr )
    _process->Invalidate();
}


/******************************************************************//**
* \brief   Causes update of the uniforms
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
void CBasicDraw::InvalidateUniforms( void )
{
  _unifroms_valid = false;
  // TODO $$$ invalidate uniform block
}


/******************************************************************//**
* \brief   Specifies the render process
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::SpecifyRenderProcess( void )
{
  if ( _vp_size[0] == 0 || _vp_size[1] == 0 )
    return false;
  if ( _process == nullptr )
    _process = std::make_unique<OpenGL::CRenderProcess>();
  if ( _process->IsValid() && _process->IsComplete() && _process->CurrentSize() == _vp_size )
    return true;

  const size_t c_depth_ID        = 0;
  const size_t c_color_ID        = 1;
  const size_t c_transp_ID       = 2;
  const size_t c_transp_attr_ID  = 3;
  
  Render::IRenderProcess::TBufferMap buffers;
  buffers.emplace( c_depth_ID,       Render::TBuffer( Render::TBufferType::DEPTH,  Render::TBufferDataType::DEFAULT ) );
  buffers.emplace( c_color_ID,       Render::TBuffer( Render::TBufferType::COLOR4, Render::TBufferDataType::DEFAULT ) );
  buffers.emplace( c_transp_ID,      Render::TBuffer( Render::TBufferType::COLOR4, Render::TBufferDataType::F16 ) );
  buffers.emplace( c_transp_attr_ID, Render::TBuffer( Render::TBufferType::COLOR4, Render::TBufferDataType::F16 ) );

  Render::IRenderProcess::TPassMap passes;

  Render::TPass opaque_pass( Render::TPassDepthTest::LESS, Render::TPassBlending::OFF );
  opaque_pass._targets.emplace_back( c_depth_ID, Render::TPass::TTarget::depth ); // depth target
  opaque_pass._targets.emplace_back( c_color_ID, 0 );                             // color target
  passes.emplace( c_opaque_pass, opaque_pass );

  Render::TPass transp_pass( Render::TPassDepthTest::LESS_READONLY, Render::TPassBlending::ADD );
  transp_pass._targets.emplace_back( c_depth_ID, Render::TPass::TTarget::depth, false ); // depth target
  transp_pass._targets.emplace_back( c_transp_ID, 0 );                                   // tranparency target
  transp_pass._targets.emplace_back( c_transp_attr_ID, 1 );                              // tranparency attribute target (adaptive tranparency)
  passes.emplace( c_tranp_pass, transp_pass );

  Render::TPass background_pass( Render::TPassDepthTest::OFF, Render::TPassBlending::OFF );
  passes.emplace( c_back_pass, background_pass );  // TODO $$$ set default clear

  Render::TPass finish_pass( Render::TPassDepthTest::OFF, Render::TPassBlending::MIX );
  finish_pass._sources.emplace_back( c_color_ID,       1 ); // color buffer source
  finish_pass._sources.emplace_back( c_transp_ID,      2 ); // tranparency buffer source
  finish_pass._sources.emplace_back( c_transp_attr_ID, 3 ); // transparency attribute buffer source
  passes.emplace( c_finish_pass, finish_pass );  // TODO $$$ set default clear off

  _process->SpecifyBuffers( buffers );
  _process->SpecifyPasses( passes );

  return _process->Create( _vp_size );
}


/******************************************************************//**
* \brief   Set the uniforms and update the unform blocks.
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::UpdateGeneralUniforms( void )
{
  if ( _current_prog == nullptr )
    return false;
  if (_unifroms_valid )
    return true;

  // setup uniforms
  // TODO $$$ automatic type selector, uniform type introspection 
  // { { "u_proj", _projection },{ "u_view", _view }, { "u_model", _model } } ;
  _current_prog->SetUniformM44( "u_proj",  _projection );
  _current_prog->SetUniformM44( "u_view",  _view );
  _current_prog->SetUniformM44( "u_model", _model );

  _unifroms_valid = true;
  return true;
}


/******************************************************************//**
* \brief   Set the uniforms and update the unform blocks.
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::UpdateColorUniforms( 
  const Render::TColor &color ) //!< in: new color
{
  if ( _current_prog == nullptr )
    return false;
 
  _current_prog->SetUniformF4( "u_color", color );
  return true;
}


/******************************************************************//**
* \brief   Draw full screen sapce.
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
void CBasicDraw::DrawScereenspace( void )
{
  const std::vector<char>  bufferdescr = Render::IDrawBuffer::VADescription( Render::TVA::b0_xy );
  const std::vector<float> coords{ -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };

  Render::IDrawBuffer &buffer = DrawBuffer();
  buffer.SpecifyVA( bufferdescr.size(), bufferdescr.data() );
  buffer.UpdateVB( 0, sizeof(float), coords.size(), coords.data() );

  buffer.DrawArray( Render::TPrimitive::trianglestrip, 0, 4, true );
  buffer.Release();
}


/******************************************************************//**
* \brief   Start the rendering.
*
* Specify the render buffers
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::Begin( void )
{
  if ( _drawing || Init() == false )
  {
    assert( false );
    return false;
  }

  // specify render process
  SpecifyRenderProcess();
  _process->PrepareClear( c_opaque_pass );
  _process->PrepareClear( c_tranp_pass );

  glClearColor( _bg_color[0], _bg_color[1], _bg_color[2], _bg_color[3] );
  _process->Prepare( c_back_pass );
  
  _current_pass = 0;
  _drawing      = true;
  return true;
}


/******************************************************************//**
* \brief   Prepares and activates render to background.
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::ActivateBackground( void )
{
  if ( _drawing == false )
  {
    assert( false );
    return false;
  }

  // activate pass
  _current_pass = c_back_pass;
  _process->PrepareNoClear( _current_pass );

  // activate shader
  _current_prog = _opaque_prog.get();
  _current_prog->Use();
  
  // set uniforms
  _unifroms_valid = false;
  UpdateGeneralUniforms();
  
  return true;
}


/******************************************************************//**
* \brief   Prepares and activates render to the opaque buffer.
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::ActivateOpaque( void )
{
  if ( _drawing == false )
  {
    assert( false );
    return false;
  }

  // activate pass
  _current_pass = c_opaque_pass;
  _process->PrepareNoClear( _current_pass );
  //_process->Prepare( _current_pass );

  // activate shader
  _current_prog = _opaque_prog.get();
  _current_prog->Use();
  _unifroms_valid = false;

  // set uniforms
  _unifroms_valid = false;
  UpdateGeneralUniforms();

  return true;
}


/******************************************************************//**
* \brief   Prepares and activates render  to the transparent buffer
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::ActivateTransparent( void )
{
  if ( _drawing == false )
  {
    assert( false );
    return false;
  }

  // activate pass
  _current_pass = c_tranp_pass;
  _process->PrepareNoClear( _current_pass );

  // activate shader
  _current_prog = _transp_prog.get();
  _current_prog->Use();
  _unifroms_valid = false;

  // set uniforms
  _unifroms_valid = false;
  UpdateGeneralUniforms();

  return true;
}


/******************************************************************//**
* \brief   Finish the rendering.
*
* Finally write to the default frame buffer.
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::Finish( void )
{
  if ( _drawing == false )
  {
    assert( false );
    return false;
  }

  _process->PrepareNoClear( c_finish_pass );
  _finish_prog->Use();
  DrawScereenspace();
  glUseProgram( 0 );

  _current_pass = 0;
  _drawing      = false;
  return true;
}


/******************************************************************//**
* \brief   Interim clear of the depth buffer.
* 
* \author  gernot
* \date    2018-03-16
* \version 1.0
**********************************************************************/
bool CBasicDraw::ClearDepth( void )
{
  if ( _drawing == false )
  {
    assert( false );
    return false;
  }

  _process->PrepareNoClear( c_opaque_pass );
  glClear( GL_DEPTH_BUFFER_BIT );
  _process->Release();

  return true;
}


/******************************************************************//**
* \brief   Draw an array od primitives with a single color
* 
* \author  gernot
* \date    2018-03-15
* \version 1.0
**********************************************************************/
bool CBasicDraw::Draw( 
  Render::TPrimitive    primitive_type, //!< in: type of the primitives
  size_t                size,           //!< in: size vertex coordiantes
  size_t                coords_size,    //!< in: size of coordinate buffer
  const Render::t_fp   *coords,         //!< in: coordiant buffer
  const Render::TColor &color,          //!< in: color for drawing
  const TStyle         &style )         //!< in: additional style parameters 
{
  if ( _drawing == false )
  {
    assert( false );
    return false;
  }

  if ( size != 2 && size !=3 && size !=4 )
  {
    assert( false );
    return false;
  }

  bool is_point = primitive_type == Render::TPrimitive::points;

  bool is_line =
    primitive_type == Render::TPrimitive::lines ||
    primitive_type == Render::TPrimitive::lines_adjacency ||
    primitive_type == Render::TPrimitive::linestrip ||
    primitive_type == Render::TPrimitive::linestrip_adjacency ||
    primitive_type == Render::TPrimitive::lineloop;

  bool is_polygon =
    primitive_type == Render::TPrimitive::triangles ||
    primitive_type == Render::TPrimitive::triangle_adjacency ||
    primitive_type == Render::TPrimitive::trianglestrip ||
    primitive_type == Render::TPrimitive::trianglestrip_adjacency ||
    primitive_type == Render::TPrimitive::trianglefan;

  // buffer specification
  Render::TVA va_id = size == 2 ? Render::TVA::b0_xy : (size == 3 ? Render::TVA::b0_xyz : Render::TVA::b0_xyzw);
  const std::vector<char> bufferdescr = Render::IDrawBuffer::VADescription( va_id );

  // create buffer
  Render::IDrawBuffer &buffer = DrawBuffer();
  buffer.SpecifyVA( bufferdescr.size(), bufferdescr.data() );
  buffer.UpdateVB( 0, sizeof(float), coords_size, coords );
  
  // set style and context
  if ( is_line )
  {
    glEnable( GL_POLYGON_OFFSET_FILL );
    glPolygonOffset( 1.0, 1.0 );
    glLineWidth( style._thickness );
  }

  // set uniforms
  UpdateGeneralUniforms();
  UpdateColorUniforms( color );

  // bind "white" color texture
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, _color_texture ); 

  // draw_buffer
  size_t no_of_vertices = coords_size / size;
  buffer.DrawArray( primitive_type, 0, no_of_vertices, true );
  buffer.Release();

  // reset style and context
  if ( is_line )
  {
    glDisable( GL_POLYGON_OFFSET_FILL );
    glLineWidth( 1.0f );
  }

  return true;
}


/******************************************************************//**
* \brief   Render a text
* 
* \author  gernot
* \date    2018-03-18
* \version 1.0
**********************************************************************/
bool CBasicDraw::DrawText( 
  TFontId                font_id,     //!< in: id of the font
  const char            *text,        //!< in: the text
  float                  height,      //!< in: the height of the text
  float                  width_scale, //!< in: scale of the text in the y direction
  const Render::TPoint3 &pos,         //!< in: reference position
  const Render::TColor  &color )      //!< in: color of the text
{
  if ( _drawing == false )
  {
    assert( false );
    return false;
  }

  // get the font (load the font if not loaded yet)
  Render::IFont *font = nullptr;
  if ( LoadFont( font_id, font ) == false )
    return false;

  // TODO $$$ generalise
  CFreetypeTexturedFont *freetypeFont = dynamic_cast<CFreetypeTexturedFont*>( font );
  if ( freetypeFont == nullptr )
    return false;

  // set uniforms
  UpdateGeneralUniforms();
  UpdateColorUniforms( color );

  // bind "white" color texture
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, _color_texture );

  // set blending
  bool set_depth_and_belnding = _current_pass == c_opaque_pass || _current_pass == c_back_pass || _current_pass == 0;
  if ( set_depth_and_belnding )
  {
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );
    glDepthMask( GL_TRUE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA ); // premulitplied alpha
    //glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); // D * (1-alpha) + S * alpha
  }

  // TODO $$$ generalise
  bool ret = freetypeFont->DrawText( *this, text, height, width_scale, pos );

  // reset blending
  if ( set_depth_and_belnding )
  {
    if ( _process != nullptr && _current_pass != 0 )
    {
      _process->PrepareMode( _current_pass );
    }
    else
    {
      glEnable( GL_DEPTH_TEST );
      glDepthFunc( GL_LESS );
      glDepthMask( GL_TRUE );
      glDisable( GL_BLEND );
    }
  }

  return ret;
}


//---------------------------------------------------------------------
// CFreetypeTextureText
//---------------------------------------------------------------------


struct TFreetypeGlyph
{
  FT_Glyph_Metrics           _metrics { 0 }; //!< glyph metrics
  unsigned int               _x       = 0;   //!< glyph start x
  unsigned int               _y       = 0;   //!< glyph start y
  unsigned int               _cx      = 0;   //!< glyph width
  unsigned int               _cy      = 0;   //!< glyph height
  std::vector<unsigned char> _image;         //!< image data
};

/******************************************************************//**
* @brief   freetype font data 
*
* @author  gernot
* @date    2018-03-18
* @version 1.0
**********************************************************************/
struct TFreetypeTFont
{
  FT_Library                  _hdl;              //!< library handle
  FT_Face                     _face;             //!< font face
  unsigned int                _width        = 0; //!< total length 
  unsigned int                _max_height   = 0; //!< maximum height
  int                         _max_glyph_cy = 0; //!< maximum glyph metrics height 
  int                         _max_glyph_y  = 0; //!< maximum glyph metrics bearing y 
  int                         _min_char     = 0; //!< minimum character
  int                         _max_char     = 0; //!< maximum character
  std::vector<TFreetypeGlyph> _glyphs;           //!< glyph information
};


/******************************************************************//**
* @brief   ctor.
*
* @author  gernot
* @date    2018-03-18
* @version 1.0
**********************************************************************/
CFreetypeTexturedFont::CFreetypeTexturedFont( 
  const char *font_filename ) //!< in: path of the font file 
  : _font_filename( font_filename )
{}


/******************************************************************//**
* @brief   dtor.
*
* @author  gernot
* @date    2018-03-18
* @version 1.0
**********************************************************************/
CFreetypeTexturedFont::~CFreetypeTexturedFont()
{}


/******************************************************************//**
* @brief   Destroy all internal objects and cleanup.
*
* @author  gernot
* @date    2018-03-18
* @version 1.0
**********************************************************************/
void CFreetypeTexturedFont::Destroy( void )
{
  _font.reset( nullptr );

  glDeleteTextures( 1, &_texture_obj );
  _texture_obj = 0;

  // ...
}


/******************************************************************//**
* @brief   Load the glyphs.
*
* @author  gernot
* @date    2018-03-18
* @version 1.0
**********************************************************************/
bool CFreetypeTexturedFont::Load( void )
{
  if ( _font != nullptr )
    return _valid;

  _font = std::make_unique<TFreetypeTFont>();
  TFreetypeTFont &data = *_font.get();


  // init freetype library
  FT_Error err_code = FT_Init_FreeType( &_font->_hdl );
  if ( err_code != 0 )
  {          
    std::cout << "error: failed to initilaize freetype library (error code: " << err_code << ")" << std::endl;
    throw std::runtime_error( "init freetype library" );
  }

  // load the font from file
  err_code = FT_New_Face( _font->_hdl, _font_filename.c_str(), 0, &_font->_face );
  if ( err_code != 0 )
  {          
    std::cout << "error: failed to load font file " << _font_filename << " (error code: " << err_code << ")" << std::endl;
    throw std::runtime_error( "load font file" );
  }

  FT_Face face = _font->_face;

  // set font size
  static FT_UInt pixel_width  = 0;
  static FT_UInt pixel_height = 48;
  err_code = FT_Set_Pixel_Sizes( face, pixel_width, pixel_height );
  if ( err_code != 0 )
  {          
    std::cout << "error: failed to set font size (error code: " << err_code << ")" << std::endl;
    throw std::runtime_error( "load font file" );
  }

  FT_GlyphSlot glyph = face->glyph;

  // evaluate texture size  and metrics
  // FreeType Glyph Conventions [https://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html]

  data._width    = 1;  
  data._min_char = 32;
  data._max_char = 256;
  data._glyphs  = std::vector<TFreetypeGlyph>( data._max_char - data._min_char );
  for ( int i = data._min_char; i < data._max_char ; ++ i )
  {
    FT_Error err_code_glyph = FT_Load_Char( face, i, FT_LOAD_RENDER );
    if ( err_code_glyph != 0 )
      continue;

    unsigned int cx = glyph->bitmap.width;
    unsigned int cy = glyph->bitmap.rows;
    
    TFreetypeGlyph &glyph_data = data._glyphs[i-32];

    glyph_data._metrics = glyph->metrics;
    glyph_data._x       = data._width;
    glyph_data._y       = 1;
    glyph_data._cx      = cx;
    glyph_data._cy      = cy;
    
    glyph_data._image = std::vector<unsigned char>( cx * cy * 4, 0 );
    for ( unsigned int i = 0; i < cx * cy; ++ i)
    {
      unsigned char b = glyph->bitmap.buffer[i];
      if ( i > 0 )
      {
        glyph_data._image[i*4 + 0] = b;
        glyph_data._image[i*4 + 1] = b;
        glyph_data._image[i*4 + 2] = b;
        glyph_data._image[i*4 + 3] = b;
      }
    }
    
    data._width      += cx+1;
    data._max_height  = std::max( data._max_height, cy );

    data._max_glyph_cy = std::max( data._max_glyph_cy, (int)glyph->metrics.height );
    data._max_glyph_y  = std::max( data._max_glyph_y,  (int)glyph->metrics.horiBearingY );
  }
  data._max_height += 2;

  // create texture

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
  glActiveTexture( GL_TEXTURE0 );
  glGenTextures( 1, &_texture_obj );
  glBindTexture( GL_TEXTURE_2D, _texture_obj );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, data._width, data._max_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 ); 

  // load glyohs and copy glyphs to texture

  for ( int i = data._min_char; i < data._max_char; ++ i )
  {
    TFreetypeGlyph &glyph_data = data._glyphs[i-data._min_char];
    if ( glyph_data._cx == 0 || glyph_data._cy == 0 )
      continue;

    glTexSubImage2D(GL_TEXTURE_2D, 0, glyph_data._x, glyph_data._y, glyph_data._cx, glyph_data._cy, GL_RGBA, GL_UNSIGNED_BYTE, glyph_data._image.data() );
  }
  glBindTexture( GL_TEXTURE_2D, 0 );
  glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

  _valid = true;
  return _valid;
}


/******************************************************************//**
* \brief   Calculates box of a string, in relation to its height
* (maximum height of the font from the bottom to the top)  
* 
* \author  gernot
* \date    2018-03-18
* \version 1.0
**********************************************************************/
bool CFreetypeTexturedFont::CalculateTextSize( 
  const char *str,      //!< in: the text
  float       height,   //!< in: the maximum height of the text from the bottom to the top  
  float      &box_x,    //!< out: with of the text
  float      &box_btm,  //!< out: height from the base line to the top 
  float      &box_top ) //!< out: height form the base line to the bottom (usually negative)
{
  if ( _font == nullptr )
    return false;

  int min_c = _font->_min_char;
  int max_c = _font->_max_char;

  std::string std_str( str );
  FT_Pos metrics_width  = 0;
  FT_Pos metrics_height = 0;
  FT_Pos metrics_top    = 0;
  for ( char c : std_str )
  {
    // get the glyph data
    int i_c = *(unsigned char*)(&c);
    if ( i_c < min_c || i_c > max_c )
      continue;
    TFreetypeGlyph &glyph = _font->_glyphs[i_c-min_c];
    
    metrics_width += glyph._metrics.horiAdvance;
    metrics_height = std::max( metrics_height, glyph._metrics.height );
    metrics_top    = std::max( metrics_top, glyph._metrics.horiBearingY );
  }

  float scale = height / (float)_font->_max_glyph_cy;

  box_x   = scale * metrics_width;
  box_btm = scale * metrics_top;
  box_top = scale * (metrics_top - metrics_height);

  return true;
}


/******************************************************************//**
* \brief   render a text 
* 
* \author  gernot
* \date    2018-03-18
* \version 1.0
**********************************************************************/
bool CFreetypeTexturedFont::DrawText( 
  CBasicDraw            &draw,        //!< in: draw library
  const char            *str,         //!< in: the text
  float                  height,      //!< in: the maximum height of the text from the bottom to the top 
  float                  width_scale, //!< in: scale of the text in the y direction
  const Render::TPoint3 &pos )        //!< in: the reference position
{
  static bool debug_test = false;
  if ( debug_test )
    DebugFontTexture( draw );

  if ( _font == nullptr )
    return false;

  int min_c = _font->_min_char;
  int max_c = _font->_max_char;

  float scale_x = height / (float)_font->_max_glyph_cy;
  float scale_y = scale_x * width_scale;

  // set up vertex coordinate attribute array

  std::string std_str( str );
  FT_Pos metrics_width  = 0;
  std::vector<float> vertex_attributes; // x y z u v
  vertex_attributes.reserve( 5 * 6 * std_str.length() );
  for ( char c : std_str )
  {
    // get the glyph data
    int i_c = *(unsigned char*)(&c);
    if ( i_c < min_c || i_c > max_c )
      continue;
    TFreetypeGlyph &glyph = _font->_glyphs[i_c-min_c];
    
    // clculate font metrics vertex cooridnate box
    FT_Pos metrics_coord[]{
      metrics_width + glyph._metrics.horiBearingX,                        // min x
      glyph._metrics.horiBearingY - glyph._metrics.height,                // min y
      metrics_width + glyph._metrics.horiBearingX + glyph._metrics.width, // max x
      glyph._metrics.horiBearingY                                         // max y
    };

    // incerement width to the start of the next glyph
    metrics_width += glyph._metrics.horiAdvance;

    if ( glyph._metrics.width == 0 || glyph._metrics.height == 0 )
      continue;

    // clculate vertex coordinate box
    float glyph_coords[]{
      pos[0] + scale_x * (float)metrics_coord[0], pos[1] + scale_y * (float)metrics_coord[1],
      pos[0] + scale_x * (float)metrics_coord[2], pos[1] + scale_y * (float)metrics_coord[3]
    };

    // calculate texture coordiantes box
    float glyph_tex_coords[]{
      (float)glyph._x / (float)_font->_width,
      (float)(glyph._y + glyph._cy) / (float)_font->_max_height,
      (float)(glyph._x + glyph._cx) / (float)_font->_width,
      (float)glyph._y / (float)_font->_max_height,
    };

    // set up vertex attribute array
    std::array<std::array<float, 5>, 4> quad{
      std::array<float, 5>{ glyph_coords[0], glyph_coords[1], pos[2], glyph_tex_coords[0], glyph_tex_coords[1] },
      std::array<float, 5>{ glyph_coords[2], glyph_coords[1], pos[2], glyph_tex_coords[2], glyph_tex_coords[1] },
      std::array<float, 5>{ glyph_coords[2], glyph_coords[3], pos[2], glyph_tex_coords[2], glyph_tex_coords[3] },
      std::array<float, 5>{ glyph_coords[0], glyph_coords[3], pos[2], glyph_tex_coords[0], glyph_tex_coords[3] }
    };
    std::array<int, 6> indices{ 0, 1, 2, 0, 2, 3 };
    for ( auto i : indices )
      vertex_attributes.insert( vertex_attributes.end(), quad[i].begin(), quad[i].end() );
  }

  // buffer specification
  Render::TVA va_id = Render::TVA::b0_xyz_uv;
  const std::vector<char> bufferdescr = Render::IDrawBuffer::VADescription( va_id );

  // create buffer
  Render::IDrawBuffer &buffer = draw.DrawBuffer();
  buffer.SpecifyVA( bufferdescr.size(), bufferdescr.data() );
  buffer.UpdateVB( 0, sizeof(float), vertex_attributes.size(), vertex_attributes.data() );
  
  // bind glyph texture 
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, _texture_obj );

  // draw_buffer
  size_t no_of_vertices = vertex_attributes.size() / 5; // 5 because of x y z u v
  buffer.DrawArray( Render::TPrimitive::triangles, 0, no_of_vertices, true ); // TODO Render::TPrimitive::trianglestrip + indices / primitive restart
  buffer.Release();

  // unbind glyph texture
  glBindTexture( GL_TEXTURE_2D, 0 );

  return true;
}


/******************************************************************//**
* \brief   Draw the entire font texture for debug reasons.
* 
* \author  gernot
* \date    2018-03-18
* \version 1.0
**********************************************************************/
void CFreetypeTexturedFont::DebugFontTexture( 
  CBasicDraw &draw ) //!< in: draw library
{
  if ( _font == nullptr )
    return;

  int min_c = _font->_min_char;
  int max_c = _font->_max_char;

  float t_0 = 0.0;
  float t_1 = 0.4f;
  float h = 10.0f * (float)_font->_max_height / (float)_font->_width;

  // setup vertex atributes (x y z u v)
  std::vector<float> vertex_attributes{
    -1.0f,  -1.0f, 0.0f, t_0, 1.0f,
     1.0f,  -1.0f, 0.0f, t_1, 1.0f,
     1.0f, h-1.0f, 0.0f, t_1, 0.0f,
    
    -1.0f,  -1.0f, 0.0f, t_0, 1.0f,
     1.0f, h-1.0f, 0.0f, t_1, 0.0f,
    -1.0f, h-1.0f, 0.0f, t_0, 0.0f,
  };

  // buffer specification
  Render::TVA va_id = Render::TVA::b0_xyz_uv;
  const std::vector<char> bufferdescr = Render::IDrawBuffer::VADescription( va_id );

  // create buffer
  Render::IDrawBuffer &buffer = draw.DrawBuffer();
  buffer.SpecifyVA( bufferdescr.size(), bufferdescr.data() );
  buffer.UpdateVB( 0, sizeof(float), vertex_attributes.size(), vertex_attributes.data() );
  
  // bind glyph texture 
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, _texture_obj );

  // draw_buffer
  size_t no_of_vertices = vertex_attributes.size() / 5; // 5 because of x y z u v
  buffer.DrawArray( Render::TPrimitive::triangles, 0, no_of_vertices, true ); // TODO Render::TPrimitive::trianglestrip + indices / primitive restart
  buffer.Release();

  // unbind glyph texture
  glBindTexture( GL_TEXTURE_2D, 0 );
}

} // OpenGL