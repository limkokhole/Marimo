﻿#ifndef _SHADER_CLASS_HEADER_DEFINE_FILE_
#define _SHADER_CLASS_HEADER_DEFINE_FILE_

//////////////////////////////////////////////////////////////////////////
// 关于ShaderConst和Uniform的含义:
// Const表示常量,VertexShader 和 PixelShader 分别包含各自的Const常量
// 当两个Shader合并使用时, 同名同长度(不同长度应该在以后检测报错)的Const会合
// 并为一个记录, 这称为 Uniform
// Canvas3D 可以将指定名字的Uniform赋予全局属性，使其在Canvas3D中具有一致的值，
// 这称为 Canvas-Uniform
namespace Marimo {
  class DataPool;
}
typedef Marimo::DataPool MODataPool;

class GXGraphics;
class GTextureBase;
namespace clstd
{
  class SmartStockA;
} // namespace clstd
class clConstBuffer;
//class SmartRepository;
struct MOSHADER_ELEMENT_SOURCE;
struct MTLFILEPARAMDESC;
struct STANDARDMTLUNIFORMTABLE;

class GVertexDeclaration : public GResource
{
public:
  GVertexDeclaration() : GResource(0, RESTYPE_VERTEX_DECLARATION) {}

  GXSTDINTERFACE(GXHRESULT          AddRef            ());
  GXSTDINTERFACE(GXHRESULT          Release           ());
  GXSTDINTERFACE(GXUINT             GetStride         ());  // 顶点格式所需要的最小长度
  GXSTDINTERFACE(GXINT              GetElementOffset  (GXDeclUsage Usage, GXUINT UsageIndex, LPGXVERTEXELEMENT lpDesc = NULL));
  GXSTDINTERFACE(GXLPCVERTEXELEMENT GetVertexElement  ());
  // TODO: 将来实现 GXSTDINTERFACE(GXINT        GetOffset         (GXIN GXDeclUsage Usage, GXIN GXUINT UsageIndex, GXOUT LPGXVERTEXELEMENT lpDesc = NULL));
  // TODO: 将来实现 GXSTDINTERFACE(GXUINT       GetElementCount   ());
};

class GUniformBinderDecl : public GResource
{
public:
  GUniformBinderDecl() : GResource(0, RESTYPE_UNKNOWN) {}

  GXSTDINTERFACE(GXHRESULT    AddRef            ());
  GXSTDINTERFACE(GXHRESULT    Release           ());
};

class GShader : public GResource
{
public:
  typedef clvector<GXDefinition>      ParamArray;
public:
  GShader() : GResource(0, RESTYPE_SHADER) { }
#ifdef ENABLE_VIRTUALIZE_ADDREF_RELEASE
  GXSTDINTERFACE(GXHRESULT    AddRef            ());
  GXSTDINTERFACE(GXHRESULT    Release           ());
#endif // #ifdef ENABLE_VIRTUALIZE_ADDREF_RELEASE

  GXSTDINTERFACE(GXHRESULT    LoadFromFile      (MOSHADER_ELEMENT_SOURCE* pSdrElementSrc));
  GXSTDINTERFACE(GXHRESULT    LoadFromMemory    (const clBufferBase* pVertexBuf, const clBufferBase* pPixelBuf));
  //GXSTDINTERFACE(GXDWORD      GetFlags          () GXCONST);
  GXSTDINTERFACE(GXINT        GetCacheSize      () GXCONST);
  GXSTDINTERFACE(GXGraphics*  GetGraphicsUnsafe () GXCONST);
  GXSTDINTERFACE(GXLPCWSTR    GetProfileDesc    () GXCONST);  // 取配置文件(包含各个平台Shader文件)的名字,如果是从内存加载,则为NULL
public:
  static GXDLL GXVOID     ResolveProfileDescW (GXLPCWSTR szProfileDesc, clStringW* pstrFilename, clStringA* pstrMacros);
  static GXDLL GXHRESULT  Load                (GXLPCWSTR szShaderDesc, GXLPCWSTR szResourceDir, GXLPCSTR szPlatformSect, MOSHADER_ELEMENT_SOURCE* pElement, GXOUT MTLFILEPARAMDESC* pMtlParam);
  static GXDLL GXBOOL     LoadElementSource   (clStockA* pSmart, GXLPCSTR szSection, MOSHADER_ELEMENT_SOURCE* pElement, clStringArrayA* aDataPool);
  static GXDLL GXBOOL     ComposeSource       (MOSHADER_ELEMENT_SOURCE* pElement, GXDWORD dwPlatfomCode, GXOUT MOSHADERBUFFERS* pSources, GXOUT GXDefinitionArray* aMacros);
  static GXDLL GXBOOL     LoadUniformSet      (clStockA* pSmart, GXLPCSTR szSection, ParamArray* aUniforms);
  static GXDLL GXBOOL     LoadStateSet        (clStockA* pSmart, GXLPCSTR szSection, ParamArray* aStates);
};

class GShaderStub : public GResource
{
public:
  struct UNIFORMDESC
  {
    clStringW     Name;
    GXUniformType eType;
    GXUINT        nOffset;
    GXUINT        cbCount;
    GXINT         nBindIdx;
  };
public:
  GShaderStub()
    : GResource(1, RESTYPE_SHADER_STUB) { }

  GXSTDINTERFACE(GXHRESULT      SetShaderRef            (GShader* pShader));
  GXSTDINTERFACE(GXHRESULT      BindData                (MODataPool* pDataPool, GXLPCSTR szStruct));
  GXSTDINTERFACE(GXHRESULT      FindDataPoolByName      (GXLPCSTR szName, MODataPool** ppDataPool));  // DataPool 不用后要 Release
  GXSTDINTERFACE(GXHRESULT      BindCommonUniform       (LPCDATALAYOUT lpDataLayout));
  GXSTDINTERFACE(GXINT          FindUniformDef          (LPCDATALAYOUT lpDataLayout));
  GXSTDINTERFACE(GXBOOL         CommitUniform           (int nDefIdx, GXLPCVOID lpData, GXUINT uCommonOffset));
  GXSTDINTERFACE(GShader*       GetShaderUnsafe         () GXCONST);
  GXSTDINTERFACE(GXUINT         GetHandleByName         (GXLPCSTR szName) GXCONST);
  GXSTDINTERFACE(GXUINT         GetHandleByIndex        (GXUINT nIndex) GXCONST);
  GXSTDINTERFACE(GXUniformType  GetHandleType           (GXUINT handle) GXCONST);
  GXSTDINTERFACE(GXUINT         GetSamplerStageByHandle (GXUINT handle) GXCONST);
  GXSTDINTERFACE(GXBOOL         SetUniformByHandle      (clBufferBase* pUnusualUnifom, GXUINT uHandle, float* fValue, GXINT nFloatCount));
  GXSTDINTERFACE(GXBOOL         GetUniformByIndex       (GXUINT nIndex, UNIFORMDESC* pDesc) GXCONST);
  GXSTDINTERFACE(GXBOOL         SetTextureByHandle      (GTextureBase** pTextureArray, GXUINT uHandle, GTextureBase* pTexture));
  GXSTDINTERFACE(GXBOOL         SetTextureByIndex       (GTextureBase** pTextureArray, GXUINT nIndex, GTextureBase* pTexture));
#ifdef REFACTOR_SHADER
  GXSTDINTERFACE(GXBOOL         CommitToDevice          (GXLPVOID lpUniform, GXSIZE_T cbSize));
  GXSTDINTERFACE(GXBOOL         UpdateCanvasUniform     (GXLPCBYTE lpCanvasUniform, GXLPVOID lpUniform, GXSIZE_T cbSize));
  GXSTDINTERFACE(GXBOOL         UpdateUniform           (int nDefIdx, GXLPCBYTE lpCanvasUniform, GXLPVOID lpUniform, GXSIZE_T cbSize));
#endif // #ifdef REFACTOR_SHADER

#ifdef ENABLE_VIRTUALIZE_ADDREF_RELEASE
  // 接口实现
  GXSTDINTERFACE(GXHRESULT      AddRef              ());
  GXSTDINTERFACE(GXHRESULT      Release             ());
#endif // #ifdef ENABLE_VIRTUALIZE_ADDREF_RELEASE
  // TODO: 这个没太想好, 等实现 OpenGL/ES 时再考虑
  //GXSTDINTERFACE(GXHRESULT      SetTextureSlot      (GXLPCSTR pName, GXINT nSlot));
  //GXSTDINTERFACE(GXINT          GetTextureSlot      (GXLPCSTR pName));
};

class GXEffect : public GResource
{
public:
  GXEffect() : GResource(2, RESTYPE_SHADER_EFFECT){}
#ifdef ENABLE_VIRTUALIZE_ADDREF_RELEASE
  GXSTDINTERFACE(GXHRESULT  AddRef                ());
  GXSTDINTERFACE(GXHRESULT  Release               ());
#endif // #ifdef ENABLE_VIRTUALIZE_ADDREF_RELEASE

  GXSTDINTERFACE(GXHRESULT  SetTextureSlot        (GXLPCSTR pName, GXINT nSlot));
  GXSTDINTERFACE(GXINT      GetTextureSlot        (GXLPCSTR pName));

  GXSTDINTERFACE(GXUINT     GetConstantBufferSize ()); // 或者叫context?
};

class GXMaterialInst : public GResource
{
public:
  enum ParamType
  {
    PT_UNIFORMS,
    PT_RENDERSTATE,
  };
  typedef clvector<GXDefinition>  UniformArray;
public:
  GXMaterialInst() : GResource(2, RESTYPE_SHADER_MATERIAL){}
#ifdef ENABLE_VIRTUALIZE_ADDREF_RELEASE
  GXSTDINTERFACE(GXHRESULT  AddRef                      ());
  GXSTDINTERFACE(GXHRESULT  Release                     ());
#endif // #ifdef ENABLE_VIRTUALIZE_ADDREF_RELEASE

  GXSTDINTERFACE(GXBOOL     IsSequential                ()); // 废掉
  GXSTDINTERFACE(int        GetRenderQueue              () const);
  GXSTDINTERFACE(GXGraphics*GetGraphicsUnsafe           ());
  GXSTDINTERFACE(GXHRESULT  GetFilenameW                (clStringW* pstrFilename));
  GXSTDINTERFACE(GXHRESULT  Clone                       (GXMaterialInst** ppCuplicateMtlInst));

  GXSTDINTERFACE(GXHRESULT  SaveFileW                   (GXLPCWSTR szFilename));
  GXSTDINTERFACE(GXHRESULT  SaveRepository              (clSmartRepository* pStorage));

  GXSTDINTERFACE(GXHRESULT  LoadFileW                   (GXGraphics* pGraphics, GXLPCWSTR szFilename));
  GXSTDINTERFACE(GXHRESULT  LoadRepository              (GXGraphics* pGraphics, clSmartRepository* pStorage));

  // 初始化接口
  // 这些函数建议都不要频繁调用, 可能会严重影响性能.
  GXSTDINTERFACE(GXHRESULT  GetUniformList              (UniformArray* pUniforms));
  GXSTDINTERFACE(GXHRESULT  BindData                    (MODataPool* pDataPool, GXLPCSTR szStruct));
  GXSTDINTERFACE(GXHRESULT  BindDataByName              (GXLPCSTR szPoolName, GXLPCSTR szStruct));
  GXSTDINTERFACE(GXHRESULT  SetParameters               (ParamType eType, GXDEFINITION* pParameters, int nCount = 0));  // 更新列表中的参数值, 如果nCount为0, 则列表的结尾必须以空字符串或者NULL结尾
  GXSTDINTERFACE(GXHRESULT  SetFloat1ByName             (GXLPCSTR szName, float val));
  GXSTDINTERFACE(GXHRESULT  SetFloat2ByName             (GXLPCSTR szName, const float2& vFloat2));
  GXSTDINTERFACE(GXHRESULT  SetFloat3ByName             (GXLPCSTR szName, const float3& vFloat3));
  GXSTDINTERFACE(GXHRESULT  SetFloat4ByName             (GXLPCSTR szName, const float4& vFloat4));
  GXSTDINTERFACE(GXHRESULT  SetMatrixByName             (GXLPCSTR szName, const float4x4& mat));

  GXSTDINTERFACE(GXHRESULT  SetTextureByName            (GXLPCSTR szName, GTextureBase* pTexture));
  GXSTDINTERFACE(GXHRESULT  SetTextureByIndex           (GXUINT nIndex, GTextureBase* pTexture));
  GXSTDINTERFACE(GXHRESULT  SetTextureByNameFromFileW   (GXLPCSTR szName, GXLPCWSTR szFilename));
  GXSTDINTERFACE(GXHRESULT  SetTextureByIndexFromFileW  (GXUINT nIndex, GXLPCWSTR szFilename));
  // -- 初始化接口
};

//////////////////////////////////////////////////////////////////////////
typedef GXMaterialInst* GXLPMATERIALINST;

// 根据绑定声明生成声明源代码
GXBOOL MOGenerateDeclarationCodes(DATALAYOUT* lpDataLayout, GXDWORD dwPlatfomCode, clBuffer** ppBuffer);

#endif // end of _SHADER_CLASS_HEADER_DEFINE_FILE_