#ifndef AVEN_GL_H
#define AVEN_GL_H

#include <GLES2/gl2.h>

typedef struct {
    PFNGLACTIVETEXTUREPROC ActiveTexture;
    PFNGLATTACHSHADERPROC AttachShader;
    PFNGLBINDATTRIBLOCATIONPROC BindAttribLocation;
    PFNGLBINDBUFFERPROC BindBuffer;
    PFNGLBINDFRAMEBUFFERPROC BindFramebuffer;
    PFNGLBINDRENDERBUFFERPROC BindRenderbuffer;
    PFNGLBINDTEXTUREPROC BindTexture;
    PFNGLBLENDCOLORPROC BlendColor;
    PFNGLBLENDEQUATIONPROC BlendEquation;
    PFNGLBLENDEQUATIONSEPARATEPROC BlendEquationSeparate;
    PFNGLBLENDFUNCPROC BlendFunc;
    PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate;
    PFNGLBUFFERDATAPROC BufferData;
    PFNGLBUFFERSUBDATAPROC BufferSubData;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus;
    PFNGLCLEARPROC Clear;
    PFNGLCLEARCOLORPROC ClearColor;
    PFNGLCLEARDEPTHFPROC ClearDepth;
    PFNGLCLEARSTENCILPROC ClearStencil;
    PFNGLCOLORMASKPROC ColorMask;
    PFNGLCOMPILESHADERPROC CompileShader;
    PFNGLCOMPRESSEDTEXIMAGE2DPROC CompressedTexImage2D;
    PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC CompressedTexSubImage2D;
    PFNGLCOPYTEXIMAGE2DPROC CopyTexImage2D;
    PFNGLCOPYTEXSUBIMAGE2DPROC CopyTexSubImage2D;
    PFNGLCREATEPROGRAMPROC CreateProgram;
    PFNGLCREATESHADERPROC CreateShader;
    PFNGLCULLFACEPROC CullFace;
    PFNGLDELETEBUFFERSPROC DeleteBuffers;
    PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers;
    PFNGLDELETEPROGRAMPROC DeleteProgram;
    PFNGLDELETERENDERBUFFERSPROC DeleteRenderbuffers;
    PFNGLDELETESHADERPROC DeleteShader;
    PFNGLDELETETEXTURESPROC DeleteTextures;
    PFNGLDEPTHFUNCPROC DepthFunc;
    PFNGLDEPTHMASKPROC DepthMask;
    PFNGLDEPTHRANGEFPROC DepthRangef;
    PFNGLDETACHSHADERPROC DetachShader;
    PFNGLDISABLEPROC Disable;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray;
    PFNGLDRAWARRAYSPROC DrawArrays;
    PFNGLDRAWELEMENTSPROC DrawElements;
    PFNGLENABLEPROC Enable;
    PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
    PFNGLFINISHPROC Finish;
    PFNGLFLUSHPROC Flush;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer;
    PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D;
    PFNGLFRONTFACEPROC FrontFace;
    PFNGLGENBUFFERSPROC GenBuffers;
    PFNGLGENERATEMIPMAPPROC GenerateMipmap;
    PFNGLGENFRAMEBUFFERSPROC GenFramebuffers;
    PFNGLGENRENDERBUFFERSPROC GenRenderbuffers;
    PFNGLGENTEXTURESPROC GenTextures;
    PFNGLGETACTIVEATTRIBPROC GetActiveAttrib;
    PFNGLGETACTIVEUNIFORMPROC GetActiveUniform;
    PFNGLGETATTACHEDSHADERSPROC GetAttachedShaders;
    PFNGLGETATTRIBLOCATIONPROC GetAttribLocation;
    PFNGLGETBOOLEANVPROC GetBooleanv;
    PFNGLGETBUFFERPARAMETERIVPROC GetBufferParameteriv;
    PFNGLGETERRORPROC GetError;
    PFNGLGETFLOATVPROC GetFloatv;
    PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC
        GetFramebufferAttachmentParameteriv;
    PFNGLGETINTEGERVPROC GetIntegerv;
    PFNGLGETPROGRAMIVPROC GetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog;
    PFNGLGETRENDERBUFFERPARAMETERIVPROC GetRenderbufferParameteriv;
    PFNGLGETSHADERIVPROC GetShaderiv;
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;
    PFNGLGETSHADERPRECISIONFORMATPROC GetShaderPrecisionFormat;
    PFNGLGETSHADERSOURCEPROC GetShaderSource;
    PFNGLGETSTRINGPROC GetString;
    PFNGLGETTEXPARAMETERFVPROC GetTexParameterfv;
    PFNGLGETTEXPARAMETERIVPROC GetTexParameteriv;
    PFNGLGETUNIFORMFVPROC GetUniformfv;
    PFNGLGETUNIFORMIVPROC GetUniformiv;
    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
    PFNGLGETVERTEXATTRIBFVPROC GetVertexAttribfv;
    PFNGLGETVERTEXATTRIBIVPROC GetVertexAttribiv;
    PFNGLGETVERTEXATTRIBPOINTERVPROC GetVertexAttribPointerv;
    PFNGLHINTPROC Hint;
    PFNGLISBUFFERPROC IsBuffer;
    PFNGLISENABLEDPROC IsEnabled;
    PFNGLISFRAMEBUFFERPROC IsFramebuffer;
    PFNGLISPROGRAMPROC IsProgram;
    PFNGLISRENDERBUFFERPROC IsRenderbuffer;
    PFNGLISSHADERPROC IsShader;
    PFNGLISTEXTUREPROC IsTexture;
    PFNGLLINEWIDTHPROC LineWidth;
    PFNGLLINKPROGRAMPROC LinkProgram;
    PFNGLPIXELSTOREIPROC PixelStorei;
    PFNGLPOLYGONOFFSETPROC PolygonOffset;
    PFNGLREADPIXELSPROC ReadPixels;
    PFNGLRELEASESHADERCOMPILERPROC ReleaseShaderCompiler;
    PFNGLRENDERBUFFERSTORAGEPROC RenderbufferStorage;
    PFNGLSAMPLECOVERAGEPROC SampleCoverage;
    PFNGLSCISSORPROC Scissor;
    PFNGLSHADERBINARYPROC ShaderBinary;
    PFNGLSHADERSOURCEPROC ShaderSource;
    PFNGLSTENCILFUNCPROC StencilFunc;
    PFNGLSTENCILFUNCSEPARATEPROC StencilFuncSeparate;
    PFNGLSTENCILMASKPROC StencilMask;
    PFNGLSTENCILMASKSEPARATEPROC StencilMaskSeparate;
    PFNGLSTENCILOPPROC StencilOp;
    PFNGLSTENCILOPSEPARATEPROC StencilOpSeparate;
    PFNGLTEXIMAGE2DPROC TexImage2D;
    PFNGLTEXPARAMETERFPROC TexParameterf;
    PFNGLTEXPARAMETERFVPROC TexParameterfv;
    PFNGLTEXPARAMETERIPROC TexParameteri;
    PFNGLTEXPARAMETERIVPROC TexParameteriv;
    PFNGLTEXSUBIMAGE2DPROC TexSubImage2D;
    PFNGLUNIFORM1FPROC Uniform1f;
    PFNGLUNIFORM1FVPROC Uniform1fv;
    PFNGLUNIFORM1FPROC Uniform1i;
    PFNGLUNIFORM1FVPROC Uniform1iv;
    PFNGLUNIFORM1FPROC Uniform2f;
    PFNGLUNIFORM1FVPROC Uniform2fv;
    PFNGLUNIFORM1FPROC Uniform2i;
    PFNGLUNIFORM1FVPROC Uniform2iv;
    PFNGLUNIFORM1FPROC Uniform3f;
    PFNGLUNIFORM1FVPROC Uniform3fv;
    PFNGLUNIFORM1FPROC Uniform3i;
    PFNGLUNIFORM1FVPROC Uniform3iv;
    PFNGLUNIFORM1FPROC Uniform4f;
    PFNGLUNIFORM1FVPROC Uniform4fv;
    PFNGLUNIFORM1FPROC Uniform4i;
    PFNGLUNIFORM1FVPROC Uniform4iv;
    PFNGLUNIFORMMATRIX2FVPROC UniformMatrix2fv;
    PFNGLUNIFORMMATRIX2FVPROC UniformMatrix3fv;
    PFNGLUNIFORMMATRIX2FVPROC UniformMatrix4fv;
    PFNGLUSEPROGRAMPROC UseProgram;
    PFNGLVALIDATEPROGRAMPROC ValidateProgram;
    PFNGLVERTEXATTRIB1FPROC VertexAttrib1f;
    PFNGLVERTEXATTRIB1FPROC VertexAttrib1fv;
    PFNGLVERTEXATTRIB1FPROC VertexAttrib2f;
    PFNGLVERTEXATTRIB1FPROC VertexAttrib2fv;
    PFNGLVERTEXATTRIB1FPROC VertexAttrib3f;
    PFNGLVERTEXATTRIB1FPROC VertexAttrib3fv;
    PFNGLVERTEXATTRIB1FPROC VertexAttrib4f;
    PFNGLVERTEXATTRIB1FPROC VertexAttrib4fv;
    PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;
    PFNGLVIEWPORTPROC Viewport;
} AvenGl;

typedef void (*AvenGlProcFn)(void);
typedef AvenGlProcFn (*AvenGlLoadProcFn)(const char *);

static inline AvenGl aven_gl_load(AvenGlLoadProcFn load) {
    AvenGl gl = { 0 };

    gl.ActiveTexture = (PFNGLACTIVETEXTUREPROC)load(
        "glActiveTexture"
    );
    gl.AttachShader = (PFNGLATTACHSHADERPROC)load(
        "glAttachShader"
    );
    gl.BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)load(
        "glBindAttribLocation"
    );
    gl.BindBuffer = (PFNGLBINDBUFFERPROC)load(
        "glBindBuffer"
    );
    gl.BindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)load(
        "glBindFramebuffer"
    );
    gl.BindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)load(
        "glBindRenderbuffer"
    );
    gl.BindTexture = (PFNGLBINDTEXTUREPROC)load(
        "glBindTexture"
    );
    gl.BlendColor = (PFNGLBLENDCOLORPROC)load(
        "glBlendColor"
    );
    gl.BlendEquation = (PFNGLBLENDEQUATIONPROC)load(
        "glBlendEquation"
    );
    gl.BlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)load(
        "glBlendEquationSeparate"
    );
    gl.BlendFunc = (PFNGLBLENDFUNCPROC)load(
        "glBlendFunc"
    );
    gl.BlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)load(
        "glBlendFuncSeparate"
    );
    gl.BufferData = (PFNGLBUFFERDATAPROC)load(
        "glBufferData"
    );
    gl.BufferSubData = (PFNGLBUFFERSUBDATAPROC)load(
        "glBufferSubData"
    );
    gl.CheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load(
        "glCheckFramebufferStatus"
    );
    gl.Clear = (PFNGLCLEARPROC)load(
        "glClear"
    );
    gl.ClearColor = (PFNGLCLEARCOLORPROC)load(
        "glClearColor"
    );
    gl.ClearDepth = (PFNGLCLEARDEPTHFPROC)load(
        "glClearDepth"
    );
    gl.ClearStencil = (PFNGLCLEARSTENCILPROC)load(
        "glClearStencil"
    );
    gl.ColorMask = (PFNGLCOLORMASKPROC)load(
        "glColorMask"
    );
    gl.CompileShader = (PFNGLCOMPILESHADERPROC)load(
        "glCompileShader"
    );
    gl.CompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)load(
        "glCompressedTexImage2D"
    );
    gl.CompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)load(
        "glCompressedTexSubImage2D"
    );
    gl.CopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)load(
        "glCopyTexImage2D"
    );
    gl.CopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)load(
        "glCopyTexSubImage2D"
    );
    gl.CreateProgram = (PFNGLCREATEPROGRAMPROC)load(
        "glCreateProgram"
    );
    gl.CreateShader = (PFNGLCREATESHADERPROC)load(
        "glCreateShader"
    );
    gl.CullFace = (PFNGLCULLFACEPROC)load(
        "glCullFace"
    );
    gl.DeleteBuffers = (PFNGLDELETEBUFFERSPROC)load(
        "glDeleteBuffers"
    );
    gl.DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)load(
        "glDeleteFramebuffers"
    );
    gl.DeleteProgram = (PFNGLDELETEPROGRAMPROC)load(
        "glDeleteProgram"
    );
    gl.DeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)load(
        "glDeleteRenderbuffers"
    );
    gl.DeleteShader = (PFNGLDELETESHADERPROC)load(
        "glDeleteShader"
    );
    gl.DeleteTextures = (PFNGLDELETETEXTURESPROC)load(
        "glDeleteTextures"
    );
    gl.DepthFunc = (PFNGLDEPTHFUNCPROC)load(
        "glDepthFunc"
    );
    gl.DepthMask = (PFNGLDEPTHMASKPROC)load(
        "glDepthMask"
    );
    gl.DepthRangef = (PFNGLDEPTHRANGEFPROC)load(
        "glDepthRangef"
    );
    gl.DetachShader = (PFNGLDETACHSHADERPROC)load(
        "glDetachShader"
    );
    gl.Disable = (PFNGLDISABLEPROC)load(
        "glDisable"
    );
    gl.DisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)load(
        "glDisableVertexAttribArray"
    );
    gl.DrawArrays = (PFNGLDRAWARRAYSPROC)load(
        "glDrawArrays"
    );
    gl.DrawElements = (PFNGLDRAWELEMENTSPROC)load(
        "glDrawElements"
    );
    gl.Enable = (PFNGLENABLEPROC)load(
        "glEnable"
    );
    gl.EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load(
        "glEnableVertexAttribArray"
    );
    gl.Finish = (PFNGLFINISHPROC)load(
        "glFinish"
    );
    gl.Flush = (PFNGLFLUSHPROC)load(
        "glFlush"
    );
    gl.FramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)load(
        "glFramebufferRenderbuffer"
    );
    gl.FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)load(
        "glFramebufferTexture2D"
    );
    gl.FrontFace = (PFNGLFRONTFACEPROC)load(
        "glFrontFace"
    );
    gl.GenBuffers = (PFNGLGENBUFFERSPROC)load(
        "glGenBuffers"
    );
    gl.GenerateMipmap = (PFNGLGENERATEMIPMAPPROC)load(
        "glGenerateMipmap"
    );
    gl.GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)load(
        "glGenFramebuffers"
    );
    gl.GenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)load(
        "glGenRenderbuffers"
    );
    gl.GenTextures = (PFNGLGENTEXTURESPROC)load(
        "glGenTextures"
    );
    gl.GetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)load(
        "glGetActiveAttrib"
    );
    gl.GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)load(
        "glGetActiveUniform"
    );
    gl.GetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)load(
        "glGetAttachedShaders"
    );
    gl.GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)load(
        "glGetAttribLocation"
    );
    gl.GetBooleanv = (PFNGLGETBOOLEANVPROC)load(
        "glGetBooleanv"
    );
    gl.GetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)load(
        "glGetBufferParameteriv"
    );
    gl.GetError = (PFNGLGETERRORPROC)load(
        "glGetError"
    );
    gl.GetFloatv = (PFNGLGETFLOATVPROC)load(
        "glGetFloatv"
    );
    gl.GetFramebufferAttachmentParameteriv =
        (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)load(
            "glGetFramebufferAttachmentParameteriv"
        );
    gl.GetIntegerv = (PFNGLGETINTEGERVPROC)load(
        "glGetIntegerv"
    );
    gl.GetProgramiv = (PFNGLGETPROGRAMIVPROC)load(
        "glGetProgramiv"
    );
    gl.GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)load(
        "glGetProgramInfoLog"
    );
    gl.GetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)load(
        "glGetRenderbufferParameteriv"
    );
    gl.GetShaderiv = (PFNGLGETSHADERIVPROC)load(
        "glGetShaderiv"
    );
    gl.GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)load(
        "glGetShaderInfoLog"
    );
    gl.GetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC)load(
        "glGetShaderPrecisionFormat"
    );
    gl.GetShaderSource = (PFNGLGETSHADERSOURCEPROC)load(
        "glGetShaderSource"
    );
    gl.GetString = (PFNGLGETSTRINGPROC)load(
        "glGetString"
    );
    gl.GetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)load(
        "glGetTexParameterfv"
    );
    gl.GetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)load(
        "glGetTexParameteriv"
    );
    gl.GetUniformfv = (PFNGLGETUNIFORMFVPROC)load(
        "glGetUniformfv"
    );
    gl.GetUniformiv = (PFNGLGETUNIFORMIVPROC)load(
        "glGetUniformiv"
    );
    gl.GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)load(
        "glGetUniformLocation"
    );
    gl.GetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)load(
        "glGetVertexAttribfv"
    );
    gl.GetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)load(
        "glGetVertexAttribiv"
    );
    gl.GetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)load(
        "glGetVertexAttribPointerv"
    );
    gl.Hint = (PFNGLHINTPROC)load(
        "glHint"
    );
    gl.IsBuffer = (PFNGLISBUFFERPROC)load(
        "glIsBuffer"
    );
    gl.IsEnabled = (PFNGLISENABLEDPROC)load(
        "glIsEnabled"
    );
    gl.IsFramebuffer = (PFNGLISFRAMEBUFFERPROC)load(
        "glIsFramebuffer"
    );
    gl.IsProgram = (PFNGLISPROGRAMPROC)load(
        "glIsProgram"
    );
    gl.IsRenderbuffer = (PFNGLISRENDERBUFFERPROC)load(
        "glIsRenderbuffer"
    );
    gl.IsShader = (PFNGLISSHADERPROC)load(
        "glIsShader"
    );
    gl.IsTexture = (PFNGLISTEXTUREPROC)load(
        "glIsTexture"
    );
    gl.LineWidth = (PFNGLLINEWIDTHPROC)load(
        "glLineWidth"
    );
    gl.LinkProgram = (PFNGLLINKPROGRAMPROC)load(
        "glLinkProgram"
    );
    gl.PixelStorei = (PFNGLPIXELSTOREIPROC)load(
        "glPixelStorei"
    );
    gl.PolygonOffset = (PFNGLPOLYGONOFFSETPROC)load(
        "glPolygonOffset"
    );
    gl.ReadPixels = (PFNGLREADPIXELSPROC)load(
        "glReadPixels"
    );
    gl.ReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC)load(
        "glReleaseShaderCompiler"
    );
    gl.RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)load(
        "glRenderbufferStorage"
    );
    gl.SampleCoverage = (PFNGLSAMPLECOVERAGEPROC)load(
        "glSampleCoverage"
    );
    gl.Scissor = (PFNGLSCISSORPROC)load(
        "glScissor"
    );
    gl.ShaderBinary = (PFNGLSHADERBINARYPROC)load(
        "glShaderBinary"
    );
    gl.ShaderSource = (PFNGLSHADERSOURCEPROC)load(
        "glShaderSource"
    );
    gl.StencilFunc = (PFNGLSTENCILFUNCPROC)load(
        "glStencilFunc"
    );
    gl.StencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)load(
        "glStencilFuncSeparate"
    );
    gl.StencilMask = (PFNGLSTENCILMASKPROC)load(
        "glStencilMask"
    );
    gl.StencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)load(
        "glStencilMaskSeparate"
    );
    gl.StencilOp = (PFNGLSTENCILOPPROC)load(
        "glStencilOp"
    );
    gl.StencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)load(
        "glStencilOpSeparate"
    );
    gl.TexImage2D = (PFNGLTEXIMAGE2DPROC)load(
        "glTexImage2D"
    );
    gl.TexParameterf = (PFNGLTEXPARAMETERFPROC)load(
        "glTexParameterf"
    );
    gl.TexParameterfv = (PFNGLTEXPARAMETERFVPROC)load(
        "glTexParameterfv"
    );
    gl.TexParameteri = (PFNGLTEXPARAMETERIPROC)load(
        "glTexParameteri"
    );
    gl.TexParameteriv = (PFNGLTEXPARAMETERIVPROC)load(
        "glTexParameteriv"
    );
    gl.TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)load(
        "glTexSubImage2D"
    );
    gl.Uniform1f = (PFNGLUNIFORM1FPROC)load(
        "glUniform1f"
    );
    gl.Uniform1fv = (PFNGLUNIFORM1FVPROC)load(
        "glUniform1fv"
    );
    gl.Uniform1i = (PFNGLUNIFORM1FPROC)load(
        "glUniform1i"
    );
    gl.Uniform1iv = (PFNGLUNIFORM1FVPROC)load(
        "glUniform1iv"
    );
    gl.Uniform2f = (PFNGLUNIFORM1FPROC)load(
        "glUniform2f"
    );
    gl.Uniform2fv = (PFNGLUNIFORM1FVPROC)load(
        "glUniform2fv"
    );
    gl.Uniform2i = (PFNGLUNIFORM1FPROC)load(
        "glUniform2i"
    );
    gl.Uniform2iv = (PFNGLUNIFORM1FVPROC)load(
        "glUniform2iv"
    );
    gl.Uniform3f = (PFNGLUNIFORM1FPROC)load(
        "glUniform3f"
    );
    gl.Uniform3fv = (PFNGLUNIFORM1FVPROC)load(
        "glUniform3fv"
    );
    gl.Uniform3i = (PFNGLUNIFORM1FPROC)load(
        "glUniform3i"
    );
    gl.Uniform3iv = (PFNGLUNIFORM1FVPROC)load(
        "glUniform3iv"
    );
    gl.Uniform4f = (PFNGLUNIFORM1FPROC)load(
        "glUniform4f"
    );
    gl.Uniform4fv = (PFNGLUNIFORM1FVPROC)load(
        "glUniform4fv"
    );
    gl.Uniform4i = (PFNGLUNIFORM1FPROC)load(
        "glUniform4i"
    );
    gl.Uniform4iv = (PFNGLUNIFORM1FVPROC)load(
        "glUniform4iv"
    );
    gl.UniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)load(
        "glUniformMatrix2fv"
    );
    gl.UniformMatrix3fv = (PFNGLUNIFORMMATRIX2FVPROC)load(
        "glUniformMatrix3fv"
    );
    gl.UniformMatrix4fv = (PFNGLUNIFORMMATRIX2FVPROC)load(
        "glUniformMatrix4fv"
    );
    gl.UseProgram = (PFNGLUSEPROGRAMPROC)load(
        "glUseProgram"
    );
    gl.ValidateProgram = (PFNGLVALIDATEPROGRAMPROC)load(
        "glValidateProgram"
    );
    gl.VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)load(
        "glVertexAttrib1f"
    );
    gl.VertexAttrib1fv = (PFNGLVERTEXATTRIB1FPROC)load(
        "glVertexAttrib1fv"
    );
    gl.VertexAttrib2f = (PFNGLVERTEXATTRIB1FPROC)load(
        "glVertexAttrib2f"
    );
    gl.VertexAttrib2fv = (PFNGLVERTEXATTRIB1FPROC)load(
        "glVertexAttrib2fv"
    );
    gl.VertexAttrib3f = (PFNGLVERTEXATTRIB1FPROC)load(
        "glVertexAttrib3f"
    );
    gl.VertexAttrib3fv = (PFNGLVERTEXATTRIB1FPROC)load(
        "glVertexAttrib3fv"
    );
    gl.VertexAttrib4f = (PFNGLVERTEXATTRIB1FPROC)load(
        "glVertexAttrib4f"
    );
    gl.VertexAttrib4fv = (PFNGLVERTEXATTRIB1FPROC)load(
        "glVertexAttrib4fv"
    );
    gl.VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)load(
        "glVertexAttribPointer"
    );
    gl.Viewport = (PFNGLVIEWPORTPROC)load(
        "glViewport"
    );

    return gl;
}

typedef enum {
    AVEN_GL_BUFFER_USAGE_STATIC = GL_STATIC_DRAW,
    AVEN_GL_BUFFER_USAGE_DYNAMIC = GL_DYNAMIC_DRAW,
    AVEN_GL_BUFFER_USAGE_STREAM = GL_STREAM_DRAW,
} AvenGlBufferUsage;

#endif // AVEN_GL_H
