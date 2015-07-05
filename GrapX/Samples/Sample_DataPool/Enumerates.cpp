#include <Marimo.H>
//#include <include/DataInfrastructure.h>
#include "GrapX/DataPoolIterator.h"
#include "TestDataPool.h"
#define CHECK_VAR
void EnumerateVariables(DataPool* pDataPool, int nDepth, DataPoolIterator& itBegin, DataPoolIterator& itEnd);

void EunmerateArray(DataPool* pDataPool, int nDepth, DataPoolElementIterator& itBegin, DataPoolElementIterator& itEnd)
{
  MOVariable var;
#ifdef CHECK_VAR
  MOVariable varCheck;
#endif // #ifdef CHECK_VAR
  for(auto it = itBegin; it != itEnd; ++it)
  {
    it.ToVariable(var);
#ifdef CHECK_VAR
    pDataPool->QueryByExpression(it.FullNameA(), &varCheck);
#endif // #ifdef CHECK_VAR

    TRACE("%*c%s[%08x:%d] %*s=\"%s\"\n", nDepth * 2, '!', it.TypeName(), 
      it.pBuffer ? (GXINT_PTR)it.pBuffer->GetPtr() : NULL, it.offset(),
      30, it.FullNameA(), var.IsValid() ? var.ToStringA() : "<null>");

#ifdef CHECK_VAR
    ASSERT(varCheck.GetPtr() == var.GetPtr());
    ASSERT(varCheck.GetCaps() == var.GetCaps());
#endif // #ifdef CHECK_VAR

    var.GetFullName();

    EnumerateVariables(pDataPool, nDepth + 1, it.begin(), it.end());
  }
}

void EnumerateVariables(DataPool* pDataPool, int nDepth, DataPoolIterator& itBegin, DataPoolIterator& itEnd)
{
  MOVariable var;
#ifdef CHECK_VAR
  MOVariable varCheck;
#endif // #ifdef CHECK_VAR
  clStringW nameCheck;
  for(auto it = itBegin; it != itEnd; ++it)
  {
    MOVariable var = it.ToVariable();
    auto length = it.array_length();
    auto bArray = it.IsArray();
    auto name = it.FullNameA();

    // #.iterator ֻ������������Ӱ�춯̬����buffer�Ĳ���
    // #.QueryByExpression ��ѯ��������ʱ�ᴴ��һ����һ������������0�Ķ�̬������
    // ��������var�п�����Ч����varCheck����Ч��
#ifdef CHECK_VAR
    pDataPool->QueryByExpression(name, &varCheck);
#endif // #ifdef CHECK_VAR

    TRACE("%*c%s %s[%08x:%d])(x%d) %*s=\"%s\"\n", nDepth * 2, '+', it.TypeName(), it.VariableName(), 
      it.pBuffer ? (GXINT_PTR)it.pBuffer->GetPtr() : NULL, it.offset(),
      length, 30, name, var.IsValid() ? var.ToStringA() : "<null>");

    nameCheck = var.GetFullName();

#ifdef CHECK_VAR
    //*
    if(var.IsValid())
    {
      ASSERT(varCheck.GetPtr() == var.GetPtr());
      ASSERT(varCheck.GetCaps() == var.GetCaps());
    }
    else if(varCheck.IsValid())
    {
      // VTBL* m_vtbl;
      ASSERT(((GXLONG_PTR*)&var)[0] == ((GXLONG_PTR*)&varCheck)[0]);

      // DataPool* m_pDataPool;
      ASSERT(((GXLONG_PTR*)&var)[1] == ((GXLONG_PTR*)&varCheck)[1]);

      // DPVDD* m_pVdd;
      ASSERT(((GXLONG_PTR*)&var)[2] == ((GXLONG_PTR*)&varCheck)[2]);

      // clBufferBase* m_pBuffer;
      ASSERT(((GXLONG_PTR*)&var)[3] == 0 && ((GXLONG_PTR*)&varCheck)[3] != 0);

      // GXUINT m_AbsOffset; // ����� m_pBuffer ָ���ƫ��      
      ASSERT(*(GXUINT*)(&(((GXLONG_PTR*)&var)[4])) == *(GXUINT*)(&(((GXLONG_PTR*)&varCheck)[4])));
    }//*/
#endif // #ifdef CHECK_VAR

    if(bArray) {
      EunmerateArray(pDataPool, nDepth + 1, it.array_begin(), it.array_end());
      for(auto itVar = var.array_begin(); itVar != var.array_end(); ++itVar)
      {
        auto v1 = itVar.ToVariable();
        auto v2 = var[itVar - var.array_begin()];
        TRACE("[]name:%s\n", var.GetName());
        CompareVariable(v1, v2);
      }
    }
    else {
      EnumerateVariables(pDataPool, nDepth + 1, it.begin(), it.end());

      for(auto itVar = var.begin(); itVar != var.end(); ++itVar)
      {
        auto v1 = itVar.ToVariable();
        auto v2 = var[itVar.VariableName()];
        CompareVariable(v1, v2);
      }
    }
  }
}

void EnumerateVariables(DataPool* pDataPool)
{
  TRACE("=======================================================\n");
  EnumerateVariables(pDataPool, 0, pDataPool->named_begin(), pDataPool->named_end());
  TRACE("=======================================================\n");
}

//////////////////////////////////////////////////////////////////////////
struct ENUMERATE_CONTEXT
{
  int nNumString;
  int nNumArray;  // ֻ�Ƕ�̬����
};


b32 EnumeratePtrControl(ENUMERATE_CONTEXT& ctx, int nDepth, DataPoolIterator* pParent, DataPoolIterator& itBegin, DataPoolIterator& itEnd);

void EnumStrings(ENUMERATE_CONTEXT& ctx, int nDepth, DataPoolIterator& itArray)
{
  MOVariable var;
  auto itEnd = itArray.array_end();
  for(auto it = itArray.array_begin(); it != itEnd; ++it)
  {
    it.ToVariable(var);
    TRACE("%*c %s=\"%s\"\n", nDepth * 2, 'T', it.FullNameA(), var.IsValid() ? var.ToStringA() : "<null>");
    ctx.nNumString++;
  }
}

//void EnumStructs(ENUMERATE_CONTEXT& ctx, int nDepth, DataPool::iterator& itStructs)
//{
//  auto itEnd = itStructs.array_end();
//  for(auto itElement = itStructs.array_begin(); itElement != itEnd; ++itElement)
//  {
//    EnumeratePtrControl(ctx, nDepth + 1, &itElement, itElement.begin(), itElement.end());
//  }
//
//}
//
//void CheckIter(const DataPool::iterator& it)
//{
//  MOVariable var1;
//  MOVariable var2;
//  it.pDataPool->QueryByExpression(it.FullNameA(), &var1);
//  it.ToVariable(var2);
//  ASSERT(var1.GetPtr() == var2.GetPtr());
//}


b32 EnumeratePtrControl(ENUMERATE_CONTEXT& ctx, int nDepth, DataPoolIterator* pParent, DataPoolIterator& itBegin, DataPoolIterator& itEnd)
{
  b32 bval = FALSE;
  MOVariable var;
  MOVariable varCheck;

  //if(pParent && pParent->pVarDesc->IsDynamicArray())
  //{
  //  TRACE("*%08x\n", pParent->pBuffer);
  //  ctx.nNumArray++;
  //}

  for(auto itMember = itBegin; itMember != itEnd; ++itMember)
  {
    if(itMember.pVarDesc->IsDynamicArray() && itMember.child_buffer())
    {
      for(auto itTest = itMember.array_begin(); itTest != itMember.array_end(); ++itTest)
      {
        CLNOP;
      }
      TRACE("*%s[x%d %08x]\n", itMember.FullNameA(), 0, itMember.child_buffer());
      //CheckIter(itMember);
      ctx.nNumArray++;
    }

    switch(itMember.pVarDesc->GetTypeCategory())
    {
    case T_STRING:
      if(itMember.IsArray())
      {
        if(pParent && pParent->IsArray())
        {
          int nCount = pParent->array_length();
          auto itStep = itMember;
          auto parentindex = pParent->index;
          ASSERT(pParent->index == 0);

          for(int i = 0; i < nCount; i++)
          {
            EnumStrings(ctx, nDepth + 1, itStep);
            pParent->StepArrayMember(itStep);
          }

          pParent->index = parentindex; // �ָ�������
          bval = TRUE;
        }
        else {
          EnumStrings(ctx, nDepth + 1, itMember);
        }
      }
      else {
        if(pParent && pParent->IsArray())
        {
          int nCount = pParent->array_length();
          auto itStep = itMember;
          auto parentindex = pParent->index;
          ASSERT(pParent->index == 0);

          for(int i = 0; i < nCount; i++)
          {
            //EnumStrings(ctx, nDepth + 1, itStep);
            itStep.ToVariable(var);
            TRACE("%*c %s=\"%s\"\n", nDepth * 2, 'T', itStep.FullNameA(), var.IsValid() ? var.ToStringA() : "<null>");
            ctx.nNumString++;
            pParent->StepArrayMember(itStep);
          }

          pParent->index = parentindex; // �ָ�������
          bval = TRUE;
        }
        else {
          itMember.ToVariable(var);
          TRACE("%*c %s=\"%s\"\n", nDepth * 2, 'T', itMember.FullNameA(), var.IsValid() ? var.ToStringA() : "<null>");
          ctx.nNumString++;
        }
      }
      break;

    case T_STRUCT:
      if(itMember.IsArray())
      {
        //*
        if(pParent && pParent->IsArray())
        {
          int nCount = pParent->array_length();
          auto itStep = itMember;
          auto parentindex = pParent->index;
          ASSERT(pParent->index == 0);

          for(int i = 0; i < nCount; i++)
          {
            auto itElement = itStep.array_begin();
            EnumeratePtrControl(ctx, nDepth + 1, &itElement, itElement.begin(), itElement.end());

            pParent->StepArrayMember(itStep);
          }

          pParent->index = parentindex; // �ָ�������
          bval = TRUE;
        }
        else {
          auto itElement = itMember.array_begin();
          EnumeratePtrControl(ctx, nDepth + 1, &itElement, itElement.begin(), itElement.end());
        }
        /*/
        auto itEnd = itMember.array_end();
        auto itElement = itMember.array_begin();
        for(auto itElement = itMember.array_begin(); itElement != itEnd; ++itElement)
        {
          if(EnumeratePtrControl(ctx, nDepth + 1, &itElement, itElement.begin(), itElement.end())) {
            break;
          }
        }
        //*/
      }
      else {
        if(pParent && pParent->IsArray())
        {
          int nCount = pParent->array_length();
          auto itStep = itMember;
          auto parentindex = pParent->index;
          ASSERT(pParent->index == 0);

          for(int i = 0; i < nCount; i++)
          {
            //auto itElement = itStep.array_begin();
            EnumeratePtrControl(ctx, nDepth + 1, &itStep, itStep.begin(), itStep.end());

            pParent->StepArrayMember(itStep);
          }

          pParent->index = parentindex; // �ָ�������
          bval = TRUE;
        }
        else
        {
          EnumeratePtrControl(ctx, nDepth + 1, &itMember, itMember.begin(), itMember.end());
        }
      }
      break;
    }
  }

  return bval;
}

void EnumeratePtrControl(DataPool* pDataPool)
{
  ENUMERATE_CONTEXT ctx = {0};
  TRACE("=======================================================\n");
  EnumeratePtrControl(ctx, 0, NULL, pDataPool->named_begin(), pDataPool->named_end());
  TRACE("Arrays:%d Strings:%d\n", ctx.nNumArray, ctx.nNumString);
  TRACE("=======================================================\n");
}

void EnumeratePtrControl2(DataPool* pDataPool)
{
  ENUMERATE_CONTEXT ctx = {0};
  TRACE("=======================================================\n");
  //EnumeratePtrControl(ctx, 0, pDataPool->begin(), pDataPool->end());
  /*
  DataPoolUtility::EnumerateVariables(0, pDataPool->begin(), pDataPool->end(), 
    [&ctx](DataPool::iterator& it, clBufferBase* pBuffer, int nDepth)
  {
    //TRACE("*[%08x] %s\n", pBuffer, it.FullNameA());
    if(pBuffer)
    {
      ctx.nNumArray++;
    }
  },

    [&ctx](DataPool::iterator& it, int nDepth) 
  {
    if(it.pVarDesc->TypeCategory() == T_STRING)
    {
      ctx.nNumString++;
      TRACE("*%s\n", it.FullNameA());
    }

    if(it.pVarDesc->IsDynamicArray())
    {
      //if(it.child_buffer())
      //{
      //  //ctx.nNumArray++;
      //}
      //TRACE("#[%08x] %s\n", it.child_buffer(), it.FullNameA());
    }
    else
    {
      //TRACE("#[%08x] %s\n", it.pBuffer, it.FullNameA());
    }
  });
  /*/

  DataPoolUtility::EnumerateVariables2<DataPoolIterator, DataPoolUtility::named_element_iterator,
    DataPoolUtility::named_element_reverse_iterator>(pDataPool->named_begin(), pDataPool->named_end(), 
    [&ctx](int bArray, DataPoolIterator& it, int nDepth) 
  {
    if(bArray)
    {
      ASSERT(it.pVarDesc->IsDynamicArray() && it.index == (GXUINT)-1);
      if(it.child_buffer())
      {
        ctx.nNumArray++;
      }
    }

    if(it.pVarDesc->GetTypeCategory() == T_STRING)
    {
      ctx.nNumString++;
      //TRACE("*%s\n", it.FullNameA());
    }

    if(it.pVarDesc->IsDynamicArray())
    {
      TRACE("#[%08x] %s\n", it.pBuffer, it.FullNameA());
    }
    else
    {
      TRACE("#[%08x] %s\n", it.pBuffer, it.FullNameA());
    }
  });

  //*/
  TRACE("Arrays:%d Strings:%d\n", ctx.nNumArray, ctx.nNumString);
  TRACE("=======================================================\n");
}