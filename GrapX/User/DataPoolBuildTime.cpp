﻿#include "GrapX.h"
#include "GrapX.Hxx"

#include "clStringSet.h"
#include "clStaticStringsDict.h"

#include "GrapX/DataPool.h"
#include "GrapX/DataPoolVariable.h"

#include "DataPoolImpl.h"
#include "DataPoolBuildTime.h"
using namespace clstd;

namespace Marimo
{
  DataPoolBuildTime::DataPoolBuildTime(DataPoolCreation dwFlags)
    : m_dwBuildFlags(dwFlags)
    , m_bPtr64(0)
    , m_bFixedPool(1)
    , m_nNumOfStructs(0)
    , m_nNumOfBuckets(0)
  {
    m_VarHashInfo.eType   = StaticStringsDict::HashType_Failed;
    m_VarHashInfo.nBucket = 0;
    m_VarHashInfo.nPos    = 0;
  }

  GXBOOL DataPoolBuildTime::IntCheckTypeDecl(LPCTYPEDECL pTypeDecl, GXBOOL bCheck)
  {
    STATIC_ASSERT(sizeof(BUILDTIME_TYPE_DECLARATION) >= sizeof(TYPE_DECLARATION)); // 这个断言不能完全保证数据正确，但至少不会大出错

    for(int i = 0;; i++) {
      BUILDTIME_TYPE_DECLARATION type;
      (*(TYPE_DECLARATION*)&type)= pTypeDecl[i];
      type.dwFlags = 0;

      if(type.Name == NULL) { break; }
      if(bCheck && (type.Cate <= T_UNDEFINE || type.Cate >= T_MAX || ! DataPool::IsIllegalName(type.Name)) ) {
        CLOG_ERROR("%s: Bad type category.\n", __FUNCTION__);
        return FALSE;
      }
      sBuildTimeTypeDeclDict[type.Name] = type;
    }
    return TRUE;
  }

  void DataPoolBuildTime::PutTypeToDict(GXLPCSTR szTypeName)
  {
    //GXLPCSTR szPrimitiveType = szTypeName[0] == '#' ? &szTypeName[1] : szTypeName;
    const auto itFloatType = sBuildTimeTypeDeclDict.find(szTypeName);
    const BUILDTIME_TYPE_DECLARATION& type = itFloatType->second;
    ASSERT(itFloatType != sBuildTimeTypeDeclDict.end());

    BT_TYPE_DESC sDesc;

    sDesc.szName       = szTypeName;
    sDesc.nNameIndex   = (GXUINT)NameSet.index(szTypeName);
    sDesc.Cate         = type.Cate;
    sDesc.nMemberIndex = 0;
    sDesc.nMemberCount = 0;
    sDesc.nTypeAddress = 0;
    sDesc.HashInfo.eType   = StaticStringsDict::HashType_Failed;
    sDesc.HashInfo.nBucket = 0;
    sDesc.HashInfo.nPos    = 0;

    // 这里要先在字典中加入这个类型，原因是：
    // 对于T_STRUCT类型，可能会在其中定义了自己类型的动态数组
    // 预先加入字典，在包含自身类型的递归中能找到类型定义，避免无限递归
    BT_TYPE_DESC& t = (m_TypeDict.insert(clmake_pair(szTypeName, sDesc)).first)->second;

    //VarDesc.Name = var.Name;
    switch(type.Cate)
    {
    case T_BYTE:
    case T_SBYTE:
      t.cbSize = sizeof(u8);
      break;
    case T_WORD:
    case T_SWORD:
      t.cbSize = sizeof(u16);
      break;
    case T_DWORD:
    case T_SDWORD:
    case T_FLOAT:
      t.cbSize = sizeof(u32);
      break;
    case T_QWORD:
    case T_SQWORD:
      t.cbSize = sizeof(u64);
      break;

    case T_OBJECT:
      t.cbSize = m_bPtr64 ? sizeof(u64) : sizeof(GUnknown*);
      m_bFixedPool = 0;
      break;

    case T_STRING:
    case T_STRINGA:
      // 字符串类相当于指针，所以printf format系才能使用
      STATIC_ASSERT(sizeof(clStringW) == sizeof(void*) && sizeof(clStringA) == sizeof(void*));
      t.cbSize = m_bPtr64 ? sizeof(u64) : sizeof(void*);
      m_bFixedPool = 0;
      break;
    case T_STRUCT:
    case T_STRUCTALWAYS:
      {
        BTVarDescArray aMemberDesc;
        aMemberDesc.reserve(20);
        t.cbSize = ComputeVariableSize(type.as.Struct, aMemberDesc, type.StructAlign);
        t.nMemberCount = (GXUINT)aMemberDesc.size();
        t.nMemberIndex = (GXUINT)m_aStructMember.size();
        m_aStructMember.insert(m_aStructMember.end(), aMemberDesc.begin(), aMemberDesc.end());

        if(TEST_FLAG(m_dwBuildFlags, DataPoolCreation_NoHashTable))
        {
          // do nothing...
        } else {
          TryHash(t.HashInfo, aMemberDesc);
          m_nNumOfBuckets += t.HashInfo.indices.size();
        }

        m_nNumOfStructs++;
      }
      break;

    case T_ENUM:
    case T_FLAG:
      {
        t.cbSize = sizeof(DataPool::Enum);
        //t.nMemberIndex = m_aEnumPck.size();
        t.nMemberIndex = (GXUINT)m_aEnumPck.size();
        DATAPOOL_ENUM_DESC sEnum;
        for(t.nMemberCount = 0; type.as.Enumer[t.nMemberCount].Name != NULL; t.nMemberCount++)
        {
          sEnum.nName = (GXUINT)NameSet.index(type.as.Enumer[t.nMemberCount].Name);
          sEnum.Value = type.as.Enumer[t.nMemberCount].Value;
          m_aEnumPck.push_back(sEnum);
        }
        m_nNumOfStructs++;
      }
      break;
    default:
      ASSERT(0); // 增加了新类型或者类型异常
      break;
    }
  }

  GXINT DataPoolBuildTime::ComputeVariableSize(LPCVARDECL pVarDecl, BTVarDescArray& aVariableDesc, GXUINT nAlignSize)
  {
    GXINT cbVariableSize = 0;
    GXINT cbSubSize = 0;
    int nVarIndex = 0;
    const GXUINT nAlignMask = ((nAlignSize - 1) <= 15 && clstd::IsPowerOfTwo(nAlignSize)) ? (nAlignSize - 1) : 0;
    ASSERT(nAlignSize == NOT_CROSS_16_BYTES_BOUNDARY || nAlignSize == 0 || nAlignSize == 1 ||
      nAlignSize == 4 || nAlignSize == 8 || nAlignSize == 16);
    ASSERT(nAlignMask == 0 || nAlignMask == 3 || nAlignMask == 7 || nAlignMask == 15);

    for(;; nVarIndex++)
    {
      BT_VARIABLE_DESC VarDesc;
      const VARIABLE_DECLARATION& var = pVarDecl[nVarIndex];
      if(var.Type == NULL || var.Name == NULL) {
        break;
      }

      const int nDim = var.Count < 0 ? 0 : 
        (var.Count <= 1 ? 1 : var.Count);

      //clStringA strTypeName = var.Type;
      //if(var.Count < 0) {  // 变长数组
      //  strTypeName.Insert(0, '#');
      //}
      auto itType = m_TypeDict.find(var.Type);

      if(itType == m_TypeDict.end())
      {
        PutTypeToDict(var.Type);
        itType = m_TypeDict.find(var.Type);
        ASSERT(itType != m_TypeDict.end());
      }

      VarDesc.szName      = var.Name;
      VarDesc.nNameIndex  = (GXUINT)NameSet.index(var.Name);
      VarDesc.pTypeDesc   = &itType->second;
      VarDesc.bConst      = 0;
      VarDesc.nCount      = nDim;

      // 动态数组声明为clBuffer*
      if(var.Count < 0) {
        ASSERT(VarDesc.nCount == 0);
        VarDesc.bDynamic = 1;
        m_bFixedPool     = 0;
        cbSubSize = (m_bPtr64 ? sizeof(u64) : sizeof(clBuffer*));
      }
      else
      {
        VarDesc.bDynamic = 0;
        cbSubSize = VarDesc.GetSize();
      }

      if(nAlignSize == NOT_CROSS_16_BYTES_BOUNDARY)
      {
        GXUINT cbNextBoundary = (cbVariableSize + 16) & (~15);

        // 如果跨越16字节边界，则把开始偏移移到下一个边界
        if((cbVariableSize & 15) != 0 && cbVariableSize + cbSubSize > cbNextBoundary)
        {
          cbVariableSize = cbNextBoundary;
        }
        VarDesc.nOffset = cbVariableSize;
        cbVariableSize += cbSubSize;
      }
      else {
        VarDesc.nOffset = cbVariableSize;
        cbVariableSize += cbSubSize;
        if(nAlignMask != 0)
        {
          // 结构体/全局变量 对齐
          cbVariableSize = (cbVariableSize + nAlignMask) & (~nAlignMask);
        }
      }

      ASSERT(((GXLONG_PTR)VarDesc.pTypeDesc & 3) == 0); // 一定是4字节对齐的
      aVariableDesc.push_back(VarDesc);
    }
    
    if(nAlignSize == NOT_CROSS_16_BYTES_BOUNDARY) {
      cbVariableSize = ALIGN_16(cbVariableSize);
    }
    return cbVariableSize;
  }

  GXBOOL DataPoolBuildTime::CheckVarList(LPCVARDECL pVarDecl)
  {
    typedef clhash_set<clStringA> TVariableSet;
    TVariableSet VariableSet;
    GXBOOL result = TRUE;

    int i = 0;
    while (pVarDecl[i].Type != NULL && pVarDecl[i].Name != NULL)
    {
      auto it = sBuildTimeTypeDeclDict.find(pVarDecl[i].Type);
      if(it == sBuildTimeTypeDeclDict.end()) {
        CLOG_ERROR("%s: Bad variable type(%s).\n", __FUNCTION__, pVarDecl[i].Type);
        result = FALSE;
        ++i;
        continue;
      }

      if(TEST_FLAG(it->second.dwFlags, BuildtimeTypeDeclaration_Used | BuildtimeTypeDeclaration_Checked)) {

        // 如果不是动态数组，则检查这个变量是不是在检查中
        if(pVarDecl[i].Count >= 0 && ((it->second.dwFlags & (BuildtimeTypeDeclaration_Used | 
          BuildtimeTypeDeclaration_Checked)) == BuildtimeTypeDeclaration_Used)) {
            CLOG_ERROR("%s:  Can not use type(%s) when it is defining.\n", __FUNCTION__, it->second.Name);
            result = FALSE;
        }
        //++i;
        //continue;
      }
      else
      {
        SET_FLAG(it->second.dwFlags, BuildtimeTypeDeclaration_Used);

        // 结构类型 递归检查
        if(it->second.Cate == T_STRUCT) {
          if( ! CheckVarList(it->second.as.Struct)) {
            result = FALSE;
          }
        }
      }

      if( ! DataPool::IsIllegalName(pVarDecl[i].Name)) {
        CLOG_ERROR("%s: Bad variable name.\n", __FUNCTION__);
        result = FALSE;
      }

      // 重名检查
      TVariableSet::iterator itVarSet = VariableSet.find(pVarDecl[i].Name);
      if(itVarSet == VariableSet.end()) {
        VariableSet.insert(pVarDecl[i].Name);
      }
      else {
        CLOG_ERROR("%s: Duplicate variable name(%s).", __FUNCTION__, pVarDecl[i].Name);
        result = FALSE;
      }

      SET_FLAG(it->second.dwFlags, BuildtimeTypeDeclaration_Checked);
      i++;
    }
    return i > 0 && result; // 不能为空
  }

  void DataPoolBuildTime::TryHash(HASH_ALGORITHM& hash_info, const BTVarDescArray& aDescs)
  {
    clstd::StaticStringsDict ssd(aDescs.size());
    b32 bResult = ssd.TestHashable([&aDescs](clsize index, CLLPCSTR* str, clsize* len) -> b32
    {
      *str = aDescs[index].szName;
      *len = GXSTRLEN(*str);
      return TRUE;
    });

    if(bResult) {
      hash_info.eType   = ssd.eType;
      hash_info.nBucket = ssd.nBucket;
      hash_info.nPos    = ssd.nPos;
      hash_info.indices.clear();
      ssd.Copy(hash_info.indices);
    }
    else {
      hash_info.eType = clstd::StaticStringsDict::HashType_Failed;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  
} // namespace Marimo
